/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "danbooru.hpp"

#include <sstream>
#include <chrono>
#include <ranges>
#include <algorithm>
#include <future>

#include <magic_enum.hpp>

#include <env.hpp>

using namespace danbooru;

timestamp danbooru::parse_timestamp(std::string_view ts) {
    std::string date { ts };

    std::stringstream ss { date };
    timestamp res;
    ss >> std::chrono::parse("%FT%T%Ez", res);

    return res;
}

std::string danbooru::format_timestamp(timestamp time) {
    return std::format(timestamp_format, std::chrono::time_point_cast<std::chrono::milliseconds>(time));
}

std::string page_selector::str() const {
    std::stringstream ss;

    switch (pos) {
        case page_pos::before: ss << 'b'; break;
        case page_pos::after: ss << 'a'; break;
        default: break;
    }

    ss << value;
    return ss.str();
}

json page_selector::json() const {
    return *this;
}

page_selector::operator danbooru::json() const {
    return json::object({ { "page", str() } });
}

page_selector page_selector::at(uint32_t value) {
    return { .pos = page_pos::absolute, .value = value };
}

page_selector page_selector::before(uint32_t value) {
    return { .pos = page_pos::before, .value = value };
}

page_selector page_selector::after(uint32_t value) {
    return { .pos = page_pos::after, .value = value };
}

api::api()
    : _rl {
        util::environment::get_or_default<uint64_t>("DANBOORU_RATE_LIMIT", 5),
        std::chrono::seconds(1)
    }
    , _auth {
        util::environment::get<std::string>("DANBOORU_LOGIN"),
        util::environment::get<std::string>("DANBOORU_API_KEY"),
        cpr::AuthMode::BASIC
    }
    , _user_agent { std::format("hoshino.bot user {}", util::environment::get<std::string>("DANBOORU_LOGIN")) } {

    spdlog::info("Rate limit: {} / s", _rl.bucket_size());

    /* Verify login */
    auto res = fetch("profile", json::object({ { "only", "id,name,level" } })).get();

    if (!res.contains("id") || !res.contains("name")) {
        throw std::runtime_error { std::format("Failed to get user info: {}", res.dump())};
    }

    _user_id = res["id"];
    _user_name = res["name"];
    _level = *magic_enum::enum_cast<user_level>(res["level"].get<int32_t>());

    /* Be nice to evazion, tell him who we are */
    _user_agent = std::format("{} (#{})", _user_agent, _user_id);

    spdlog::info("Logging in as {} (user #{}), level: {}", _user_id, _user_id, magic_enum::enum_name(_level));
}

std::future<std::vector<tag>> api::tags(page_selector page, size_t limit) {
    if (limit > page_limit) {
        throw std::invalid_argument { std::format("limit of {} is too large (max: {})", limit, page_limit) };
    }

    return fetch<std::vector<tag>>("tags", { page, { "limit", std::to_string(limit) } }, [](const json& j) -> std::vector<tag> {
        return j;
    });
}

std::future<util::unordered_string_map<int32_t>> danbooru::fetch_and_insert_tags(
    api& booru, database::connection& db, const util::unordered_string_set& tags, database::insert_mode mode) {

    return std::async([&booru, &db, &tags, mode]() {
        util::unordered_string_map<int32_t> tag_ids;

        auto tx = db.work();

        /* Fetch which IDs we already know*/
        std::vector<std::string_view> tags_to_fetch;
        for (std::string_view tag : tags) {
            int32_t id = db.tag_id(tx, tag);
            if (id) {
                tag_ids.emplace(tag, id);
            } else {
                tags_to_fetch.emplace_back(tag);
            }
        }

        if (!tags_to_fetch.empty()) {
            std::vector<std::future<json>> futures;
            futures.reserve((tags_to_fetch.size() + page_limit - 1) / page_limit);

            /* Queue requests */
            for (const auto& chunk : tags_to_fetch | std::views::chunk(page_limit)) {
                futures.emplace_back(
                    booru.fetch("tags", {
                        { "limit", page_limit },
                        { "search", { { "name", chunk } } }
                    })
                );
            }

            /* Process results and insert tags  */
            for (std::vector<tag> res : futures | std::views::transform(&std::future<json>::get)) {
                for (tag& src : res) {
                    tag_ids[src.name] = src.id;

                    /* Calculate this ourselves */
                    src.post_count = 0;
                    db.insert(tx, src, mode);
                }
            }

            /* Make sure we got everything */
            std::vector<std::string_view> missing_tags;
            for (std::string_view tag : tags) {
                if (auto it = tag_ids.find(tag); it == tag_ids.end() || it->second <= 0) {
                    /* Can't insert here, could invalidate iterators */
                    missing_tags.push_back(tag);
                }
            }

            /* Generate new tag IDs for nonexistent tags */
            int32_t next_tag = db.lowest_tag(tx) - 1;
            for (std::string_view new_tag : missing_tags) {
                tag tag {
                    .id = next_tag--,
                    .name = std::string { new_tag },
                    .post_count = 0,
                    .category = tag_category::general,
                    .is_deprecated = false,
                    .created_at = {},
                    .updated_at = {},
                };

                tag_ids[tag.name] = tag.id;
                db.insert(tx, tag, mode);
            }

            spdlog::debug("Fetched {} new tags out of {}, created {} new ones", tags_to_fetch.size() - missing_tags.size(), tags.size(), missing_tags.size());
        }

        tx.commit();


        return tag_ids;
    });
    
}

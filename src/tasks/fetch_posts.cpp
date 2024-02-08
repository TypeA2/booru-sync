/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fetch_posts.hpp"

#include <array>
#include <chrono>

#include <logging.hpp>
#include <util.hpp>

#include "danbooru.hpp"
#include "database.hpp"

using namespace danbooru;
using namespace database;

void tasks::fetch_posts::execute(std::stop_token token, api& booru, connection& db) {
    posts table = db.posts();

    int32_t latest_post = table.latest_id();

    spdlog::info("Latest post: post #{}", latest_post);

    static auto value_or = []<typename T>(danbooru::json & v, T def) {
        if (v.is_null()) {
            return def;
        } else {
            return v.get<T>();
        }
    };

    while (!token.stop_requested()) {
        /*
         * - Fetch post
         *   - Fetch asset
         *   - Fetch versions
         *     - Fetch tags
         */
        json response = booru.fetch("posts", { { "limit", 200 }, page_selector::after(latest_post)}).get();

        if (response.empty()) {
            break;
        }

        util::unordered_string_map<int32_t> tags;

        /* Gather tags */
        size_t total_tags = 0;
        for (const json& post : response | std::views::reverse) {
            for (const auto& tag : post["tag_string"].get<std::string_view>() | std::views::split(' ')) {
                tags.emplace(std::string { tag.begin(), tag.end() }, 0);
                ++total_tags;
            }
        }

        spdlog::debug("Processed {} tags, {} unique tags", total_tags, tags.size());

        auto get_tags_tx = db.work();
        std::vector<std::string_view> fetch_tags;
        for (auto& [tag, id] : tags) {
            auto row = get_tags_tx.exec_prepared("get_tag_id_by_name", tag);

            switch (row.size()) {
                case 0:
                    /* Unknown tag, fetch later */
                    fetch_tags.emplace_back(tag);
                    break;

                case 1:
                    /* Got ID */
                    id = std::get<0>(row[0].as<int32_t>());
                    break;

                default:
                    throw std::runtime_error {
                        std::format("Got {} rows while searching tag \"{}\"", row.size(), tag)
                    };
            }
        }
        get_tags_tx.commit();

        std::vector<std::future<json>> requests;
        requests.reserve((fetch_tags.size() + page_limit - 1) / page_limit);

        for (const auto& chunk : fetch_tags | std::views::chunk(page_limit)) {
            json names = json::array();
            std::ranges::copy(chunk, std::back_inserter(names));

            json body = json::object({
                { "limit", page_limit },
                { "search", json::object({{"name", names}}) }
               });

            requests.emplace_back(booru.fetch("tags", body));
        }

        auto tags_table = db.tags();

        auto insert_tags_tx = db.work();
        for (const json& res : requests | std::views::transform(&std::future<json>::get)) {
            for (tag src : res | std::views::transform(&tag::parse)) {
                spdlog::trace("tag #{}: {}", src.id, src.name);
                tags[src.name] = src.id;
                tags_table.insert(insert_tags_tx, src);
            }
        }

        /* Make sure we got everything */
        for (std::string_view tag : fetch_tags) {
            if (auto it = tags.find(tag); it == tags.end() || it->second <= 0) {
                throw std::runtime_error {
                    std::format("Tag \"{}\" was not retrieved when it should've been", tag)
                };
            }
        }

        spdlog::debug("Fetched {} new tags", fetch_tags.size());

        insert_tags_tx.abort();

        break;
    }
}

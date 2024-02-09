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

    while (!token.stop_requested()) {
        /*
         * - Fetch post
         *   - Fetch asset
         *   - Fetch versions
         *     - Fetch tags
         */
        json response = booru.fetch("posts",
            { { "limit", 200 }, { "page", page_selector::after(latest_post).str() } }
        ).get();

        if (response.empty()) {
            break;
        }

        spdlog::debug("Posts: [{}, {}]", response.front()["id"].get<int32_t>(), response.back()["id"].get<int32_t>());

        util::unordered_string_map<int32_t> tag_ids;

        /* Gather tags */
        size_t total_tags = 0;
        for (const json& post : response | std::views::reverse) {
            for (const auto& tag : post["tag_string"].get<std::string_view>() | std::views::split(' ')) {
                tag_ids.emplace(std::string { tag.begin(), tag.end() }, 0);
                ++total_tags;
            }
        }

        spdlog::debug("Processed {} tags, {} unique tags", total_tags, tag_ids.size());

        auto get_tags_tx = db.work();
        std::vector<std::string_view> fetch_tags;
        for (auto& [tag, id] : tag_ids) {
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
                tag_ids[src.name] = src.id;
                tags_table.insert(insert_tags_tx, src);
            }
        }

        /* Make sure we got everything */
        for (std::string_view tag : fetch_tags) {
            if (auto it = tag_ids.find(tag); it == tag_ids.end() || it->second <= 0) {
                throw std::runtime_error {
                    std::format("Tag \"{}\" was not retrieved when it should've been", tag)
                };
            }
        }

        spdlog::debug("Fetched {} new tags", fetch_tags.size());

        insert_tags_tx.commit();

        auto insert_posts_tx = db.work();

        auto posts = db.posts();
        auto media_assets = db.media_assets();

        for (const json& src : response | std::views::reverse) {
            post res {
                .id           = src["id"],
                .uploader_id  = src["uploader_id"],
                .approver_id  = util::value_or<int32_t>(src["approver_id"], 0),
                /* .tags      = {} */
                .rating       = *magic_enum::enum_cast<post_rating>(src["rating"].get<std::string_view>()),
                .parent       = util::value_or<int32_t>(src["parent_id"], 0),
                .source       = src["source"],
                .media_asset  = src["media_asset"]["id"],
                .fav_count    = src["fav_count"],
                .has_children = src["has_children"],
                .up_score     = src["up_score"],
                .down_score   = src["down_score"],
                .is_pending   = src["is_pending"],
                .is_flagged   = src["is_flagged"],
                .is_deleted   = src["is_deleted"],
                .is_banned    = src["is_banned"],
                .pixiv_id     = util::value_or<int32_t>(src["pixiv_id"], 0),
                .bit_flags    = src["bit_flags"],
                .last_comment = nullable_timestamp(src["last_commented_at"]),
                .last_bump    = nullable_timestamp(src["last_comment_bumped_at"]),
                .last_note    = nullable_timestamp(src["last_noted_at"]),
                .created_at   = parse_timestamp(src["created_at"]),
                .updated_at   = parse_timestamp(src["updated_at"]),
            };

            res.tags.reserve(src["tag_count"]);

            for (const auto& tag : src["tag_string"].get<std::string_view>() | std::views::split(' ')) {
                auto it = tag_ids.find(tag);
                res.tags.push_back(it->second);
            }

            media_asset asset = media_asset::parse(src["media_asset"]);

            media_assets.insert(insert_posts_tx, asset);
            posts.insert(insert_posts_tx, res);
        }

        insert_posts_tx.commit();

        spdlog::debug("Inserted {} new posts, up to {}", response.size(), posts.latest_id());

        break;
    }
}

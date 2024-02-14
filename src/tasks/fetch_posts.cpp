/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fetch_posts.hpp"

#include <array>
#include <chrono>

#include <spdlog/spdlog.h>

#include <logging.hpp>
#include <util.hpp>

#include "danbooru.hpp"
#include "database.hpp"

using namespace danbooru;
using namespace database;

namespace detail {
    static constexpr std::string_view post_attributes_to_fetch =
        "id,uploader_id,approver_id,tag_string,rating,parent_id,source,media_asset[id],fav_count,"
        "has_children,up_score,down_score,is_pending,is_flagged,is_deleted,is_banned,pixiv_id,"
        "bit_flags,last_commented_at,last_comment_bumped_at,last_noted_at,created_at,updated_at";

    [[nodiscard]] static std::vector<api_response::post> get_sorted_posts(api& booru, int32_t start_at) {
        std::vector<api_response::post> posts = booru.fetch("posts",
            {
                { "limit", post_limit },
                { "page", page_selector::after(start_at).str() },
                { "only", post_attributes_to_fetch }
            }
        ).get();

        std::ranges::sort(posts, {}, [](const api_response::post& post) { return post.id; });

        return posts;
    }

    [[nodiscard]] static std::string get_id_string(std::span<api_response::post> posts) {
        /* Comma-separated list of IDs */
        std::stringstream id_string_stream;
        id_string_stream << posts.front().id;
        for (const auto& post : posts) {
            id_string_stream << ',' << post.id;
        }

        return id_string_stream.str();
    }

    [[nodiscard]] static std::vector<api_response::post_version> get_sorted_post_versions(api& booru, std::span<api_response::post> posts) {
        std::string id_string = get_id_string(posts);

        std::vector<api_response::post_version> res;

        int32_t latest_id = 0;
        for (;;) {
            std::vector<api_response::post_version> versions = booru.fetch("post_versions",
                {
                    { "limit", page_limit },
                    { "page", page_selector::after(latest_id).str() },
                    { "search", { { "post_id", id_string } } }
                }
            ).get();

            if (versions.empty()) {
                break;
            }

            std::ranges::sort(versions, {}, [](const api_response::post_version& v) { return v.id; });

            latest_id = versions.back().id;

            /* Move since we don't need the old vector anymore */
            res.insert(res.end(), std::make_move_iterator(versions.begin()), std::make_move_iterator(versions.end()));
        }

        return res;
    }

    [[nodiscard]] static std::vector<std::string_view> get_unknown_tags(util::unordered_string_map<int32_t>& tag_ids, connection& db) {
        auto tx = db.work();

        std::vector<std::string_view> unknown_tags;
        for (auto& [tag, id] : tag_ids) {
            int32_t result = db.tag_id(tx, tag);
            if (result) {
                id = result;
            } else {
                unknown_tags.emplace_back(tag);
            }
        }
        tx.commit();

        return unknown_tags;
    }
}

void tasks::fetch_posts::execute(std::stop_token token, api& booru, connection& db) {
    int32_t latest_post = db.latest_post();

    spdlog::info("Latest post: post #{}", latest_post);

    while (!token.stop_requested()) {
        auto begin = clock::now();

        auto posts = detail::get_sorted_posts(booru, latest_post);

        if (posts.empty()) {
            break;
        }

        spdlog::debug("Posts: [{}, {}] ({})", posts.front().id, posts.back().id, posts.size());

        size_t total_tags = 0;
        util::unordered_string_set tags;
        for (const api_response::post& post : posts) {
            for (const auto& tag : post.tag_string | std::views::split(' ')) {
                tags.emplace(std::string_view { tag.begin(), tag.end() });
                ++total_tags;
            }
        }

        auto tag_ids = fetch_and_insert_tags(booru, db, tags, insert_mode::overwrite).get();
        std::unordered_map<int32_t, int32_t> tag_counts;
        for (const auto& [tag, id] : tag_ids) {
            tag_counts.emplace(id, 0);
        }

        spdlog::trace("Processed {} tags, {} unique tags", total_tags, tag_ids.size());

        auto tx = db.work();

        for (const api_response::post& src : posts) {
            post res {
                .id           = src.id,
                .uploader_id  = src.uploader_id,
                .approver_id  = src.approver_id,
                .tags         = src.tag_string
                                    | std::views::split(' ')
                                    | std::views::transform([&tag_ids](const auto& tag) { return tag_ids.find(tag)->second; })
                                    | std::ranges::to<std::vector>(),
                .rating       = src.rating,
                .parent       = src.parent_id,
                .source       = src.source,
                .media_asset  = src.media_asset.id,
                .fav_count    = src.fav_count,
                .has_children = src.has_children,
                .up_score     = src.up_score,
                .down_score   = src.down_score,
                .is_pending   = src.is_pending,
                .is_flagged   = src.is_flagged,
                .is_deleted   = src.is_deleted,
                .is_banned    = src.is_banned,
                .pixiv_id     = src.pixiv_id,
                .bit_flags    = src.bit_flags,
                .last_comment = src.last_commented_at,
                .last_bump    = src.last_comment_bumped_at,
                .last_note    = src.last_noted_at,
                .created_at   = src.created_at,
                .updated_at   = src.updated_at,
            };

            for (int32_t tag : res.tags) {
                tag_counts[tag] += 1;
            }

            db.insert(tx, res);
        }

        for (const auto& [tag_id, count] : tag_counts) {
            db.increment_post_count(tx, tag_id, count);
        }

        tx.commit();

        latest_post = db.latest_post();

        auto elapsed = clock::now() - begin;

        spdlog::info("Inserted {} new posts, up to {} ({})", posts.size(), latest_post, elapsed);
    }
}

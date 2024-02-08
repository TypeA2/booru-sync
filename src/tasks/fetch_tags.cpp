/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fetch_tags.hpp"

#include <array>
#include <chrono>

#include <logging.hpp>
#include <util.hpp>

#include "danbooru.hpp"
#include "database.hpp"

using namespace danbooru;
using namespace database;

void tasks::fetch_tags::execute(std::stop_token token, api& booru, connection& db) {
    tags table = db.tags();
    int32_t latest_id = table.latest_id();

    spdlog::info("Fetching from tag #{}", latest_id);

    while (!token.stop_requested()) {
        util::timer timer;
        auto res = booru.tags(page_selector::after(latest_id), page_limit).get();

        auto fetch = timer.elapsed_reset();

        if (res.empty()) {
            break;
        }

        latest_id = res.front().id;

        for (auto& tag : res) {
            tag.post_count = 0;
        }

        auto modify = timer.elapsed_reset();

        table.insert(res);

        auto insert = timer.elapsed_reset();

        spdlog::debug("latest tag: {} ({}), took: {}, {}, {}", latest_id, res.front().name, fetch, modify, insert);
    }
}

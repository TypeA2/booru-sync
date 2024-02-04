/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fetch_tags.hpp"

#include <array>

#include <logging.hpp>

#include "danbooru.hpp"
#include "database.hpp"

using namespace danbooru;
using namespace database;

void tasks::fetch_tags::execute(std::stop_token token, api& booru, instance& db) {
    tags& table = db.tags();

    int32_t latest_id = table.last();

    spdlog::info("Fetching from tag #{}", latest_id);

    while (!token.stop_requested()) {
        booru.tags(_tags, page_selector::after(latest_id), page_limit);

        if (_tags.empty()) {
            break;
        }

        table.insert(_tags);

        latest_id = _tags.front().id;

        spdlog::debug("Last tag: {}", latest_id);
    }
}

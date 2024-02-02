/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fetch_tags.hpp"

#include <array>

#include <logging.hpp>

#include "danbooru.hpp"
#include "database.hpp"

void tasks::fetch_tags::execute(std::stop_token token, danbooru& booru, database& db) {
    auto tx = db.work();
    auto latest_id = tx.query_value<int32_t>("SELECT COALESCE(MAX(id), 0) FROM tags");
    tx.commit();


}

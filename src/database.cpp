#include "database.hpp"

#include <spdlog/spdlog.h>

using namespace database;

namespace pqxx {
    zview string_traits<danbooru::timestamp>::to_buf(char* begin, char* end, const danbooru::timestamp& val) {
        char* new_end = into_buf(begin, end, val);

        if (new_end != end) {
            throw pqxx::conversion_overrun { std::format("failed to format timestamp: format size difference of {}", std::distance(end, new_end)) };
        }

        return zview { begin, end };
    }

    char* string_traits<danbooru::timestamp>::into_buf(char* begin, char* end, const danbooru::timestamp& val) {
        size_t buf_size = std::distance(begin, end);
        if (buf_size < danbooru::timestamp_length) {
            throw pqxx::conversion_overrun { std::format("failed to format timestamp: buffer size {} is too small", buf_size) };
        }

        auto res = std::format_to_n(begin, buf_size,
            danbooru::timestamp_format, std::chrono::time_point_cast<std::chrono::milliseconds>(val));

        *res.out = '\0';

        return res.out;
    }

    std::size_t string_traits<danbooru::timestamp>::size_buffer(const danbooru::timestamp& value) noexcept {
        /* "2024-01-01T00:00:00.000+00:00" */
        return danbooru::timestamp_length;
    }

    danbooru::timestamp string_traits<danbooru::timestamp>::from_string(std::string_view text) {
        return danbooru::parse_timestamp(text);
    }
}

connection::connection() {
    spdlog::debug("Connected to {} as {}", _conn.dbname(), _conn.username());

    _conn.prepare("get_tag_id_by_name", "SELECT id FROM tags WHERE name = $1");
    _conn.prepare("insert_media_asset", "INSERT INTO media_assets VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)");
    _conn.prepare("insert_media_asset_variant", "INSERT INTO media_asset_variants VALUES ($1, $2, $3, $4, $5)");
    _conn.prepare("insert_post", "INSERT INTO posts VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23)");
    _conn.prepare("insert_post_version", "INSERT INTO post_versions VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)");
    _conn.prepare("latest_post_version_for_post", "SELECT COALESCE(MAX(id), 0) FROM post_versions WHERE post_id = $1");
    _conn.prepare("insert_tag_weak", "INSERT INTO tags VALUES ($1, $2, $3, $4, $5, $6, $7)");
    _conn.prepare("insert_tag_overwrite",
        "INSERT INTO tags"
        "  VALUES ($1, $2, $3, $4, $5, $6, $7)"
        "  ON CONFLICT (id) DO UPDATE"
        "    SET (name, post_count, category, is_deprecated, created_at, updated_at)"
        "      = (EXCLUDED.name, EXCLUDED.post_count, EXCLUDED.category, EXCLUDED.is_deprecated, EXCLUDED.created_at, EXCLUDED.updated_at)");
    _conn.prepare("increment_post_count", "UPDATE tags SET post_count = post_count + $2 WHERE id = $1");
}

connection::~connection() {
    if (_conn.is_open()) {
        spdlog::debug("Disconnecting from {}", _conn.dbname());
    }
}

connection::operator pqxx::connection& () {
    return _conn;
}

pqxx::connection* connection::operator->() {
    return &_conn;
}

pqxx::connection& connection::conn() {
    return _conn;
}

pqxx::work connection::work() {
    return pqxx::work { _conn };
}

int32_t connection::insert(pqxx::work& tx, const danbooru::tag& tag, insert_mode mode) {
    tx.exec_prepared0(mode == insert_mode::weak ? "insert_tag_weak" : "insert_tag_overwrite",
        tag.id,
        tag.name,
        tag.post_count,
        tag.category,
        tag.is_deprecated,
        tag.created_at,
        tag.updated_at
    );

    return tag.id;
}

int32_t connection::insert(pqxx::work& tx, const danbooru::post& post) {
    tx.exec_prepared0("insert_post",
        post.id,
        post.uploader_id,
        post.approver_id,
        post.tags,
        post.rating,
        post.parent,
        post.source.empty() ? std::nullopt : std::optional { post.source },
        post.media_asset,
        post.fav_count,
        post.has_children,
        post.up_score,
        post.down_score,
        post.is_pending,
        post.is_flagged,
        post.is_deleted,
        post.is_banned,
        post.pixiv_id,
        post.bit_flags,
        post.last_comment,
        post.last_bump,
        post.last_note,
        post.created_at,
        post.updated_at
    );

    return post.id;
}

int32_t connection::insert(pqxx::work& tx, const danbooru::media_asset& asset) {
    /* First insert asset, then versions */
    tx.exec_prepared0("insert_media_asset",
        asset.id,
        asset.md5,
        asset.file_ext,
        asset.file_size,
        asset.image_width,
        asset.image_height,
        asset.duration,
        asset.pixel_hash,
        asset.status,
        asset.file_key,
        asset.is_public,
        asset.created_at,
        asset.updated_at
    );

    for (const danbooru::media_asset_variant& variant : asset.variants) {
        tx.exec_prepared0("insert_media_asset_variant",
            asset.id,
            variant.type,
            variant.width,
            variant.height,
            variant.file_ext
        );
    }

    return asset.id;
}

int32_t connection::insert(pqxx::work& tx, const danbooru::post_version& version) {
    tx.exec_prepared0("insert_post_version",
        version.id,
        version.post_id,
        version.updater_id,
        version.updated_at,
        version.version,
        version.added_tags.empty() ? std::nullopt : std::optional { version.added_tags },
        version.removed_tags.empty() ? std::nullopt : std::optional { version.removed_tags },
        version.new_rating,
        version.new_parent,
        version.new_source
    );

    return version.id;
}

void connection::increment_post_count(pqxx::work& tx, int32_t tag_id, int32_t count) {
    tx.exec_prepared0("increment_post_count", tag_id, count);
}

int32_t connection::latest_post() {
    return _table_max_id("posts");
}

int32_t connection::latest_tag() {
    return _table_max_id("tags");
}

int32_t connection::latest_media_asset() {
    return _table_max_id("post_versions");
}

int32_t connection::latest_post_version() {
    return _table_max_id("post_versions");
}

int32_t connection::latest_post_version(int32_t post_id) {
    auto tx = work();
    int32_t res = tx.exec_prepared1("latest_post_version_for_post", post_id).at(0).as<int32_t>();
    tx.commit();
    return res;
}

int32_t connection::lowest_tag() {
    auto tx = work();
    int32_t res = lowest_tag(tx);
    tx.commit();
    return res;
}

int32_t connection::lowest_tag(pqxx::work& tx) {
    return tx.exec1("SELECT COALESCE(MIN(id), 0) FROM tags").at(0).as<int32_t>();
}

int32_t connection::tag_id(pqxx::work& tx, std::string_view tag_name) {
    auto rows = tx.exec_prepared("get_tag_id_by_name", tag_name);

    if (rows.empty()) {
        return 0;
    } else if (rows.size() != 1) {
        throw std::runtime_error {
            std::format("Got {} rows for tag \"{}\"", rows.size(), tag_name)
        };
    } else {
        return *rows.at(0).at(0).get<int32_t>();
    }
}

int32_t connection::_table_max_id(std::string_view table) {
    auto tx = work();
    int32_t res = _table_max_id(tx, table);
    tx.commit();
    return res;
}

int32_t connection::_table_max_id(pqxx::work& tx, std::string_view table) {
    return tx.query_value<int32_t>(std::format("SELECT COALESCE(MAX(id), 0) FROM {}", table));
}

#include "database.hpp"

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

table::table(connection& inst) : inst { inst } { }

int32_t tags::insert(const danbooru::tag& tag) {
    auto tx = inst.work();
    int32_t max_id = insert(tx, tag);
    tx.commit();

    return tag.id;
}

int32_t tags::insert(std::span<const danbooru::tag> tags) {
    auto tx = inst.work();
    int32_t max_id = insert(tx, tags);
    tx.commit();

    return max_id;
}

int32_t tags::insert(pqxx::work& tx, const danbooru::tag& tag) {
    tx.exec_prepared("tags_insert",
        tag.id, tag.name, tag.post_count, tag.category, tag.is_deprecated, tag.created_at, tag.updated_at);

    return tag.id;
}

int32_t tags::insert(pqxx::work& tx, std::span<const danbooru::tag> tags) {
    int32_t max_id = 0;

    for (const danbooru::tag& tag : tags) {
        if (tag.id > max_id) {
            max_id = tag.id;
        }

        tx.exec_prepared("tags_insert",
            tag.id, tag.name, tag.post_count, tag.category, tag.is_deprecated, tag.created_at, tag.updated_at);
    }

    return max_id;
}

int32_t posts::insert(pqxx::work& tx, const danbooru::post& post) {
    tx.exec_prepared("insert_post",
        post.id,
        post.uploader_id,
        post.approver_id ? std::optional { post.approver_id } : std::nullopt,
        post.tags,
        post.rating,
        post.parent ? std::optional { post.parent } : std::nullopt,
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
        post.pixiv_id ? std::optional { post.pixiv_id } : std::nullopt,
        post.bit_flags,
        post.last_comment.time_since_epoch().count() == 0 ? std::nullopt : std::optional { post.last_comment },
        post.last_bump.time_since_epoch().count() == 0 ? std::nullopt : std::optional { post.last_bump },
        post.last_note.time_since_epoch().count() == 0 ? std::nullopt : std::optional { post.last_note },
        post.created_at,
        post.updated_at
    );

    return post.id;
}

int32_t media_assets::insert(pqxx::work& tx, const danbooru::media_asset& asset) {
    /* First insert asset, then versions */
    tx.exec_prepared("insert_media_asset",
        asset.id,
        asset.md5,
        asset.file_ext,
        asset.file_size,
        asset.image_width,
        asset.image_height,
        asset.duration == 0 ? std::nullopt : std::optional { asset.duration },
        asset.pixel_hash,
        asset.status,
        asset.file_key,
        asset.is_public,
        asset.created_at,
        asset.updated_at
    );

    for (const danbooru::media_asset_variant& variant : asset.variants) {
        tx.exec_prepared("insert_media_asset_variant",
            asset.id,
            variant.type,
            variant.width,
            variant.height,
            variant.file_ext
        );
    }

    return asset.id;
}

pqxx::work connection::work() {
    return pqxx::work { _conn };
}

connection::connection() {
    spdlog::debug("Connected to {} as {}", _conn.dbname(), _conn.username());

    _conn.prepare("tags_insert","INSERT INTO tags VALUES ($1, $2, $3, $4, $5, $6, $7)");
    _conn.prepare("get_tag_id_by_name", "SELECT id FROM tags WHERE name = $1");
    _conn.prepare("insert_media_asset", "INSERT INTO media_assets VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)");
    _conn.prepare("insert_media_asset_variant", "INSERT INTO media_asset_variants VALUES ($1, $2, $3, $4, $5)");
    _conn.prepare("insert_post", "INSERT INTO posts VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23)");
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

tags connection::tags() {
    return database::tags { *this };
}

posts connection::posts() {
    return database::posts { *this };
}

media_assets connection::media_assets() {
    return database::media_assets { *this };
}

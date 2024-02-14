#ifndef DANBOORU_TYPES_HPP
#define DANBOORU_TYPES_HPP

#include <chrono>

#include <nlohmann/json.hpp>

#include "util.hpp"

namespace danbooru {
    using clock = std::chrono::utc_clock;
    using timestamp = clock::time_point;

    static constexpr char timestamp_format[] = "{:%FT%T%Ez}";
    static constexpr size_t timestamp_length = 30;

    [[nodiscard]] timestamp parse_timestamp(std::string_view ts);
    [[nodiscard]] std::string format_timestamp(timestamp time);
}

NLOHMANN_JSON_NAMESPACE_BEGIN
    template <>
    struct adl_serializer<danbooru::timestamp> {
        static void to_json(json& dst, const danbooru::timestamp& ts) {
            dst = danbooru::format_timestamp(ts);
        }

        static void from_json(const json& src, danbooru::timestamp& ts) {
            ts = danbooru::parse_timestamp(src);
        }
    };

    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json& dst, const std::optional<T>& val) {
            if (val.has_value()) {
                dst = *val;
            } else {
                dst = nullptr;
            }
        }

        static void from_json(const json& src, std::optional<T>& val) {
            if (src.is_null()) {
                val = std::nullopt;
            } else {
                val = src.get<T>();
            }
        }
    };
NLOHMANN_JSON_NAMESPACE_END

namespace danbooru {
    using json = nlohmann::json;

    static constexpr size_t post_limit = 200;
    static constexpr size_t page_limit = 1000;

    enum class tag_category : uint8_t {
        general = 0,
        artist = 1,
        copyright = 3,
        character = 4,
        meta = 5,
    };
    JSON_SERIALIZE_INT_ENUM(tag_category)

    enum class post_rating : uint8_t {
        g,
        s,
        q,
        e,
    };
    JSON_SERIALIZE_STRING_ENUM(post_rating)

        enum class pool_category : uint8_t {
        series,
        collection,
    };
    JSON_SERIALIZE_STRING_ENUM(pool_category)

    enum class user_level : uint8_t {
        anonymous = 0,
        restricted = 10,
        member = 20,
        gold = 30,
        platinum = 31,
        builder = 32,
        contributor = 35,
        approver = 37,
        moderator = 40,
        admin = 50,
        owner = 60,
    };
    JSON_SERIALIZE_INT_ENUM(user_level)

    enum class asset_status : uint8_t {
        processing,
        active,
        deleted,
        expunged,
        failed,
    };
    JSON_SERIALIZE_STRING_ENUM(asset_status)

    enum class file_type : uint8_t {
        jpg,
        png,
        gif,
        webp,
        avif,
        mp4,
        webm,
        swf,
        zip,
    };
    JSON_SERIALIZE_STRING_ENUM(file_type)

    struct tag {
        int32_t id;
        std::string name;
        int32_t post_count;
        tag_category category;
        bool is_deprecated;
        timestamp created_at;
        timestamp updated_at;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(tag, id, name, post_count, category, is_deprecated, created_at, updated_at)

    struct post {
        int32_t id;
        int32_t uploader_id;
        std::optional<int32_t> approver_id;
        std::vector<int32_t> tags;
        post_rating rating;
        std::optional<int32_t> parent;
        std::string source;
        int32_t media_asset;
        int32_t fav_count;
        bool has_children;
        int32_t up_score;
        int32_t down_score;
        bool is_pending;
        bool is_flagged;
        bool is_deleted;
        bool is_banned;
        std::optional<int32_t> pixiv_id;
        int32_t bit_flags;
        std::optional<timestamp> last_comment;
        std::optional<timestamp> last_bump;
        std::optional<timestamp> last_note;
        timestamp created_at;
        timestamp updated_at;
    };

    struct media_asset_variant {
        std::string type;
        int32_t width;
        int32_t height;
        file_type file_ext;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(media_asset_variant, type, width, height, file_ext);

    struct media_asset {
        int32_t id;
        std::string md5;
        file_type file_ext;
        int64_t file_size;
        int32_t image_width;
        int32_t image_height;
        std::optional<float> duration;
        std::string pixel_hash;
        asset_status status;
        std::string file_key;
        bool is_public;
        std::vector<media_asset_variant> variants;
        timestamp created_at;
        timestamp updated_at;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(media_asset, id, md5, file_ext, file_size, image_width, image_height, duration, pixel_hash)

    struct post_version {
        int32_t id;
        int32_t post_id;
        int32_t updater_id;
        timestamp updated_at;
        int32_t version;
        std::vector<int32_t> added_tags;
        std::vector<int32_t> removed_tags;
        std::optional<post_rating> new_rating;
        std::optional<int32_t> new_parent;
        std::optional<std::string> new_source;
    };

    enum class request_type {
        get,
        post,
        get_as_post,
    };

    enum class page_pos {
        absolute,
        before,
        after,
    };

    namespace api_response {
        struct post {
            int32_t id;
            timestamp created_at;
            int32_t uploader_id;
            int32_t score;
            std::string source;
            std::string md5;
            std::optional<timestamp> last_comment_bumped_at;
            post_rating rating;
            int32_t image_width;
            int32_t image_height;
            std::string tag_string;
            int32_t fav_count;
            file_type file_ext;
            std::optional<timestamp> last_noted_at;
            std::optional<int32_t> parent_id;
            bool has_children;
            std::optional<int32_t> approver_id;
            int32_t tag_count_general;
            int32_t tag_count_artist;
            int32_t tag_count_character;
            int32_t tag_count_copyright;
            int64_t file_size;
            int32_t up_score;
            int32_t down_score;
            bool is_pending;
            bool is_flagged;
            bool is_deleted;
            int32_t tag_count;
            timestamp updated_at;
            bool is_banned;
            std::optional<int32_t> pixiv_id;
            std::optional<timestamp> last_commented_at;
            bool has_active_children;
            int32_t bit_flags;
            int32_t tag_count_meta;
            bool has_large;
            bool has_visible_children;
            media_asset media_asset;
            std::string tag_string_general;
            std::string tag_string_character;
            std::string tag_string_copyright;
            std::string tag_string_artist;
            std::string tag_string_meta;
            std::string file_url;
            std::string large_file_url;
            std::string preview_file_url;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(post,
            id, created_at, uploader_id, score, source, md5, last_comment_bumped_at, rating,
            image_width, image_height, tag_string, fav_count, file_ext, last_noted_at, parent_id,
            has_children, approver_id, tag_count_general, tag_count_artist, tag_count_character,
            tag_count_copyright, file_size, up_score, down_score, is_pending, is_flagged,
            is_deleted, tag_count, updated_at, is_banned, pixiv_id, last_commented_at,
            has_active_children, bit_flags, tag_count_meta, has_large, has_visible_children,
            media_asset, tag_string_general, tag_string_character, tag_string_copyright,
            tag_string_artist, tag_string_meta, file_url, large_file_url, preview_file_url
        )

        struct post_version {
            int32_t id;
            int32_t post_id;
            std::string tags;
            std::vector<std::string> added_tags;
            std::vector<std::string> removed_tags;
            std::optional<int32_t> updater_id;
            timestamp updated_at;
            std::optional<post_rating> rating;
            bool rating_changed;
            std::optional<int32_t> parent_id;
            bool parent_changed;
            std::string source;
            bool source_changed;
            int32_t version;
            std::string obsolete_added_tags;
            std::string obsolete_removed_tags;
            std::string unchanged_tags;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(post_version,
            id, post_id, tags, added_tags, removed_tags, updater_id, updated_at, rating, rating_changed,
            parent_id, parent_changed, source, source_changed, version, obsolete_added_tags,
            obsolete_removed_tags, unchanged_tags
        )
    }
}

#endif /* DANBOORU_TYPES_HPP */

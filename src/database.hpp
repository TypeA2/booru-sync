/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <string>
#include <chrono>
#include <mutex>
#include <ranges>
#include <algorithm>

#include <magic_enum.hpp>

/* Too new of a feature, vcpkg doesn't build libpqxx with it */
#undef __cpp_lib_source_location
#include <pqxx/pqxx>

#include "danbooru_defs.hpp"

namespace detail {
    template <typename T> requires std::is_enum_v<T>
    struct enum_traits {
        static constexpr bool converts_to_string = true;
        static constexpr bool converts_from_string = true;

        static pqxx::zview to_buf(char* begin, char* end, T val) {
            return { begin, into_buf(begin, end, val) };
        }

        static char* into_buf(char* begin, char* end, T val) {
            auto name = magic_enum::enum_name(val);

            if (name.size() >= static_cast<size_t>(end - begin)) {
                throw pqxx::conversion_overrun { std::format("can't format enum {}", name) };
            }

            std::ranges::copy(name, begin);

            begin[name.size()] = '\0';

            return begin + name.size() + 1;
        }

        static std::size_t size_buffer(T value) noexcept {
            return magic_enum::enum_name(value).size() + 1;
        }

        static T from_string(std::string_view text) {
            return magic_enum::enum_cast<T>(text);
        }
    };
}

#define DB_MAP_ENUM_TO_PSQL_ENUM(ENUM) \
    template <> inline constexpr std::string_view type_name<ENUM> = #ENUM; \
    template <> inline constexpr bool is_unquoted_safe<ENUM> = true; \
    template <> struct nullness<ENUM> : pqxx::no_null<ENUM> {}; \
    template <> struct string_traits<ENUM> : detail::enum_traits<ENUM> {};

namespace pqxx {
    DB_MAP_ENUM_TO_PSQL_ENUM(danbooru::tag_category);
    DB_MAP_ENUM_TO_PSQL_ENUM(danbooru::post_rating);
    DB_MAP_ENUM_TO_PSQL_ENUM(danbooru::pool_category);
    DB_MAP_ENUM_TO_PSQL_ENUM(danbooru::user_level);
    DB_MAP_ENUM_TO_PSQL_ENUM(danbooru::asset_status);
    DB_MAP_ENUM_TO_PSQL_ENUM(danbooru::file_type);

    template <> inline constexpr std::string_view type_name<danbooru::timestamp> = "danbooru::timestamp";
    template <> inline constexpr bool is_unquoted_safe<danbooru::timestamp> = true;

    template <> struct nullness<danbooru::timestamp> : pqxx::no_null<danbooru::timestamp> {};

    template <> struct string_traits<danbooru::timestamp> {
        static constexpr bool converts_to_string = true;
        static constexpr bool converts_from_string = true;

        static zview to_buf(char* begin, char* end, const danbooru::timestamp& val);
        static char* into_buf(char* begin, char* end, const danbooru::timestamp& val);
        static std::size_t size_buffer(const danbooru::timestamp& value) noexcept;

        static danbooru::timestamp from_string(std::string_view text);
    };
}

namespace database {
    enum class insert_mode {
        /* Discard on conflict */
        weak,

        /* Overwrite on conflict */
        overwrite,
    };
    class connection {
        pqxx::connection _conn;

        public:
        connection();
        ~connection();

        connection(const connection&) = delete;
        connection& operator=(const connection&) = delete;

        connection(connection&&) noexcept = default;
        connection& operator=(connection&&) noexcept = default;

        /* Behave like a pqxx::connection */
        operator pqxx::connection& ();
        pqxx::connection* operator->();
        pqxx::connection& conn();

        [[nodiscard]] pqxx::work work();

        int32_t insert(pqxx::work& tx, const danbooru::tag& tag, insert_mode mode);
        int32_t insert(pqxx::work& tx, const danbooru::post& post);
        int32_t insert(pqxx::work& tx, const danbooru::media_asset& asset);
        int32_t insert(pqxx::work& tx, const danbooru::post_version& version);

        void increment_post_count(pqxx::work& tx, int32_t tag_id, int32_t count = 1);

        [[nodiscard]] int32_t latest_post();
        [[nodiscard]] int32_t latest_tag();
        [[nodiscard]] int32_t latest_media_asset();
        [[nodiscard]] int32_t latest_post_version();
        [[nodiscard]] int32_t latest_post_version(int32_t post_id);

        /* Lowest tag ID, used for tags without a tag ID on the site */
        [[nodiscard]] int32_t lowest_tag();
        [[nodiscard]] int32_t lowest_tag(pqxx::work& tx);

        [[nodiscard]] int32_t tag_id(pqxx::work& tx, std::string_view tag_name);

        private:
        [[nodiscard]] int32_t _table_max_id(std::string_view table);
        [[nodiscard]] int32_t _table_max_id(pqxx::work& tx, std::string_view table);
    };
}

#endif /* DATABASE_HPP */

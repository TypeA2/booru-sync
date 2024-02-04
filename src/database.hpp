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

#include "danbooru.hpp"

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
    class instance;

    class table {
        friend class instance;

        protected:
        explicit table(instance& inst);

        instance& inst;
    };

    class tags : table {
        friend class instance;

        protected:
        explicit tags(instance& inst);

        public:
        [[nodiscard]] int32_t last();

        void insert(const danbooru::tag& tag);
        void insert(std::span<const danbooru::tag> tag);
    };

    class instance {
        std::mutex _mut;
        pqxx::connection _conn;
        tags _tags;

        public:
        instance();

        /* Behave like a pqxx::connection */
        operator pqxx::connection& ();
        pqxx::connection* operator->();

        [[nodiscard]] pqxx::work work();
        [[nodiscard]] std::unique_lock<std::mutex> lock();

        [[nodiscard]] tags& tags();
    };
}

#endif /* DATABASE_HPP */

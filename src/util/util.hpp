/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef UTIL_HPP
#define UTIL_HPP

#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include <algorithm>
#include <memory>
#include <format>
#include <chrono>
#include <type_traits>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <magic_enum.hpp>

#define JSON_SERIALIZE_STRING_ENUM(E) \
    inline void to_json(json& dst, E val) { \
        dst = magic_enum::enum_integer(val); \
    } \
    inline void from_json(const json& src, E& val) { \
        val = *magic_enum::enum_cast<E>(src.get<std::string_view>()); \
    }

#define JSON_SERIALIZE_INT_ENUM(E) \
    inline void to_json(json& dst, E val) { \
        dst = magic_enum::enum_name(val); \
    } \
    inline void from_json(const json& src, E& val) { \
        val = *magic_enum::enum_cast<E>(src.get<std::underlying_type_t<E>>()); \
    }

namespace util {
    namespace detail {
        struct range_eq : std::equal_to<> {
            template <typename T1, typename T2> requires std::ranges::sized_range<T1> || std::ranges::sized_range<T2>
            [[nodiscard]] constexpr auto operator()(T1 && lhs, T2 && rhs) const
                noexcept(noexcept(std::ranges::equal(std::forward<T1>(lhs), std::forward<T2>(rhs))))
                -> decltype(std::ranges::equal(std::forward<T1>(lhs), std::forward<T2>(rhs))) {
                return std::ranges::equal(std::forward<T1>(lhs), std::forward<T2>(rhs));
            }
        };

        struct string_hasher {
            using hash_type = std::hash<std::string_view>;
            using is_transparent = void;
            using transparent_key_equal = range_eq;

            template <std::ranges::sized_range T>
            std::size_t operator()(const T& str) const { return hash_type {}({ std::begin(str), std::end(str) }); }
            std::size_t operator()(const char* str) const { return hash_type {}(str); }
            std::size_t operator()(std::string_view str) const { return hash_type {}(str); }
            std::size_t operator()(const std::string& str) const { return hash_type {}(str); }
        };
    }

    template <typename T>
    using unordered_string_map = std::unordered_map<std::string, T, detail::string_hasher, detail::range_eq>;

    using unordered_string_set = std::unordered_set<std::string, detail::string_hasher, detail::range_eq>;

    template <typename T>
    concept istream_extractable = requires (T t, std::istream & is) {
        { is >> t } -> std::convertible_to<std::istream&>;
    };

    template <typename Clock = std::chrono::steady_clock>
    struct timer {
        using clock = Clock;
        using rep = clock::rep;
        using period = clock::period;
        using duration = clock::duration;
        using time_point = clock::time_point;

        time_point start;

        timer() : start { clock::now() } { }

        duration elapsed() {
            return clock::now() - start;
        }

        duration elapsed(time_point since) {
            return clock::now() - since;
        }

        duration elapsed_reset() {
            auto res = elapsed();
            reset();
            return res;
        }

        auto average(size_t n) {
            auto total = elapsed();

            return total / n;
        }

        void reset() {
            start = clock::now();
        }
    };

    template <typename T>
    auto value_or(const nlohmann::json& v, T def) {
        if (v.is_null()) {
            return def;
        } else {
            return v.get<T>();
        }
    }
}

template <> struct std::formatter<std::chrono::nanoseconds> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::chrono::nanoseconds& ns, format_context& ctx) const {
        if (ns < std::chrono::microseconds(1)) {
            return std::format_to(ctx.out(), "{} ns", ns.count());
        } else if (ns < std::chrono::milliseconds(1)) {
            return std::format_to(ctx.out(), "{:.3f} us", ns.count() / 1e3);
        } else if (ns < std::chrono::seconds(1)) {
            return std::format_to(ctx.out(), "{:.3f} ms", ns.count() / 1e6);
        }

        return std::format_to(ctx.out(), "{:.3f} s", ns.count() / 1e9);
    }
};

template <> struct fmt::formatter<std::chrono::nanoseconds> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::chrono::nanoseconds& ns, format_context& ctx) const {
        if (ns < std::chrono::microseconds(1)) {
            return std::format_to(ctx.out(), "{} ns", ns.count());
        } else if (ns < std::chrono::milliseconds(1)) {
            return std::format_to(ctx.out(), "{:.3f} us", ns.count() / 1e3);
        } else if (ns < std::chrono::seconds(1)) {
            return std::format_to(ctx.out(), "{:.3f} ms", ns.count() / 1e6);
        }

        return std::format_to(ctx.out(), "{:.3f} s", ns.count() / 1e9);
    }
};

#endif /* UTIL_HPP */

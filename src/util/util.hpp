/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef UTIL_HPP
#define UTIL_HPP

#include <functional>
#include <string_view>
#include <unordered_map>
#include <ranges>
#include <algorithm>
#include <memory>

namespace util {
    namespace detail {
        struct string_hasher {
            using hash_type = std::hash<std::string_view>;
            using is_transparent = void;

            template <std::ranges::sized_range T>
            std::size_t operator()(const T& str) const { return hash_type {}({ std::begin(str), std::end(str) }); }
            std::size_t operator()(const char* str) const { return hash_type {}(str); }
            std::size_t operator()(std::string_view str) const { return hash_type {}(str); }
            std::size_t operator()(const std::string& str) const { return hash_type {}(str); }
        };

        struct range_eq : std::equal_to<> {
            template <typename T1, typename T2> requires std::ranges::sized_range<T1> || std::ranges::sized_range<T2>
            [[nodiscard]] constexpr auto operator()(T1 && lhs, T2 && rhs) const
                noexcept(noexcept(std::ranges::equal(std::forward<T1>(lhs), std::forward<T2>(rhs))))
                -> decltype(std::ranges::equal(std::forward<T1>(lhs), std::forward<T2>(rhs))) {
                return std::ranges::equal(std::forward<T1>(lhs), std::forward<T2>(rhs));
            }
        };
    }

    template <typename T, typename Allocator = std::allocator<std::pair<const std::string, T>>>
    using unordered_string_map = std::unordered_map<std::string, T, detail::string_hasher, detail::range_eq, Allocator>;

    template <typename T>
    concept istream_extractable = requires (T t, std::istream & is) {
        { is >> t } -> std::convertible_to<std::istream&>;
    };
}

#endif /* UTIL_HPP */

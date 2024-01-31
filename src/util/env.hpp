/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ENV_HPP
#define ENV_HPP

#include <unordered_map>
#include <string>
#include <string_view>
#include <sstream>
#include <stdexcept>
#include <format>

#include <tclap/ValueArg.h>

#include "util.hpp"
#include "file_exists_constraint.hpp"

namespace util {
    class environment {
        environment() = default;

        public:
        [[nodiscard]] static TCLAP::ValueArg<std::filesystem::path>& arg();

        /* Get string value */
        static [[nodiscard]] std::string_view get(const std::string& key);

        /* Get typed value */
        template <istream_extractable T>
        [[nodiscard]] static auto get(const std::string& key) {
            if constexpr (std::convertible_to<T, std::string_view>) {
                return T { get(key) };
            } else {
                std::stringstream ss { std::string { get(key) }};

                T res;
                ss >> res;

                if (!ss) {
                    throw std::invalid_argument { std::format("Couldn't parse environ: {}={}", key, get(key)) };
                }

                return res;
            }
        }

        /* Whether environ is present */
        static [[nodiscard]] bool contains(const std::string& key);

        static void parse();
    };
}

#endif /* ENV_HPP */

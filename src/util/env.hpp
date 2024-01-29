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
        TCLAP::ValueArg<std::filesystem::path> _arg;
        std::filesystem::file_time_type _update_time;

        public:
        environment();

        operator TCLAP::ValueArg<std::filesystem::path>& ();

        [[nodiscard]] std::string_view operator[](const std::string& key) const;

        template <istream_extractable T>
        [[nodiscard]] auto get(this const environment& self, const std::string& key) {
            if constexpr (std::convertible_to<T, std::string_view>) {
                return T { self[key] };
            } else {
                std::stringstream ss { std::string { self[key] } };

                T res;
                ss >> res;

                if (!ss) {
                    throw std::invalid_argument { std::format("Couldn't parse environ: {}={}", key, self[key]) };
                }

                return res;
            }
        }

        [[nodiscard]] bool contains(const std::string& key) const;

        private:
        void _read_env();
    };
}

#endif /* ENV_HPP */

/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "env.hpp"

#include <fstream>
#include <system_error>

namespace util::detail {
    static util::file_exists_constraint<std::filesystem::path> file_exists { "DOTENV" };
    static TCLAP::ValueArg<std::filesystem::path> arg {
        "e", "env",
        ".env file (default: ./.env)",
        false,
        "./.env",
        &file_exists
    };

    static std::filesystem::file_time_type last_update_time;

    static void read_env() {
        const std::filesystem::path& path = arg.getValue();
        if (!exists(path)) {
            /* Default value doensn't have to exist */
            if (arg.isSet()) {
                /* Was set, doesn't exist, panic! */
                throw std::filesystem::filesystem_error {
                    std::format("environment file \"{}\" not found", path.string()),
                    std::make_error_code(std::errc::no_such_file_or_directory)
                };
            }

            /* Ignore */

        } else  if (auto update_time = last_write_time(path); update_time > last_update_time) {
            last_update_time = update_time;

            std::ifstream in { path };
            if (!in) {
                throw std::filesystem::filesystem_error {
                    std::format("failed to open environment file \"{}\"", path.string()),
                    std::make_error_code(std::errc::io_error)
                };
            }

            int err;
            for (std::string line; std::getline(in, line);) {
                if ((err = putenv(line.data())) != 0) {
                    throw std::invalid_argument { std::format("Invalid environment string: {}", line) };
                }
            }
        }
    }

}

TCLAP::ValueArg<std::filesystem::path>& util::environment::arg() {
    return detail::arg;
}

std::string_view util::environment::get(const std::string& key) {
    detail::read_env();

    if (const char* val = std::getenv(key.c_str())) {
        return val;
    }

    throw std::out_of_range { std::format("environ not found: {}", key) };
}

std::string_view util::environment::get_or_default(const std::string& key, std::string_view def) {
    detail::read_env();

    if (const char* val = std::getenv(key.c_str())) {
        return val;
    }

    return def;
}

bool util::environment::contains(const std::string& key) {
    detail::read_env();
    return std::getenv(key.c_str()) != 0;
}

void util::environment::parse() {
    detail::read_env();
}

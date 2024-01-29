/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "env.hpp"

#include <fstream>
#include <system_error>

namespace {
    static util::file_exists_constraint<std::filesystem::path> file_exists { "DOTENV" };
}

util::environment::environment()
    : _arg {
        "e", "env",
        ".env file (default: ./.env)",
        false,
        "./.env",
        &file_exists
    } {
    
}

util::environment::operator TCLAP::ValueArg<std::filesystem::path>& () {
    _read_env();
    return _arg;
}

std::string_view util::environment::operator[](const std::string& key) const {
    if (const char* val = std::getenv(key.c_str())) {
        return val;
    }

    throw std::out_of_range { std::format("environ not found: {}", key) };
}

bool util::environment::contains(const std::string& key) const {
    return std::getenv(key.c_str()) != 0;
}

void util::environment::_read_env() {
    const std::filesystem::path& path = _arg.getValue();
    if (!exists(path)) {
        /* Default value doensn't have to exist */
        if (_arg.isSet()) {
            /* Was set, doesn't exist, panic! */
            throw std::filesystem::filesystem_error {
                std::format("environment file \"{}\" not found", path.string()),
                std::make_error_code(std::errc::no_such_file_or_directory)
            };
        }

        /* Ignore */

    } else  if (auto update_time = last_write_time(path); update_time > _update_time) {
        _update_time = update_time;

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

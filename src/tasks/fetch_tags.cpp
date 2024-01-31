/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fetch_tags.hpp"

#include <array>

#include <logging.hpp>

void tasks::fetch_tags::execute(std::stop_token token, util::rate_limit& rl) {
    static std::array<std::string_view, 3> str { "foo", "bar", "baz" };
    for (size_t i = 0; i < 3 && !token.stop_requested(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        util::log.info("{}", str[i]);
    }
}

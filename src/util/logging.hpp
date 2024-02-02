/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <spdlog/spdlog.h>

namespace util {
    namespace detail {
        [[nodiscard]] spdlog::logger make_logger();
    }

    static spdlog::logger log = detail::make_logger();
}

#endif /* LOGGING_HPP */


# SPDX-License-Identifier: GPL-3.0-or-later
add_library(util STATIC
    "util.hpp" "util.cpp"
    "env.hpp" "env.cpp"
    "file_exists_constraint.hpp"
    "logging.hpp" "logging.cpp"
    "rate_limit.hpp" "rate_limit.cpp")
setup_target(TARGET util LIBRARIES spdlog::spdlog)

target_include_directories(util PUBLIC "${CMAKE_SOURCE_DIR}/src/util")

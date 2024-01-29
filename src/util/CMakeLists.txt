# SPDX-License-Identifier: GPL-3.0-or-later
add_library(util STATIC
    "util.hpp" "util.cpp"
    "env.hpp" "env.cpp"
    "file_exists_constraint.hpp"
)
setup_target(TARGET util)

target_include_directories(util PUBLIC "${CMAKE_SOURCE_DIR}/src/util")

# Fix for now
set_target_properties(util PROPERTIES LINKER_LANGUAGE CXX)

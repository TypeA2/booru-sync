# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory("util")
add_subdirectory("tools")

add_executable(booru_sync
    "booru_sync.cpp"
    "perpetual_task.hpp" "perpetual_task.cpp"
    "danbooru.hpp" "danbooru.cpp"
    "database.hpp" "danbooru_defs.hpp" "database.cpp"
    "tasks/fetch_posts.hpp" "tasks/fetch_posts.cpp"
)

setup_target(TARGET booru_sync LIBRARIES
    util
    libpqxx::pqxx
    spdlog::spdlog
    cpr::cpr
    magic_enum::magic_enum
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

target_compile_definitions(booru_sync PRIVATE JSON_DISABLE_ENUM_SERIALIZATION=1)

target_include_directories(booru_sync PRIVATE ".")

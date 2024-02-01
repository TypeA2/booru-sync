/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <string>
#include <chrono>

/* Too new of a feature, vcpkg doesn't build libpqxx with it */
#undef __cpp_lib_source_location
#include <pqxx/pqxx>

using timestamp = std::chrono::utc_clock::time_point;

enum class tag_category : uint8_t {
    general   = 0,
    artist    = 1,
    copyright = 3,
    character = 4,
    meta      = 5,
};

enum class post_rating : uint8_t {
    g,
    s,
    q,
    e,
};

enum class category : uint8_t {
    series,
    collection,
};

struct tag {
    int32_t id;
    std::string name;
    int32_t post_count;
    tag_category category;
    bool is_deprecated;
    timestamp created_at;
    timestamp updated_at;
};

class database {
    pqxx::connection _conn;

    public:
    database();

    pqxx::work work();

    /* Behave like a pqxx::connection */
    operator pqxx::connection& ();
    pqxx::connection* operator->();
};

#endif /* DATABASE_HPP */

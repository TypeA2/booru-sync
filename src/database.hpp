/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <string>
#include <chrono>

/* Too new of a feature, vcpkg doesn't build libpqxx with it */
#undef __cpp_lib_source_location
#include <pqxx/pqxx>

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

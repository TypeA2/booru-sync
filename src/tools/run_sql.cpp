/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <iostream>
#include <format>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <tclap/CmdLine.h>
#include <pqxx/pqxx>

#include <env.hpp>

int main(int argc, char** argv) {
    try {
        util::environment env;

        TCLAP::CmdLine cmd { "First-time database setup" };

        util::file_exists_constraint<std::filesystem::path> sql_exists { "PATH" };
        TCLAP::UnlabeledValueArg<std::filesystem::path> sql_path {
            "input.sql", ".sql source file",
            true, "dummy",
            &sql_exists
        };

        cmd.add(env);
        cmd.add(sql_path);
        cmd.parse(argc, argv);

        /* Load and execute SQL input file */

        std::ifstream input { sql_path.getValue() };
        std::stringstream sql_source;
        sql_source << input.rdbuf();

        pqxx::connection conn;
        pqxx::work work { conn };

        work.exec(sql_source.str());

        work.commit();

    } catch (const std::exception& e) {
        std::println(std::cerr, "Exception: {}", e.what());
        return EXIT_FAILURE;
    }
}

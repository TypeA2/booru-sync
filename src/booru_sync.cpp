/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <iostream>
#include <format>
#include <stdexcept>
#include <memory>
#include <csignal>
#include <atomic>

#include <tclap/CmdLine.h>
#include <pqxx/pqxx>

#include <env.hpp>
#include <logging.hpp>

#include "danbooru.hpp"
#include "database.hpp"

#include "tasks/fetch_tags.hpp"

static std::atomic_flag signal_flag = ATOMIC_FLAG_INIT;

static void signal_handler(int signal) {
    signal_flag.test_and_set();
    signal_flag.notify_all();
}

int main(int argc, char** argv) {
    try {
        TCLAP::CmdLine cmd { "Fetch tag records from server" };

        cmd.add(util::environment::arg());
        cmd.parse(argc, argv);
        util::environment::parse();

        if (!util::environment::contains("DANBOORU_LOGIN")) {
            std::println(std::cerr, "DANBOORU_LOGIN not set");
            return EXIT_FAILURE;
        }

        if (!util::environment::contains("DANBOORU_API_KEY")) {
            std::println(std::cerr, "DANBOORU_API_KEY not set");
            return EXIT_FAILURE;
        }

        danbooru booru;
        database db;

        std::array<std::unique_ptr<perpetual_task>, 1> tasks {
            std::make_unique<tasks::fetch_tags>(
                "fetch_tags", std::chrono::minutes(5), perpetual_task::timing_mode::per_invocation,
                booru, db
            ),
        };

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        for (const auto& task : tasks) {
            task->start();
        }

        signal_flag.wait(false);

        util::log.info("Signal received, closing tasks");

        for (const auto& task : tasks) {
            task->request_stop();
        }

        for (const auto& task : tasks) {
            task->join();
        }

    } catch (const std::exception& e) {
        std::println(std::cerr, "Exception: {}", e.what());
        return EXIT_FAILURE;
    }
}

/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "perpetual_task.hpp"

#include <mutex>
#include <condition_variable>
#include <csignal>

#include <logging.hpp>
#include <util.hpp>

perpetual_task::perpetual_task(std::string_view id, duration interval, timing_mode mode)
    : _id { id }, _interval { interval }, _mode { mode } {

}

void perpetual_task::start() {
    _task = std::jthread([this](std::stop_token token) {
        std::mutex mut;

        try {
            /* Run until a stop is requested */
            for (; !token.stop_requested();) {
                spdlog::info("[{}] Running", _id);

                auto begin = clock::now();
                this->execute(token);
                auto end = clock::now();
                auto elapsed = end - begin;

                /* Exit immediately if stop requested */
                if (token.stop_requested()) {
                    break;
                }

                /* Adjust target wake time */
                auto next_wake = end + _interval;
                if (_mode == timing_mode::per_invocation) {
                    next_wake -= elapsed;
                }

                auto until_next_wake = next_wake - clock::now();
                if (until_next_wake.count() < 0) {
                    spdlog::info("[{}] Re-running immediately", _id);
                } else {
                    spdlog::info("[{}] finished in {}, next run in {}", _id, elapsed, until_next_wake);
                }

                /* Sleep until next wake, or until stop requested */
                std::unique_lock lock { mut };
                std::condition_variable_any().wait_until(lock, token, next_wake, [] { return false; });
            }

            spdlog::info("[{}] Stop requested", _id);
        } catch (const std::exception& e) {
            spdlog::error("Exception: {}", e.what());

            if (std::raise(SIGINT)) {
                spdlog::error("Failed to raise SIGINT, exiting");
                std::exit(EXIT_FAILURE);
            }

            return;
        }
    });
}

void perpetual_task::request_stop() {
    if (running()) {
        _task.request_stop();
    }
}

void perpetual_task::join() {
    _task.join();
}

bool perpetual_task::running() const {
    return _task.joinable();
}

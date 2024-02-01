/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "perpetual_task.hpp"

#include <mutex>
#include <condition_variable>

#include <logging.hpp>
#include <util.hpp>

perpetual_task::perpetual_task(std::string_view id, duration interval, timing_mode mode)
    : _id { id }, _interval { interval }, _mode { mode } {

}

void perpetual_task::start() {
    _task = std::jthread([this](std::stop_token token) {
        std::mutex mut;

        /* Run until a stop is requested */
        for (; !token.stop_requested();) {
            util::log.info("[{}] Running", _id);

            auto begin = clock::now();
            this->execute(token);
            auto end = clock::now();
            auto elapsed = end - begin;

            /* Adjust target wake time */
            auto next_wake = end + _interval;
            if (_mode == timing_mode::per_invocation) {
                next_wake -= elapsed;
            }

            util::log.info("[{}] finished in {}, next run in {}", _id, elapsed, next_wake - clock::now());

            /* Sleep until next wake, or until stop requested */
            std::unique_lock lock { mut };
            std::condition_variable_any().wait_until(lock, token, next_wake, [] { return false; });
        }

        util::log.info("[{}] Stop requested", _id);
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

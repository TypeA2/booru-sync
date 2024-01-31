/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PERPETUAL_TASK_HPP
#define PERPETUAL_TASK_HPP

#include <chrono>
#include <thread>
#include <tuple>

class perpetual_task {
    public:
    using clock = std::chrono::steady_clock;
    using duration = clock::duration;

    enum class timing_mode {
        /* Subtract task runtime from total sleep time */
        per_invocation,

        /* Always sleep a fixed duration */
        after_run,
    };

    virtual ~perpetual_task() = default;

    perpetual_task(std::string_view id, duration interval, timing_mode mode);

    void start();
    void request_stop();
    void join();

    [[nodiscard]] bool running() const;

    protected:
    virtual void execute(std::stop_token token) = 0;

    private:
    std::string _id;
    duration _interval;
    timing_mode _mode;
    std::jthread _task;
};

template <typename... Args>
class shared_resource_task : public perpetual_task {
    std::tuple<Args...> _params;

    public:
    shared_resource_task(std::string_view id, duration interval, timing_mode mode, Args... args)
        : perpetual_task { id, interval, mode }
        , _params { args... } { }

    protected:
    void execute(std::stop_token token) final {
        std::apply([this, token](Args... args) {
            this->execute(token, args...);
        }, _params);
    }

    virtual void execute(std::stop_token token, Args... args) = 0;
};

#endif /* PERPETUAL_TASK_HPP */

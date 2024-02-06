/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PERPETUAL_TASK_HPP
#define PERPETUAL_TASK_HPP

#include <chrono>
#include <thread>
#include <tuple>
#include <functional>

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

template <typename Store, typename Invoke = Store&>
struct store_invoke_resource {
    using store = Store;
    using invoke = Invoke;
};

template <typename T>
struct shared_resource_store {
    using type = T;
};

template <typename Store, typename Invoke>
struct shared_resource_store<store_invoke_resource<Store, Invoke>> {
    using type = Store;
};

template <typename T>
using shared_resource_store_t = shared_resource_store<T>::type;

template <typename T>
struct shared_resource_invoke {
    using type = T;
};

template <typename Store, typename Invoke>
struct shared_resource_invoke<store_invoke_resource<Store, Invoke>> {
    using type = Invoke;
};

template <typename T>
using shared_resource_invoke_t = shared_resource_invoke<T>::type;

template <typename... Args>
class shared_resource_task : public perpetual_task {
    std::tuple<shared_resource_store_t<Args>...> _params;

    public:
    shared_resource_task(std::string_view id, duration interval, timing_mode mode, shared_resource_store_t<Args>&&... args)
        : perpetual_task { id, interval, mode }
        , _params { std::forward<shared_resource_store_t<Args>>(args)... } { }

    protected:
    void execute(std::stop_token token) final {
        std::apply([this, token](auto&&... args) {
            this->execute(token, std::forward<decltype(args)>(args)...);
        }, _params);

    }

    virtual void execute(std::stop_token token, shared_resource_invoke_t<Args>... args) = 0;
};

#endif /* PERPETUAL_TASK_HPP */

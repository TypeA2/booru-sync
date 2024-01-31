#ifndef RATE_LIMIT_HPP
#define RATE_LIMIT_HPP

#include <atomic>
#include <chrono>
#include <mutex>

namespace util {
    /* Token bucket rate limiter */
    class rate_limit {
        public:
        using clock_type = std::chrono::steady_clock;
        using duration = clock_type::duration;
        using time_point = clock_type::time_point;

        private:
        size_t _bucket_size;
        duration _refill_delay;

        std::mutex _lock;
        size_t _bucket;
        time_point _last_refill;

        public:
        explicit rate_limit(size_t bucket_size, duration refill_delay);

        void acquire();
    };
}

#endif /* RATE_LIMIT_HPP */

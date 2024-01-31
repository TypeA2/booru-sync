#include "rate_limit.hpp"

util::rate_limit::rate_limit(size_t bucket_size, duration refill_delay)
    : _bucket_size { bucket_size }, _refill_delay { refill_delay }
    , _bucket { _bucket_size }, _last_refill { clock_type::now() } {

}

void util::rate_limit::acquire() {
    std::unique_lock lock { _lock };
    if (_bucket == 0) {
        /* Empty bucket, sleep until refill */
        duration elapsed = clock_type::now() - _last_refill;
        if (elapsed < _refill_delay) {
            std::this_thread::sleep_for(_refill_delay - elapsed);
        }

        _bucket = _bucket_size - 1;
        _last_refill = clock_type::now();
    } else {
        _bucket -= 1;
    }
}

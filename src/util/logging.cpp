#include "logging.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

static void flush_spdlog() {
    util::log.flush();
}

spdlog::logger util::detail::make_logger() {
    std::vector<spdlog::sink_ptr> sinks {
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("booru-sync.log", false),
    };

    spdlog::logger res { "booru-sync", sinks.begin(), sinks.end() };
    res.flush_on(spdlog::level::warn);
    res.set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%t] %v");

    atexit(flush_spdlog);

    return res;
}

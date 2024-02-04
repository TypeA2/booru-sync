#include "logging.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

static void flush_spdlog() {
    spdlog::default_logger()->flush();
}

void util::logging::setup() {
    std::vector<spdlog::sink_ptr> sinks {
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("booru-sync.log", false),
    };

    auto res = std::make_shared<spdlog::logger>("booru-sync", sinks.begin(), sinks.end());
    res->flush_on(spdlog::level::warn);
    res->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%t] %v");

    atexit(flush_spdlog);

    spdlog::set_default_logger(res);
}

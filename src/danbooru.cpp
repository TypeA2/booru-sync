/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "danbooru.hpp"

#include <sstream>
#include <chrono>
#include <ranges>
#include <algorithm>

#include <magic_enum.hpp>

#include <env.hpp>

using namespace danbooru;

tag tag::parse(const json& json) {
    return tag {
        .id            = json["id"],
        .name          = json["name"],
        .post_count    = json["post_count"],
        .category      = *magic_enum::enum_cast<tag_category>(json["category"].get<uint8_t>()),
        .is_deprecated = json["is_deprecated"],
        .created_at    = parse_timestamp(json["created_at"].get<std::string_view>()),
        .updated_at    = parse_timestamp(json["updated_at"].get<std::string_view>()),
    };
}

parameter::parameter(std::string key, json val) : key { key }, val { val } { }

std::string page_selector::str() const {
    std::stringstream ss;

    switch (pos) {
        case page_pos::before: ss << 'b'; break;
        case page_pos::after: ss << 'a'; break;
        default: break;
    }

    ss << value;
    return ss.str();
}

json page_selector::json() const {
    return *this;
}

page_selector::operator danbooru::json() const {
    return json::object({ { "page", str() } });
}

parameter page_selector::param() const {
    return *this;
}

page_selector::operator parameter() const {
    return { "page", str() };
}

page_selector page_selector::at(uint32_t value) {
    return { .pos = page_pos::absolute, .value = value };
}

page_selector page_selector::before(uint32_t value) {
    return { .pos = page_pos::before, .value = value };
}

page_selector page_selector::after(uint32_t value) {
    return { .pos = page_pos::after, .value = value };
}

timestamp danbooru::parse_timestamp(std::string_view ts) {
    std::string date{ ts };

    std::stringstream ss{ date };
    timestamp res;
    ss >> std::chrono::parse("%FT%T%Ez", res);

    return res;
}

std::string danbooru::format_timestamp(timestamp time) {
    return std::format(timestamp_format, std::chrono::time_point_cast<std::chrono::milliseconds>(time));
}

api::api()
    : _rl {
        util::environment::get_or_default<uint64_t>("DANBOORU_RATE_LIMIT", 10),
        std::chrono::seconds(1)
    }
    , _auth {
        util::environment::get<std::string>("DANBOORU_LOGIN"),
        util::environment::get<std::string>("DANBOORU_API_KEY"),
        cpr::AuthMode::BASIC
    }
    , _user_agent { std::format("hoshino.bot user {}", util::environment::get<std::string>("DANBOORU_LOGIN")) } {

    spdlog::info("Rate limit: {} / s", _rl.bucket_size());

    /* Verify login */
    auto res = fetch("profile", json::object({ { "only", "id,name,level" } })).get();

    if (!res.contains("id") || !res.contains("name")) {
        throw std::runtime_error { std::format("Failed to get user info: {}", res.dump())};
    }

    _user_id = res["id"];
    _user_name = res["name"];
    _level = *magic_enum::enum_cast<user_level>(res["level"].get<int32_t>());

    /* Be nice to evazion, tell him who we are */
    _user_agent = std::format("{} (#{})", _user_agent, _user_id);

    spdlog::info("Logging in as {} (user #{}), level: {}", _user_id, _user_id, magic_enum::enum_name(_level));
}

std::future<std::vector<tag>> api::tags(page_selector page, size_t limit) {
    if (limit > page_limit) {
        throw std::invalid_argument { std::format("limit of {} is too large (max: {})", limit, page_limit) };
    }

    return post<std::vector<tag>>("tags", { page, { "limit", std::to_string(limit) } },
        [](json response) -> std::vector<tag> {
            return response | std::views::transform(tag::parse) | std::ranges::to<std::vector>();
    });
}

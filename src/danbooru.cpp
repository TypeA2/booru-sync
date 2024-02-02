/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "danbooru.hpp"

#include <sstream>

#include <magic_enum.hpp>

#include <env.hpp>
#include <logging.hpp>

std::string danbooru::page_selector::str() const {
    std::stringstream ss;

    switch (pos) {
        case page_pos::before: ss << 'b'; break;
        case page_pos::after: ss << 'a'; break;
        default: break;
    }

    ss << value;
    return ss.str();
}

cpr::Parameter danbooru::page_selector::param() const {
    return { "page", str() };
}

danbooru::page_selector danbooru::page_selector::at(uint32_t value) {
    return { .pos = page_pos::absolute, .value = value };
}

danbooru::page_selector danbooru::page_selector::before(uint32_t value) {
    return { .pos = page_pos::before, .value = value };
}

danbooru::page_selector danbooru::page_selector::after(uint32_t value) {
    return { .pos = page_pos::after, .value = value };
}

danbooru::danbooru()
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

    util::log.info("Rate limit: {} / s", _rl.bucket_size());

    /* Verify login */
    auto res = get("profile", { "only", "id,name" });

    if (!res.contains("id") || !res.contains("name")) {
        throw std::runtime_error { std::format("Failed to get user info: {}", res.dump())};
    }

    _user_id = res["id"];
    _user_name = res["name"];

    /* Be nice to evazion, tell him who we are */
    _user_agent = std::format("{} (#{})", _user_agent, _user_id);

    util::log.info("Logging in as {} (user #{})", _user_id, _user_id);

    _tags.reserve(page_limit);
}

std::span<danbooru::tag> danbooru::tags(page_selector page, size_t limit) {
    return tags(_tags, page, limit);
}

std::span<danbooru::tag> danbooru::tags(std::vector<tag>& tags, page_selector page, size_t limit) {
    if (limit > page_limit) {
        throw std::invalid_argument { std::format("limit of {} is too large (max: {})", limit, page_limit) };
    }

    auto response = get("tags", { page.param(), { "limit", std::to_string(limit) } });
}

danbooru::json danbooru::get(std::string_view url, cpr::Parameter param) {
    return get(url, cpr::Parameters { param });
}

danbooru::json danbooru::get(std::string_view url, cpr::Parameters params) {
    cpr::Session ses;
    ses.SetAuth(_auth);
    ses.SetUrl(std::format("https://danbooru.donmai.us/{}.json", url));
    ses.SetParameters(params);
    ses.SetUserAgent(_user_agent);

    _rl.acquire();
    auto res = ses.Get();

    if (res.status_code >= 400) {
        throw std::runtime_error { std::format("GET {} - {}", res.status_code, ses.GetFullRequestUrl()) };
    }

    return json::parse(res.text);
}

/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "danbooru.hpp"

#include <env.hpp>
#include <logging.hpp>

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

    auto res = get("profile", { "only", "id,name" });

    if (!res.contains("id") || !res.contains("name")) {
        throw std::runtime_error { std::format("Failed to get user info: {}", res.dump())};
    }

    _user_id = res["id"];
    _user_name = res["name"];

    _user_agent = std::format("hoshino.bot user #{}", _user_id);

    util::log.info("Logging in as {} (user #{})", _user_id, _user_id);
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

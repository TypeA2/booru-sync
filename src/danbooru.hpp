/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DANBOORU_HPP
#define DANBOORU_HPP

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <rate_limit.hpp>

class danbooru {
    util::rate_limit _rl;

    cpr::Authentication _auth;
    int32_t _user_id;
    std::string _user_name;

    std::string _user_agent;

    public:
    danbooru();

    private:
    using json = nlohmann::json;

    [[nodiscard]] json get(std::string_view url, cpr::Parameter param);
    [[nodiscard]] json get(std::string_view url, cpr::Parameters params = {});
};

#endif /* DANBOORU_HPP */

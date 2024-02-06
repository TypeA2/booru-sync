/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DANBOORU_HPP
#define DANBOORU_HPP

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <logging.hpp>

#include <rate_limit.hpp>

namespace danbooru {
    using json = nlohmann::json;

    static constexpr size_t post_limit = 200;
    static constexpr size_t page_limit = 1000;
    using timestamp = std::chrono::utc_clock::time_point;

    enum class tag_category : uint8_t {
        general = 0,
        artist = 1,
        copyright = 3,
        character = 4,
        meta = 5,
    };

    enum class post_rating : uint8_t {
        g,
        s,
        q,
        e,
    };

    enum class pool_category : uint8_t {
        series,
        collection,
    };

    enum class user_level : uint8_t {
        anonymous   =  0,
        restricted  = 10,
        member      = 20,
        gold        = 30,
        platinum    = 31,
        builder     = 32,
        contributor = 35,
        approver    = 37,
        moderator   = 40,
        admin       = 50,
        owner       = 60,
    };

    struct tag {
        int32_t id;
        std::string name;
        int32_t post_count;
        tag_category category;
        bool is_deprecated;
        timestamp created_at;
        timestamp updated_at;

        [[nodiscard]] static tag parse(json& json);
    };

    enum class page_pos {
        absolute,
        before,
        after,
    };

    struct page_selector {
        page_pos pos;
        uint32_t value;

        [[nodiscard]] std::string str() const;
        [[nodiscard]] cpr::Parameter param() const;

        [[nodiscard]] static page_selector at(uint32_t value);
        [[nodiscard]] static page_selector before(uint32_t value);
        [[nodiscard]] static page_selector after(uint32_t value);
    };

    static constexpr char timestamp_format[] = "{:%FT%T%Ez}";
    static constexpr size_t timestamp_length = 30;

    [[nodiscard]] timestamp parse_timestamp(std::string_view ts);
    [[nodiscard]] std::string format_timestamp(timestamp time);

    template <typename T, typename Func>
    concept transform_func = requires (Func func, json args) {
        { func(args) } -> std::convertible_to<T>;
    };

    class api {
        util::rate_limit _rl;

        cpr::Authentication _auth;
        int32_t _user_id;
        std::string _user_name;
        user_level _level;

        std::string _user_agent;

        public:

        api();

        std::future<std::vector<tag>> tags(page_selector page, size_t limit = page_limit);

        private:
        [[nodiscard]] std::future<json> get(std::string_view url, cpr::Parameter param);

        template <typename T = json, typename Func = std::identity> requires transform_func<T, Func>
        [[nodiscard]] std::future<T> get(std::string_view url, cpr::Parameters params = {}, Func func = {}) {
            return std::async([this, params = std::move(params), func = std::move(func)](const cpr::Url& url) -> T {
                auto ses = std::make_shared<cpr::Session>();
                ses->SetAuth(_auth);
                ses->SetUrl(url);
                ses->SetParameters(params);
                ses->SetUserAgent(_user_agent);

                _rl.acquire();

                auto res = ses->Get();

                spdlog::trace("GET - {}", ses->GetFullRequestUrl());

                if (res.status_code >= 400) {
                    throw std::runtime_error { std::format("GET {} - {}", res.status_code, ses->GetFullRequestUrl()) };
                }

                return func(json::parse(res.text));
            }, std::format("https://danbooru.donmai.us/{}.json", url));
        }
    };
}

#endif /* DANBOORU_HPP */

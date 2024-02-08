/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DANBOORU_HPP
#define DANBOORU_HPP

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <magic_enum.hpp>

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

        [[nodiscard]] static tag parse(const json& json);
    };

    struct post {
        int32_t id;
        int32_t uploader_id;
        int32_t approver_id;
        std::vector<int32_t> tags;
        post_rating rating;
        int32_t parent;
        std::string source;
        std::array<char, 32> md5;
        int32_t media_asset;
        int32_t fav_count;
        bool has_children;
        int32_t up_score;
        int32_t down_score;
        bool is_pending;
        bool is_flagged;
        bool is_deleted;
        bool is_banned;
        int32_t pixiv_id;
        int32_t bit_flags;
        timestamp last_comment;
        timestamp last_bump;
        timestamp last_note;
        timestamp created_ad;
        timestamp updated_at;
    };

    struct parameter {
        std::string key;
        json val;

        parameter(std::string key, json val);
    };

    enum class request_type {
        get,
        post,
        get_as_post,
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

        [[nodiscard]] json json() const;
        operator danbooru::json() const;

        [[nodiscard]] parameter param() const;
        operator parameter() const;

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

        [[nodiscard]] std::future<std::vector<tag>> tags(page_selector page, size_t limit = page_limit);

        template <typename T = json, typename Func = std::identity>
        [[nodiscard]] std::future<T> get(std::string_view url, json params, Func&& func = {}) {
            return  request<request_type::get, T, Func>(url, std::move(params), std::forward<Func>(func));
        }

        template <typename T = json, typename Func = std::identity>
        [[nodiscard]] std::future<T> post(std::string_view url, json params, Func&& func = {}) {
            return  request<request_type::post, T, Func>(url, std::move(params), std::forward<Func>(func));
        }

        template <typename T = json, typename Func = std::identity>
        [[nodiscard]] std::future<T> fetch(std::string_view url, json params, Func&& func = {}) {
            return  request<request_type::get_as_post, T, Func>(url, std::move(params), std::forward<Func>(func));
        }

        template <request_type Req, typename T = json, typename Func = std::identity> requires transform_func<T, Func>
        [[nodiscard]] std::future<T> request(std::string_view url, json params = {}, Func&& func = {}) {
            return std::async([this](const cpr::Url& url, json params, Func func) -> T {
                auto ses = std::make_shared<cpr::Session>();
                ses->SetAuth(_auth);
                ses->SetUrl(url);
                ses->SetUserAgent(_user_agent);

                if constexpr (Req == request_type::get) {
                    cpr::Parameters res;
                    for (auto& [key, val] : params) {
                        res.Add(cpr::Parameter { key, val });
                    }

                    ses->SetParameters(res);
                } else {
                    if constexpr (Req == request_type::get_as_post) {
                        ses->SetHeader(cpr::Header {
                            { "Content-Type", "application/json" },
                            { "X-HTTP-Method-Override", "get" }
                            });
                    } else {
                        ses->SetHeader(cpr::Header {
                            { "Content-Type", "application/json" }
                        });
                    }

                    //spdlog::trace("Body:\n{}", params.dump());
                    ses->SetBody(params.dump());
                }

                _rl.acquire();

                cpr::Response res;

                if constexpr (Req == request_type::get) {
                    res = ses->Get();
                } else {
                    res = ses->Post();
                }

                spdlog::trace("{}: {} - {}", magic_enum::enum_name<Req>(), res.status_code, ses->GetFullRequestUrl());

                if (res.status_code >= 400) {
                    throw std::runtime_error {
                        std::format("{}: {} - {}\n{}", magic_enum::enum_name<Req>(), res.status_code, ses->GetFullRequestUrl(), res.text)
                    };
                }

                return func(json::parse(res.text));
            }, std::format("https://danbooru.donmai.us/{}.json", url), std::move(params), std::forward<Func>(func));
        }
    };
}

#endif /* DANBOORU_HPP */

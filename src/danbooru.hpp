/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DANBOORU_HPP
#define DANBOORU_HPP

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <magic_enum.hpp>

#include <logging.hpp>
#include <util.hpp>
#include <rate_limit.hpp>

#include "danbooru_defs.hpp"
#include "database.hpp"

namespace danbooru {
    struct page_selector {
        page_pos pos;
        uint32_t value;

        [[nodiscard]] std::string str() const;

        [[nodiscard]] json json() const;
        operator danbooru::json() const;

        [[nodiscard]] static page_selector at(uint32_t value);
        [[nodiscard]] static page_selector before(uint32_t value);
        [[nodiscard]] static page_selector after(uint32_t value);
    };

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
                static std::array backoff { 100, 250, 250, 500, 500, 500, 1000, 1000, 1000, 1000 };

                for (int32_t delay : backoff) {
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

                        std::string body = params.dump();
                        // spdlog::trace("Body: {}", body);
                        ses->SetBody(body);
                    }

                    _rl.acquire();

                    cpr::Response res;

                    auto begin = clock::now();

                    if constexpr (Req == request_type::get) {
                        res = ses->Get();
                    } else {
                        res = ses->Post();
                    }

                    std::chrono::nanoseconds elapsed = clock::now() - begin;

                    spdlog::trace("{}: {} - {} ({})",
                        magic_enum::enum_name<Req>(),
                        res.status_code,
                        ses->GetFullRequestUrl(),
                        elapsed
                    );

                    if (res.status_code == 0) {
                        spdlog::warn("cURL error {}", magic_enum::enum_name(res.error.code));
                        spdlog::warn("{} - {} ({})", magic_enum::enum_name<Req>(), ses->GetFullRequestUrl(), elapsed);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        continue;
                    }

                    if (res.status_code >= 400) {
                        throw std::runtime_error {
                            std::format("{}: {} - {}\n{}", magic_enum::enum_name<Req>(), res.status_code, ses->GetFullRequestUrl(), res.text)
                        };
                    }

                    try {
                        json j = json::parse(res.text);
                        return func(std::move(j));
                    } catch (const nlohmann::json::exception& e) {
                        spdlog::error("JSON exception: {}", e.what());
                        spdlog::error("{}: {} - {}", magic_enum::enum_name<Req>(), res.status_code, ses->GetFullRequestUrl());
                        if (res.error) {
                            spdlog::error("cURL error {} {}",
                                magic_enum::enum_name(res.error.code),
                                res.error.message.empty() ? "" : "- " + res.error.message);
                        }

                        if constexpr (Req == request_type::get) {
                            spdlog::error("Parameters:");
                            for (const auto& [key, val] : params) {
                                spdlog::error("    {} = {}", key, val);
                            }
                        } else {
                            spdlog::error("Body: {}", params.dump(4));
                        }

                        throw;
                    }
                }

                throw std::runtime_error { std::format("Failed to fetch after {} tries, aborting", backoff.size()) };
            }, std::format("https://danbooru.donmai.us/{}.json", url), std::move(params), std::forward<Func>(func));
        }
    };

    /* Fetch tag IDs and insert them into the databse */
    [[nodiscard]] std::future<util::unordered_string_map<int32_t>> fetch_and_insert_tags(
        api& booru, database::connection& db, const util::unordered_string_set& tags, database::insert_mode mode);
}

#endif /* DANBOORU_HPP */

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

    [[nodiscard]] static timestamp parse_timestamp(std::string_view ts);

    danbooru();

    [[nodiscard]] std::span<tag> tags(page_selector page, size_t limit = page_limit);
    std::span<tag> tags(std::vector<tag>& tags, page_selector page, size_t limit = page_limit);


    private:
    std::vector<tag> _tags;

    [[nodiscard]] json get(std::string_view url, cpr::Parameter param);
    [[nodiscard]] json get(std::string_view url, cpr::Parameters params = {});
};

#endif /* DANBOORU_HPP */

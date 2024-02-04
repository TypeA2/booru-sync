#include "database.hpp"

using namespace database;

namespace pqxx {
    zview string_traits<danbooru::timestamp>::to_buf(char* begin, char* end, const danbooru::timestamp& val) {
        char* new_end = into_buf(begin, end, val);

        if (new_end != end) {
            throw pqxx::conversion_overrun { std::format("failed to format timestamp: format size difference of {}", std::distance(end, new_end)) };
        }

        return zview { begin, end };
    }

    char* string_traits<danbooru::timestamp>::into_buf(char* begin, char* end, const danbooru::timestamp& val) {
        size_t buf_size = std::distance(begin, end);
        if (buf_size < danbooru::timestamp_length) {
            throw pqxx::conversion_overrun { std::format("failed to format timestamp: buffer size {} is too small", buf_size) };
        }

        auto res = std::format_to_n(begin, buf_size,
            danbooru::timestamp_format, std::chrono::time_point_cast<std::chrono::milliseconds>(val));

        *res.out = '\0';

        return res.out;
    }

    std::size_t string_traits<danbooru::timestamp>::size_buffer(const danbooru::timestamp& value) noexcept {
        /* "2024-01-01T00:00:00.000+00:00" */
        return danbooru::timestamp_length;
    }

    danbooru::timestamp string_traits<danbooru::timestamp>::from_string(std::string_view text) {
        return danbooru::parse_timestamp(text);
    }
}

table::table(instance& inst) : inst { inst } { }

tags::tags(instance& inst) : table { inst } {
    inst->prepare("tags_insert", "INSERT INTO tags VALUES ($1, $2, $3, $4, $5, $6, $7)");
}

int32_t tags::last() {
    auto lock = inst.lock();

    auto tx = inst.work();
    int32_t res = tx.query_value<int32_t>("SELECT COALESCE(MAX(id), 0) FROM tags");
    tx.commit();
    return res;
}

void tags::insert(const danbooru::tag& tag) {
    auto lock = inst.lock();
    auto tx = inst.work();

    tx.exec_prepared("tags_insert",
        tag.id, tag.name, tag.post_count, tag.category, tag.is_deprecated, tag.created_at, tag.updated_at);
    tx.commit();
}

void tags::insert(std::span<const danbooru::tag> tags) {
    auto lock = inst.lock();
    auto tx = inst.work();

    for (const danbooru::tag& tag : tags) {
        tx.exec_prepared("tags_insert",
            tag.id, tag.name, tag.post_count, tag.category, tag.is_deprecated, tag.created_at, tag.updated_at);
    }

    tx.commit();
}

pqxx::work instance::work() {
    return pqxx::work { _conn };
}

std::unique_lock<std::mutex> instance::lock() {
    return std::unique_lock { _mut };
}

instance::instance()
    : _tags { *this } {

}

instance::operator pqxx::connection& () {
    return _conn;
}

pqxx::connection* instance::operator->() {
    return &_conn;
}

tags& instance::tags() {
    return _tags;
}

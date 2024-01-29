/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FILE_EXISTS_CONSTRAINT_HPP
#define FILE_EXISTS_CONSTRAINT_HPP

#include <filesystem>

#include <tclap/Constraint.h>

namespace util {
    template <std::convertible_to<std::filesystem::path> T>
    class file_exists_constraint : public TCLAP::Constraint<T> {
        std::string _id;

        public:
        explicit file_exists_constraint(const std::string& id) : _id { id } { }

        std::string description() const override {
            return "Path does not exist";
        }

        std::string shortID() const override {
            return _id;
        }

        bool check(const T& value) const {
            return std::filesystem::exists(value);
        }
    };
}

#endif /* FILE_EXISTS_CONSTRAINT_HPP */

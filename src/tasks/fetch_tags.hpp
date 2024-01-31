/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FETCH_TAGS_HPP
#define FETCH_TAGS_HPP

#include <rate_limit.hpp>

#include "perpetual_task.hpp"

namespace tasks {
    class fetch_tags : public shared_resource_task<util::rate_limit&> {
        public:
        using shared_resource_task::shared_resource_task;

        protected:
        void execute(std::stop_token token, util::rate_limit& rl) override;
    };
}


#endif /* FETCH_TAGS_HPP */

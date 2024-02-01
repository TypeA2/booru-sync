/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FETCH_TAGS_HPP
#define FETCH_TAGS_HPP

#include "perpetual_task.hpp"

class danbooru;
class database;
namespace tasks {
    class fetch_tags : public shared_resource_task<danbooru&, database&> {
        public:
        using shared_resource_task::shared_resource_task;

        protected:
        void execute(std::stop_token token, danbooru& booru, database& db) override;
    };
}


#endif /* FETCH_TAGS_HPP */

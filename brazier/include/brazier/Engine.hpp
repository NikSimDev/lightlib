#pragma once

#include <boost/asio/io_context.hpp>
#include <memory>
#include "vendor/ConfigManager.hpp"

namespace brazier {
    class Engine {
    public:
        static boost::asio::io_context& get_io_context();
        static void init(boost::asio::io_context& io_ctx);

    private:
        static boost::asio::io_context*& io_ctx_ptr();
    };

    inline std::shared_ptr<brazier::ConfigManager> global_config = nullptr;
}
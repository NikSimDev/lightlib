/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of brazier.
 *
 * brazier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * brazier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with brazier; if not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/cancel_after.hpp>
#include <boost/config.hpp>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <cstring>
#include <ctime>
#include <deque>

#include "vendor/Handlers/ENV.hpp"
#include "Database/Queue.hpp"
#include "Database/Cache.hpp"
#include "Database/Migrations/MigrationManager.hpp"
#include "Router/RouterRegisterer.hpp"
#include "Router/Router.hpp"
#include "Engine.hpp"
#include "Filesystem/Filesystem.hpp"
#include "vendor/ConfigManager.hpp"

#include "../include/brazier/TLS/TicketKeyStore.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

namespace brazier {

    class HttpsServer {
    public:
        struct TlsConfig {
            std::string cert_file;
            std::string key_file;
            std::string ca_file;

            std::vector<std::pair<std::string, std::string>> conf;

            bool require_client_cert = false;
            bool verify_client_cert = false;

            std::chrono::seconds handshake_timeout{ 15 };
        };

    private:
        std::chrono::seconds      keep_alive_timeout_{ 60 };

        std::size_t max_body_size_ = 1024 * 1024;
        std::uint32_t max_header_size_ = 8 * 1024;
        int         max_connections_ = 10000;

        std::vector<std::unique_ptr<net::io_context>> io_contexts_;
        std::vector<std::unique_ptr<tcp::acceptor>>   acceptors_;
        std::vector<std::unique_ptr<
            net::executor_work_guard<net::io_context::executor_type>>> work_guards_;

        ssl::context ssl_ctx_{ ssl::context::tls_server };

        std::thread       stats_thread_;
        std::atomic<bool> shutdown_flag_{ false };

        std::mutex              stats_mutex_;
        std::condition_variable stats_cv_;

        std::atomic<bool>       shutting_down_{ false };
        std::mutex              shutdown_mutex_;
        std::condition_variable shutdown_cv_;

        unsigned short port_;
        std::string    host_;

        std::vector<std::thread> threads_;

        std::atomic<int> connection_count_{ 0 };
        std::atomic<int> total_requests_{ 0 };

        TlsConfig tls_;
        bool      tls_config_from_user_ = false;

        struct ConnectionGuard {
            HttpsServer& srv;
            explicit ConnectionGuard(HttpsServer& s) noexcept : srv(s) {}
            ~ConnectionGuard() { srv.release_connection(); }

            ConnectionGuard(const ConnectionGuard&) = delete;
            ConnectionGuard& operator=(const ConnectionGuard&) = delete;
        };

        brazier::TicketKeyStore ticket_store_;

    public:
        HttpsServer(const std::string& host, unsigned short port);
        HttpsServer(const std::string& host, unsigned short port,
            const TlsConfig& tls);

        void setTlsConfig(const TlsConfig& tls);

        bool initialize();

        void run();
        void stop();

        unsigned short     getPort() const;
        const std::string& getHost() const;

        int           getMaxConnections() const { return max_connections_; }
        std::size_t   getMaxBodySize()    const { return max_body_size_; }
        std::uint32_t getMaxHeaderSize()  const { return max_header_size_; }
        std::size_t   getIoContextCount() const { return io_contexts_.size(); }

    private:
        void initializeConnections();

        void load_tls_config_from_global();
        void load_common_config_from_global();
        void load_limits_from_config();

        void configure_tls();
        void apply_ssl_conf();

        void release_connection();

        static std::size_t get_fd_limit();
        static std::size_t get_system_memory_mb();
        static int         get_worker_count();

        static std::size_t   compute_max_body_size(std::size_t ram_mb, int max_conn);
        static std::uint32_t compute_max_header_size(std::size_t ram_mb, int max_conn);

        net::awaitable<void> handle_connection(tcp::socket socket);
        net::awaitable<void> accept_loop(tcp::acceptor& acceptor);
    };

}
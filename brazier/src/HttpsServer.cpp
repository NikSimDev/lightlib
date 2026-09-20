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

#include "../include/brazier/HttpsServer.hpp"

#include <optional>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/resource.h>
#  ifdef __APPLE__
#    include <sys/sysctl.h>
#    include <sys/types.h>
#  else
#    include <sys/sysinfo.h>
#  endif
#endif

brazier::HttpsServer::HttpsServer(const std::string& host, unsigned short port)
    : acceptor_(io_)
    , port_(port), host_(host) {
    work_guard_ = std::make_unique<
        net::executor_work_guard<net::io_context::executor_type>>(io_.get_executor());
}

brazier::HttpsServer::HttpsServer(const std::string& host, unsigned short port,
    const TlsConfig& tls)
    : acceptor_(io_)
    , port_(port), host_(host), tls_(tls), tls_config_from_user_(true) {
    work_guard_ = std::make_unique<
        net::executor_work_guard<net::io_context::executor_type>>(io_.get_executor());
}

void brazier::HttpsServer::setTlsConfig(const TlsConfig& tls) {
    tls_ = tls;
    tls_config_from_user_ = true;
}

std::size_t brazier::HttpsServer::get_fd_limit() {
#ifdef _WIN32
    return 16384;
#else
    struct rlimit rl;
    if (::getrlimit(RLIMIT_NOFILE, &rl) != 0) {
        return 1024;
    }
    if (rl.rlim_cur == RLIM_INFINITY) {
        if (rl.rlim_max == RLIM_INFINITY) {
            return 65536;
        }
        return static_cast<std::size_t>(rl.rlim_max);
    }
    return static_cast<std::size_t>(rl.rlim_cur);
#endif
}

std::size_t brazier::HttpsServer::get_system_memory_mb() {
#ifdef _WIN32
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (::GlobalMemoryStatusEx(&ms)) {
        return static_cast<std::size_t>(ms.ullTotalPhys / (1024 * 1024));
    }
    return 1024;
#elif defined(__APPLE__)
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    uint64_t memsize = 0;
    size_t len = sizeof(memsize);
    if (::sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0) {
        return static_cast<std::size_t>(memsize / (1024 * 1024));
    }
    return 1024;
#else
    struct sysinfo si;
    if (::sysinfo(&si) == 0) {
        return static_cast<std::size_t>(
            (static_cast<unsigned long long>(si.totalram) * si.mem_unit) /
            (1024 * 1024));
    }
    return 1024;
#endif
}

std::size_t brazier::HttpsServer::compute_max_body_size(
    std::size_t ram_mb, int max_conn) {
    constexpr double      kBodyRamBudget = 0.25;
    constexpr std::size_t kMinBody = 64 * 1024;          
    constexpr std::size_t kMaxBodyCap = 16 * 1024 * 1024;   

    if (max_conn <= 0) max_conn = 1;

    const std::size_t ram_bytes = ram_mb * 1024 * 1024;
    const std::size_t budget = static_cast<std::size_t>(ram_bytes * kBodyRamBudget);
    const std::size_t per_conn = budget / static_cast<std::size_t>(max_conn);

    std::size_t result = per_conn;
    if (result < kMinBody)    result = kMinBody;
    if (result > kMaxBodyCap) result = kMaxBodyCap;
    return result;
}

std::size_t brazier::HttpsServer::compute_max_header_size(
    std::size_t ram_mb, int max_conn) {
    constexpr double      kHeaderRamBudget = 0.01;
    constexpr std::size_t kMinHeader = 4 * 1024;        
    constexpr std::size_t kMaxHeaderCap = 32 * 1024;       

    if (max_conn <= 0) max_conn = 1;

    const std::size_t ram_bytes = ram_mb * 1024 * 1024;
    const std::size_t budget = static_cast<std::size_t>(ram_bytes * kHeaderRamBudget);
    const std::size_t per_conn = budget / static_cast<std::size_t>(max_conn);

    std::size_t result = per_conn;
    if (result < kMinHeader)    result = kMinHeader;
    if (result > kMaxHeaderCap) result = kMaxHeaderCap;
    return result;
}

void brazier::HttpsServer::load_tls_config_from_global() {
    if (tls_config_from_user_) return;

    keep_alive_timeout_ = std::chrono::seconds(
        global_config->get("keep-alive-timeout", 60));

    handshake_timeout_ms_ = std::chrono::milliseconds(
        global_config->get("https_server.tls.handshake_timeout_ms", 15000));

    tls_.cert_file = global_config->get("https_server.tls.cert_file",
        std::string("server.crt"));
    tls_.key_file = global_config->get("https_server.tls.key_file",
        std::string("server.key"));
    tls_.ca_file = global_config->get("https_server.tls.ca_file",
        std::string(""));

    tls_.require_client_cert =
        global_config->get("https_server.tls.require_client_cert", false);
    tls_.verify_client_cert =
        global_config->get("https_server.tls.verify_client_cert", false);

    tls_.handshake_timeout = std::chrono::seconds(
        global_config->get("https_server.tls.handshake_timeout", 15));

    try {
        json conf = global_config->getJson("https_server.tls.conf");
        if (conf.is_array()) {
            for (const auto& item : conf) {
                if (!item.is_array() || item.empty() || item.size() > 2) {
                    throw std::runtime_error(
                        "https_server.tls.conf: each entry must be [command] "
                        "or [command, value]");
                }
                std::string cmd = item[0].get<std::string>();
                std::string val = item.size() > 1 ? item[1].get<std::string>() : "";
                tls_.conf.emplace_back(std::move(cmd), std::move(val));
            }
        }
    }
    catch (const std::exception& e) {
        Logger::log("https_server.tls.conf not loaded: " + std::string(e.what()),
            "WARNING");
    }
}

void brazier::HttpsServer::load_limits_from_config() {
    const std::size_t ram_mb = get_system_memory_mb();

    const int testing_conn = global_config->get("http.max_connections_testing", 0);
    const int testing_body = global_config->get("http.max_body_size_testing", 0);
    const int testing_hdr = global_config->get("http.max_header_size_testing", 0);

    const bool testing_mode =
        testing_conn > 0 || testing_body > 0 || testing_hdr > 0;

    if (testing_conn > 0) {
        max_connections_ = testing_conn;
    }
    else if (int v = global_config->get("http.max_connections", 0); v > 0) {
        max_connections_ = v;
    }
    else {
        const int fd_limit = static_cast<int>(get_fd_limit());
        max_connections_ = static_cast<int>(fd_limit * 0.8);
    }

    if (testing_body > 0) {
        max_body_size_ = static_cast<std::size_t>(testing_body);
    }
    else if (int v = global_config->get("http.max_body_size", 0); v > 0) {
        max_body_size_ = static_cast<std::size_t>(v);
    }
    else {
        max_body_size_ = compute_max_body_size(ram_mb, max_connections_);
    }

    if (testing_hdr > 0) {
        max_header_size_ = static_cast<std::size_t>(testing_hdr);
    }
    else if (int v = global_config->get("http.max_header_size", 0); v > 0) {
        max_header_size_ = static_cast<std::size_t>(v);
    }
    else {
        max_header_size_ = compute_max_header_size(ram_mb, max_connections_);
    }

    Logger::log(
        std::string(testing_mode ? "[TESTING] " : "") +
        "Final HTTP limits: max_connections=" +
        std::to_string(max_connections_) +
        ", max_body=" + std::to_string(max_body_size_ / 1024) + "KB" +
        ", max_header=" + std::to_string(max_header_size_ / 1024) + "KB" +
        " (RAM=" + std::to_string(ram_mb) + "MB)",
        testing_mode ? "WARNING" : "INFO");
}

void brazier::HttpsServer::apply_ssl_conf() {
    if (tls_.conf.empty()) return;

    SSL_CONF_CTX* cctx = SSL_CONF_CTX_new();
    if (!cctx) {
        throw std::runtime_error("SSL_CONF_CTX_new failed");
    }

    SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_SERVER | SSL_CONF_FLAG_CERTIFICATE);
    SSL_CONF_CTX_set_ssl_ctx(cctx, ssl_ctx_.native_handle());

    for (const auto& [cmd, val] : tls_.conf) {
        int rv = val.empty()
            ? SSL_CONF_cmd(cctx, cmd.c_str(), nullptr)
            : SSL_CONF_cmd(cctx, cmd.c_str(), val.c_str());

        if (rv <= 0) {
            SSL_CONF_CTX_free(cctx);
            throw std::runtime_error(
                "SSL_CONF_cmd failed for '" + cmd +
                (val.empty() ? "'" : "=" + val + "'"));
        }
    }

    if (SSL_CONF_CTX_finish(cctx) != 1) {
        SSL_CONF_CTX_free(cctx);
        throw std::runtime_error("SSL_CONF_CTX_finish failed");
    }

    SSL_CONF_CTX_free(cctx);
}

void brazier::HttpsServer::configure_tls() {
    ssl_ctx_.set_options(
        ssl::context::default_workarounds
        | ssl::context::no_sslv2
        | ssl::context::no_sslv3
        | ssl::context::single_dh_use);

    SSL_CTX_set_options(ssl_ctx_.native_handle(), SSL_OP_IGNORE_UNEXPECTED_EOF);

    apply_ssl_conf();

    ssl_ctx_.use_certificate_chain_file(tls_.cert_file);
    ssl_ctx_.use_private_key_file(tls_.key_file, ssl::context::pem);

    if (tls_.require_client_cert || tls_.verify_client_cert) {
        if (!tls_.ca_file.empty()) {
            ssl_ctx_.load_verify_file(tls_.ca_file);
        }
        auto mode = ssl::verify_peer;
        if (tls_.require_client_cert) {
            mode |= ssl::verify_fail_if_no_peer_cert;
        }
        ssl_ctx_.set_verify_mode(mode);
    }
    else {
        ssl_ctx_.set_verify_mode(ssl::verify_none);
    }
}

bool brazier::HttpsServer::initialize() {
    try {
        Logger::init("debug.log");
        Logger::registerSignalHandlers();

        json drivers = global_config->getJson("filesystem.drivers");

        for (auto& [name, cfg] : drivers.items()) {
            if (name == "default") continue;

            auto driver = std::make_shared<brazier::FileDriver>();
            driver->setRootPath(cfg.value("root", "./"));
            driver->initAsync();

            StorageManager::getInstance().registerDriver(name, driver);
        }

        std::string def = global_config->getNested<std::string>("filesystem.default",
            "local");
        if (StorageManager::getInstance().hasDriver(def)) {
            StorageManager::getInstance().setDefaultDriver(def);
        }

        load_tls_config_from_global();
        load_limits_from_config();
        configure_tls();

        tcp::endpoint endpoint(net::ip::make_address(host_), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        initializeConnections();
        RouterRegisterer::init(io_);
        Engine::init(io_);

        Logger::log("HTTPS server initialized on " + host_ + ":" +
            std::to_string(port_) + " [TLS]", "SUCCESS");
        return true;
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS initialization failed: " + std::string(e.what()), "ERROR");
        return false;
    }
}

void brazier::HttpsServer::initializeConnections() {
    try {
        Queue::connect(global_config->get("nosql.host", "127.0.0.1"),
            global_config->get("nosql.port", 6379));
    }
    catch (const std::exception& e) {
        Logger::log("Connection to queue failed: " + std::string(e.what()), "ERROR");
    }

    try {
        Cache::connect(global_config->get("redis.host", "127.0.0.1"),
            global_config->get("redis.port", 6379));
    }
    catch (const std::exception& e) {
        Logger::log("Connection to NOSQL database failed: " + std::string(e.what()),
            "ERROR");
    }

    try {
        Database db;
        (new MigrationManager(db))->Initialize();
    }
    catch (const std::exception& e) {
        Logger::log("Database migration failed: " + std::string(e.what()), "ERROR");
    }
}

void brazier::HttpsServer::run() {
    try {
        net::co_spawn(io_, accept_loop(), net::detached);

        int threads_count = std::thread::hardware_concurrency();
        if (threads_count == 0) threads_count = 1;

        Logger::log("Starting " + std::to_string(threads_count) +
            " HTTPS worker threads", "INFO");

        for (int i = 0; i < threads_count; ++i) {
            threads_.emplace_back([this] { io_.run(); });
        }

        shutdown_flag_.store(false, std::memory_order_release);

        stats_thread_ = std::thread([this] {
            std::unique_lock<std::mutex> lock(stats_mutex_);
            while (!shutdown_flag_.load(std::memory_order_acquire)) {
                const bool woke = stats_cv_.wait_for(
                    lock,
                    std::chrono::seconds(10),
                    [this] {
                        return shutdown_flag_.load(std::memory_order_acquire);
                    });

                if (woke) break; 

                lock.unlock();
                Logger::log("HTTPS STATS - Active connections: " +
                    std::to_string(connection_count_.load()) +
                    ", Total requests: " + std::to_string(total_requests_.load()),
                    "INFO");
                lock.lock();
            }
            });

        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }

        if (stats_thread_.joinable()) stats_thread_.join();

        Logger::log("HTTPS server stopped", "INFO");
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS server run failed: " + std::string(e.what()), "ERROR");
        throw;
    }
}

void brazier::HttpsServer::stop() {
    Logger::log("HTTPS server stopping (graceful)...", "INFO");

    shutdown_flag_.store(true, std::memory_order_release);
    shutting_down_.store(true, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_cv_.notify_all();
    }

    {
        boost::system::error_code ignore;
        acceptor_.close(ignore);
    }

    work_guard_.reset();

    {
        std::unique_lock<std::mutex> lock(shutdown_mutex_);
        const bool drained = shutdown_cv_.wait_for(
            lock,
            std::chrono::seconds(10),
            [this] {
                return connection_count_.load(std::memory_order_acquire) == 0;
            });

        if (!drained) {
            Logger::log("Graceful shutdown timeout: " +
                std::to_string(connection_count_.load()) +
                " connections still active, forcing stop", "WARNING");
        }
    }

    io_.stop();

    Logger::log("HTTPS server stop() signaled", "INFO");
}

unsigned short brazier::HttpsServer::getPort() const { return port_; }
const std::string& brazier::HttpsServer::getHost() const { return host_; }

void brazier::HttpsServer::release_connection() {
    if (connection_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        std::lock_guard<std::mutex> lock(shutdown_mutex_);
        shutdown_cv_.notify_all();
    }
}

net::awaitable<void> brazier::HttpsServer::accept_loop() {
    for (;;) {
        if (shutting_down_.load(std::memory_order_acquire)) {
            co_return;
        }

        beast::error_code ec;
        tcp::socket socket = co_await acceptor_.async_accept(
            net::redirect_error(net::use_awaitable, ec));

        if (ec) {
            if (ec == net::error::operation_aborted ||
                shutting_down_.load(std::memory_order_acquire)) {
                co_return;
            }
            Logger::log("Accept error: " + ec.message(), "ERROR");
            continue;
        }

        const int prev = connection_count_.fetch_add(1, std::memory_order_acq_rel);
        if (prev >= max_connections_) {
            connection_count_.fetch_sub(1, std::memory_order_acq_rel);
            Logger::log("Connection limit reached (" +
                std::to_string(prev) + "/" +
                std::to_string(max_connections_) + "), rejecting", "WARNING");
            boost::system::error_code ignore;
            socket.close(ignore);
            continue;
        }

        net::co_spawn(io_, handle_connection(std::move(socket)), net::detached);
    }
}

net::awaitable<void> brazier::HttpsServer::handle_connection(tcp::socket socket) {
    ConnectionGuard guard(*this);

    try {
        socket.set_option(tcp::no_delay(true));
        socket.set_option(boost::asio::socket_base::keep_alive(true));

        ssl::stream<tcp::socket> stream(std::move(socket), ssl_ctx_);

        {
            net::steady_timer hs_timer(co_await net::this_coro::executor);
            hs_timer.expires_after(tls_.handshake_timeout);
            hs_timer.async_wait([&](beast::error_code ec) {
                if (!ec) {
                    beast::error_code ignore;
                    stream.next_layer().close(ignore);
                }
                });

            beast::error_code hs_ec;
            co_await stream.async_handshake(
                ssl::stream_base::server,
                net::redirect_error(net::use_awaitable, hs_ec));

            hs_timer.cancel();

            if (hs_ec) {
                Logger::log("TLS handshake failed: " + hs_ec.message(), "WARNING");
                co_return;
            }
        }

        std::optional<http::request_parser<http::string_body>> parser;

        http::response<http::string_body> res;
        beast::flat_buffer buffer;
        bool keep_alive = true;

        net::steady_timer idle_timer(co_await net::this_coro::executor);
        const auto IDLE_TIMEOUT = keep_alive_timeout_;
        bool timed_out = false;

        auto reset_timer = [&]() {
            idle_timer.expires_after(IDLE_TIMEOUT);
            idle_timer.async_wait([&](beast::error_code ec) {
                if (!ec) {
                    timed_out = true;
                    beast::error_code ignore;
                    stream.next_layer().close(ignore);
                }
                });
            };

        reset_timer();

        while (keep_alive && !timed_out) {
            parser.emplace();
            parser->body_limit(max_body_size_);
            parser->header_limit(static_cast<std::uint32_t>(max_header_size_));

            beast::error_code ec;

            co_await http::async_read(
                stream, buffer, *parser,
                net::redirect_error(net::use_awaitable, ec));

            if (ec == http::error::body_limit) {
                http::response<http::string_body> err{
                    http::status::payload_too_large, 11 };
                err.set(http::field::content_type, "text/plain");
                err.set(http::field::connection, "close");
                err.body() = "Payload too large";
                err.prepare_payload();

                beast::error_code write_ec;
                co_await http::async_write(
                    stream, err,
                    net::redirect_error(net::use_awaitable, write_ec));
                break;
            }

            if (ec == http::error::header_limit) {
                http::response<http::string_body> err{
                    http::status::request_header_fields_too_large, 11 };
                err.set(http::field::content_type, "text/plain");
                err.set(http::field::connection, "close");
                err.body() = "Header too large";
                err.prepare_payload();

                beast::error_code write_ec;
                co_await http::async_write(
                    stream, err,
                    net::redirect_error(net::use_awaitable, write_ec));
                break;
            }

            if (ec == http::error::end_of_stream) break;
            if (ec) break;

            reset_timer();

            http::request<http::string_body> req = parser->release();
            total_requests_.fetch_add(1, std::memory_order_relaxed);
            keep_alive = req.keep_alive();

            res = {};
            res.version(req.version());
            res.keep_alive(keep_alive);
            res.set(http::field::connection, keep_alive ? "keep-alive" : "close");
            res.set(http::field::server, "brazier");
            res.set(http::field::strict_transport_security, "max-age=31536000");

            try {
                co_await Router::handle_request(req, res);
            }
            catch (const std::exception& e) {
                Logger::log("Router error: " + std::string(e.what()), "ERROR");
                res.result(http::status::internal_server_error);
                res.set(http::field::content_type, "application/json");
                res.body() = R"({"error":"internal server error"})";
                keep_alive = false;
            }

            if (res.body().empty() && res.count(http::field::content_length) == 0) {
                res.content_length(0);
            }
            else if (!res.body().empty() && res.count(http::field::content_length) == 0) {
                res.content_length(res.body().size());
            }
            res.prepare_payload();

            ec.clear();
            co_await http::async_write(
                stream, res,
                net::redirect_error(net::use_awaitable, ec));

            if (ec) break;

            buffer.consume(buffer.size());
            if (!keep_alive) break;
        }

        idle_timer.cancel();

        {
            net::steady_timer sd_timer(co_await net::this_coro::executor);
            sd_timer.expires_after(std::chrono::seconds(2));
            sd_timer.async_wait([&](beast::error_code ec) {
                if (!ec) {
                    beast::error_code ignore;
                    stream.next_layer().close(ignore);
                }
                });

            beast::error_code sd_ec;
            co_await stream.async_shutdown(
                net::redirect_error(net::use_awaitable, sd_ec));

            sd_timer.cancel();

            if (sd_ec && sd_ec != net::error::eof
                && sd_ec != net::error::operation_aborted
                && sd_ec != ssl::error::stream_truncated
                && sd_ec != net::error::connection_reset
                && sd_ec != net::error::broken_pipe) {
            }
        }

        {
            beast::error_code ec;
            stream.next_layer().shutdown(tcp::socket::shutdown_both, ec);
            stream.next_layer().close(ec);
        }
    }
    catch (const boost::system::system_error& e) {
        auto code = e.code();
        if (code != net::error::connection_reset &&
            code != net::error::connection_aborted &&
            code != net::error::eof &&
            code != net::error::operation_aborted &&
            code != net::error::broken_pipe &&
            code != ssl::error::stream_truncated) {
            Logger::log("HTTPS connection error: " + std::string(e.what()), "ERROR");
        }
    }
    catch (const std::exception& e) {
        Logger::log("HTTPS connection error: " + std::string(e.what()), "ERROR");
    }
    catch (...) {
        Logger::log("Unknown HTTPS connection error", "ERROR");
    }

    co_return;
}
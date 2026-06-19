/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of lightlib.
 *
 * lightlib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * lightlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lightlib; if not, see <https://www.gnu.org/licenses/>.
 */

#include "../include/lightlib/App/Http/Helpers/HttpClient.hpp"
#include "../include/lightlib/vendor/Debug/Logger.hpp"

namespace lightlib {

    HttpClient::HttpClient() : ctx_(ssl::context::tlsv12_client) {
        Logger::log("HttpClient: Initializing SSL context", "DEBUG");
        ctx_.set_default_verify_paths();
        ctx_.set_verify_mode(ssl::verify_peer);
        Logger::log("HttpClient: SSL context initialized successfully", "DEBUG");
    }

    void HttpClient::set_verify_ssl(bool verify) {
        verify_ssl_ = verify;
        if (!verify_ssl_) {
            ctx_.set_verify_mode(ssl::verify_none);
            Logger::log("HttpClient: SSL verification DISABLED (for testing only!)", "WARNING");
        }
        else {
            ctx_.set_verify_mode(ssl::verify_peer);
            Logger::log("HttpClient: SSL verification ENABLED", "DEBUG");
        }
    }

    net::awaitable<Response> HttpClient::get(const std::string& url, const json& body) {
        Logger::log("HttpClient: GET request to " + url, "INFO");
        auto response = co_await send_request(url, http::verb::get, body);
        Logger::log("HttpClient: GET request completed with status " + std::to_string(response.result_int()), "DEBUG");
        co_return response;
    }

    net::awaitable<Response> HttpClient::post(const std::string& url, const json& body) {
        Logger::log("HttpClient: POST request to " + url, "INFO");
        Logger::log("HttpClient: POST body size: " + std::to_string(body.dump().size()) + " bytes", "DEBUG");
        auto response = co_await send_request(url, http::verb::post, body);
        Logger::log("HttpClient: POST request completed with status " + std::to_string(response.result_int()), "DEBUG");
        co_return response;
    }

    net::awaitable<Response> HttpClient::put(const std::string& url, const json& body) {
        Logger::log("HttpClient: PUT request to " + url, "INFO");
        Logger::log("HttpClient: PUT body size: " + std::to_string(body.dump().size()) + " bytes", "DEBUG");
        auto response = co_await send_request(url, http::verb::put, body);
        Logger::log("HttpClient: PUT request completed with status " + std::to_string(response.result_int()), "DEBUG");
        co_return response;
    }

    net::awaitable<Response> HttpClient::del(const std::string& url, const json& body) {
        Logger::log("HttpClient: DELETE request to " + url, "INFO");
        auto response = co_await send_request(url, http::verb::delete_, body);
        Logger::log("HttpClient: DELETE request completed with status " + std::to_string(response.result_int()), "DEBUG");
        co_return response;
    }

    HttpClient::UrlParts HttpClient::parse_url(const std::string& url) {
        Logger::log("HttpClient: Parsing URL: " + url, "DEBUG");
        UrlParts parts;

        std::regex url_regex(R"(^(https?)://([^:/]+)(?::([0-9]{1,5}))?(/[^?#]*)?(?:\?([^#]*))?(?:#.*)?$)");
        std::smatch matches;

        if (!std::regex_match(url, matches, url_regex)) {
            Logger::log("HttpClient: Invalid URL format: " + url, "ERROR");
            throw std::runtime_error("Invalid URL format: " + url);
        }

        parts.protocol = matches[1];
        parts.host = matches[2];
        parts.port = matches[3];
        parts.path = matches[4];
        parts.query = matches[5];

        if (parts.port.empty()) {
            parts.port = (parts.protocol == "https") ? "443" : "80";
        }

        if (parts.path.empty()) {
            parts.path = "/";
        }

        Logger::log("HttpClient: Parsed URL - protocol: " + parts.protocol +
            ", host: " + parts.host +
            ", port: " + parts.port +
            ", path: " + parts.path, "DEBUG");

        return parts;
    }

    net::awaitable<Response> HttpClient::send_request(const std::string& url, http::verb method, const json& body) {
        Logger::log("HttpClient: Sending request to " + url, "INFO");
        UrlParts url_parts = parse_url(url);
        bool use_ssl = (url_parts.protocol == "https");

        Logger::log("HttpClient: Using " + std::string(use_ssl ? "HTTPS" : "HTTP") + " protocol", "DEBUG");

        if (use_ssl) {
            co_return co_await send_https_request(url_parts, method, body);
        }
        else {
            co_return co_await send_http_request(url_parts, method, body);
        }
    }

    net::awaitable<Response> HttpClient::send_http_request(const UrlParts& url_parts, http::verb method, const json& body) {
        auto executor = co_await net::this_coro::executor;

        tcp::resolver resolver(executor);
        tcp::socket socket(executor);

        auto const results = co_await resolver.async_resolve(
            url_parts.host,
            url_parts.port,
            net::use_awaitable
        );

        if (results.empty()) {
            throw std::runtime_error("No DNS records found for " + url_parts.host);
        }

        bool timed_out = false;
        net::steady_timer timer(executor, timeout_);

        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                socket.close(ignore);
                Logger::log("HttpClient: Connection timeout", "WARNING");
            }
            });

        co_await socket.async_connect(*results.begin(), net::use_awaitable);
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("Connection timeout");
        }

        Logger::log("HttpClient: Connected successfully", "DEBUG");

        Request req{ method, url_parts.path, 11 };
        setup_common_headers(req, url_parts.host);

        if (method == http::verb::post || method == http::verb::put || method == http::verb::delete_) {
            req.set(http::field::content_type, "application/json");
            if (!body.empty()) {
                req.body() = body.dump();
            }
        }
        else if (method == http::verb::get && !body.empty()) {
            std::string query_string = json_to_query_string(body);
            if (!query_string.empty()) {
                req.target(url_parts.path + "?" + query_string);
            }
        }

        req.prepare_payload();

        timed_out = false;
        timer.expires_after(timeout_);
        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                socket.close(ignore);
                Logger::log("HttpClient: Write timeout", "WARNING");
            }
            });

        co_await http::async_write(socket, req, net::use_awaitable);
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("Write timeout");
        }

        Logger::log("HttpClient: Request sent, reading response", "DEBUG");

        timed_out = false;
        timer.expires_after(timeout_);
        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                socket.close(ignore);
                Logger::log("HttpClient: Read timeout", "WARNING");
            }
            });

        beast::flat_buffer buffer;
        Response res;
        co_await http::async_read(socket, buffer, res, net::use_awaitable);
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("Read timeout");
        }

        Logger::log("HttpClient: Response received with status " + std::to_string(res.result_int()) +
            ", body size: " + std::to_string(res.body().size()) + " bytes", "DEBUG");

        boost::system::error_code ignore;
        socket.shutdown(tcp::socket::shutdown_both, ignore);
        socket.close(ignore);

        co_return res;
    }

    net::awaitable<Response> HttpClient::send_https_request(const UrlParts& url_parts, http::verb method, const json& body) {
        auto executor = co_await net::this_coro::executor;

        beast::ssl_stream<beast::tcp_stream> stream(executor, ctx_);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), url_parts.host.c_str())) {
            boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
            throw boost::system::system_error(ec);
        }

        tcp::resolver resolver(executor);
        auto const results = co_await resolver.async_resolve(
            url_parts.host,
            url_parts.port,
            net::use_awaitable
        );

        if (results.empty()) {
            throw std::runtime_error("No DNS records found for " + url_parts.host);
        }

        bool timed_out = false;
        net::steady_timer timer(executor, timeout_);

        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                beast::get_lowest_layer(stream).socket().close(ignore);
                Logger::log("HttpClient: HTTPS connection timeout", "WARNING");
            }
            });

        boost::system::error_code ec;
        beast::get_lowest_layer(stream).connect(results, ec);
        if (ec) {
            throw std::runtime_error("Connection failed: " + ec.message());
        }
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("HTTPS connection timeout");
        }

        Logger::log("HttpClient: Starting SSL handshake", "DEBUG");

        timed_out = false;
        timer.expires_after(timeout_);
        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                beast::get_lowest_layer(stream).socket().close(ignore);
                Logger::log("HttpClient: SSL handshake timeout", "WARNING");
            }
            });

        co_await stream.async_handshake(ssl::stream_base::client, net::use_awaitable);
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("SSL handshake timeout");
        }

        Logger::log("HttpClient: SSL handshake completed successfully", "DEBUG");

        Request req{ method, url_parts.path, 11 };
        setup_common_headers(req, url_parts.host);

        if (method == http::verb::post || method == http::verb::put || method == http::verb::delete_) {
            req.set(http::field::content_type, "application/json");
            if (!body.empty()) {
                req.body() = body.dump();
            }
        }
        else if (method == http::verb::get && !body.empty()) {
            std::string query_string = json_to_query_string(body);
            if (!query_string.empty()) {
                req.target(url_parts.path + "?" + query_string);
            }
        }

        req.prepare_payload();

        timed_out = false;
        timer.expires_after(timeout_);
        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                beast::get_lowest_layer(stream).socket().close(ignore);
                Logger::log("HttpClient: HTTPS write timeout", "WARNING");
            }
            });

        co_await http::async_write(stream, req, net::use_awaitable);
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("HTTPS write timeout");
        }

        Logger::log("HttpClient: Request sent, reading response", "DEBUG");

        timed_out = false;
        timer.expires_after(timeout_);
        timer.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                timed_out = true;
                boost::system::error_code ignore;
                beast::get_lowest_layer(stream).socket().close(ignore);
                Logger::log("HttpClient: HTTPS read timeout", "WARNING");
            }
            });

        beast::flat_buffer buffer;
        Response res;
        co_await http::async_read(stream, buffer, res, net::use_awaitable);
        timer.cancel();

        if (timed_out) {
            throw std::runtime_error("HTTPS read timeout");
        }

        Logger::log("HttpClient: HTTPS response received with status " + std::to_string(res.result_int()), "DEBUG");

        stream.shutdown(ec);

        co_return res;
    }

    void HttpClient::setup_common_headers(Request& req, const std::string& host) {
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::accept, "*/*");
        req.set(http::field::connection, "close");
        Logger::log("HttpClient: Headers set for host: " + host, "DEBUG");
    }

    std::string HttpClient::json_to_query_string(const json& j) {
        std::string result;

        if (!j.is_object()) {
            Logger::log("HttpClient: JSON must be an object for query string conversion", "ERROR");
            throw std::runtime_error("JSON must be an object for query string conversion");
        }

        for (auto it = j.begin(); it != j.end(); ++it) {
            if (!result.empty()) {
                result += "&";
            }

            std::string key = encode_url(it.key());
            std::string value;

            if (it.value().is_string()) {
                value = encode_url(it.value().get<std::string>());
            }
            else {
                value = encode_url(it.value().dump());
            }

            result += key + "=" + value;
        }

        Logger::log("HttpClient: Generated query string length: " + std::to_string(result.size()), "DEBUG");
        return result;
    }

    std::string HttpClient::encode_url(const std::string& value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isalnum(uc) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            }
            else {
                escaped << std::uppercase;
                escaped << '%' << std::setw(2) << int(uc);
                escaped << std::nouppercase;
            }
        }

        return escaped.str();
    }

    void HttpClient::set_timeout(const std::chrono::milliseconds& timeout) {
        if (timeout.count() <= 0) {
            Logger::log("HttpClient: Invalid timeout value: " + std::to_string(timeout.count()) + "ms", "ERROR");
            throw std::invalid_argument("Timeout must be positive");
        }
        timeout_ = timeout;
        Logger::log("HttpClient: Timeout set to " + std::to_string(timeout.count()) + "ms", "INFO");
    }

    bool HttpClient::is_success(const Response& res) const {
        bool success = res.result() >= http::status::ok &&
            res.result() < http::status::multiple_choices;
        Logger::log("HttpClient: is_success(" + std::to_string(res.result_int()) + ") = " + (success ? "true" : "false"), "DEBUG");
        return success;
    }

} 
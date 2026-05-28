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

lightlib::HttpClient::HttpClient() : ctx_(ssl::context::tlsv12_client) {
    ctx_.set_default_verify_paths();
    ctx_.set_verify_mode(ssl::verify_peer);
}

lightlib::Response lightlib::HttpClient::get(const std::string& url, const json& body) {
    return send_request(url, http::verb::get, body);
}

lightlib::Response lightlib::HttpClient::post(const std::string& url, const json& body) {
    return send_request(url, http::verb::post, body);
}

lightlib::Response lightlib::HttpClient::put(const std::string& url, const json& body) {
    return send_request(url, http::verb::put, body);
}

lightlib::Response lightlib::HttpClient::del(const std::string& url, const json& body) {
    return send_request(url, http::verb::delete_, body);
}

lightlib::HttpClient::UrlParts lightlib::HttpClient::parse_url(const std::string & url) {
    UrlParts parts;

    std::regex url_regex(R"(^(https?)://([^:/]+)(?::([0-9]{1,5}))?(/[^?]*)?(?:\?(.*))?$)");
    std::smatch matches;

    if (std::regex_match(url, matches, url_regex)) {
        parts.protocol = matches[1];
        parts.host = matches[2];
        parts.port = matches[3];
        parts.path = matches[4];

        if (parts.port.empty()) {
            parts.port = (parts.protocol == "https") ? "443" : "80";
        }

        if (parts.path.empty()) {
            parts.path = "/";
        }
    }
    else {
        throw std::runtime_error("Invalid URL format: " + url);
    }

    return parts;
}

lightlib::Response lightlib::HttpClient::send_request(const std::string& url, http::verb method, const json& body) {
    try {
        UrlParts url_parts = parse_url(url);

        bool use_ssl = (url_parts.protocol == "https");

        if (use_ssl) {
            return lightlib::HttpClient::send_https_request(url_parts, method, body);
        }
        else {
            return send_http_request(url_parts, method, body);
        }

    }
    catch (const std::exception& e) {
        Response res{ http::status::internal_server_error, 11 };
        res.set(http::field::content_type, "text/plain");
        res.body() = std::string("Error: ") + e.what();
        res.prepare_payload();
        return res;
    }
}

lightlib::Response lightlib::HttpClient::send_http_request(const UrlParts& url_parts, http::verb method, const json& body) {
    net::io_context ioc;

    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);
    net::steady_timer timer(ioc, timeout_);

    auto const results = resolver.resolve(url_parts.host, url_parts.port);

    net::async_connect(socket, results,
        [&timer](boost::system::error_code ec, const tcp::endpoint&) {
            timer.cancel();
        });

    Request req{ method, url_parts.path, 11 };
    setup_common_headers(req, url_parts.host);

    if (method == http::verb::post || method == http::verb::put) {
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
    }
    else if (!body.empty()) {
        req.target(url_parts.path + "?" + json_to_query_string(body));
    }

    req.prepare_payload();
    http::write(socket, req);

    timer.async_wait([&socket](boost::system::error_code ec) {
        if (!ec) socket.close();
        });

    ioc.run();

    if (!socket.is_open()) {
        throw std::runtime_error("Connection timeout");
    }

    beast::flat_buffer buffer;
    Response res;
    http::read(socket, buffer, res);

    boost::system::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);

    return res;
}

lightlib::Response lightlib::HttpClient::send_https_request(const UrlParts& url_parts, http::verb method, const json& body) {
    net::io_context ioc;

    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx_);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), url_parts.host.c_str())) {
        beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
        throw beast::system_error{ ec };
    }

    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(url_parts.host, url_parts.port);

    beast::get_lowest_layer(stream).connect(results);

    stream.handshake(ssl::stream_base::client);

    Request req{ method, url_parts.path, 11 };
    setup_common_headers(req, url_parts.host);

    if (method == http::verb::post || method == http::verb::put) {
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
    }
    else if (!body.empty()) {
        req.target(url_parts.path + "?" + json_to_query_string(body));
    }

    req.prepare_payload();

    http::write(stream, req);

    beast::flat_buffer buffer;
    Response res;
    http::read(stream, buffer, res);

    beast::error_code ec;
    stream.shutdown(ec);

    return res;
}

void lightlib::HttpClient::setup_common_headers(Request& req, const std::string& host) {
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::accept, "*/*");
    req.set(http::field::connection, "close");
}

std::string lightlib::HttpClient::json_to_query_string(const json& j) {
    std::string result;

    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            if (!result.empty()) result += "&";
            result += encode_url(it.key()) + "=" + encode_url(it.value().dump());
        }
    }

    return result;
}

std::string lightlib::HttpClient::encode_url(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        }
        else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

void lightlib::HttpClient::set_timeout(const std::chrono::milliseconds& timeout) {
    timeout_ = timeout;
}

bool lightlib::HttpClient::is_success(const lightlib::Response& res) {
    return res.result() >= http::status::ok && res.result() < http::status::multiple_choices;
}
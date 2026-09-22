#include "../include/brazier/Http"

class BenchmarkController : public brazier::Controller {
public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    boost::asio::awaitable<void> test(const Request& req, Response& res, const Params& params) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "";
        co_return;
    }
};
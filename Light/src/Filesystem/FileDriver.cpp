#include "../../include/lightlib/Filesystem/FileDriver.hpp"

using namespace lightlib;

void FileDriver::initAsync(size_t threadCount) {
	if (!ioPool_) {
		ioPool_ = std::make_shared<boost::asio::thread_pool>(threadCount);
	}
}

void FileDriver::setRootPath(const std::string& path) {
	rootPath_ = path;
	if (!fs::exists(rootPath_)) {
		fs::create_directories(rootPath_);
	}
}

std::string FileDriver::getRootPath() const {
	return rootPath_;
}

void FileDriver::put(const std::string& path, const std::string& content) {
    std::string fullPath = rootPath_ + "/" + path;
    auto parentPath = std::filesystem::path(fullPath).parent_path();
    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);

    if (ec) {
        throw std::runtime_error("Cannot create directories for: " + path);
    }

    std::ofstream file(fullPath, std::ios::out);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }

    file << content;
}

std::string FileDriver::get(const std::string& path) {
    std::string fullPath = rootPath_ + "/" + path;
    std::ifstream file(fullPath, std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bool FileDriver::exists(const std::string& path) const {
    return fs::exists(rootPath_ + "/" + path);
}

void FileDriver::deleteFile(const std::string& path) {
    if (!exists(path)) {
        throw std::runtime_error("File does not exist: " + path);
    }
    fs::remove(rootPath_ + "/" + path);
}

void FileDriver::copy(const std::string& source, const std::string& destination) {
    if (!exists(source)) {
        throw std::runtime_error("Source file does not exist: " + source);
    }
    fs::copy(rootPath_ + "/" + source, rootPath_ + "/" + destination);
}

void FileDriver::move(const std::string& source, const std::string& destination) {
    if (!exists(source)) {
        throw std::runtime_error("Source file does not exist: " + source);
    }

    fs::rename(rootPath_ + "/" + source, rootPath_ + "/" + destination);
}

boost::asio::awaitable<void> FileDriver::putAsync(const std::string& path, const std::string& content) {
    if (!ioPool_) {
        throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
    }

    auto executor = co_await boost::asio::this_coro::executor;

    co_await boost::asio::post(executor, boost::asio::use_awaitable);

    co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);

    try {
        put(path, content);
    }
    catch (const std::exception& e) {
        throw e;
    }

    co_return;
}

boost::asio::awaitable<std::string> FileDriver::getAsync(const std::string& path) {
    if (!ioPool_) {
        throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
    }

    co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);

    std::string result;
    try {
        result = get(path);
    }
    catch (const std::exception& e) {
        throw e;
    }

    co_return result;
}

boost::asio::awaitable<bool> FileDriver::existsAsync(const std::string& path) const {
    if (!ioPool_) {
        throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
    }

    co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
    co_return exists(path);
}

boost::asio::awaitable<void> FileDriver::deleteFileAsync(const std::string& path) {
    if (!ioPool_) {
        throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
    }

    co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
    deleteFile(path);
    co_return;
}

boost::asio::awaitable<void> FileDriver::copyAsync(const std::string& source, const std::string& destination) {
    if (!ioPool_) {
        throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
    }

    co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
    copy(source, destination);
    co_return;
}

boost::asio::awaitable<void> FileDriver::moveAsync(const std::string& source, const std::string& destination) {
    if (!ioPool_) {
        throw std::runtime_error("Storage not initialized for async operations. Call initAsync() first.");
    }

    co_await boost::asio::post(*ioPool_, boost::asio::use_awaitable);
    move(source, destination);
    co_return;
}
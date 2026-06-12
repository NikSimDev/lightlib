#include "../../include/lightlib/Filesystem/Filesystem.hpp"

lightlib::Filesystem::Filesystem()
	: driver_(StorageManager::getInstance().getDefaultDriver())
	, driverName_("default")
{
	if (!driver_) {
		throw std::runtime_error("No default driver registered");
	}
}

lightlib::Filesystem::Filesystem(const std::string& name)
    : driver_(StorageManager::getInstance().getDriver(name))
    , driverName_(name)
{
    if (!driver_) {
        throw std::runtime_error("Driver not found: " + name);
    }
}

lightlib::Filesystem::Filesystem(std::shared_ptr<BaseDriver> driver)
    : driver_(driver)
    , driverName_("custom")
{
    if (!driver_) {
        throw std::runtime_error("Driver is null");
    }
}

lightlib::Filesystem lightlib::Filesystem::driver(const std::string& name) {
	return lightlib::Filesystem(name);
}

bool lightlib::Filesystem::hasDriver(const std::string& name) {
    return lightlib::StorageManager::getInstance().hasDriver(name);
}

std::string lightlib::Filesystem::getDriverName() const {
    return driverName_;
}

std::string lightlib::Filesystem::getDriverType() const {
    return driver_->getDriverType();
}

void lightlib::Filesystem::put(const std::string& path, const std::string& content) {
    driver_->put(path, content);
}

std::string lightlib::Filesystem::get(const std::string& path) {
    return driver_->get(path);
}

bool lightlib::Filesystem::exists(const std::string& path) const {
    return driver_->exists(path);
}

void lightlib::Filesystem::deleteFile(const std::string& path) {
    driver_->deleteFile(path);
}

void lightlib::Filesystem::copy(const std::string& source, const std::string& destination) {
    driver_->copy(source, destination);
}

void lightlib::Filesystem::move(const std::string& source, const std::string& destination) {
    driver_->move(source, destination);
}

boost::asio::awaitable<void> lightlib::Filesystem::putAsync(const std::string& path, const std::string& content) {
    co_await driver_->putAsync(path, content);
}

boost::asio::awaitable<std::string> lightlib::Filesystem::getAsync(const std::string& path) {
    co_return co_await driver_->getAsync(path);
}

boost::asio::awaitable<bool> lightlib::Filesystem::existsAsync(const std::string& path) const {
    co_return co_await driver_->existsAsync(path);
}

boost::asio::awaitable<void> lightlib::Filesystem::deleteFileAsync(const std::string& path) {
    co_await driver_->deleteFileAsync(path);
}

boost::asio::awaitable<void> lightlib::Filesystem::copyAsync(const std::string& source, const std::string& destination) {
    co_await driver_->copyAsync(source, destination);
}

boost::asio::awaitable<void> lightlib::Filesystem::moveAsync(const std::string& source, const std::string& destination) {
    co_await driver_->moveAsync(source, destination);
}
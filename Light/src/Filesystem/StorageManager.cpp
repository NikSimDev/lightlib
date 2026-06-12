#include "../../include/lightlib/Filesystem/StorageManager.hpp"

using namespace lightlib;

StorageManager& StorageManager::getInstance() {
	static StorageManager instance;
	return instance;
}

void StorageManager::registerDriver(const std::string& name, std::shared_ptr<BaseDriver> driver) {
	drivers_[name] = driver;
}

void StorageManager::unregisterDriver(const std::string& name) {
	drivers_.erase(name);
}

void StorageManager::setDefaultDriver(const std::string& name) {
	if (drivers_.find(name) == drivers_.end()) {
		throw std::runtime_error("Driver not found: " + name);
	}
	defaultDriverName_ = name;
}

std::shared_ptr<BaseDriver> StorageManager::getDefaultDriver() const {
	return getDriver(defaultDriverName_);
}

std::shared_ptr<BaseDriver> StorageManager::getDriver(const std::string& name) const {
	if (drivers_.find(name) == drivers_.end()) {
		throw std::runtime_error("Driver not found: " + name);
	}

	return drivers_.at(name);
}

bool StorageManager::hasDriver(const std::string& name) const {
	return drivers_.find(name) != drivers_.end();
}

std::vector<std::string> StorageManager::getDriverNames() const {
	std::vector<std::string> names;
	for (const auto& pair : drivers_) {
		names.push_back(pair.first);
	}
	return names;
}
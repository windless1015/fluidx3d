#include "ModelRegistry.h"

namespace lbm {

ModelRegistry& ModelRegistry::instance() {
	static ModelRegistry registry;
	return registry;
}

void ModelRegistry::registerFactory(const std::string& typeName, ModelFactory factory) {
	factories_[typeName] = std::move(factory);
}

bool ModelRegistry::hasFactory(const std::string& typeName) const {
	return factories_.find(typeName) != factories_.end();
}

std::vector<std::string> ModelRegistry::registeredTypes() const {
	std::vector<std::string> types;
	types.reserve(factories_.size());
	for(const auto& item : factories_) types.push_back(item.first);
	return types;
}

std::unique_ptr<IModel> ModelRegistry::create(const std::string& typeName, const Config& config) const {
	auto it = factories_.find(typeName);
	if(it == factories_.end()) {
		throw std::runtime_error("ModelRegistry: type not registered: " + typeName);
	}
	return it->second(config);
}

void ModelRegistry::Register(const std::string& typeName, ModelFactory factory) {
	ModelRegistry::instance().registerFactory(typeName, std::move(factory));
}

std::unique_ptr<IModel> ModelRegistry::Create(const std::string& typeName, const Config& config) {
	return ModelRegistry::instance().create(typeName, config);
}

} // namespace lbm

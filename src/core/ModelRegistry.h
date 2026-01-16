/*
 * ModelRegistry.h - Model Factory with Automatic Registration
 */

#pragma once

#include "IModel.h"
#include "Config.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <stdexcept>

namespace lbm {

using ModelFactory = std::function<std::unique_ptr<IModel>(const Config&)>;

class ModelRegistry {
public:
	static ModelRegistry& instance();

	ModelRegistry(const ModelRegistry&) = delete;
	ModelRegistry& operator=(const ModelRegistry&) = delete;

	void registerFactory(const std::string& typeName, ModelFactory factory);
	bool hasFactory(const std::string& typeName) const;
	std::vector<std::string> registeredTypes() const;
	std::unique_ptr<IModel> create(const std::string& typeName, const Config& config) const;

	static void Register(const std::string& typeName, ModelFactory factory);
	static std::unique_ptr<IModel> Create(const std::string& typeName, const Config& config);

private:
	ModelRegistry() = default;
	std::unordered_map<std::string, ModelFactory> factories_;
};

class ModelRegistrar {
public:
	ModelRegistrar(const std::string& typeName, ModelFactory factory) {
		ModelRegistry::Register(typeName, std::move(factory));
	}
};

#define REGISTER_MODEL(TypeName, ModelClass) \
	static ::lbm::ModelRegistrar _registrar_##ModelClass( \
		TypeName, \
		[](const ::lbm::Config& config) -> std::unique_ptr<::lbm::IModel> { \
			(void)config; \
			auto model = std::make_unique<ModelClass>(); \
			return model; \
		} \
	)

#define REGISTER_MODEL_FACTORY(TypeName, FactoryFunc) \
	static ::lbm::ModelRegistrar _registrar_##FactoryFunc( \
		TypeName, \
		FactoryFunc \
	)

} // namespace lbm

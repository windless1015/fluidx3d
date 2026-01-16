#pragma once

#include "Config.h"
#include "FieldHandle.h"
#include <string>
#include <vector>

namespace lbm {

using Real = float;

enum class ModelState {
	Uninitialized,
	Initialized,
	Running,
	Finalized
};

class IModel {
public:
	virtual ~IModel() = default;

	virtual void initialize(const ModelConfig& config) = 0;
	virtual void advance(Real dt) = 0;
	virtual void finalize() = 0;

	virtual const std::string& name() const = 0;
	virtual const std::string& type() const = 0;
	virtual ModelState state() const = 0;

	virtual bool hasField(const std::string& fieldName) const = 0;
	virtual FieldHandle getField(const std::string& fieldName) = 0;
	virtual std::vector<std::string> fieldNames() const = 0;

	virtual void preStep(Real dt) { (void)dt; }
	virtual void postStep(Real dt) { (void)dt; }
};

} // namespace lbm

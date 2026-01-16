#pragma once

#include "LBMConfig.h"
#include "LBMCore.h"
#include "FreeSurfaceModule.h"
#include <memory>

namespace lbm {

class LBMModel {
public:
	LBMModel()
		: useFreeSurface_(false) {}

	~LBMModel() = default;

	LBMModel(const LBMModel&) = delete;
	LBMModel& operator=(const LBMModel&) = delete;

	void configure(const LBMConfig& config) {
		config_ = config;
		core_ = std::make_unique<LBMCore>(config_);
		fsModule_ = std::make_unique<FreeSurfaceModule>();
	}

	void enableFreeSurface(bool enable) {
		useFreeSurface_ = enable;
		if(fsModule_ != nullptr) fsModule_->setEnabled(enable);
	}

	void initialize() {
		if(core_ != nullptr) core_->initialize();
		if(useFreeSurface_ && fsModule_ != nullptr) fsModule_->initialize(config_);
	}

	void step() {
		if(core_ != nullptr) core_->step();
	}

	LBMCore* core() { return core_.get(); }
	FreeSurfaceModule* freeSurface() { return fsModule_.get(); }

	bool isFreeSurfaceEnabled() const { return useFreeSurface_; }

private:
	LBMConfig config_;
	bool useFreeSurface_;

	std::unique_ptr<LBMCore> core_;
	std::unique_ptr<FreeSurfaceModule> fsModule_;
};

} // namespace lbm

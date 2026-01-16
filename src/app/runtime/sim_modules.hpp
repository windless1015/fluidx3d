#pragma once

#include "../../lbm_api/LBMModel.h"
#include <string>

struct ISimModule {
	virtual ~ISimModule() = default;
	virtual void on_init(lbm::LBMCore& core) {}
	virtual void on_step(lbm::LBMCore& core) {}
};

struct VTKModuleParams {
	bool enabled = false;
	unsigned int interval = 100u;
	std::string prefix = "output/dam_break_";
};

class VTKModule : public ISimModule {
public:
	explicit VTKModule(const VTKModuleParams& params);
	void on_step(lbm::LBMCore& core) override;

private:
	VTKModuleParams params_;
	ulong last_written_ = max_ulong;
};

class MassMonitorModule : public ISimModule {
public:
	void on_init(lbm::LBMCore& core) override;
	void on_step(lbm::LBMCore& core) override;

private:
	double initial_mass_ = 0.0;
};

#pragma once

#include "LBMConfig.h"
#include "../lbm/lbm.hpp"
#include <cstdint>

namespace lbm {

class LBMCore {
public:
	explicit LBMCore(const LBMConfig& config)
		: config_(config),
		  nCells_(config.nx*config.ny*config.nz),
		  stepCount_(0),
		  lbm_(nullptr),
		  mask_(nullptr),
		  fsCellType_(nullptr) {
		allocateMemory();
	}

	~LBMCore() {
		freeMemory();
	}

	LBMCore(const LBMCore&) = delete;
	LBMCore& operator=(const LBMCore&) = delete;

	void initialize() {
		if(lbm_ == nullptr) allocateMemory();
		if(lbm_ != nullptr) lbm_->run(0u);
	}

	void step() {
		if(lbm_ == nullptr) initialize();
		if(lbm_ != nullptr) {
			lbm_->run(1u);
			stepCount_++;
		}
	}

	void setExternalForce(real3 force) {
		config_.gravity = force;
		if(lbm_ != nullptr) lbm_->set_f(force.x, force.y, force.z);
	}

	void setActiveMask(const uint8_t* mask) {
		mask_ = mask;
	}

	void setFSCellType(const uint8_t* cellType) {
		fsCellType_ = cellType;
	}

	const real* getDensityField() const {
		if(lbm_ == nullptr) return nullptr;
		if(lbm_->get_D() != 1u) return nullptr;
		return lbm_->lbm_domain[0]->rho.data();
	}

	const real3* getVelocityField() const {
		return nullptr; // not a packed real3 layout in current LBM buffers
	}

	const real* getDistributions() const {
		return nullptr; // distributions are internal in current LBM implementation
	}

	real* getDistributionsMutable() {
		return nullptr; // distributions are internal in current LBM implementation
	}

	bool checkHealth() {
		return lbm_ != nullptr;
	}

	LBM* internal_lbm() { return lbm_; }
	const LBM* internal_lbm() const { return lbm_; }

	int nx() const { return config_.nx; }
	int ny() const { return config_.ny; }
	int nz() const { return config_.nz; }
	int nCells() const { return nCells_; }

private:
	LBMConfig config_;
	int nCells_;
	int stepCount_;
	LBM* lbm_;

	const uint8_t* mask_;
	const uint8_t* fsCellType_;

	void allocateMemory() {
		if(lbm_ != nullptr) return;
		const real tau = config_.tau > 0.0f ? config_.tau : (1.0f/3.0f+0.5f);
		const real nu = (tau-0.5f)/3.0f;
		const real fx = config_.gravity.x;
		const real fy = config_.gravity.y;
		const real fz = config_.gravity.z;
		lbm_ = new LBM((uint)config_.nx, (uint)config_.ny, (uint)config_.nz, nu, fx, fy, fz, config_.sigma);
	}

	void freeMemory() {
		delete lbm_;
		lbm_ = nullptr;
	}
};

} // namespace lbm

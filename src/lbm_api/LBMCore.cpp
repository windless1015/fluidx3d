#include "LBMCore.h"
#include "../cuda/lbm_cuda.hpp"

namespace lbm {

LBMCore::LBMCore(const LBMConfig& config)
	: config_(config),
	  nCells_(config.nx*config.ny*config.nz),
	  stepCount_(0),
	  mask_(nullptr),
	  fsCellType_(nullptr) {
	allocateMemory();
}

LBMCore::~LBMCore() {
	freeMemory();
}

void LBMCore::initialize() {
	if(initialized_) return;
	allocateMemory();
	CudaLBMParams p{};
	p.Nx = (unsigned int)config_.nx;
	p.Ny = (unsigned int)config_.ny;
	p.Nz = (unsigned int)config_.nz;
	p.N = (unsigned long long)nCells_;
	const real tau = config_.tau > 0.0f ? config_.tau : (1.0f/3.0f+0.5f);
	p.nu = (tau-0.5f)/3.0f;
	p.fx = config_.gravity.x;
	p.fy = config_.gravity.y;
	p.fz = config_.gravity.z;
	p.sigma = config_.sigma;
	p.w = 1.0f/(3.0f*p.nu+0.5f);
	p.def_6_sigma = 6.0f*p.sigma;
	backend_.initialize(p);
	backend_.upload_host_fields(rho_.data(), u_.data(), flags_.data(), phi_.data());
	backend_.kernel_initialize();
	backend_.synchronize();
	t_ = 0ull;
	initialized_ = true;
}

void LBMCore::step() {
	if(!initialized_) initialize();
	backend_.kernel_surface_capture_outgoing(t_);
	backend_.kernel_stream_collide(t_);
	backend_.kernel_surface_mass_exchange();
	backend_.kernel_surface_flag_transition(t_);
	backend_.kernel_surface_phi_recompute();
	backend_.kernel_update_fields(t_);
	backend_.synchronize();
	t_++;
	stepCount_++;
}

void LBMCore::sync_host_fields() {
	if(!initialized_) return;
	backend_.download_fields(rho_.data(), u_.data(), flags_.data(), phi_.data());
}

void LBMCore::allocateMemory() {
	if(!rho_.empty()) return;
	const size_t n = (size_t)nCells_;
	rho_.assign(n, 1.0f);
	u_.assign(n*3u, 0.0f);
	phi_.assign(n, 0.0f);
	flags_.assign(n, (unsigned char)0u);
}

void LBMCore::freeMemory() {
	rho_.clear();
	u_.clear();
	phi_.clear();
	flags_.clear();
}

} // namespace lbm

#pragma once

#include "LBMConfig.h"
#include "../cuda/lbm_cuda.hpp"
#include "../core/utilities.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace lbm {

class LBMCore {
public:
	explicit LBMCore(const LBMConfig& config)
		: config_(config),
		  nCells_(config.nx*config.ny*config.nz),
		  stepCount_(0),
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

	void step() {
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

	void setExternalForce(real3 force) {
		config_.gravity = force;
	}

	void setActiveMask(const uint8_t* mask) {
		mask_ = mask;
	}

	void setFSCellType(const uint8_t* cellType) {
		fsCellType_ = cellType;
	}

	const real* getDensityField() const {
		return rho_.data();
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
		return initialized_;
	}

	int nx() const { return config_.nx; }
	int ny() const { return config_.ny; }
	int nz() const { return config_.nz; }
	int nCells() const { return nCells_; }

	ulong nCellsU() const { return (ulong)nCells_; }
	uint get_Nx() const { return (uint)config_.nx; }
	uint get_Ny() const { return (uint)config_.ny; }
	uint get_Nz() const { return (uint)config_.nz; }
	ulong get_t() const { return t_; }
	uint get_Dx() const { return 1u; }
	uint get_Dy() const { return 1u; }
	uint get_Dz() const { return 1u; }
	uint get_D() const { return 1u; }
	uint get_velocity_set() const { return 19u; }
	float get_nu() const {
		const real tau = config_.tau > 0.0f ? config_.tau : (1.0f/3.0f+0.5f);
		return (tau-0.5f)/3.0f;
	}
	float get_tau() const {
		const real tau = config_.tau > 0.0f ? config_.tau : (1.0f/3.0f+0.5f);
		return tau;
	}
	float get_fx() const { return config_.gravity.x; }
	float get_fy() const { return config_.gravity.y; }
	float get_fz() const { return config_.gravity.z; }
	float get_sigma() const { return config_.sigma; }

	void coordinates(const ulong n, uint& x, uint& y, uint& z) const {
		const uint Nx = (uint)config_.nx;
		const uint Ny = (uint)config_.ny;
		const ulong t = n%((ulong)Nx*(ulong)Ny);
		x = (uint)(t%(ulong)Nx);
		y = (uint)(t/(ulong)Nx);
		z = (uint)(n/((ulong)Nx*(ulong)Ny));
	}

	void set_flag(const ulong n, const uchar flag) {
		if(n < (ulong)nCells_) flags_[n] = flag;
	}

	void set_phi(const ulong n, const float phi) {
		if(n < (ulong)nCells_) phi_[n] = phi;
	}

	void set_flag_phi(const ulong n, const uchar flag, const float phi) {
		if(n < (ulong)nCells_) {
			flags_[n] = flag;
			phi_[n] = phi;
		}
	}

	void for_each_cell(const std::function<void(ulong, uint, uint, uint)>& fn) const {
		const ulong N = (ulong)nCells_;
		parallel_for(N, [&](ulong n) {
			uint x = 0u, y = 0u, z = 0u;
			coordinates(n, x, y, z);
			fn(n, x, y, z);
		});
	}

	double compute_total_mass() const {
		if(!initialized_) return 0.0;
		sync_host_fields();
		double total = 0.0;
		for(int i = 0; i < nCells_; i++) {
			if((flags_[i] & (TYPE_F|TYPE_I)) != 0) {
				total += (double)phi_[i] * (double)rho_[i];
			}
		}
		return total;
	}

	void write_vtk(const string& path) {
		(void)path;
	}

	void sync_host_fields() {
		if(!initialized_) return;
		backend_.download_fields(rho_.data(), u_.data(), flags_.data(), phi_.data());
	}

	const float* phi_host() const { return phi_.data(); }
	const float* rho_host() const { return rho_.data(); }
	const unsigned char* flags_host() const { return flags_.data(); }

private:
	LBMConfig config_;
	int nCells_;
	int stepCount_;

	const uint8_t* mask_;
	const uint8_t* fsCellType_;
	CudaLBMBackend backend_;
	std::vector<float> rho_;
	std::vector<float> u_;
	std::vector<float> phi_;
	std::vector<unsigned char> flags_;
	ulong t_ = 0ull;
	bool initialized_ = false;

	void allocateMemory() {
		if(!rho_.empty()) return;
		const size_t n = (size_t)nCells_;
		rho_.assign(n, 1.0f);
		u_.assign(n*3u, 0.0f);
		phi_.assign(n, 0.0f);
		flags_.assign(n, (unsigned char)0u);
	}

	void freeMemory() {
		rho_.clear();
		u_.clear();
		phi_.clear();
		flags_.clear();
	}
};

} // namespace lbm

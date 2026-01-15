#pragma once

#include "../core/defines.hpp"
#include <cstdint>

struct CudaLBMParams {
	unsigned int Nx = 1u;
	unsigned int Ny = 1u;
	unsigned int Nz = 1u;
	unsigned long long N = 1ull;
	float nu = 1.0f/6.0f;
	float fx = 0.0f;
	float fy = 0.0f;
	float fz = 0.0f;
	float sigma = 0.0f;
	float w = 1.0f;
	float def_6_sigma = 0.0f;
};

class CudaLBMBackend {
public:
	CudaLBMBackend() = default;
	~CudaLBMBackend();

	void initialize(const CudaLBMParams& params);
	void upload_host_fields(const float* rho, const float* u, const unsigned char* flags, const float* phi);
	void kernel_initialize();
	void kernel_stream_collide(unsigned long long t);
	void kernel_surface_capture_outgoing(unsigned long long t);
	void kernel_surface_mass_exchange();
	void kernel_surface_flag_transition(unsigned long long t);
	void kernel_surface_phi_recompute();
	void kernel_update_fields(unsigned long long t);
	void download_fields(float* rho, float* u, unsigned char* flags, float* phi) const;
	void synchronize() const;

private:
	CudaLBMParams params{};
	void* fi = nullptr;
	float* rho = nullptr;
	float* u = nullptr;
	unsigned char* flags = nullptr;
	float* mass = nullptr;
	float* massex = nullptr;
	float* phi = nullptr;
	bool allocated = false;
};

#pragma once

#include "LBMConfig.h"
#include <cstdint>

namespace lbm {

enum class CellType : uint8_t {
	GAS = 0,
	LIQUID = 1,
	INTERFACE = 2
};

class FreeSurfaceModule {
public:
	FreeSurfaceModule()
		: enabled_(false),
		  nx_(0),
		  ny_(0),
		  nz_(0),
		  nCells_(0),
		  d_cellType_(nullptr),
		  d_fill_(nullptr),
		  d_mass_(nullptr) {}

	~FreeSurfaceModule() {
		cleanup();
	}

	FreeSurfaceModule(const FreeSurfaceModule&) = delete;
	FreeSurfaceModule& operator=(const FreeSurfaceModule&) = delete;

	void initialize(const LBMConfig& config) {
		nx_ = config.nx;
		ny_ = config.ny;
		nz_ = config.nz;
		nCells_ = nx_*ny_*nz_;
	}

	void cleanup() {
		d_cellType_ = nullptr;
		d_fill_ = nullptr;
		d_mass_ = nullptr;
	}

	bool isEnabled() const { return enabled_; }
	void setEnabled(bool e) { enabled_ = e; }

	uint8_t* getCellType() const { return d_cellType_; }
	real* getFill() const { return d_fill_; }
	real* getMass() const { return d_mass_; }

	int nCells() const { return nCells_; }
	int nx() const { return nx_; }
	int ny() const { return ny_; }
	int nz() const { return nz_; }

	void initFlatSurface(real level, real rho0) {
		(void)level;
		(void)rho0;
	}

	void setRegion(int x0, int x1, int y0, int y1, int z0, int z1, CellType type, real fill, real rho0) {
		(void)x0; (void)x1; (void)y0; (void)y1; (void)z0; (void)z1;
		(void)type; (void)fill; (void)rho0;
	}

	void fixInterfaceLayer() {}
	void updateMass(const real* f_soa) { (void)f_soa; }
	void distributeMass() {}
	void cleanupInterface() {}
	void reclassifyCells() {}
	void reconstructInterface(real* f, const real3* u_field) { (void)f; (void)u_field; }
	bool checkHealth() { return true; }

private:
	bool enabled_;
	int nx_, ny_, nz_, nCells_;

	uint8_t* d_cellType_;
	real* d_fill_;
	real* d_mass_;
};

} // namespace lbm

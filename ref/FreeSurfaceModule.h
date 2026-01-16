#pragma once

#include "LBMConfig.h"
#include <cstdint>

namespace lbm {

// Cell Types for Free Surface
enum class CellType : uint8_t {
    GAS = 0,
    LIQUID = 1,
    INTERFACE = 2,
    // Boundary/Solid cells are typically handled by LBMCore's boundary flags.
    // However, in some VOF implementations, solid might be explicitly marked.
    // For now, we stick to the 3-phase fluid types.
};

class FreeSurfaceModule {
public:
    FreeSurfaceModule();
    ~FreeSurfaceModule();

    // Disable copy/move
    FreeSurfaceModule(const FreeSurfaceModule&) = delete;
    FreeSurfaceModule& operator=(const FreeSurfaceModule&) = delete;

    // Lifecycle
    void initialize(const LBMConfig& config);
    void cleanup();

    // Configuration
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    // Data Access (Device Pointers)
    uint8_t* getCellType() const { return d_cellType_; }
    real* getFill() const { return d_fill_; }
    real* getMass() const { return d_mass_; }
    
    // Limits
    int nCells() const { return nCells_; }
    int nx() const { return nx_; }
    int ny() const { return ny_; }
    int nz() const { return nz_; }

    // Logic
    // Initialize fluid level at z = level
    void initFlatSurface(real level, real rho0);
    
    // Configurable Boundaries / Regions
    // Setup a rectangular region with specific type (e.g., set top layer to GAS)
    // inclusive bounds [x0, x1], [y0, y1], [z0, z1]
    void setRegion(int x0, int x1, int y0, int y1, int z0, int z1, CellType type, real fill, real rho0);

    // Auto-detect and fix Interface layer based on Liquid/Gas neighbors
    // Call this after manually setting regions with setRegion to validiate boundaries.
    void fixInterfaceLayer();

    // Physics Steps
    // Update mass based on advection (requires streamed distributions from Core)
    void updateMass(const real* f_soa);
    
    // Mass Redistribution (Smoothing)
    void distributeMass();
    
    // Artifact Removal (Topology Cleanup)
    void cleanupInterface();

    // Update cell types based on fill level
    // Update cell types based on fill level
    void reclassifyCells();

    // Reconstruction
    // Reconstruct distribution functions in GAS cells adjacent to INTERFACE/LIQUID
    // This allows meaningful streaming from GAS to FLUID.
    // Argument: f (mutable pointer to LBMCore's distributions)
    void reconstructInterface(real* f, const real3* u_field);

    // Diagnostics
    bool checkHealth();

private:
    bool enabled_;
    int nx_, ny_, nz_, nCells_;

    // Device Memory
    uint8_t* d_cellType_; // GAS/LIQUID/INTERFACE
    real* d_fill_;        // Volume fraction (epsilon)
    real* d_mass_;        // Mass (m)
};

} // namespace lbm

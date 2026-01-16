#pragma once

#include "LBMConfig.h"

namespace lbm {

class LBMCore {
public:
    explicit LBMCore(const LBMConfig& config);
    ~LBMCore();

    // Disable copy/move to prevent double-free of GPU resources
    LBMCore(const LBMCore&) = delete;
    LBMCore& operator=(const LBMCore&) = delete;

    // Lifecycle
    void initialize();
    void step();
    
    // External Control
    void setExternalForce(real3 force);
    // Optional: Set a mask (0=Inactive/Gas, >0=Active). 
    // If set, streaming/collision only runs on active cells.
    void setActiveMask(const uint8_t* mask);
    
    // Free Surface Integration
    void setFSCellType(const uint8_t* cellType);

    // Data Access (Device Pointers)
    // These return pointers to live device memory for external modules or visualization
    const real* getDensityField() const;
    const real3* getVelocityField() const;
    const real* getDistributions() const; // Current distribution (read-only)
    real* getDistributionsMutable();      // Mutable access for FreeSurface reconstruction

    // Diagnostics
    bool checkHealth();

    // Dimensions
    int nx() const { return config_.nx; }
    int ny() const { return config_.ny; }
    int nz() const { return config_.nz; }
    int nCells() const { return nCells_; }

private:
    LBMConfig config_;
    int nCells_;
    int stepCount_;

    // Device Data
    real* f_[2];        // Double-buffered distributions (SoA)
    real* rho_;         // Density (Macroscopic)
    real3* u_;          // Velocity (Macroscopic)
    uint8_t* cellType_; // Cell flags (for boundaries)

    // Optional active mask (external)
    const uint8_t* mask_;
    const uint8_t* fsCellType_;

    // Current buffer index (0 or 1)
    int currentBuffer_;

    // Internal Helpers
    void allocateMemory();
    void freeMemory();
    void swapBuffers();
};

} // namespace lbm

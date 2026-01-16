#pragma once

#include <cuda_runtime.h>

namespace lbm {

// Precision configuration
using real = float;
using real3 = float3;

// Boundary flags (bitmask)
enum WallFlags {
    WALL_NONE  = 0,
    WALL_X_MIN = 1 << 0,
    WALL_X_MAX = 1 << 1,
    WALL_Y_MIN = 1 << 2,
    WALL_Y_MAX = 1 << 3,
    WALL_Z_MIN = 1 << 4,
    WALL_Z_MAX = 1 << 5,
    WALL_ALL   = 0xFF
};

// Core configuration
struct LBMConfig {
    // Domain dimensions
    int nx;
    int ny;
    int nz;

    // Physical parameters
    real tau;          // Relaxation time
    real3 gravity;     // External body force (e.g., 0, 0, -9.81)

    // Boundary conditions
    int wallFlags;     // Bitwise OR of WallFlags

    // Initialization
    real rho0;         // Initial density (usually 1.0)
    real3 u0;          // Initial velocity
};

} // namespace lbm

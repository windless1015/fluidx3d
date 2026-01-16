#pragma once

#include "../core/utilities.hpp"

namespace lbm {

using real = float;
using real3 = float3;

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

struct LBMConfig {
	int nx = 0;
	int ny = 0;
	int nz = 0;

	real tau = 0.0f;
	real3 gravity = real3(0.0f);
	real sigma = 0.0f;

	int wallFlags = WALL_NONE;

	real rho0 = 1.0f;
	real3 u0 = real3(0.0f);
};

} // namespace lbm

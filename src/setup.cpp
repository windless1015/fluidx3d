/**
 * FluidX3D - Dam Break 3D Demo
 * 
 * This is a minimal setup file for demonstrating LBM + Free Surface simulation.
 * Required extensions in defines.hpp: FP16S, VOLUME_FORCE, SURFACE, INTERACTIVE_GRAPHICS
 */

#include "setup.hpp"

void main_setup() {
	// ============================================================================
	// Simulation Parameters
	// ============================================================================
	// Grid: 128 x 256 x 256
	// Viscosity (nu): 0.005
	// Volume Force: (0, 0, -0.0002) - gravity in -z direction
	// Surface Tension (sigma): 0.0001
	LBM lbm(128u, 256u, 256u, 0.005f, 0.0f, 0.0f, -0.0002f, 0.0001f);

	// ============================================================================
	// Geometry Setup - Dam Break Initial Condition
	// ============================================================================
	const uint Nx = lbm.get_Nx(), Ny = lbm.get_Ny(), Nz = lbm.get_Nz();
	
	parallel_for(lbm.get_N(), [&](ulong n) {
		uint x = 0u, y = 0u, z = 0u;
		lbm.coordinates(n, x, y, z);

		// Water column: z < 6/8 * Nz and y < Ny/8
		if (z < Nz * 6u / 8u && y < Ny / 8u) {
			lbm.flags[n] = TYPE_F;  // Fluid cell
			lbm.phi[n] = 1.0f;      // Fill level = full
		}
		else {
			lbm.flags[n] = TYPE_G;  // Gas cell
			lbm.phi[n] = 0.0f;      // Fill level = empty
		}

		// Solid walls on all boundaries (non-periodic)
		if (x == 0u || x == Nx - 1u || y == 0u || y == Ny - 1u || z == 0u || z == Nz - 1u) {
			lbm.flags[n] = TYPE_S;
		}
	});

	// ============================================================================
	// Run Simulation with Mass Conservation Logging
	// ============================================================================
	lbm.run(0u);  // Initialize

	double initial_mass = (double)lbm.lbm_domain[0]->compute_total_mass();
	print_info("Initial total mass: " + to_string((float)initial_mass, 6u));

	const uint log_interval = 100u;

	// Set visualization mode: raytracing for single GPU, rasterizing for multi-GPU
	lbm.graphics.visualization_modes = lbm.get_D() == 1u ? VIS_PHI_RAYTRACE : VIS_PHI_RASTERIZE;

	// Main simulation loop - run indefinitely
	while(true) {
		lbm.run(log_interval);

		// Calculate current total mass
		double current_mass = (double)lbm.lbm_domain[0]->compute_total_mass();
		double mass_diff_percent = (current_mass - initial_mass) / initial_mass * 100.0;

		print_info("Step " + to_string(lbm.get_t()) +
			" | Initial mass: " + to_string((float)initial_mass, 6u) +
			" | Current mass: " + to_string((float)current_mass, 6u) +
			" | Change: " + to_string((float)mass_diff_percent, 4u) + "%");
	}
}
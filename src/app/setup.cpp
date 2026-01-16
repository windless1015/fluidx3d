/**
 * FluidX3D - Dam Break 3D Demo
 * 
 * This is a minimal setup file for demonstrating LBM + Free Surface simulation.
 * Required extensions in defines.hpp: FP16S, VOLUME_FORCE, SURFACE, INTERACTIVE_GRAPHICS
 */

#include "setup.hpp"
#include "app_graphics.hpp"
#include "scenes/dam_break.hpp"
#include "runtime/sim_loop.hpp"
#include "runtime/sim_modules.hpp"
#include <atomic>
#include <thread>

void main_setup() {
	// ============================================================================
	// Simulation Parameters
	// ============================================================================
	// Grid: 128 x 256 x 256
	// Viscosity (nu): 0.005
	// Volume Force: (0, 0, -0.0002) - gravity in -z direction
	// Surface Tension (sigma): 0.0001
	lbm::LBMModel model;
	DamBreakParams params;
	setup_dam_break(model, params);
	LBM* lbm = model.core()->internal_lbm();

#ifdef GRAPHICS
	app_graphics = new LBM_Graphics(lbm);
	app_graphics->visualization_modes = lbm->get_D() == 1u ? VIS_PHI_RAYTRACE : VIS_PHI_RASTERIZE;
#endif // GRAPHICS

	// ============================================================================
	// Geometry Setup - Dam Break Initial Condition
	// ============================================================================
	const uint Nx = lbm->get_Nx(), Ny = lbm->get_Ny(), Nz = lbm->get_Nz();
	
	parallel_for(lbm->get_N(), [&](ulong n) {
		uint x = 0u, y = 0u, z = 0u;
		lbm->coordinates(n, x, y, z);

		// Water column: z < 6/8 * Nz and y < Ny/8
		if (z < Nz * 6u / 8u && y < Ny / 8u) {
			lbm->flags[n] = TYPE_F;  // Fluid cell
			lbm->phi[n] = 1.0f;      // Fill level = full
		}
		else {
			lbm->flags[n] = TYPE_G;  // Gas cell
			lbm->phi[n] = 0.0f;      // Fill level = empty
		}

		// Solid walls on all boundaries (non-periodic)
		if (x == 0u || x == Nx - 1u || y == 0u || y == Ny - 1u || z == 0u || z == Nz - 1u) {
			lbm->flags[n] = TYPE_S;
		}
	});

	// ============================================================================
	// Run Simulation with Mass Conservation Logging
	// ============================================================================
	SimLoopParams loop_params;
	loop_params.log_interval = 1u;
	std::vector<std::unique_ptr<ISimModule>> modules;
	modules.emplace_back(std::make_unique<MassMonitorModule>());
#if ENABLE_VTK_OUTPUT
	VTKModuleParams vtk_params;
	vtk_params.enabled = true;
	vtk_params.interval = 100u;
	modules.emplace_back(std::make_unique<VTKModule>(vtk_params));
#endif
	run_simulation(model, loop_params, std::move(modules));
}

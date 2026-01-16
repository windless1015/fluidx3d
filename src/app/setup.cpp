/**
 * FluidX3D - Dam Break 3D Demo
 * 
 * This is a minimal setup file for demonstrating LBM + Free Surface simulation.
 * Required extensions in defines.hpp: FP16S, VOLUME_FORCE, SURFACE, INTERACTIVE_GRAPHICS
 */

#include "setup.hpp"
#include "scenes/dam_break.hpp"
#include "runtime/sim_loop.hpp"
#include "runtime/sim_modules.hpp"
#include "app_model.hpp"

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
	app_core = model.core();

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

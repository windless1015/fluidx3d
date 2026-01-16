#include "dam_break.hpp"
#include "../app_graphics.hpp"

void setup_dam_break(lbm::LBMModel& model, const DamBreakParams& params) {
	lbm::LBMConfig cfg;
	cfg.nx = (int)params.nx;
	cfg.ny = (int)params.ny;
	cfg.nz = (int)params.nz;
	cfg.tau = 3.0f*params.nu+0.5f;
	cfg.gravity = lbm::real3(params.fx, params.fy, params.fz);
	cfg.sigma = params.sigma;
	model.configure(cfg);
	model.enableFreeSurface(true);

	lbm::LBMCore* core = model.core();
	if(core == nullptr) return;

#ifdef GRAPHICS
	app_graphics = new LBM_Graphics(core->internal_lbm());
	app_graphics->visualization_modes = core->internal_lbm()->get_D() == 1u ? VIS_PHI_RAYTRACE : VIS_PHI_RASTERIZE;
#endif // GRAPHICS

	const uint Nx = core->get_Nx();
	const uint Ny = core->get_Ny();
	const uint Nz = core->get_Nz();
	const uint water_z = Nz*params.water_z_num/params.water_z_den;
	const uint water_y = Ny*params.water_y_num/params.water_y_den;

	core->for_each_cell([&](ulong n, uint x, uint y, uint z) {

		if(z < water_z && y < water_y) {
			core->set_flag_phi(n, TYPE_F, 1.0f);  // Fluid cell
		} else {
			core->set_flag_phi(n, TYPE_G, 0.0f);  // Gas cell
		}

		if(x == 0u || x == Nx - 1u || y == 0u || y == Ny - 1u || z == 0u || z == Nz - 1u) {
			core->set_flag(n, TYPE_S);
		}
	});
}

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

	LBM* lbm = model.core()->internal_lbm();
	if(lbm == nullptr) return;

#ifdef GRAPHICS
	app_graphics = new LBM_Graphics(lbm);
	app_graphics->visualization_modes = lbm->get_D() == 1u ? VIS_PHI_RAYTRACE : VIS_PHI_RASTERIZE;
#endif // GRAPHICS

	const uint Nx = lbm->get_Nx();
	const uint Ny = lbm->get_Ny();
	const uint Nz = lbm->get_Nz();
	const uint water_z = Nz*params.water_z_num/params.water_z_den;
	const uint water_y = Ny*params.water_y_num/params.water_y_den;

	parallel_for(lbm->get_N(), [&](ulong n) {
		uint x = 0u, y = 0u, z = 0u;
		lbm->coordinates(n, x, y, z);

		if(z < water_z && y < water_y) {
			lbm->flags[n] = TYPE_F;  // Fluid cell
			lbm->phi[n] = 1.0f;      // Fill level = full
		} else {
			lbm->flags[n] = TYPE_G;  // Gas cell
			lbm->phi[n] = 0.0f;      // Fill level = empty
		}

		if(x == 0u || x == Nx - 1u || y == 0u || y == Ny - 1u || z == 0u || z == Nz - 1u) {
			lbm->flags[n] = TYPE_S;
		}
	});
}

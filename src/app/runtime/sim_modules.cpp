#include "sim_modules.hpp"
#include "../info.hpp"

VTKModule::VTKModule(const VTKModuleParams& params) : params_(params) {}

void VTKModule::on_step(lbm::LBMCore& core) {
	if(!params_.enabled || params_.interval == 0u) return;
	const ulong t = core.get_t();
	if(t % params_.interval == 0u && t != last_written_) {
		last_written_ = t;
		core.write_vtk(params_.prefix + to_string(t) + ".vti");
	}
}

void MassMonitorModule::on_init(lbm::LBMCore& core) {
	initial_mass_ = core.compute_total_mass();
	print_info("Initial total mass: " + to_string((float)initial_mass_, 6u));
}

void MassMonitorModule::on_step(lbm::LBMCore& core) {
	const double current_mass = core.compute_total_mass();
	const double diff_percent = (current_mass - initial_mass_) / initial_mass_ * 100.0;
	print_info("Step " + to_string(core.get_t()) +
		" | Initial mass: " + to_string((float)initial_mass_, 6u) +
		" | Current mass: " + to_string((float)current_mass, 6u) +
		" | Change: " + to_string((float)diff_percent, 4u) + "%");
}

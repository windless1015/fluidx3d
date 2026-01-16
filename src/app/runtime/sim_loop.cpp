#include "sim_loop.hpp"
#include "../info.hpp"
#include "../../core/utilities.hpp"
#include "../../core/defines.hpp"
#include <atomic>
#include <thread>

void run_simulation(lbm::LBMModel& model, const SimLoopParams& params) {
	LBM* lbm = model.core() ? model.core()->internal_lbm() : nullptr;
	if(lbm == nullptr) return;

	lbm->run(0u);

	double initial_mass = (double)lbm->lbm_domain[0]->compute_total_mass();
	print_info("Initial total mass: " + to_string((float)initial_mass, 6u));

	std::atomic_bool vtk_thread_running(true);
	std::thread vtk_thread;
	if(params.enable_vtk && params.vtk_interval > 0u) {
		vtk_thread = std::thread([&]() {
			ulong last_written_t = max_ulong;
			while(vtk_thread_running) {
				const ulong t = lbm->get_t();
				if(t % params.vtk_interval == 0u && t != last_written_t) {
					last_written_t = t;
					lbm->write_vtk(string(params.vtk_prefix) + to_string(t) + ".vti");
				} else {
					sleep(0.010);
				}
			}
		});
	}

	while(true) {
		lbm->run(params.log_interval);

		if(params.enable_vtk && params.vtk_interval > 0u) {
			if(lbm->get_t() % params.vtk_interval == 0u) {
				lbm->write_vtk(string(params.vtk_prefix) + to_string(lbm->get_t()) + ".vti");
			}
		}

		double current_mass = (double)lbm->lbm_domain[0]->compute_total_mass();
		double mass_diff_percent = (current_mass - initial_mass) / initial_mass * 100.0;

		print_info("Step " + to_string(lbm->get_t()) +
			" | Initial mass: " + to_string((float)initial_mass, 6u) +
			" | Current mass: " + to_string((float)current_mass, 6u) +
			" | Change: " + to_string((float)mass_diff_percent, 4u) + "%");
	}

	vtk_thread_running = false;
	if(vtk_thread.joinable()) vtk_thread.join();
}

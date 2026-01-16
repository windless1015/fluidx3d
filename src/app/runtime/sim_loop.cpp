#include "sim_loop.hpp"

void run_simulation(lbm::LBMModel& model, const SimLoopParams& params, std::vector<std::unique_ptr<ISimModule>> modules) {
	if(model.core() == nullptr) return;
	model.initialize();

	for(const auto& module : modules) {
		if(module) module->on_init(*model.core());
	}

	while(true) {
		model.step();
		for(unsigned int i = 1u; i < params.log_interval; i++) {
			model.step();
		}
		for(const auto& module : modules) {
			if(module) module->on_step(*model.core());
		}
	}
}

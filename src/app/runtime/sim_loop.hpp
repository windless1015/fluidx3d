#pragma once

#include "../../lbm_api/LBMModel.h"
#include "sim_modules.hpp"
#include <memory>
#include <vector>

struct SimLoopParams {
	unsigned int log_interval = 1u;
};

void run_simulation(lbm::LBMModel& model, const SimLoopParams& params, std::vector<std::unique_ptr<ISimModule>> modules);

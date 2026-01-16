#pragma once

#include "../../lbm_api/LBMModel.h"

struct SimLoopParams {
	unsigned int log_interval = 1u;
	bool enable_vtk = false;
	unsigned int vtk_interval = 100u;
	const char* vtk_prefix = "output/dam_break_";
};

void run_simulation(lbm::LBMModel& model, const SimLoopParams& params);

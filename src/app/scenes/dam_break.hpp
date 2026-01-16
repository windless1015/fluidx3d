#pragma once

#include "../../lbm_api/LBMModel.h"
#include "../../core/defines.hpp"

struct DamBreakParams {
	uint nx = 128u;
	uint ny = 256u;
	uint nz = 256u;

	float nu = 0.005f;
	float fx = 0.0f;
	float fy = 0.0f;
	float fz = -0.0002f;
	float sigma = 0.0001f;

	uint water_z_num = 6u;
	uint water_z_den = 8u;
	uint water_y_num = 1u;
	uint water_y_den = 8u;
};

void setup_dam_break(lbm::LBMModel& model, const DamBreakParams& params);

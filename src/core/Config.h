#pragma once

#include <string>

namespace lbm {

struct Config {
	std::string type;
};

struct ModelConfig {
	Config base;
	double dt = 0.0;
};

} // namespace lbm

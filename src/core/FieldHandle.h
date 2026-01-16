#pragma once

#include <cstddef>
#include <string>

namespace lbm {

struct FieldHandle {
	void* data = nullptr;
	size_t count = 0;
	std::string type;
};

} // namespace lbm

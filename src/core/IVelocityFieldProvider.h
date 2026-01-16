#pragma once

#include "../core/utilities.hpp"

namespace lbm {

class IVelocityFieldProvider {
public:
	virtual ~IVelocityFieldProvider() = default;
	virtual bool sampleVelocity(const float3& pos, float3& outVelocity) const = 0;
};

} // namespace lbm

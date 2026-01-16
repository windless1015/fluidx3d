#pragma once

#include "../core/utilities.hpp"

namespace lbm {

class ICollisionGeometry {
public:
	virtual ~ICollisionGeometry() = default;
	virtual bool queryCollision(const float3& pos, float radius, float3& outNormal) const = 0;
};

} // namespace lbm

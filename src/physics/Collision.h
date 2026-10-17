#pragma once
#include <glm/glm.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include "Model.h"
bool ComputeModelAABB(Model* model, glm::vec3& outMin, glm::vec3& outMax);
JPH::ShapeRefC CreateCollisionShapeFromAABB(const glm::vec3& min, const glm::vec3& max);
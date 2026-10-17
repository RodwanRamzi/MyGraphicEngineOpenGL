#include "Collision.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <float.h>
#include <algorithm>
#include <cmath>

bool ComputeModelAABB(Model* model, glm::vec3& outMin, glm::vec3& outMax) {
    if (!model || model->meshes.empty()) return false;
    outMin = glm::vec3(FLT_MAX); outMax = glm::vec3(-FLT_MAX);
    for (const auto& mesh : model->meshes)
        for (const auto& v : mesh.vertices) {
            outMin = glm::min(outMin, v.position);
            outMax = glm::max(outMax, v.position);
        }
    return true;
}
JPH::ShapeRefC CreateCollisionShapeFromAABB(const glm::vec3& min, const glm::vec3& max) {
    glm::vec3 size = max - min;
    float x = size.x, y = size.y, z = size.z;
    if (y > x * 1.5f && y > z * 1.5f) {
        float radius = std::max(x, z) * 0.5f;
        float halfHeight = y * 0.5f - radius;
        if (halfHeight < 0.1f) halfHeight = 0.1f;
        return new JPH::CapsuleShape(halfHeight, radius);
    }
    if (std::abs(x - y) < 0.1f && std::abs(x - z) < 0.1f) {
        float radius = (x + y + z) / 6.0f;
        return new JPH::SphereShape(radius);
    }
    return new JPH::BoxShape(JPH::Vec3(x * 0.5f, y * 0.5f, z * 0.5f));
}
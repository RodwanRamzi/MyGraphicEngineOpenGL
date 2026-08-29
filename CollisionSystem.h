#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "GameObject.h"

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class CollisionSystem {
public:
    static AABB GetAABB(const GameObject& gameObject);
    static bool CheckAABBCollision(const AABB& a, const AABB& b);
    static bool CheckAABBCollision(const GameObject& a, const GameObject& b);
};
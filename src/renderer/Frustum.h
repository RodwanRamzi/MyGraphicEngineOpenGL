#pragma once
#include <glm/glm.hpp>
struct Frustum { glm::vec4 planes[6]; };
Frustum extractFrustum(const glm::mat4& viewProj);
bool isSphereInFrustum(const Frustum& frustum, const glm::vec3& center, float radius);
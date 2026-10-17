#include "Frustum.h"

Frustum extractFrustum(const glm::mat4& viewProj) {
    Frustum frustum;
    frustum.planes[0] = glm::vec4(viewProj[0][3] + viewProj[0][0],
                                   viewProj[1][3] + viewProj[1][0],
                                   viewProj[2][3] + viewProj[2][0],
                                   viewProj[3][3] + viewProj[3][0]);
    frustum.planes[1] = glm::vec4(viewProj[0][3] - viewProj[0][0],
                                   viewProj[1][3] - viewProj[1][0],
                                   viewProj[2][3] - viewProj[2][0],
                                   viewProj[3][3] - viewProj[3][0]);
    frustum.planes[2] = glm::vec4(viewProj[0][3] + viewProj[0][1],
                                   viewProj[1][3] + viewProj[1][1],
                                   viewProj[2][3] + viewProj[2][1],
                                   viewProj[3][3] + viewProj[3][1]);
    frustum.planes[3] = glm::vec4(viewProj[0][3] - viewProj[0][1],
                                   viewProj[1][3] - viewProj[1][1],
                                   viewProj[2][3] - viewProj[2][1],
                                   viewProj[3][3] - viewProj[3][1]);
    frustum.planes[4] = glm::vec4(viewProj[0][3] + viewProj[0][2],
                                   viewProj[1][3] + viewProj[1][2],
                                   viewProj[2][3] + viewProj[2][2],
                                   viewProj[3][3] + viewProj[3][2]);
    frustum.planes[5] = glm::vec4(viewProj[0][3] - viewProj[0][2],
                                   viewProj[1][3] - viewProj[1][2],
                                   viewProj[2][3] - viewProj[2][2],
                                   viewProj[3][3] - viewProj[3][2]);

    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        if (length > 0.0f)
            frustum.planes[i] /= length;
    }
    return frustum;
}

bool isSphereInFrustum(const Frustum& frustum,
                       const glm::vec3& center,
                       float radius) {
    for (int i = 0; i < 6; ++i) {
        float distance = glm::dot(frustum.planes[i], glm::vec4(center, 1.0f));
        if (distance < -radius)
            return false;
    }
    return true;
}
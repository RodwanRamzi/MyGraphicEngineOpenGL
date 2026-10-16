#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;
layout (location = 6) in ivec4 aJoints;
layout (location = 7) in vec4  aWeights;

uniform mat4 lightMatrix;
uniform mat4 model;
uniform int  boneCount;
uniform int  isSkinned;
uniform mat4 boneMatrices[128];

void main() {
    vec4 localPos = vec4(aPos, 1.0);

    if (isSkinned == 1 && boneCount > 0) {
        vec4 w = aWeights;
        float sum = w.x + w.y + w.z + w.w;
        if (sum > 0.0) w /= sum;

        int j0 = clamp(aJoints.x, 0, boneCount - 1);
        int j1 = clamp(aJoints.y, 0, boneCount - 1);
        int j2 = clamp(aJoints.z, 0, boneCount - 1);
        int j3 = clamp(aJoints.w, 0, boneCount - 1);

        mat4 skinMatrix =
            w.x * boneMatrices[j0] +
            w.y * boneMatrices[j1] +
            w.z * boneMatrices[j2] +
            w.w * boneMatrices[j3];

        localPos = skinMatrix * localPos;
    }

    gl_Position = lightMatrix * model * localPos;
}
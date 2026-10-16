#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;
layout (location = 6) in ivec4 aJoints;
layout (location = 7) in vec4  aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 texCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 camMatrix;

#define MAX_BONES 128
uniform mat4 boneMatrices[MAX_BONES];
uniform int  boneCount;
uniform int  isSkinned;

void main() {
    vec4 localPos       = vec4(aPos, 1.0);
    vec3 localNormal    = aNormal;
    vec3 localTangent   = aTangent;
    vec3 localBitangent = aBitangent;

    // Pre-declare nm so it exists regardless of isSkinned
    mat3 nm = mat3(1.0);

    if (isSkinned == 1) {
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
        nm       = mat3(skinMatrix);
    }

    localNormal    = normalize(nm * localNormal);
    localTangent   = normalize(nm * localTangent);
    localBitangent = normalize(nm * localBitangent);

    FragPos  = vec3(model * localPos);
    Normal   = normalize(mat3(transpose(inverse(model))) * localNormal);
    texCoord = aTexCoord;
    gl_Position = camMatrix * vec4(FragPos, 1.0);

    vec3 T = normalize(mat3(model) * localTangent);
    vec3 B = normalize(mat3(model) * localBitangent);
    vec3 N = normalize(mat3(model) * localNormal);
    TBN = mat3(T, B, N);
}
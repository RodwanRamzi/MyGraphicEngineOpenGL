#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gColor;
layout (location = 3) out vec3 gMetallicRoughness;
layout (location = 4) out vec3 gEmissive;   

in vec3 FragPos;
in vec3 Normal;
in vec2 texCoord;
in mat3 TBN;

uniform sampler2D diffuse0;
uniform sampler2D normal0;
uniform sampler2D metallicRoughness0;
uniform sampler2D heightMap0;
uniform sampler2D emissiveMap0;   

uniform vec3 camPos;

uniform int uUseAlbedoMap;
uniform int uUseNormalMap;
uniform int uUseMetallicRoughnessMap;
uniform int uUseAOMap;
uniform int uUseEmissiveMap;    

uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;
uniform vec3 uEmissiveColor;
uniform float uEmissiveIntensity;

void main() {
    vec2 flippedUV = vec2(texCoord.x, 1.0 - texCoord.y);

    // Albedo
    vec3 albedo;
    if (uUseAlbedoMap == 0) {
        albedo = uAlbedo;
    } else {
        albedo = texture(diffuse0, flippedUV).rgb;
    }

    vec3 normal = normalize(Normal);

    float metallic;
    float roughness;
    if (uUseMetallicRoughnessMap == 1) {
        metallic = texture(metallicRoughness0, flippedUV).r;
        roughness = texture(metallicRoughness0, flippedUV).g;
    } else {
        metallic = uMetallic;
        roughness = uRoughness;
    }

    float ao = (uUseAOMap == 1) ? 1.0 : uAO;

    // ---- Emissive ----
    vec3 emissive;
    if (uUseEmissiveMap == 1) {
        emissive = texture(emissiveMap0, flippedUV).rgb * uEmissiveIntensity;
    } else {
        emissive = uEmissiveColor * uEmissiveIntensity;
    }

    gPosition = FragPos;
    gNormal = normal;
    gColor = albedo;
    gMetallicRoughness = vec3(metallic, roughness, ao);
    gEmissive = emissive;   // ← جديد
}
#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal;      // alpha = sssMask
layout (location = 2) out vec3 gColor;
layout (location = 3) out vec3 gMetallicRoughness;  // r=metallic, g=roughness, b=ao
layout (location = 4) out vec3 gEmissive;

in vec3 FragPos;
in vec3 Normal;
in vec2 texCoord;
in mat3 TBN;

// ---- Textures ----
uniform sampler2D diffuse0;
uniform sampler2D normal0;
uniform sampler2D metallicRoughness0;
uniform sampler2D heightMap0;
uniform sampler2D emissiveMap0;
uniform sampler2D ao0;

// ---- glTF material factors ----
uniform vec4  baseColorFactor;
uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3  emissiveFactor;
uniform float emissiveStrength;

// ---- Presence flags (set by Mesh::Draw) ----
uniform bool  hasBaseColorTexture;
uniform bool  hasMetallicRoughnessTexture;
uniform bool  hasNormalTexture;
uniform bool  hasEmissiveTexture;
uniform bool  hasAOTexture;

// ---- Editor overrides (existing system) ----
uniform int   uUseAlbedoMap;
uniform int   uUseNormalMap;
uniform int   uUseMetallicRoughnessMap;
uniform int   uUseAOMap;
uniform int   uUseEmissiveMap;

uniform vec3  uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;
uniform vec3  uEmissiveColor;
uniform float uEmissiveIntensity;

uniform float uSSSMask;

uniform vec3 camPos;

void main() {
    // vec2 uv = vec2(texCoord.x, 1.0 - texCoord.y);
    vec2 uv = texCoord;

    // ============================================
    //  ALBEDO
    // ============================================
    // Start from glTF factor, multiply by texture if present.
    // Editor override: if uUseAlbedoMap == 0, use uAlbedo instead.
    vec3 albedo;
    if (uUseAlbedoMap == 0) {
        albedo = uAlbedo;
    } else {
        albedo = baseColorFactor.rgb;
        if (hasBaseColorTexture) {
            albedo *= texture(diffuse0, uv).rgb;
        }
    }

    // ============================================
    //  NORMAL
    // ============================================
    vec3 normal = normalize(Normal);
    // If your TBN + normal map path is elsewhere, keep it there.
    // This shader just uses geometric normal.

    // ============================================
    //  METALLIC / ROUGHNESS
    // ============================================
    float metallic;
    float roughness;

    if (uUseMetallicRoughnessMap == 1) {
        // glTF packing: R=AO, G=roughness, B=metallic
        metallic  = metallicFactor;
        roughness = roughnessFactor;
        if (hasMetallicRoughnessTexture) {
            vec3 mr = texture(metallicRoughness0, uv).rgb;
            roughness *= mr.g;
            metallic  *= mr.b;
        }
    } else {
        metallic  = uMetallic;
        roughness = uRoughness;
    }

    // ============================================
    //  AMBIENT OCCLUSION
    // ============================================
    float ao;
    if (uUseAOMap == 1 && hasAOTexture) {
        ao = texture(ao0, uv).r;
    } else if (uUseAOMap == 1) {
        // glTF: AO often packed in MR texture's R channel
        if (hasMetallicRoughnessTexture) {
            ao = texture(metallicRoughness0, uv).r;
        } else {
            ao = 1.0;
        }
    } else {
        ao = uAO;
    }

    // ============================================
    //  EMISSIVE
    // ============================================
    vec3 emissive;
    if (uUseEmissiveMap == 1) {
        emissive = emissiveFactor * emissiveStrength;
        if (hasEmissiveTexture) {
            emissive *= texture(emissiveMap0, uv).rgb;
        }
        emissive *= uEmissiveIntensity;
    } else {
        emissive = uEmissiveColor * uEmissiveIntensity;
    }

    // ============================================
    //  OUTPUT TO G-BUFFER
    // ============================================
    gPosition          = FragPos;
    gNormal            = vec4(normal, uSSSMask);
    gColor             = albedo;
    gMetallicRoughness = vec3(metallic, roughness, ao);
    gEmissive          = emissive;
}
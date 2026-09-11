#version 460 core

const float PI = 3.14159265359;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

in vec2 TexCoords;

// ============================================================
//                        UNIFORMS
// ============================================================

// ---- G-Buffer ----
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gMetallicRoughness;
uniform sampler2D gEmissive;

// ---- Auxiliary buffers ----
uniform sampler2D shadowMap;
uniform sampler2D ssao;

// ---- Camera ----
uniform vec3  camPos;
uniform vec3  viewPos;
uniform mat4  viewMatrix;
uniform mat4  inverseViewMatrix;
uniform mat4  cameraMatrix;

// ---- Environment ----
uniform samplerCube environmentMap;
uniform bool  enableEnvReflections;
uniform float envReflectionIntensity;

// ---- Skybox / Gradient sky ----
uniform bool  showSkybox;
uniform vec3  skyTopColor;
uniform vec3  skyHorizonColor;
uniform vec3  skyBottomColor;
uniform vec3  sunColor;
uniform vec3  sunDirection;
uniform float sunIntensity;
uniform float cloudDensity;
uniform float cloudOpacity;

// ---- Shadow (directional) ----
uniform mat4  lightSpaceMatrix;
uniform vec3  lightDir;
uniform vec4  lightColor;

// ---- SSAO ----
uniform bool  enableSSAO;
uniform float ssaoRadius;
uniform float ssaoBias;
uniform float ssaoPower;

// ---- Contact shadows ----
uniform bool  enableContactShadows;

// ---- Bloom ----
uniform bool  enableBloom;
uniform float bloomThreshold;
uniform float bloomIntensity;

// ---- Tone mapping ----
uniform float saturation;
uniform float contrast;
uniform float gamma;
uniform float exposure;

// ---- Emissive fallback (uniform color) ----
uniform vec3  uEmissiveColor;
uniform float uEmissiveIntensity;

// ---- Debug view ----
uniform int debugView; // 0=off, 1=Position, 2=Normal, 3=Albedo,
                       // 4=Metallic/Roughness, 5=Emissive, 6=SSAO, 7=Depth

// ---- Dynamic lights ----
#define MAX_LIGHTS 128
uniform int   numLights;
uniform vec3  lightPositions[MAX_LIGHTS];
uniform vec3  lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRanges[MAX_LIGHTS];
uniform int   lightTypes[MAX_LIGHTS]; // 0=Point, 1=Spot, 2=Directional, 3=Ambient
uniform vec3  lightDirections[MAX_LIGHTS];
uniform float lightInnerCone[MAX_LIGHTS];
uniform float lightOuterCone[MAX_LIGHTS];

// ============================================================
//                        PBR
// ============================================================
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

// ============================================================
//                    SHADOW (PCF)
// ============================================================
float ShadowCalculation(vec3 fragPos, vec3 normal) {
    vec3 fragPosOffset = fragPos + normal * 0.02;
    vec4 fragPosLight   = lightSpaceMatrix * vec4(fragPosOffset, 1.0);
    vec3 projCoords     = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

float ContactShadow(vec3 fragPos, vec3 normal, vec3 lightDir, vec2 uv) {
    float shadow   = 0.0;
    float steps    = 8.0;
    float stepSize = 0.02;

    vec3 startPos = fragPos + normal * 0.005;
    vec3 dir      = -lightDir;

    for (int i = 0; i < int(steps); i++) {
        float t = float(i) * stepSize;
        vec3 samplePos = startPos + dir * t;

        vec4 clipPos  = cameraMatrix * vec4(samplePos, 1.0);
        vec2 sampleUV = clipPos.xy / clipPos.w * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
            sampleUV.y < 0.0 || sampleUV.y > 1.0) break;

        vec3  sampleFragPos = texture(gPosition, sampleUV).rgb;
        float sampleDepth   = length(sampleFragPos - camPos);
        float currentDepth  = length(samplePos - camPos);

        if (currentDepth > sampleDepth + 0.002) {
            shadow = 1.0 - (float(i) / steps);
            break;
        }
    }
    return shadow * 0.7;
}

// ============================================================
//                    SUN + CLOUDS
// ============================================================
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float fbm(vec2 p) {
    float value     = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        vec2 q = floor(p * frequency);
        vec2 r = fract(p * frequency);
        float a = hash(q);
        float b = hash(q + vec2(1.0, 0.0));
        float c = hash(q + vec2(0.0, 1.0));
        float d = hash(q + vec2(1.0, 1.0));
        vec2 u = r * r * (3.0 - 2.0 * r);
        float noise = mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
        value     += amplitude * noise;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

vec3 addSun(vec3 worldViewDir, vec3 skyColor) {
    vec3  sunDir         = normalize(sunDirection);
    float sunAngle       = dot(worldViewDir, sunDir);
    float sunIntensityV  = smoothstep(0.998, 1.0, sunAngle);
    float glowIntensity  = smoothstep(0.98, 0.99, sunAngle) * 0.5;

    vec3 result = skyColor;
    result += sunColor * sunIntensityV * sunIntensity;
    result += sunColor * 0.7 * glowIntensity * sunIntensity * 0.5;
    return result;
}

vec3 addClouds(vec3 worldViewDir, vec3 skyColor) {
    vec2 cloudUV = worldViewDir.xz / (worldViewDir.y + 0.1) * 0.5 + vec2(0.5);
    cloudUV += vec2(0.1, 0.2);

    float cloudDensityVal = fbm(cloudUV * cloudDensity);
    cloudDensityVal = smoothstep(0.4, 0.8, cloudDensityVal);
    cloudDensityVal *= cloudOpacity;

    vec3 cloudColor = mix(vec3(1.0), vec3(0.9, 0.85, 0.8), cloudDensityVal);
    return mix(skyColor, cloudColor, cloudDensityVal);
}

vec3 getGradientSky(vec3 viewDir) {
    float skyFactor = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3  skyColor;

    if (skyFactor > 0.5) {
        float t = (skyFactor - 0.5) * 2.0;
        skyColor = mix(skyHorizonColor, skyTopColor, t);
    } else {
        float t = skyFactor * 2.0;
        skyColor = mix(skyBottomColor, skyHorizonColor, t);
    }
    return skyColor;
}

// ============================================================
//                 DEBUG VIEW HELPER
// ============================================================
vec3 GetDebugColor(int view, vec3 fragPos, vec3 normal, vec3 albedo,
                   float metallic, float roughness)
{
    if (view == 1) return fragPos * 0.1;
    if (view == 2) return normal * 0.5 + 0.5;
    if (view == 3) return albedo;
    if (view == 4) return vec3(metallic, roughness, 0.0);
    if (view == 5) return texture(gEmissive, TexCoords).rgb;
    if (view == 6) return vec3(texture(ssao, TexCoords).r);
    if (view == 7) {
        float dist = length(fragPos - camPos);
        return vec3(1.0 - clamp(dist / 50.0, 0.0, 1.0));
    }
    return vec3(0.0);
}

// ============================================================
//                    LIGHTING HELPERS
// ============================================================
vec3 CalculatePBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic,
                  float roughness, vec3 radiance)
{
    vec3  H    = normalize(V + L);
    float NDF  = DistributionGGX(N, H, roughness);
    float G    = GeometrySmith(N, V, L, roughness);
    vec3  F0   = mix(vec3(0.04), albedo, metallic);
    vec3  F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3  specular    = numerator / denominator;

    vec3  kD    = (vec3(1.0) - F) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============================================================
//                         MAIN
// ============================================================
void main() {
    // ---- Read G-Buffer ----
    vec3  fragPos  = texture(gPosition, TexCoords).rgb;
    vec3  normal   = texture(gNormal, TexCoords).rgb;
    vec3  albedo   = texture(gColor, TexCoords).rgb;
    vec3  mr       = texture(gMetallicRoughness, TexCoords).rgb;
    float metallic = mr.r;
    float roughness = mr.g;

    // ---- Debug view (early-out) ----
    if (debugView > 0) {
        FragColor  = vec4(GetDebugColor(debugView, fragPos, normal, albedo,
                                        metallic, roughness), 1.0);
        BloomColor = vec4(0.0);
        return;
    }

    // ---- Skybox (background pixel) ----
    if (length(fragPos) < 0.001) {
        vec3 viewDirViewSpace = normalize(vec3(TexCoords * 2.0 - 1.0, -1.0));
        vec3 worldViewDir     = normalize((inverseViewMatrix * vec4(viewDirViewSpace, 0.0)).xyz);

        vec3 skyColor;
        if (showSkybox) {
            skyColor = texture(environmentMap, worldViewDir).rgb;
        } else {
            skyColor = getGradientSky(worldViewDir);
            skyColor = addSun(worldViewDir, skyColor);
            skyColor = addClouds(worldViewDir, skyColor);
        }
        FragColor  = vec4(skyColor, 1.0);
        BloomColor = vec4(0.0);
        return;
    }

    // ---- Surface vectors ----
    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - fragPos);
    vec3 L_dir = normalize(-lightDir);

    // ---- Shadows ----
    float shadow        = (dot(N, -lightDir) > 0.0)
                          ? ShadowCalculation(fragPos, N) : 0.0;
    float contactShadow = enableContactShadows
                          ? ContactShadow(fragPos, N, L_dir, TexCoords) : 0.0;

    float shadowFactor = 1.0 - shadow;
    shadowFactor = clamp(shadowFactor - contactShadow * 0.5, 0.0, 1.0);

    // ---- Ambient occlusion ----
    float ao = enableSSAO ? pow(texture(ssao, TexCoords).r, ssaoPower) : 1.0;

    // ---- Base ambient contribution ----
    vec3 ambient = albedo * 0.15 * ao;

    // ---- Directional light (legacy, uses uniform lightColor + lightDir) ----
    vec3 dirLight = CalculatePBR(N, V, L_dir, albedo, metallic, roughness,
                                 lightColor.rgb);
    dirLight *= shadowFactor;

    // ============================================================
    //              DYNAMIC LIGHTS (array)
    // ============================================================
    vec3 lightAccum = vec3(0.0);

    for (int i = 0; i < numLights; i++) {
        vec3  lightPos_i = lightPositions[i];
        vec3  lightCol   = lightColors[i];
        float intensity  = lightIntensities[i];
        int   type       = lightTypes[i];

        // ---- Ambient (type 3) — no PBR, just albedo * color * intensity ----
        if (type == 3) {
            lightAccum += lightCol * intensity * albedo;
            continue;
        }

        vec3  L    = normalize(lightPos_i - fragPos);
        float dist = length(lightPos_i - fragPos);
        float range = lightRanges[i];

        float attenuation = clamp(1.0 - (dist * dist) / (range * range), 0.0, 1.0);
        attenuation *= attenuation;
        attenuation  = max(attenuation, 0.0);

        // ---- Spot (type 1) ----
        if (type == 1) {
            vec3  spotDir = normalize(lightDirections[i]);
            float angle   = dot(-L, spotDir);
            float inner   = lightInnerCone[i];
            float outer   = lightOuterCone[i];
            float spotFactor = clamp((angle - outer) / (inner - outer), 0.0, 1.0);
            attenuation *= spotFactor;
        }

        // ---- Directional (type 2) ----
        if (type == 2) {
            L = -normalize(lightDirections[i]);
            attenuation = 1.0;
        }

        vec3 radiance = lightCol * intensity * attenuation;
        lightAccum += CalculatePBR(N, V, L, albedo, metallic, roughness, radiance);
    }

    // ---- Combine direct + ambient ----
    vec3 color = ambient + dirLight + lightAccum;

    // ============================================================
    //              ENVIRONMENT REFLECTIONS
    // ============================================================
    if (enableEnvReflections && roughness < 0.9) {
        vec3 R = reflect(-V, N);
        R.x = -R.x;

        float mipLevel = roughness * 4.0; // MAX_MIP_LEVEL = 4.0
        vec3  envColor = textureLod(environmentMap, R, mipLevel).rgb;

        float fresnel      = pow(1.0 - max(dot(N, V), 0.0), 5.0);
        float reflectivity = mix(0.04, 1.0, metallic);
        float roughFactor  = 1.0 - roughness * 0.3;

        float strength = envReflectionIntensity * reflectivity * fresnel * roughFactor;
        strength = clamp(strength, 0.0, 1.0);

        color += envColor * strength;
    }

    // ============================================================
    //              TONE MAPPING + COLOR GRADING
    // ============================================================
    color  = color / (color + vec3(1.0));   // Reinhard
    color *= exposure;
    color  = (color - 0.5) * contrast + 0.5;

    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luma), color, saturation);

    color = pow(color, vec3(1.0 / gamma));
    color = clamp(color, 0.0, 1.0);

    // ---- Emissive ----
    color += texture(gEmissive, TexCoords).rgb;

    FragColor = vec4(color, 1.0);

    // ============================================================
    //              BLOOM
    // ============================================================
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (enableBloom && brightness > bloomThreshold) {
        BloomColor = vec4(color * bloomIntensity, 1.0);
    } else {
        BloomColor = vec4(0.0);
    }
}
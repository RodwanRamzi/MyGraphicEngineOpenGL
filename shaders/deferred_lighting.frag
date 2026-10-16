#version 460 core

const float PI = 3.14159265359;
const int   NUM_CASCADES = 4;

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
uniform sampler2D shadowMap;          // legacy single shadow (fallback)

// ---- CSM ----
uniform sampler2DArray shadowMapArray;
uniform mat4  lightMatrices[NUM_CASCADES];
uniform float cascadeSplits[NUM_CASCADES];
uniform float cameraNear;
uniform float cameraFar;
uniform bool  useCSM;

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

uniform float ambientIntensity;

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

// ---- Shadow (directional, legacy) ----
uniform mat4  lightSpaceMatrix;
uniform vec3  lightDir;
uniform vec4  lightColor;

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

// ---- Emissive fallback ----
uniform vec3  uEmissiveColor;
uniform float uEmissiveIntensity;

// ---- SSS ----
uniform float sssIntensity;
uniform vec3  sssColor;
uniform float sssWidth;

// ---- Debug view ----
uniform int debugView;

// ---- Dynamic lights ----
#define MAX_LIGHTS 128
uniform int   numLights;
uniform vec3  lightPositions[MAX_LIGHTS];
uniform vec3  lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRanges[MAX_LIGHTS];
uniform int   lightTypes[MAX_LIGHTS];
uniform vec3  lightDirections[MAX_LIGHTS];
uniform float lightInnerCone[MAX_LIGHTS];
uniform float lightOuterCone[MAX_LIGHTS];

// ============================================================
//                        PBR
// ============================================================
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughnessSafe) {
    float a  = roughnessSafe * roughnessSafe;
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
//                    CSM SHADOW
// ============================================================
int selectCascade(float viewDepth) {
    for (int i = 0; i < NUM_CASCADES; ++i) {
        if (viewDepth < cascadeSplits[i]) return i;
    }
    return NUM_CASCADES - 1;
}

// 3x3 PCF sample for a given cascade
float sampleCascade(int cascade, vec2 uv, float compare, float bias) {
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMapArray, 0).xy);
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMapArray,
                vec3(uv + vec2(x, y) * texelSize, float(cascade))).r;
            sum += (compare - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    return sum / 9.0;
}

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

float sampleCascadePoisson(int cascade, vec2 uv, float compare, float bias) {
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMapArray, 0).xy);
    float sum = 0.0;

    // Rotate the disk based on UV (to avoid banding)
    float angle = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453) * 6.2831853;
    float s = sin(angle);
    float c = cos(angle);
    mat2 rot = mat2(c, -s, s, c);

    for (int i = 0; i < 16; ++i) {
        vec2 offset = rot * poissonDisk[i] * texelSize * 1.5;
        float pcfDepth = texture(shadowMapArray,
            vec3(uv + offset, float(cascade))).r;
        sum += (compare - bias > pcfDepth) ? 1.0 : 0.0;
    }
    return sum / 16.0;
}

// Transform world pos to cascade light-space UV + depth
bool worldToCascade(int cascade, vec3 worldPos,
                    out vec2 uv, out float depth)
{
    vec4 lp = lightMatrices[cascade] * vec4(worldPos, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj = proj * 0.5 + 0.5;

    uv    = proj.xy;
    depth = proj.z;

    // In range if inside [0,1] x [0,1] and z <= 1
    return (proj.z <= 1.0 &&
            proj.x >= 0.0 && proj.x <= 1.0 &&
            proj.y >= 0.0 && proj.y <= 1.0);
}

vec3 applyNormalOffset(vec3 fragPos, vec3 normal, float biasScale) {
    float cosTheta = max(dot(normal, normalize(-lightDir)), 0.0);
    float offsetScale = biasScale * (1.0 - cosTheta);
    return fragPos + normal * offsetScale;
}

// Main CSM shadow with cascade blending
float ShadowCalculationCSM(vec3 fragPos, vec3 normal, float viewDepth) {
    int cascade = selectCascade(viewDepth);

    float cosTheta = max(dot(normal, normalize(-lightDir)), 0.0);
    float bias = max(0.005 * (1.0 - cosTheta), 0.0005);

    vec3 offsetPos = applyNormalOffset(fragPos, normal, 0.05);

    vec2 uv;
    float depth;
    if (!worldToCascade(cascade, offsetPos, uv, depth)) {
        return 0.0;
    }

    float shadow = sampleCascadePoisson(cascade, uv, depth, bias);

    // ---- Cascade blending (wide) ----
    if (cascade < NUM_CASCADES - 1) {
        float splitStart = (cascade > 0) ? cascadeSplits[cascade - 1] : 0.0;
        float splitEnd   = cascadeSplits[cascade];
        float cascadeSize = splitEnd - splitStart;

        // Blend over 20% of the cascade size
        float blendBand = cascadeSize * 0.2;
        float distToSplit = splitEnd - viewDepth;

        if (distToSplit < blendBand) {
            vec2 uv2;
            float d2;
            if (worldToCascade(cascade + 1, offsetPos, uv2, d2)) {
                float shadow2 = sampleCascade(cascade + 1, uv2, d2, bias);

                // Smoothstep for smoother transition
                float t = 1.0 - (distToSplit / blendBand);
                t = t * t * (3.0 - 2.0 * t);  // smoothstep
                shadow = mix(shadow, shadow2, t);
            }
        }
    }

    return shadow;
}

// ============================================================
//              LEGACY SHADOW (single map, PCF)
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

// ============================================================
//                    CONTACT SHADOWS
// ============================================================
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
    if (view == 6) {
        float dist = length(fragPos - camPos);
        return vec3(1.0 - clamp(dist / 50.0, 0.0, 1.0));
    }
    if (view == 7) {  // Cascade coloring
        float viewDepth = -(viewMatrix * vec4(fragPos, 1.0)).z;
        int cascade = selectCascade(viewDepth);

        if (cascade == 0) return vec3(1.0, 0.0, 0.0);  // أحمر
        if (cascade == 1) return vec3(0.0, 1.0, 0.0);  // أخضر
        if (cascade == 2) return vec3(0.0, 0.0, 1.0);  // أزرق
        return vec3(1.0, 1.0, 0.0);                     // أصفر
    }

    return vec3(0.0);
}

// ============================================================
//                    LIGHTING HELPERS
// ============================================================
vec3 CalculatePBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic,
                  float roughness, vec3 radiance)
{
    float roughnessSafe = max(roughness, 0.15);

    vec3  H    = normalize(V + L);
    float NDF  = DistributionGGX(N, H, roughnessSafe);
    float G    = GeometrySmith(N, V, L, roughnessSafe);
    vec3  F0   = mix(vec3(0.04), albedo, metallic);
    vec3  F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3  specular    = numerator / denominator;

    vec3  kD    = (vec3(1.0) - F) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalculatePBR_SSS(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic,
                      float roughness, vec3 radiance, float sssMask,
                      float curvature)
{
    float roughnessSafe = max(roughness, 0.15);

    vec3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughnessSafe);
    float G   = GeometrySmith(N, V, L, roughnessSafe);
    vec3  F0  = mix(vec3(0.04), albedo, metallic);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    float sssWidth_curved = sssWidth * (1.0 + curvature * 0.5);
    sssWidth_curved = clamp(sssWidth_curved, 0.0, 1.0);

    float NdotL_orig = max(dot(N, L), 0.0);
    float NdotL_wrap = (dot(N, L) + sssWidth_curved) / (1.0 + sssWidth_curved);
    NdotL_wrap = max(NdotL_wrap, 0.0);

    float NdotL = mix(NdotL_orig, NdotL_wrap, sssMask);

    vec3 diffuse = kD * albedo / PI;
    vec3 sssTint = sssColor * sssIntensity * sssMask;
    diffuse = mix(diffuse, diffuse * sssTint, 0.5);

    return (diffuse + specular) * radiance * NdotL;
}

// ============================================================
//                    ACES TONE MAPPING
// ============================================================
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 GetIBL(vec3 N, float roughness, float ao) {
    vec3 irradiance = textureLod(environmentMap, N, 8.0).rgb;
    return irradiance * ao;
}

// ============================================================
//                         MAIN
// ============================================================
void main() {
    // ---- Read G-Buffer ----
    vec3  fragPos  = texture(gPosition, TexCoords).rgb;
    vec4  normalData = texture(gNormal, TexCoords);
    vec3  normal   = normalData.rgb;
    float sssMask  = normalData.a;
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

    // ---- Curvature from screen-space derivatives ----
    float curvature = 0.0;
    if (sssMask > 0.01) {
        float texelSize = 1.0 / textureSize(gNormal, 0).x;
        vec3 nRight = texture(gNormal, TexCoords + vec2(texelSize, 0)).rgb;
        vec3 nLeft  = texture(gNormal, TexCoords - vec2(texelSize, 0)).rgb;
        vec3 nUp    = texture(gNormal, TexCoords + vec2(0, texelSize)).rgb;
        vec3 nDown  = texture(gNormal, TexCoords - vec2(0, texelSize)).rgb;

        float diff = length(nRight - nLeft) + length(nUp - nDown);
        curvature = clamp(diff * 2.0, 0.0, 1.0);
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

    // ---- Shadows (CSM or legacy) ----
    float shadow = 0.0;
    if (dot(N, -lightDir) > 0.0) {
        if (useCSM) {
            // View-space depth for cascade selection
            float viewDepth = -(viewMatrix * vec4(fragPos, 1.0)).z;
            shadow = ShadowCalculationCSM(fragPos, N, viewDepth);
        } else {
            shadow = ShadowCalculation(fragPos, N);
        }
    }

    float contactShadow = enableContactShadows
                          ? ContactShadow(fragPos, N, L_dir, TexCoords) : 0.0;

    float shadowFactor = 1.0 - shadow;
    shadowFactor = clamp(shadowFactor - contactShadow * 0.5, 0.0, 1.0);

    // ---- Ambient occlusion ----
    float ao = 1.0;

    // ---- Hemispheric Ambient ----
    vec3 skyAmbient    = vec3(0.3, 0.4, 0.6);
    vec3 groundAmbient = vec3(0.15, 0.12, 0.10);
    vec3 hemi = mix(groundAmbient, skyAmbient, N.y * 0.5 + 0.5);

    // ---- Fake Bounce Light ----
    vec3 bounce = albedo * vec3(0.1, 0.08, 0.05) * max(-N.y, 0.0);

    vec3 ambient = (albedo * hemi * ao + bounce * ao) * ambientIntensity;

    // ---- Directional light (SSS version) ----
    vec3 dirLight = CalculatePBR_SSS(N, V, L_dir, albedo, metallic, roughness,
                                       lightColor.rgb, sssMask, curvature);
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

        if (type == 3) {
            lightAccum += lightCol * intensity * albedo;
            continue;
        }

        vec3  L    = normalize(lightPos_i - fragPos);
        float dist = length(lightPos_i - fragPos);
        float range = lightRanges[i];

        float d2 = dist * dist;
        float r2 = range * range;

        float window = clamp(1.0 - (d2 * d2) / (r2 * r2), 0.0, 1.0);
        window *= window;

        float attenuation = window / (d2 + 1.0);

        if (type == 1) {
            vec3  spotDir = normalize(lightDirections[i]);
            float angle   = dot(-L, spotDir);
            float inner   = lightInnerCone[i];
            float outer   = lightOuterCone[i];
            float spotFactor = clamp((angle - outer) / (inner - outer), 0.0, 1.0);
            attenuation *= spotFactor;
        }

        if (type == 2) {
            L = -normalize(lightDirections[i]);
            attenuation = 1.0;
        }

        vec3 radiance = lightCol * intensity * attenuation;
        lightAccum += CalculatePBR_SSS(N, V, L, albedo, metallic, roughness,
                                        radiance, sssMask, curvature);
    }

    vec3 color = ambient + dirLight + lightAccum;

    // ============================================================
    //              ENVIRONMENT REFLECTIONS
    // ============================================================
    if (enableEnvReflections && roughness < 0.9) {
        vec3 R = reflect(-V, N);
        R.x = -R.x;

        float mipLevel = roughness * 4.0;
        vec3  envColor = textureLod(environmentMap, R, mipLevel).rgb;

        float fresnel      = pow(1.0 - max(dot(N, V), 0.0), 5.0);
        float reflectivity = mix(0.04, 1.0, metallic);
        float roughFactor  = 1.0 - roughness * 0.3;

        float strength = envReflectionIntensity * reflectivity * fresnel * roughFactor;
        strength = clamp(strength, 0.0, 1.0);

        color += envColor * strength;
    }

    // ---- Emissive ----
    color += texture(gEmissive, TexCoords).rgb;

    // ============================================================
    //              HDR BLOOM EXTRACTION (Soft Knee)
    // ============================================================
    if (enableBloom) {
        float brightness = max(max(color.r, color.g), color.b);

        float knee = bloomThreshold * 0.6;
        float soft = clamp(brightness - bloomThreshold + knee, 0.0, 2.0 * knee);
        soft = soft * soft / (4.0 * knee + 0.00001);
        float contribution = max(soft, brightness - bloomThreshold) / max(brightness, 0.00001);

        BloomColor = vec4(color * contribution, 1.0);
    } else {
        BloomColor = vec4(0.0);
    }

    // ============================================================
    //              TONE MAPPING
    // ============================================================
    color *= exposure;
    color = ACESFilm(color);
    color = (color - 0.5) * contrast + 0.5;

    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luma), color, saturation);

    color = pow(color, vec3(1.0 / gamma));
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, 1.0);
}
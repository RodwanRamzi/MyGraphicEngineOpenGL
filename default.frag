#version 460 core
const float PI = 3.14159265359;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

in vec3 crntPos;
in vec3 Normal;
in vec2 texCoord;
in vec4 fragPosLight;
in mat3 TBN;

uniform sampler2D diffuse0;
uniform sampler2D normal0;
uniform sampler2D metallicRoughness0;
uniform sampler2D heightMap0;        // ✅ POM Height Map
uniform sampler2D shadowMap;

uniform vec3 camPos;
uniform vec4 lightColor;
uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 lightPos2;
uniform mat4 lightSpaceMatrix;
uniform vec3 skyColor;

uniform int useNormalMap;
uniform int useMetallicRoughness;
uniform int useHeightMap;            // ✅ تشغيل/إيقاف POM
uniform float heightScale;           // ✅ قوة POM
uniform float saturation;
uniform float exposure;

// ==================== POM FUNCTIONS ====================
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float heightScale = 0.05;
    float minLayers = 8.0;
    float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;
    
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(heightMap0, currentTexCoords).r;
    
    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(heightMap0, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }
    
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightMap0, prevTexCoords).r - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords;
}

// ==================== PBR FUNCTIONS ====================
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * 
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

float ShadowCalculation(vec4 fragPosLight) {
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth - 0.005 > closestDepth ? 0.7 : 0.0;
    return shadow;
}

// ==================== GET MATERIAL PROPERTIES ====================
void getMaterialProperties(vec2 uv, out vec3 albedo, out vec3 normal, out float metallic, out float roughness)
{
    // 1. Diffuse / Albedo
    albedo = texture(diffuse0, uv).rgb;
    
    // 2. Normal Map (مع POM)
    normal = normalize(Normal);
    if (useNormalMap == 1) {
        normal = normalize(TBN * (texture(normal0, uv).xyz * 2.0 - 1.0));
    }
    
    // 3. Metallic + Roughness (من metallicRoughness0)
    metallic = 0.0;
    roughness = 1.0;
    if (useMetallicRoughness == 1) {
        metallic = texture(metallicRoughness0, uv).b;  // Blue = Metallic
        roughness = texture(metallicRoughness0, uv).g; // Green = Roughness
        if (roughness <= 0.0) roughness = 0.05;
        if (metallic < 0.0) metallic = 0.0;
    }
}

// ==================== LIGHTING FUNCTIONS ====================

// ---------- DIRECTIONAL LIGHT ----------
vec4 direcLight(vec3 albedo, vec3 N, float metallic, float roughness)
{
    vec3 V = normalize(camPos - crntPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    float shadow = ShadowCalculation(fragPosLight);
    vec3 radiance = lightColor.rgb;
    
    // النتيجة النهائية
    vec3 directLight = (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);
    
    // ✅ Ambient خفيف جداً لإظهار النسيج
    vec3 ambient = albedo * 0.15;  // 15% فقط من لون النسيج
    
    return vec4(ambient + directLight, 1.0);
}

// ---------- POINT LIGHT 1 ----------
vec4 pointLight(vec3 albedo, vec3 N, float metallic, float roughness)
{
    vec3 V = normalize(camPos - crntPos);
    vec3 lightVec = lightPos - crntPos;
    float dist = length(lightVec);
    float attenuation = 1.0 / (dist * dist + 0.01);
    vec3 L = normalize(lightVec);
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = lightColor.rgb * attenuation;
    vec3 directLight = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // ✅ Ambient خفيف
    vec3 ambient = albedo * 0.05;
    
    return vec4(ambient + directLight, 1.0);
}

// ---------- POINT LIGHT 2 (Warm) ----------
vec4 pointLight2(vec3 albedo, vec3 N, float metallic, float roughness)
{
    vec3 V = normalize(camPos - crntPos);
    vec3 lightVec = lightPos2 - crntPos;
    float dist = length(lightVec);
    float attenuation = 1.0 / (dist * dist + 0.01);
    vec3 L = normalize(lightVec);
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = vec3(1.0, 0.5, 0.3) * attenuation;
    vec3 directLight = (kD * albedo / PI + specular) * radiance * NdotL;
    
    vec3 ambient = skyColor * albedo * 0.1 * kD;
    
    return vec4(ambient + directLight, 1.0);
}

// ---------- SPOT LIGHT ----------
vec4 spotLight(vec3 albedo, vec3 N, float metallic, float roughness)
{
    vec3 V = normalize(camPos - crntPos);
    vec3 lightVec = lightPos - crntPos;
    float dist = length(lightVec);
    float attenuation = 1.0 / (dist * dist + 0.01);
    vec3 L = normalize(lightVec);
    vec3 H = normalize(V + L);

    float outerCone = 0.90f;
    float innerCone = 0.95f;
    float angle = dot(vec3(0.0f, -1.0f, 0.0f), -L);
    float spotIntensity = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = lightColor.rgb * attenuation * spotIntensity;
    vec3 directLight = (kD * albedo / PI + specular) * radiance * NdotL;
    
    vec3 ambient = skyColor * albedo * 0.1 * kD;
    
    return vec4(ambient + directLight, 1.0);
}

// ==================== MAIN ====================
void main() {
    // Step 1: POM - Adjust UVs based on height map
    vec3 viewDir = normalize(TBN * (camPos - crntPos));
    vec2 finalTexCoord = texCoord;
    
    if (useHeightMap == 1) {
        float heightValue = texture(heightMap0, texCoord).r;
        if (heightValue > 0.0) {
            finalTexCoord = ParallaxMapping(texCoord, viewDir);
        }
    }
    
    // Step 2: Get material properties with POM-adjusted UVs
    vec3 albedo;
    vec3 normal;
    float metallic;
    float roughness;
    getMaterialProperties(finalTexCoord, albedo, normal, metallic, roughness);
    
    // Step 3: Calculate all 4 lights
    vec4 dirLight = direcLight(albedo, normal, metallic, roughness);
    vec4 pointLight = pointLight(albedo, normal, metallic, roughness);
    vec4 pointLight2 = pointLight2(albedo, normal, metallic, roughness);
    vec4 spotLight = spotLight(albedo, normal, metallic, roughness);
    
    // Step 4: Combine
    vec3 color = dirLight.rgb * 0.7 + pointLight.rgb * 0.8 + pointLight2.rgb * 0.5 + spotLight.rgb * 0.6;
    
    // Step 5: Post-Processing
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luma), color, saturation);
    color = vec3(1.0) - exp(-color * exposure);
    
    FragColor = vec4(color, 1.0);
    
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    BloomColor = (brightness > 1.0) ? vec4(color, 1.0) : vec4(0.0);
}
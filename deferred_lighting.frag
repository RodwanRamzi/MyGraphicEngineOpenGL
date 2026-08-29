#version 460 core
const float PI = 3.14159265359;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gMetallicRoughness;
uniform sampler2D shadowMap;
uniform sampler2D ssao;

uniform vec3 camPos;
uniform vec4 lightColor;
uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 lightPos2;
uniform mat4 lightSpaceMatrix;

uniform float saturation;
uniform float exposure;

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

// ==================== SHADOW ====================
float ShadowCalculation(vec3 fragPos, vec3 normal) {
    vec3 fragPosOffset = fragPos + normal * 0.02;
    vec4 fragPosLight = lightSpaceMatrix * vec4(fragPosOffset, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth ? 1.0 : 0.0);
        }
    }
    return shadow / 9.0;
}

void main() {
    // Read G-Buffer
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 albedo = texture(gColor, TexCoords).rgb;
    vec3 mr = texture(gMetallicRoughness, TexCoords).rgb;
    
    float metallic = mr.r;
    float roughness = mr.g;
    
    
    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - fragPos);
    float shadow = (dot(N, -lightDir) > 0.0) ? ShadowCalculation(fragPos, N) : 0.0;
    float ao = texture(ssao, TexCoords).r;
    
    // ==================== AMBIENT ====================
    vec3 ambient = albedo * 0.15 * ao;
    
    // ==================== DIRECTIONAL LIGHT ====================
    vec3 L_dir = normalize(-lightDir);
    vec3 H_dir = normalize(V + L_dir);

    float NDF_dir = DistributionGGX(N, H_dir, roughness);
    float G_dir = GeometrySmith(N, V, L_dir, roughness);
    vec3 F0_dir = vec3(0.04);
    F0_dir = mix(F0_dir, albedo, metallic);
    vec3 F_dir = fresnelSchlick(max(dot(H_dir, V), 0.0), F0_dir);

    vec3 numerator_dir = NDF_dir * G_dir * F_dir;
    float denominator_dir = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L_dir), 0.0) + 0.0001;
    vec3 specular_dir = numerator_dir / denominator_dir;

    vec3 kD_dir = vec3(1.0) - F_dir;
    kD_dir *= 1.0 - metallic;

    float NdotL_dir = max(dot(N, L_dir), 0.0);
    vec3 dirLight = (kD_dir * albedo / PI + specular_dir) * lightColor.rgb * NdotL_dir * (1.0 - shadow);
    
    // ==================== POINT LIGHT 1 ====================
    vec3 lightVec1 = lightPos - fragPos;
    float dist1 = length(lightVec1);
    float attenuation1 = 1.0 / (dist1 * dist1 + 0.01);
    vec3 L1 = normalize(lightVec1);
    vec3 H1 = normalize(V + L1);

    float NDF1 = DistributionGGX(N, H1, roughness);
    float G1 = GeometrySmith(N, V, L1, roughness);
    vec3 F0_1 = vec3(0.04);
    F0_1 = mix(F0_1, albedo, metallic);
    vec3 F1 = fresnelSchlick(max(dot(H1, V), 0.0), F0_1);

    vec3 numerator1 = NDF1 * G1 * F1;
    float denominator1 = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L1), 0.0) + 0.0001;
    vec3 specular1 = numerator1 / denominator1;

    vec3 kD1 = vec3(1.0) - F1;
    kD1 *= 1.0 - metallic;

    float NdotL1 = max(dot(N, L1), 0.0);
    vec3 pointLight1 = (kD1 * albedo / PI + specular1) * lightColor.rgb * attenuation1 * NdotL1;
    
    // ==================== POINT LIGHT 2 ====================
    vec3 lightVec2 = lightPos2 - fragPos;
    float dist2 = length(lightVec2);
    float attenuation2 = 1.0 / (dist2 * dist2 + 0.01);
    vec3 L2 = normalize(lightVec2);
    vec3 H2 = normalize(V + L2);

    float NDF2 = DistributionGGX(N, H2, roughness);
    float G2 = GeometrySmith(N, V, L2, roughness);
    vec3 F0_2 = vec3(0.04);
    F0_2 = mix(F0_2, albedo, metallic);
    vec3 F2 = fresnelSchlick(max(dot(H2, V), 0.0), F0_2);

    vec3 numerator2 = NDF2 * G2 * F2;
    float denominator2 = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L2), 0.0) + 0.0001;
    vec3 specular2 = numerator2 / denominator2;

    vec3 kD2 = vec3(1.0) - F2;
    kD2 *= 1.0 - metallic;

    float NdotL2 = max(dot(N, L2), 0.0);
    vec3 pointLight2 = (kD2 * albedo / PI + specular2) * lightColor.rgb * attenuation2 * NdotL2;
    
    // ==================== SPOT LIGHT ====================
    vec3 lightVec3 = lightPos - fragPos;
    float dist3 = length(lightVec3);
    float attenuation3 = 1.0 / (dist3 * dist3 + 0.01);
    vec3 L3 = normalize(lightVec3);
    vec3 H3 = normalize(V + L3);

    float outerCone = 0.90f;
    float innerCone = 0.95f;
    float angle = dot(vec3(0.0f, -1.0f, 0.0f), -L3);
    float spotIntensity = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

    float NDF3 = DistributionGGX(N, H3, roughness);
    float G3 = GeometrySmith(N, V, L3, roughness);
    vec3 F0_3 = vec3(0.04);
    F0_3 = mix(F0_3, albedo, metallic);
    vec3 F3 = fresnelSchlick(max(dot(H3, V), 0.0), F0_3);

    vec3 numerator3 = NDF3 * G3 * F3;
    float denominator3 = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L3), 0.0) + 0.0001;
    vec3 specular3 = numerator3 / denominator3;

    vec3 kD3 = vec3(1.0) - F3;
    kD3 *= 1.0 - metallic;

    float NdotL3 = max(dot(N, L3), 0.0);
    vec3 spotLight3 = (kD3 * albedo / PI + specular3) * lightColor.rgb * attenuation3 * spotIntensity * NdotL3;
    
    // ==================== COMBINE ====================
    vec3 color = ambient + dirLight + pointLight1 * 0.8 + pointLight2 * 0.5 + spotLight3 * 0.6;
    
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luma), color, saturation);
    
    color = vec3(1.0) - exp(-color * exposure);
    
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
    
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BloomColor = vec4(color, 1.0);
    else
        BloomColor = vec4(0.0);
}
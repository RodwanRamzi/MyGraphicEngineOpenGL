#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D colorTexture;
uniform sampler2D noiseTexture;

uniform mat4 inverseViewMatrix;
uniform mat4 inverseProjectionMatrix;
uniform vec3 camPos;
uniform float fogDensity;
uniform float fogHeight;
uniform float fogFalloff;
uniform int fogSteps;
uniform float fogMaxDistance;
uniform vec3 fogColor;
uniform vec3 lightPos;
uniform float volumetricLightIntensity;

// Reconstruct world position from depth
vec3 WorldPosFromDepth(float depth, vec2 uv) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = inverseProjectionMatrix * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = inverseViewMatrix * viewPos;
    return worldPos.xyz;
}

// Simple shadow for volumetric light (simplified)
float VolumetricShadow(vec3 samplePos, vec3 lightDir) {
    // إذا كان لديك shadowMap, استخدمه هنا، وإلا أرجع 0
    return 0.0;
}

// حساب الإضاءة الحجمية
float calculateVolumetricLight(vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(lightPos - fragPos);
    float lightDepth = length(lightPos - fragPos);
    float stepSize = lightDepth / float(fogSteps);
    float density = 0.0;
    for (int i = 0; i < fogSteps; i++) {
        vec3 samplePos = fragPos + viewDir * stepSize * float(i);
        float shadow = VolumetricShadow(samplePos, lightDir);
        density += (1.0 - shadow) * fogDensity * stepSize;
    }
    return exp(-density) * volumetricLightIntensity;
}

void main() {
    float depth = texture(depthTexture, TexCoords).r;
    vec3 worldPos = WorldPosFromDepth(depth, TexCoords);
    vec3 viewDir = normalize(worldPos - camPos);
    float dist = length(worldPos - camPos);

    // كثافة الضباب المعتمدة على الارتفاع
    float heightFactor = exp(-fogFalloff * (worldPos.y - fogHeight));
    float density = fogDensity * heightFactor;

    // راي مارشينغ
    float totalFog = 0.0;
    float stepSize = min(dist, fogMaxDistance) / float(fogSteps);
    vec3 rayPos = camPos;
    for (int i = 0; i < fogSteps; i++) {
        rayPos += viewDir * stepSize;
        float y = rayPos.y;
        float h = exp(-fogFalloff * (y - fogHeight));
        totalFog += density * h * stepSize;
        if (totalFog > 1.0) break;
    }
    totalFog = clamp(totalFog, 0.0, 1.0);

    // الإضاءة الحجمية
    float volLight = calculateVolumetricLight(worldPos, viewDir);
    vec3 color = texture(colorTexture, TexCoords).rgb;
    color += volLight * volumetricLightIntensity; // إضافة الإضاءة الحجمية

    vec3 finalColor = mix(color, fogColor, totalFog);
    FragColor = vec4(finalColor, 1.0);
}
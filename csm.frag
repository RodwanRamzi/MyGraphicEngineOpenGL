#version 460 core
uniform int numCascades;
uniform float cascadeSplitDistances[4];
uniform sampler2DArrayShadow cascadeShadowMaps;
uniform mat4 cascadeLightMatrices[4];

float CSMShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir) {
    // اختيار الـ Cascade المناسب
    int cascadeIndex = 0;
    float depth = length(fragPos - camPos);
    for (int i = 0; i < numCascades - 1; i++) {
        if (depth > cascadeSplitDistances[i]) {
            cascadeIndex = i + 1;
        }
    }
    
    // حساب إحداثيات الـ Shadow Map
    vec4 fragPosLight = cascadeLightMatrices[cascadeIndex] * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // PCF مع الـ Cascade
    float shadow = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec3 coord = vec3(projCoords.xy + vec2(x, y) * texelSize, cascadeIndex);
            shadow += texture(cascadeShadowMaps, coord).r;
        }
    }
    return shadow / 9.0;
}
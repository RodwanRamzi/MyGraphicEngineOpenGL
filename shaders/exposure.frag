#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D currentLuminance;
uniform sampler2D previousExposure;
uniform float deltaTime;
uniform float minExposure;
uniform float maxExposure;
uniform float keyValue;
uniform float adaptationSpeed;
uniform float currentExposure;

void main() {
    float avgLum = texture(currentLuminance, vec2(0.5)).r;
    avgLum = max(avgLum, 1e-4);
    
    float rawLum = avgLum / max(currentExposure, 0.001);
    float targetExposure = keyValue / rawLum;
    targetExposure = clamp(targetExposure, minExposure, maxExposure);
    
    float prevExposure = texture(previousExposure, vec2(0.5)).r;
    float alpha = 1.0 - exp(-deltaTime * adaptationSpeed);
    float newExposure = mix(prevExposure, targetExposure, alpha);
    
    FragColor = vec4(newExposure, 0.0, 0.0, 1.0);
}
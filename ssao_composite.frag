#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D ssaoTexture;

uniform float exposure;
uniform float saturation;
uniform float ssaoStrength;

void main() {
    vec3 sceneColor = texture(sceneTexture, TexCoords).rgb;
    float ssao = texture(ssaoTexture, TexCoords).r;
    
    // ✅ SSAO يكون مؤثراً فقط في المناطق المظلمة (الزوايا)
    // وفي المناطق المضيئة (الأسطح المسطحة) لا يؤثر
    float ssaoFactor = mix(1.0, ssao, ssaoStrength);
    
    // ✅ تطبيق SSAO مع الحفاظ على النسيج
    vec3 finalColor = sceneColor * ssaoFactor;
    
    // Saturation
    float luma = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    finalColor = mix(vec3(luma), finalColor, saturation);
    
    // Exposure
    finalColor = vec3(1.0) - exp(-finalColor * exposure);
    
    // Gamma
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    
    FragColor = vec4(finalColor, 1.0);
}
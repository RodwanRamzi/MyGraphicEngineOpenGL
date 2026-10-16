#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D gtaoTexture;

uniform float exposure;
uniform float saturation;
uniform float gtaoStrength;

void main() {
    vec3 sceneColor = texture(sceneTexture, TexCoords).rgb;
    float gtao = texture(gtaoTexture, TexCoords).r;
    
    // ✅ gtao يكون مؤثراً فقط في المناطق المظلمة (الزوايا)
    // وفي المناطق المضيئة (الأسطح المسطحة) لا يؤثر
    float gtaoFactor = mix(1.0, gtao, gtaoStrength);
    
    // ✅ تطبيق gtao مع الحفاظ على النسيج
    vec3 finalColor = sceneColor * gtaoFactor;
    
    // Saturation
    float luma = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    finalColor = mix(vec3(luma), finalColor, saturation);
    
    // Exposure
    finalColor = vec3(1.0) - exp(-finalColor * exposure);
    
    // Gamma
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    
    FragColor = vec4(finalColor, 1.0);
}
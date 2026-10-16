#version 460 core
out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D screenTexture;  // HDR Scene (raw)
uniform sampler2D bloom;          // Bloom (HDR)

// Controls
uniform float exposure;           // تعرض الكاميرا (1.0 = افتراضي)
uniform float bloomStrength;      // قوة الـ Bloom (0.02 - 0.15)
uniform float saturationAmount;   // التشبع (0.0 - 2.0)
uniform float contrast;           // التباين (0.8 - 1.5)
uniform float gamma;              // 2.2 افتراضي

// ==========================================
// ACES Tone Mapping (Filmic)
// ==========================================
vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    // 1. Sample HDR scene + HDR bloom
    vec3 hdrColor   = texture(screenTexture, texCoords).rgb;
    vec3 bloomColor = texture(bloom, texCoords).rgb;

    // 2. Combine in HDR space
    vec3 combined = hdrColor + bloomColor * bloomStrength;

    // 3. Exposure (in HDR)
    combined *= exposure;

    // 4. ACES Tone Mapping (HDR → LDR) — مرة واحدة بس!
    vec3 mapped = ACESFilm(combined);

    // 5. Color Grading (in LDR)
    // Contrast
    mapped = (mapped - 0.5) * contrast + 0.5;

    // Saturation
    float luma = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
    mapped = mix(vec3(luma), mapped, saturationAmount);

    // 6. Gamma Correction
    mapped = pow(max(mapped, vec3(0.0)), vec3(1.0 / gamma));

    // 7. Final Clamp
    FragColor = vec4(clamp(mapped, 0.0, 1.0), 1.0);
}
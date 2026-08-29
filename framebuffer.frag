#version 460 core
out vec4 FragColor;

in vec2 texCoords; 

uniform sampler2D screenTexture; 
uniform sampler2D bloom;         

// ✅ Changed to uniforms so ImGui can control them!
uniform float exposure;
uniform float saturationAmount;

void main()
{
    // 1. Sample the raw scene and the bloom mask
    vec3 hdrColor = texture(screenTexture, texCoords).rgb;
    vec3 bloomColor = texture(bloom, texCoords).rgb;

    // 2. Add the bloom to the scene
    vec3 combined = hdrColor + bloomColor;

    // 3. Saturation logic 
    float luma = 0.2126f * combined.r + 0.7152f * combined.g + 0.0722f * combined.b;
    vec3 grayscale = vec3(luma);
    vec3 saturatedColor = mix(grayscale, combined, saturationAmount);

    // 4. Exposure / Tone mapping 
    vec3 toneMapped = vec3(1.0f) - exp(-saturatedColor * exposure);

    // 5. Gamma Correction
    float gamma = 2.2f;
    FragColor.rgb = pow(toneMapped, vec3(1.0f / gamma));
    FragColor.a = 1.0f;
}
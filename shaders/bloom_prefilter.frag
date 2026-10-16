#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform float threshold;  
uniform float knee;      
uniform float exposure;   

void main() {
    vec3 color = texture(hdrBuffer, TexCoords).rgb;
    color *= exposure;   

    // --- Soft Knee Threshold (Jimenez) ---
    float brightness = max(max(color.r, color.g), color.b);
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);

    FragColor = vec4(color * contribution, 1.0);
}
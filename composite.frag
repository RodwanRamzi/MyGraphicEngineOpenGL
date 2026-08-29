#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;   // HDR color from lighting pass
uniform sampler2D bloom;   // Blurred bright texture
uniform float bloomStrength; 
uniform float contrast; 

void main() {
    vec3 hdrColor = texture(scene, vec2(TexCoords.x, 1.0 - TexCoords.y)).rgb; // Flip Y!
    vec3 bloomColor = texture(bloom, vec2(TexCoords.x, 1.0 - TexCoords.y)).rgb;
    
    // Combine scene and bloom
    vec3 result = hdrColor + bloomColor * bloomStrength;
    
    // Apply contrast
    result = (result - 0.5) * contrast + 0.5;
    
    // Tone Mapping
    result = result / (result + vec3(1.0));
    
    // Gamma
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, 1.0);
}
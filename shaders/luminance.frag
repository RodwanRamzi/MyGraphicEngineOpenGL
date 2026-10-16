#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform int maxMipLevel;

void main() {
    vec3 c = textureLod(sceneTexture, vec2(0.5), float(maxMipLevel)).rgb;
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    lum = max(lum, 1e-4);
    FragColor = vec4(lum, 0.0, 0.0, 1.0);
}
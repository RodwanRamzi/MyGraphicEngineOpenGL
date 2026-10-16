#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D ssrTexture;
uniform float ssrIntensity;

void main() {
    vec3 ssr = texture(ssrTexture, TexCoords).rgb;
    FragColor = vec4(ssr * ssrIntensity, 1.0);
}
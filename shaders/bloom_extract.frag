#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;
uniform float threshold; // Only extract pixels brighter than this

void main() {
    vec4 color = texture(scene, TexCoords);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold)
        FragColor = color;
    else
        FragColor = vec4(0.0);
}
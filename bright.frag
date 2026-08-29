#version 460 core
out vec4 FragColor;
in vec2 texCoord;
uniform sampler2D scene;
uniform float threshold;
void main() {
    vec3 color = texture(scene, texCoord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0);
}
#version 460 core
out vec4 FragColor;
uniform vec3 gridColor;
void main() {
    FragColor = vec4(gridColor, 1.0);
}
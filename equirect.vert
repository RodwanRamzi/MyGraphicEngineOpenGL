#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 camMatrix;

void main() {
    localPos = aPos;
    gl_Position = camMatrix * vec4(aPos, 1.0);
}
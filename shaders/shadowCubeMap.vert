#version 460 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 shadowMatrices[6];
uniform int faceIndex; 
void main()
{
    gl_Position = shadowMatrices[faceIndex] * model * vec4(aPos, 1.0f);
    //gl_Position = model * vec4(aPos, 1.0);
}
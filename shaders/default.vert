#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTex;
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;

out vec3 crntPos;
out vec3 Normal;
out vec3 color;
out vec2 texCoord;
out vec4 fragPosLight;
out mat3 TBN;

uniform mat4 camMatrix;
uniform mat4 model;
uniform mat4 translation;
uniform mat4 rotation;
uniform mat4 scale;
uniform mat4 lightProjection;

void main()
{
    mat4 finalModel = model * translation * rotation * scale;
    gl_Position = camMatrix * finalModel * vec4(aPos, 1.0f);
    crntPos = vec3(finalModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(finalModel))) * aNormal;
    color = aColor;
    
    // UVs (Fixed for proper texturing!)
    texCoord = mat2(1.0, 0.0, 0.0, -1.0) * aTex + vec2(0.0, 1.0);
    
    // Shadow coordinates
    fragPosLight = lightProjection * vec4(crntPos, 1.0f);
    
    // TBN
    vec3 T = normalize(mat3(finalModel) * aTangent);
    vec3 B = normalize(mat3(finalModel) * aBitangent);
    vec3 N = normalize(mat3(finalModel) * aNormal);
    TBN = mat3(T, B, N);
}
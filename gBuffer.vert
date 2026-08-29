#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;      // ✅ يجب أن يكون هذا موجوداً!
///layout (location = 3) in vec3 aColor;          // اختياري
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 texCoord;          // ✅ يجب أن يكون هذا موجوداً!
out mat3 TBN;

uniform mat4 model;
uniform mat4 camMatrix;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    texCoord = aTexCoord;    // ✅ تأكد من تمرير texCoord!
    gl_Position = camMatrix * vec4(FragPos, 1.0);
    
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    TBN = mat3(T, B, N);
}
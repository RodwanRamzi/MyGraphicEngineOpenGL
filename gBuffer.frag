#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gColor;
layout (location = 3) out vec3 gMetallicRoughness;

in vec3 FragPos;
in vec3 Normal;
in vec2 texCoord;
in mat3 TBN;

uniform sampler2D diffuse0;
uniform sampler2D normal0;
uniform sampler2D metallicRoughness0;
uniform sampler2D heightMap0;

uniform vec3 camPos;

uniform int useNormalMap;
uniform int useMetallicRoughness;
uniform int useHeightMap;

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    // ✅ قلب الـ UV عمودياً
    vec2 flippedUV = vec2(texCoord.x, 1.0 - texCoord.y);
    vec3 texColor = texture(diffuse0, flippedUV).rgb;
    
    gColor = texColor;
    gMetallicRoughness = vec3(0.0, 1.0, 0.0);
}
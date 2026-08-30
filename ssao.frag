#version 460 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;
uniform vec3 samples[64];
uniform float radius;      // <-- we'll set this
uniform float bias;
uniform vec2 noiseScale;
uniform mat4 projection;

void main() {
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 randomVec = texture(noiseTexture, TexCoords * noiseScale).rgb;

    // TBN matrix
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {
        // Multiply the sample vector by the radius uniform
        vec3 samplePos = TBN * samples[i] * radius;
        samplePos = fragPos + samplePos;

        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(gPosition, offset.xy).z;
        float rangeCheck = abs(fragPos.z - sampleDepth) < bias ? 1.0 : 0.0;
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / 64.0);
    FragColor = occlusion;
}
#version 460 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;

uniform mat4 view;         // ← view matrix
uniform mat4 projection;   // ← projection matrix

uniform vec3  samples[64];
uniform float radius;
uniform float bias;
uniform vec2  noiseScale;

void main() {
    // ---- Read G-Buffer ----
    vec3 fragPosWorld = texture(gPosition, TexCoords).rgb;
    vec3 normalWorld  = texture(gNormal, TexCoords).rgb;

    // ---- Background check ----
    // (لو الـ fragment مش موجود، القيمة = 0 → نرجع 1.0)
    if (length(fragPosWorld) < 0.0001) {
        FragColor = 1.0;
        return;
    }

    // ---- Convert to view space ----
    vec3 fragPosView = (view * vec4(fragPosWorld, 1.0)).xyz;
    vec3 normalView  = normalize(mat3(view) * normalWorld);

    // ---- Random vector (view space) ----
    vec3 randomVec = texture(noiseTexture, TexCoords * noiseScale).rgb;
    randomVec = randomVec * 2.0 - 1.0;   // [0,1] → [-1,1]

    // ---- TBN matrix (view space) ----
    vec3 tangent   = normalize(randomVec - normalView * dot(randomVec, normalView));
    vec3 bitangent = cross(normalView, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normalView);

    // ---- Sample occlusion ----
    float occlusion = 0.0;
    int   validSamples = 0;
    const int KERNEL_SIZE = 64;

    for (int i = 0; i < KERNEL_SIZE; ++i) {
        // Sample position in view space
        vec3 samplePosView = fragPosView + (TBN * samples[i]) * radius;

        // Project to screen space
        vec4 offset = projection * vec4(samplePosView, 1.0);
        offset.xyz /= offset.w;
        offset.xyz  = offset.xyz * 0.5 + 0.5;

        // Skip samples outside screen
        if (offset.x < 0.0 || offset.x > 1.0 ||
            offset.y < 0.0 || offset.y > 1.0) continue;

        validSamples++;

        // Read the actual depth at the sample's screen position
        vec3 samplePosWorld = texture(gPosition, offset.xy).rgb;
        if (length(samplePosWorld) < 0.0001) continue;   // background

        vec3  samplePosViewActual = (view * vec4(samplePosWorld, 1.0)).xyz;
        float sampleDepth = samplePosViewActual.z;

        // Range check
        float rangeCheck = abs(fragPosView.z - sampleDepth) < radius ? 1.0 : 0.0;

        // Occlusion test
        occlusion += (sampleDepth <= samplePosView.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    // ---- Normalize ----
    if (validSamples > 0) {
        occlusion = 1.0 - (occlusion / float(validSamples));
    } else {
        occlusion = 1.0;
    }
    
    FragColor = float(validSamples) / 64.0;
    return;

    FragColor = occlusion;
}
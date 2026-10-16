#version 460 core

out float FragColor;
in vec2 TexCoords;

// ============================================================
//                    GTAO — Ground Truth AO
// ============================================================
// Simplified: 8 directions, 6 steps each

uniform sampler2D gDepth;      // linear depth OR depth buffer
uniform sampler2D gNormal;     // world-space normals (will convert)
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 view;             // to convert normals to view space
uniform float radius;
uniform float intensity;

const int DIRECTIONS = 8;
const int STEPS = 6;

// ============================================================
//  Reconstruct view-space position from depth + uv
// ============================================================
vec3 viewPosFromDepth(vec2 uv, float depth) {
    // Convert [0,1] depth to NDC [-1,1]
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inverseProjection * clip;
    return view.xyz / view.w;
}

void main() {
    float depth = texture(gDepth, TexCoords).r;

    // Background check (depth = 1.0 in most cases)
    if (depth >= 1.0) {
        FragColor = 1.0;
        return;
    }

    // ---- Reconstruct position and normal (view space) ----
    vec3 P = viewPosFromDepth(TexCoords, depth);

    // Normal: G-Buffer stores world-space, convert to view-space
    vec3 N_world = texture(gNormal, TexCoords).rgb;
    vec3 N = normalize(mat3(view) * N_world);

    // ---- Estimate screen-space radius ----
    // radius is in world units; convert to screen pixels
    float screenRadius = radius * projection[1][1] / max(-P.z, 0.001);
    float texelSize = 1.0 / float(textureSize(gDepth, 0).x);
    screenRadius *= texelSize;

    // ---- Accumulate occlusion ----
    float occlusion = 0.0;
    float validCount = 0.0;

    for (int d = 0; d < DIRECTIONS; ++d) {
        float angle = (float(d) / float(DIRECTIONS)) * 6.2831853;
        vec2 dir = vec2(cos(angle), sin(angle));

        float maxHorizon = -1.0;

        for (int s = 1; s <= STEPS; ++s) {
            float stepScale = float(s) / float(STEPS);
            vec2 sampleUV = TexCoords + dir * screenRadius * stepScale;

            // Skip samples outside screen
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
                sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;

            float sampleDepth = texture(gDepth, sampleUV).r;
            if (sampleDepth >= 1.0) continue;

            // Reconstruct sample position in view space
            vec3 S = viewPosFromDepth(sampleUV, sampleDepth);

            // Vector from P to S
            vec3 D = S - P;
            float dist = length(D);
            if (dist < 0.001) continue;

            vec3 V = D / dist;

            // Horizon = cosine of angle between normal and direction
            float cosH = dot(N, V);

            // Weight by distance falloff
            float w = 1.0 - (dist / radius);
            w = max(w, 0.0);

            maxHorizon = max(maxHorizon, cosH * w);

            validCount += 1.0;
        }

        // --- Calculate occlusion contribution ---
        // GTAO formula: occlusion = max(0, sin(angle) - sin(horizon))
        // We use cos-based approximation:
        float cosHorizon = clamp(maxHorizon, 0.0, 1.0);
        float ao = 1.0 - cosHorizon;   // larger when horizon is high
        ao = pow(ao, 1.0);              // tweak falloff

        occlusion += ao;
    }

    // ---- Normalize ----
    occlusion /= float(DIRECTIONS);

    // Apply intensity
    occlusion *= intensity;

    // Final AO value: 1.0 = no occlusion, 0.0 = fully occluded
    float ao = clamp(1.0 - occlusion, 0.0, 1.0);

    FragColor = ao;
}
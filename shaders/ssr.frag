#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

// ---- G-Buffer ----
uniform sampler2D gPosition;      // World Space
uniform sampler2D gNormal;        // World Space
uniform sampler2D gMetallicRoughness;
uniform sampler2D lightingResult;

// ---- Camera ----
uniform mat4 projection;
uniform mat4 view;

// ---- SSR Params ----
uniform float maxDistance;
uniform int   maxSteps;
uniform float thickness;

// ============================================================
//              Helpers
// ============================================================
vec2 projectToScreen(vec3 viewPos) {
    vec4 clip = projection * vec4(viewPos, 1.0);
    return (clip.xy / clip.w) * 0.5 + 0.5;
}

// Read scene depth in VIEW SPACE
float getViewDepth(vec2 uv) {
    vec3 worldPos = texture(gPosition, uv).rgb;
    vec3 viewPos = (view * vec4(worldPos, 1.0)).xyz;
    return viewPos.z;   // سالب
}

// ============================================================
//              Ray Marching (View Space)
// ============================================================
vec4 rayMarch(vec3 originView, vec3 dirView) {
    float stepSize = maxDistance / float(maxSteps);
    vec3 rayPos = originView;
    vec3 prevPos = rayPos;
    
    for (int i = 0; i < maxSteps; i++) {
        prevPos = rayPos;
        rayPos += dirView * stepSize;
        
        // وقف لو الـ ray رجع ورا الكاميرا
        if (rayPos.z > -0.1) break;
        
        vec2 uv = projectToScreen(rayPos);
        
        // خارج الشاشة؟
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
            break;
        
        // اقرأ عمق المشهد (View Space)
        float sceneDepth = getViewDepth(uv);
        
        // تصادم: الـ ray عدى السطح (أكثر سالبية)
        if (rayPos.z < sceneDepth && (sceneDepth - rayPos.z) < thickness) {
            
            // Binary Search للتظبيط
            vec3 lo = prevPos;
            vec3 hi = rayPos;
            for (int j = 0; j < 8; j++) {
                vec3 mid = (lo + hi) * 0.5;
                vec2 midUV = projectToScreen(mid);
                float midDepth = getViewDepth(midUV);
                
                if (mid.z < midDepth) {
                    hi = mid;
                } else {
                    lo = mid;
                }
            }
            
            vec2 finalUV = projectToScreen(hi);
            vec3 hitColor = texture(lightingResult, finalUV).rgb;
            
            // Edge fade
            float edgeFade = smoothstep(0.0, 0.15, finalUV.x) *
                             smoothstep(1.0, 0.85, finalUV.x) *
                             smoothstep(0.0, 0.15, finalUV.y) *
                             smoothstep(1.0, 0.85, finalUV.y);
            
            return vec4(hitColor, edgeFade);
        }
    }
    
    return vec4(0.0);
}

// ============================================================
//              Main
// ============================================================
void main() {

    FragColor = vec4(TexCoords.x, TexCoords.y, 0.5, 1.0);
    return;
    
    // 1. اقرأ G-Buffer
    vec3 worldPos = texture(gPosition, TexCoords).rgb;
    
    // Skybox check (worldPos = 0 للسكاي)
    if (length(worldPos) < 0.001) {
        FragColor = vec4(0.0);
        return;
    }
    
    vec3 worldNormal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 mr = texture(gMetallicRoughness, TexCoords).rgb;
    float metallic = mr.r;
    float roughness = mr.g;
    
    // 2. Skip لو مش معدي (مؤقتاً: عرض كل حاجة للـ debug)
    // if (roughness > 0.8 || metallic < 0.05) {
    //     FragColor = vec4(0.0);
    //     return;
    // }
    
    // 3. حوّل لـ View Space
    vec3 viewPos = (view * vec4(worldPos, 1.0)).xyz;
    vec3 viewNormal = normalize(mat3(view) * worldNormal);
    
    // 4. Reflect direction
    vec3 viewDir = normalize(viewPos);
    vec3 reflectDir = reflect(viewDir, viewNormal);
    
    // 5. Ray March
    vec4 ssr = rayMarch(viewPos, reflectDir);
    
    // 6. Fresnel
    float NdotV = max(dot(viewNormal, -viewDir), 0.0);
    float fresnel = pow(1.0 - NdotV, 5.0);
    fresnel = mix(0.04, 1.0, fresnel);
    
    // 7. Roughness fade
    float roughFade = clamp(1.0 - roughness * 1.25, 0.0, 1.0);
    
    vec3 finalSSR = ssr.rgb * ssr.a * fresnel * roughFade;
    
    // ⚠️ للـ DEBUG فقط — شيل * 5 لما يشتغل
    FragColor = vec4(ssr.rgb * 3.0, 1.0);
}
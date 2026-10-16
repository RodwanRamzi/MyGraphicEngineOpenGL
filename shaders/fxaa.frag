#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

// ---- Inputs ----
uniform sampler2D screenTexture;
uniform vec2 screenSize;

// ---- LUT ----
uniform sampler3D uLUT;
uniform bool  uLUTEnabled;
uniform float uLUTStrength;

// ---- Sharpening ----
uniform bool  enableSharpening;
uniform float sharpeningStrength;

// ---- Vignette ----
uniform bool  enableVignette;
uniform float vignetteStrength;
uniform float vignetteRadius;

// ---- Chromatic Aberration ----
uniform bool  enableChromaticAberration;
uniform float chromaticStrength;

// ---- Film Grain ----
uniform bool  enableFilmGrain;
uniform float grainStrength;
uniform float grainTime;

// ---- Bloom ----
uniform sampler2D bloomTexture;
uniform bool  enableBloom;
uniform float bloomIntensity;

// ---- FXAA ----
uniform bool enableFXAA;

// ============================================================
//                    SHARPENING (5-tap)
// ============================================================
vec3 applySharpen(vec2 uv, vec2 texelSize, float strength) {
    vec3 center = texture(screenTexture, uv).rgb;
    vec3 top    = texture(screenTexture, uv + vec2(0.0, texelSize.y)).rgb;
    vec3 bottom = texture(screenTexture, uv - vec2(0.0, texelSize.y)).rgb;
    vec3 left   = texture(screenTexture, uv - vec2(texelSize.x, 0.0)).rgb;
    vec3 right  = texture(screenTexture, uv + vec2(texelSize.x, 0.0)).rgb;
    
    vec3 laplacian = top + bottom + left + right - 4.0 * center;
    return center - laplacian * strength;
}

// ============================================================
//              CHROMATIC ABERRATION
// ============================================================
vec3 applyChromaticAberration(vec2 uv, vec2 texelSize, float strength) {
    vec2 center = vec2(0.5, 0.5);
    vec2 dir = uv - center;
    vec2 offset = dir * strength;
    
    float r = texture(screenTexture, uv + offset).r;
    float g = texture(screenTexture, uv).g;
    float b = texture(screenTexture, uv - offset).b;
    
    return vec3(r, g, b);
}

// ============================================================
//                    VIGNETTE
// ============================================================
vec3 applyVignette(vec2 uv, vec3 color, float strength, float radius) {
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center);
    float vignette = smoothstep(radius, radius - 0.5, dist);
    return color * mix(1.0, vignette, strength);
}

// ============================================================
//                    FILM GRAIN
// ============================================================
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 applyFilmGrain(vec2 uv, vec3 color, float strength, float t) {
    float noise = hash(uv * 1000.0 + t);
    noise = (noise - 0.5) * 2.0;
    return color + noise * strength;
}

// ============================================================
//                        LUT
// ============================================================
vec3 applyLUT(vec3 color, sampler3D lut, float strength) {
    vec3 c = clamp(color, 0.0, 1.0);
    
    const float SIZE = 32.0;
    float scale  = (SIZE - 1.0) / SIZE;
    float offset = 1.0 / (2.0 * SIZE);
    
    vec3 uvw = c * scale + offset;
    vec3 graded = texture(lut, uvw).rgb;
    
    return mix(color, graded, strength);
}

// ============================================================
//                    FXAA
// ============================================================
vec3 applyFXAA(vec2 uv, vec2 texelSize) {
    vec3 rgbNW = texture(screenTexture, uv + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = texture(screenTexture, uv + vec2( 1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = texture(screenTexture, uv + vec2(-1.0,  1.0) * texelSize).rgb;
    vec3 rgbSE = texture(screenTexture, uv + vec2( 1.0,  1.0) * texelSize).rgb;
    vec3 rgbM  = texture(screenTexture, uv).rgb;
    
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * (1.0/8.0), 1.0/128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(8.0), max(vec2(-8.0), dir * rcpDirMin)) * texelSize;
    
    vec3 rgbA = 0.5 * (
        texture(screenTexture, uv + dir * (1.0/3.0 - 0.5)).rgb +
        texture(screenTexture, uv + dir * (2.0/3.0 - 0.5)).rgb
    );
    
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, uv + dir * -0.5).rgb +
        texture(screenTexture, uv + dir *  0.5).rgb
    );
    
    float lumaB = dot(rgbB, luma);
    
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;
    return rgbB;
}

// ============================================================
//                        MAIN
// ============================================================
void main() {
    vec2 texelSize = 1.0 / screenSize;
    
    // 1. اقرأ اللون الأساسي (مع Sharpening لو مفعّل)
    vec3 color;
    if (enableSharpening) {
        color = applySharpen(TexCoords, texelSize, sharpeningStrength);
    } else {
        color = texture(screenTexture, TexCoords).rgb;
    }
    
    // 2. Chromatic Aberration
    if (enableChromaticAberration) {
        color = applyChromaticAberration(TexCoords, texelSize, chromaticStrength);
    }
    
    // 3. FXAA (على الصورة الحالية — قبل الـ Bloom والـ Post FX)
    // (مؤقتاً معطّل — لو عايز تفعّله، شيل التعليق)
    // if (enableFXAA) {
    //     color = applyFXAA(TexCoords, texelSize);
    // }
    
    // 4. Bloom (يتضاف على الصورة في LDR)
    if (enableBloom) {
        vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;
        color += bloomColor * bloomIntensity;
    }
    
    // 5. LUT (Color Grading)
    if (uLUTEnabled) {
        color = applyLUT(color, uLUT, uLUTStrength);
    }
    
    // 6. Vignette
    if (enableVignette) {
        color = applyVignette(TexCoords, color, vignetteStrength, vignetteRadius);
    }
    
    // 7. Film Grain
    if (enableFilmGrain) {
        color = applyFilmGrain(TexCoords, color, grainStrength, grainTime);
    }
    
    // ✅ Clamp نهائي لتجنب القيم السالبة أو > 1
    color = clamp(color, 0.0, 1.0);
    
    FragColor = vec4(color, 1.0);
}
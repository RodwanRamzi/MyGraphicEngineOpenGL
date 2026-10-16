#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D srcTexture;        // الـ Mip الأصغر
uniform sampler2D higherMipTexture;  // الـ Mip الأكبر (نفس المستوى)
uniform vec2 srcTexelSize;
uniform float filterRadius;          // مثلاً 1.0

vec3 upsampleTent(vec2 uv, float radius) {
    vec2 texel = srcTexelSize * radius;

    vec3 result = texture(srcTexture, uv).rgb * 4.0;
    result += texture(srcTexture, uv + vec2(-1.0,  0.0) * texel).rgb * 2.0;
    result += texture(srcTexture, uv + vec2( 1.0,  0.0) * texel).rgb * 2.0;
    result += texture(srcTexture, uv + vec2( 0.0, -1.0) * texel).rgb * 2.0;
    result += texture(srcTexture, uv + vec2( 0.0,  1.0) * texel).rgb * 2.0;
    result += texture(srcTexture, uv + vec2(-1.0, -1.0) * texel).rgb * 1.0;
    result += texture(srcTexture, uv + vec2( 1.0, -1.0) * texel).rgb * 1.0;
    result += texture(srcTexture, uv + vec2(-1.0,  1.0) * texel).rgb * 1.0;
    result += texture(srcTexture, uv + vec2( 1.0,  1.0) * texel).rgb * 1.0;

    return result / 16.0;
}

void main() {
    vec3 upsampled = upsampleTent(TexCoords, filterRadius);
    
    // Additive مع الـ Mip الأعلى اللي كنا حفظناه
    vec3 higher = texture(higherMipTexture, TexCoords).rgb;
    
    FragColor = vec4(upsampled + higher, 1.0);
}
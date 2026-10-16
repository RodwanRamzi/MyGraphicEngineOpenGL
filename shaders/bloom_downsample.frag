#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D srcTexture;
uniform vec2 srcTexelSize;   // 1.0 / textureSize

// Box filter 13-tap من Jimenez 2014
vec3 downsample13Tap(vec2 uv) {
    vec2 texel = srcTexelSize;

    vec3 A = texture(srcTexture, uv + texel * vec2(-2.0,  2.0)).rgb;
    vec3 B = texture(srcTexture, uv + texel * vec2( 0.0,  2.0)).rgb;
    vec3 C = texture(srcTexture, uv + texel * vec2( 2.0,  2.0)).rgb;
    
    vec3 D = texture(srcTexture, uv + texel * vec2(-2.0,  0.0)).rgb;
    vec3 E = texture(srcTexture, uv).rgb;
    vec3 F = texture(srcTexture, uv + texel * vec2( 2.0,  0.0)).rgb;
    
    vec3 G = texture(srcTexture, uv + texel * vec2(-2.0, -2.0)).rgb;
    vec3 H = texture(srcTexture, uv + texel * vec2( 0.0, -2.0)).rgb;
    vec3 I = texture(srcTexture, uv + texel * vec2( 2.0, -2.0)).rgb;
    
    vec3 J = texture(srcTexture, uv + texel * vec2(-1.0,  1.0)).rgb;
    vec3 K = texture(srcTexture, uv + texel * vec2( 1.0,  1.0)).rgb;
    vec3 L = texture(srcTexture, uv + texel * vec2(-1.0, -1.0)).rgb;
    vec3 M = texture(srcTexture, uv + texel * vec2( 1.0, -1.0)).rgb;

    vec3 result = E * 0.125;
    result += (A + C + G + I) * 0.03125;
    result += (B + D + F + H) * 0.0625;
    result += (J + K + L + M) * 0.125;

    return result;
}

void main() {
    FragColor = vec4(downsample13Tap(TexCoords), 1.0);
}
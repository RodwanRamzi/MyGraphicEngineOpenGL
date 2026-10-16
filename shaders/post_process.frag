// Uniform
uniform sampler3D uLUT;
uniform bool uLUTEnabled;
uniform float uLUTStrength;  // 0.0 = off, 1.0 = full

// LUT function
vec3 ApplyLUT(vec3 color, sampler3D lut, float strength) {
    // Clamp to [0, 1]
    vec3 c = clamp(color, 0.0, 1.0);
    
    // Sample LUT — scale to avoid edge bleeding
    float scale = (32.0 - 1.0) / 32.0;
    float offset = 1.0 / (2.0 * 32.0);
    vec3 uvw = c * scale + offset;
    
    vec3 graded = texture(lut, uvw).rgb;
    
    // Blend with original
    return mix(color, graded, strength);
}
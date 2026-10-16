#version 460 core
in vec2 TexCoords;
out float FragColor;

uniform sampler2D gtaoInput;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(gtaoInput, 0));
    float result = 0.0;
    for (int x = -3; x < 3; ++x) {
        for (int y = -3; y < 3; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(gtaoInput, TexCoords + offset).r;
        }
    }
    result /= 36.0;
    FragColor = result;
}
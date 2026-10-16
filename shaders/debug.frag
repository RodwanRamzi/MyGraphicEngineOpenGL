#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMetallicRoughness;

uniform int debugMode; // 0 = albedo, 1 = position, 2 = normal, 3 = final shaded

void main()
{
    if (debugMode == 0)
        FragColor = vec4(texture(gAlbedo, TexCoords).rgb, 1.0);
    else if (debugMode == 1)
        FragColor = vec4(texture(gPosition, TexCoords).rgb, 1.0);// debug.frag
    else if (debugMode == 2)
        FragColor = vec4(texture(gNormal, TexCoords).rgb * 0.5 + 0.5, 1.0);
    else
        FragColor = vec4(0.2, 0.8, 0.2, 1.0); // placeholder for lighting
}
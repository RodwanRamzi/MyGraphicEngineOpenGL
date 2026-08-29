// light.frag
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 camPos;
uniform vec4 lightColor;

void main() {
    // Retrieve data from G-Buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    // Simple Directional Light
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(Normal, lightDirNorm), 0.0);
    vec3 diffuse = diff * Albedo * vec3(lightColor);

    // Specular
    vec3 viewDir = normalize(camPos - FragPos);
    vec3 reflectDir = reflect(-lightDirNorm, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specularColor = spec * Specular * vec3(lightColor);

    FragColor = vec4(diffuse + specularColor, 1.0);
}
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gMetallicRoughness;

uniform vec3 camPos;
uniform vec4 lightColor;
uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 lightPos2;

uniform mat4 view;

void main() {
    // قراءة البيانات من G-Buffer
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 albedo = texture(gColor, TexCoords).rgb;
    
    // إذا كانت الخلفية، اخرج
    if (length(fragPos) < 0.01) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // حساب الإضاءة (نفس الكود ولكن لكل بكسل مرة واحدة)
    vec3 V = normalize(camPos - fragPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    
    // Diffuse
    float NdotL = max(dot(normal, L), 0.0);
    vec3 diffuse = albedo * NdotL * vec3(lightColor);
    
    // Specular
    float spec = pow(max(dot(normal, H), 0.0), 32.0);
    vec3 specular = spec * vec3(lightColor) * 0.5;
    
    // Ambient
    vec3 ambient = albedo * 0.03;
    
    vec3 color = ambient + diffuse + specular;
    
    FragColor = vec4(color, 1.0);
}
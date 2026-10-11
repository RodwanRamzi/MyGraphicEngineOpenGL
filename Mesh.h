#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <string>

#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"

class Shader;
class Camera;

class Mesh {
public:
    Mesh() = default;

    std::vector<Vertex>  vertices;
    std::vector<GLuint>  indices;
    std::vector<Texture> textures;

    VAO VAO;
    // لا VBO/EBO كـ members

    glm::vec3 aabbMin{  FLT_MAX,  FLT_MAX,  FLT_MAX };
    glm::vec3 aabbMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    Mesh(std::vector<Vertex>& vertices,
         std::vector<GLuint>& indices,
         std::vector<Texture>& textures);

    void Draw(Shader& shader, Camera& camera,
              glm::mat4 matrix = glm::mat4(1.0f),
              glm::vec3 translation = glm::vec3(0.0f),
              glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
              glm::vec3 scale = glm::vec3(1.0f),
              bool bindTextures = true);

    void DrawDepth(Shader& shader, const glm::mat4& modelMatrix);
};

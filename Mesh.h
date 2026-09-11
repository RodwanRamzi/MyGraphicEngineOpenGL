#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <string>

#include "VAO.h"
#include "VBO.h"      // ← Vertex comes from here
#include "EBO.h"
#include "Texture.h"

// ============================================================
//              FORWARD DECLARATIONS
// ============================================================
class Shader;
class Camera;

// ============================================================
//                        MESH
// ============================================================
class Mesh {
public:
    std::vector<Vertex>  vertices;   // ← Vertex from VBO.h
    std::vector<GLuint>  indices;
    std::vector<Texture> textures;

    VAO VAO;

    Mesh(std::vector<Vertex>& vertices,
        std::vector<GLuint>& indices,
        std::vector<Texture>& textures);

    void Draw(Shader& shader,
        Camera& camera,
        glm::mat4     matrix,
        glm::vec3     translation = glm::vec3(0.0f),
        glm::quat     rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3     scale = glm::vec3(1.0f),
        bool          bindTextures = true);

    void DrawDepth(Shader& shader, const glm::mat4& modelMatrix);
};
#include "Mesh.h"
#include "Camera.h"      //
#include <glm/gtc/type_ptr.hpp>   // ← for glm::value_ptr
#include <glm/gtc/quaternion.hpp>   
#include <iostream>

// ============================================================
//                     CONSTRUCTOR
// ============================================================
Mesh::Mesh(std::vector<Vertex>& vertices,
    std::vector<GLuint>& indices,
    std::vector<Texture>& textures)
{
    std::cout << "[Mesh] Constructor called!" << std::endl;
    std::cout << "[Mesh] vertices=" << vertices.size()
        << ", indices=" << indices.size()
        << ", textures=" << textures.size() << std::endl;

    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    std::cout << "[Mesh] Setting up buffers..." << std::endl;

    VAO.Bind();

    VBO VBO(vertices);
    EBO EBO(indices);

    VAO.LinkAttrib(VBO, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
    VAO.LinkAttrib(VBO, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    VAO.LinkAttrib(VBO, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)(offsetof(Vertex, color)));
    VAO.LinkAttrib(VBO, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)(offsetof(Vertex, texUV)));
    VAO.LinkAttrib(VBO, 4, 3, GL_FLOAT, sizeof(Vertex), (void*)(offsetof(Vertex, tangent)));
    VAO.LinkAttrib(VBO, 5, 3, GL_FLOAT, sizeof(Vertex), (void*)(offsetof(Vertex, bitangent)));

    VAO.Unbind();
    VBO.Unbind();
    EBO.Unbind();

    std::cout << "[Mesh] Constructor finished!" << std::endl;
}

// ============================================================
//                         DRAW
// ============================================================
void Mesh::Draw(Shader& shader,
    Camera& camera,
    glm::mat4 matrix,
    glm::vec3 translation,
    glm::quat rotation,
    glm::vec3 scale,
    bool      bindTextures)
{
    shader.Activate();
    VAO.Bind();

    // ============================================================
    //                  TEXTURE BINDING
    // ============================================================
    // Only bind textures if the caller hasn't already set them up.
    // The G-Buffer pass binds its own textures (from Material Overrides)
    // and passes bindTextures = false to prevent this block from
    // overwriting them.
    // ============================================================
    if (bindTextures)
    {
        unsigned int numDiffuse = 0;
        unsigned int numSpecular = 0;
        unsigned int numNormal = 0;
        unsigned int numMetallicRoughness = 0;
        unsigned int numHeightMap = 0;
        unsigned int numEmissive = 0;
        unsigned int numAO = 0;

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            std::string num;
            std::string type = textures[i].type;

            if (type == "diffuse" || type == "baseColor" || type == "albedo") {
                num = std::to_string(numDiffuse++);
                type = "diffuse";
            }
            else if (type == "specular") {
                num = std::to_string(numSpecular++);
            }
            else if (type == "normal" || type == "normalMap") {
                num = std::to_string(numNormal++);
                type = "normal";
            }
            else if (type == "metallicRoughness" || type == "metallic") {
                num = std::to_string(numMetallicRoughness++);
                type = "metallicRoughness";
            }
            else if (type == "heightMap" || type == "height" || type == "displacement") {
                num = std::to_string(numHeightMap++);
                type = "heightMap";
            }
            else if (type == "ao" || type == "ambientOcclusion") {
                num = std::to_string(numAO++);
                type = "ao";
            }
            else if (type == "emissive" || type == "emissiveMap") {
                num = std::to_string(numEmissive++);
                type = "emissive";
            }

            textures[i].texUnit(shader, (type + num).c_str(), i);
            textures[i].Bind();
        }
    }

    // ---- Camera uniforms ----
    GLint camPosLoc = glGetUniformLocation(shader.ID, "camPos");
    if (camPosLoc != -1) {
        glUniform3f(camPosLoc, camera.Position.x, camera.Position.y, camera.Position.z);
    }

    GLint camMatrixLoc = glGetUniformLocation(shader.ID, "camMatrix");
    if (camMatrixLoc != -1) {
        camera.Matrix(shader, "camMatrix");
    }

    // ---- Transformation matrices ----
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

    GLint loc;

    loc = glGetUniformLocation(shader.ID, "translation");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(trans));

    loc = glGetUniformLocation(shader.ID, "rotation");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(rot));

    loc = glGetUniformLocation(shader.ID, "scale");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(sca));

    loc = glGetUniformLocation(shader.ID, "model");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));

    // ---- Draw ----
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

// ============================================================
//                      DRAW DEPTH
// ============================================================
void Mesh::DrawDepth(Shader& shader, const glm::mat4& modelMatrix) {
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"),
        1, GL_FALSE, glm::value_ptr(modelMatrix));
    glBindVertexArray(VAO.ID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}
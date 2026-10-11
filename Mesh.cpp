#include "Mesh.h"
#include "Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

Mesh::Mesh(std::vector<Vertex>& vertices,
           std::vector<GLuint>& indices,
           std::vector<Texture>& textures)
{
    this->vertices = vertices;
    this->indices  = indices;
    this->textures = textures;

    VAO.Bind();               // ← VAO أولاً (حرج!)
    VBO vbo(vertices);        // ← locals بأسماء صغيرة
    EBO ebo(indices);         // ← EBO binding يُخزَّن في الـ VAO ✅

    VAO.LinkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
    VAO.LinkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    VAO.LinkAttrib(vbo, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, color));
    VAO.LinkAttrib(vbo, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, texUV));
    VAO.LinkAttrib(vbo, 4, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    VAO.LinkAttrib(vbo, 5, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    for (const auto& v : vertices) {
        aabbMin = glm::min(aabbMin, v.position);
        aabbMax = glm::max(aabbMax, v.position);
    }

    VAO.Unbind();
    vbo.Unbind();
    ebo.Unbind();
}

void Mesh::Draw(Shader& shader, Camera& camera, glm::mat4 matrix,
                glm::vec3 translation, glm::quat rotation,
                glm::vec3 scale, bool bindTextures)
{
    shader.Activate();
    VAO.Bind();

    if (bindTextures) {
        unsigned int numDiffuse = 0, numSpecular = 0, numNormal = 0;
        unsigned int numMR = 0, numHeight = 0, numEmissive = 0, numAO = 0;

        for (unsigned int i = 0; i < textures.size(); i++) {
            std::string num;
            std::string type = textures[i].type;

            if (type == "diffuse" || type == "baseColor" || type == "albedo") {
                num = std::to_string(numDiffuse++); type = "diffuse";
            } else if (type == "specular") {
                num = std::to_string(numSpecular++);
            } else if (type == "normal" || type == "normalMap") {
                num = std::to_string(numNormal++); type = "normal";
            } else if (type == "metallicRoughness" || type == "metallic") {
                num = std::to_string(numMR++); type = "metallicRoughness";
            } else if (type == "heightMap" || type == "height" || type == "displacement") {
                num = std::to_string(numHeight++); type = "heightMap";
            } else if (type == "ao" || type == "ambientOcclusion") {
                num = std::to_string(numAO++); type = "ao";
            } else if (type == "emissive" || type == "emissiveMap") {
                num = std::to_string(numEmissive++); type = "emissive";
            }

            textures[i].texUnit(shader, (type + num).c_str(), i);
            textures[i].Bind();
        }
    }

    GLint loc;
    loc = glGetUniformLocation(shader.ID, "camPos");
    if (loc != -1) glUniform3f(loc, camera.Position.x, camera.Position.y, camera.Position.z);

    loc = glGetUniformLocation(shader.ID, "camMatrix");
    if (loc != -1) camera.Matrix(shader, "camMatrix");

    glm::mat4 trans = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 rot   = glm::mat4_cast(rotation);
    glm::mat4 sca   = glm::scale(glm::mat4(1.0f), scale);

    loc = glGetUniformLocation(shader.ID, "translation");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(trans));

    loc = glGetUniformLocation(shader.ID, "rotation");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(rot));

    loc = glGetUniformLocation(shader.ID, "scale");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(sca));

    loc = glGetUniformLocation(shader.ID, "model");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::DrawDepth(Shader& shader, const glm::mat4& modelMatrix) {
    GLint loc = glGetUniformLocation(shader.ID, "model");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    if (VAO.ID == 0 || indices.empty()) return;

    glBindVertexArray(VAO.ID);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}
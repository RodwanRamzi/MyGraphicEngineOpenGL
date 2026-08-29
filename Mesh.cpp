#include "Mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    VAO.Bind();

    VBO VBO(vertices);

    VAO.LinkAttrib(VBO, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);          // Position
    VAO.LinkAttrib(VBO, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(3 * sizeof(float)));  // Normal
    VAO.LinkAttrib(VBO, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)(6 * sizeof(float)));  // Color
    VAO.LinkAttrib(VBO, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)(9 * sizeof(float)));  // ✅ TexUV
    VAO.LinkAttrib(VBO, 4, 3, GL_FLOAT, sizeof(Vertex), (void*)(11 * sizeof(float))); // Tangent
    VAO.LinkAttrib(VBO, 5, 3, GL_FLOAT, sizeof(Vertex), (void*)(14 * sizeof(float))); // Bitangent

    EBO ebo(indices);
    VAO.Unbind();
    VBO.Unbind();
    ebo.Unbind();
}

void Mesh::Draw
(
    Shader& shader,
    Camera& camera,
    glm::mat4 matrix,
    glm::vec3 translation,
    glm::quat rotation,
    glm::vec3 scale
)
{

    shader.Activate();
    VAO.Bind();

    // ==================== معالجة النسيج (Textures) ====================
    unsigned int numDiffuse = 0;
    unsigned int numSpecular = 0;
    unsigned int numNormal = 0;
    unsigned int numMetallicRoughness = 0;
    unsigned int numHeightMap = 0;
    unsigned int numBaseColor = 0;           // ✅ GLTF يسميها baseColor

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        std::string num;
        std::string type = textures[i].type;

        // ✅ دعم جميع أنواع النسيج
        if (type == "diffuse" || type == "baseColor" || type == "albedo")
        {
            num = std::to_string(numDiffuse++);
            type = "diffuse";  // توحيد الاسم
        }
        else if (type == "specular")
        {
            num = std::to_string(numSpecular++);
        }
        else if (type == "normal" || type == "normalMap")
        {
            num = std::to_string(numNormal++);
            type = "normal";  // توحيد الاسم
        }
        else if (type == "metallicRoughness" || type == "metallic")
        {
            num = std::to_string(numMetallicRoughness++);
            type = "metallicRoughness";  // توحيد الاسم
        }
        else if (type == "heightMap" || type == "height" || type == "displacement")
        {
            num = std::to_string(numHeightMap++);
            type = "heightMap";  // توحيد الاسم
        }

        textures[i].texUnit(shader, (type + num).c_str(), i);
        textures[i].Bind();
    }


    // ==================== تعيين الـ Camera ====================
    GLint camPosLoc = glGetUniformLocation(shader.ID, "camPos");
    if (camPosLoc != -1) {
        glUniform3f(camPosLoc, camera.Position.x, camera.Position.y, camera.Position.z);
    }

    GLint camMatrixLoc = glGetUniformLocation(shader.ID, "camMatrix");
    if (camMatrixLoc != -1) {
        camera.Matrix(shader, "camMatrix");
    }

    // ==================== بناء مصفوفات التحويل ====================
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

    // ==================== تعيين الـ Uniforms ====================
    GLint loc;

    loc = glGetUniformLocation(shader.ID, "translation");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(trans));

    loc = glGetUniformLocation(shader.ID, "rotation");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(rot));

    loc = glGetUniformLocation(shader.ID, "scale");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(sca));

    loc = glGetUniformLocation(shader.ID, "model");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));

    // ==================== رسم المِش ====================
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::DrawDepth(Shader& shader, const glm::mat4& modelMatrix) {
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glBindVertexArray(VAO.ID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}
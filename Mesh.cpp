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

    std::cout << "=== Vertex layout ===" << std::endl;
    std::cout << "sizeof(Vertex)       = " << sizeof(Vertex) << std::endl;
    std::cout << "offsetof(position)   = " << offsetof(Vertex, position) << std::endl;
    std::cout << "offsetof(normal)     = " << offsetof(Vertex, normal) << std::endl;
    std::cout << "offsetof(texUV)      = " << offsetof(Vertex, texUV) << std::endl;
    std::cout << "offsetof(tangent)    = " << offsetof(Vertex, tangent) << std::endl;
    std::cout << "offsetof(bitangent)  = " << offsetof(Vertex, bitangent) << std::endl;
    std::cout << "offsetof(joints)     = " << offsetof(Vertex, joints) << std::endl;
    std::cout << "offsetof(weights)    = " << offsetof(Vertex, weights) << std::endl;

    VAO.LinkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
    VAO.LinkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    VAO.LinkAttrib(vbo, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, color));
    VAO.LinkAttrib(vbo, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, texUV));
    VAO.LinkAttrib(vbo, 4, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    VAO.LinkAttrib(vbo, 5, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    // ---- Skinning attributes ----
    VAO.LinkAttribInt(vbo, 6, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, joints));
    VAO.LinkAttrib(vbo,    7, 4, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, weights));

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

// ---- Bind textures ----
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

    // ---- Camera ----
    GLint loc;
    loc = glGetUniformLocation(shader.ID, "camPos");
    if (loc != -1) glUniform3f(loc, camera.Position.x, camera.Position.y, camera.Position.z);

    loc = glGetUniformLocation(shader.ID, "camMatrix");
    if (loc != -1) camera.Matrix(shader, "camMatrix");

    // ---- Model transform ----
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

    // ---- Material factors ----
    loc = glGetUniformLocation(shader.ID, "baseColorFactor");
    if (loc != -1) glUniform4fv(loc, 1, glm::value_ptr(baseColorFactor));

    loc = glGetUniformLocation(shader.ID, "metallicFactor");
    if (loc != -1) glUniform1f(loc, metallicFactor);

    loc = glGetUniformLocation(shader.ID, "roughnessFactor");
    if (loc != -1) glUniform1f(loc, roughnessFactor);

    loc = glGetUniformLocation(shader.ID, "emissiveFactor");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(emissiveFactor));

    loc = glGetUniformLocation(shader.ID, "emissiveStrength");
    if (loc != -1) glUniform1f(loc, emissiveStrength);

    // ---- Texture presence flags ----
    loc = glGetUniformLocation(shader.ID, "hasBaseColorTexture");
    if (loc != -1) glUniform1i(loc, hasBaseColorTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "hasMetallicRoughnessTexture");
    if (loc != -1) glUniform1i(loc, hasMRTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "hasNormalTexture");
    if (loc != -1) glUniform1i(loc, hasNormalTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "hasEmissiveTexture");
    if (loc != -1) glUniform1i(loc, hasEmissiveTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "hasAOTexture");
    if (loc != -1) glUniform1i(loc, hasAOTexture ? 1 : 0);
    
    loc = glGetUniformLocation(shader.ID, "uUseAlbedoMap");
    if (loc != -1) glUniform1i(loc, hasBaseColorTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "uUseMetallicRoughnessMap");
    if (loc != -1) glUniform1i(loc, hasMRTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "uUseNormalMap");
    if (loc != -1) glUniform1i(loc, hasNormalTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "uUseAOMap");
    if (loc != -1) glUniform1i(loc, hasAOTexture ? 1 : 0);

    loc = glGetUniformLocation(shader.ID, "uUseEmissiveMap");
    if (loc != -1) glUniform1i(loc, hasEmissiveTexture ? 1 : 0);

    // ---- Bone matrices for skinning ----
    if (isSkinned && !boneMatrices.empty()) {
        // Cap at 128 bones
        int count = std::min((int)boneMatrices.size(), 128);
        
        // Set array size
        loc = glGetUniformLocation(shader.ID, "boneCount");
        if (loc != -1) glUniform1i(loc, count);
        
        // Set flag
        loc = glGetUniformLocation(shader.ID, "isSkinned");
        if (loc != -1) glUniform1i(loc, 1);
        
        // Set matrices
        for (int i = 0; i < count; ++i) {
            std::string name = "boneMatrices[" + std::to_string(i) + "]";
            loc = glGetUniformLocation(shader.ID, name.c_str());
            if (loc != -1)
                glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(boneMatrices[i]));
        }
    } else {
        loc = glGetUniformLocation(shader.ID, "isSkinned");
        if (loc != -1) glUniform1i(loc, 0);   // ← force 0
    }

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::DrawDepth(Shader& shader, const glm::mat4& modelMatrix) {
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"),
                       1, GL_FALSE, glm::value_ptr(modelMatrix));

    if (isSkinned && !boneMatrices.empty()) {
        glUniform1i(glGetUniformLocation(shader.ID, "isSkinned"), 1);
        glUniform1i(glGetUniformLocation(shader.ID, "boneCount"),
                    (int)boneMatrices.size());
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "boneMatrices"),
                           (GLsizei)boneMatrices.size(), GL_FALSE,
                           glm::value_ptr(boneMatrices[0]));
    } else {
        glUniform1i(glGetUniformLocation(shader.ID, "isSkinned"), 0);
        glUniform1i(glGetUniformLocation(shader.ID, "boneCount"), 0);
    }

    glBindVertexArray(VAO.ID);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}
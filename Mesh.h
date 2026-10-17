#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"

class Camera;

class Mesh
{
public:
    std::vector<Vertex>  vertices;
    std::vector<GLuint>  indices;
    std::vector<Texture> textures;

    VAO VAO;

    glm::vec3 aabbMin{  FLT_MAX,  FLT_MAX,  FLT_MAX };
    glm::vec3 aabbMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    // ---- Skinning (for animated models) ----
    std::vector<glm::mat4> boneMatrices;    // inverse-bind × joint world
    bool isSkinned = false;                  // true if this mesh has joints
    int skinIndex = -1;

    // ---- Material factors (from glTF materials) ----
    glm::vec4 baseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
    float     metallicFactor  { 1.0f };
    float     roughnessFactor { 1.0f };
    glm::vec3 emissiveFactor  { 0.0f, 0.0f, 0.0f };
    float     emissiveStrength{ 1.0f };

    // ---- Texture presence flags ----
    bool hasBaseColorTexture  { false };
    bool hasMRTexture         { false };
    bool hasNormalTexture     { false };
    bool hasEmissiveTexture   { false };
    bool hasAOTexture         { false };

    Mesh(std::vector<Vertex>&  vertices,
         std::vector<GLuint>&  indices,
         std::vector<Texture>& textures);

    void Draw(Shader& shader,
              Camera& camera,
              glm::mat4 matrix      = glm::mat4(1.0f),
              glm::vec3 translation = glm::vec3(0.0f),
              glm::quat rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
              glm::vec3 scale       = glm::vec3(1.0f),
              bool bindTextures     = true);

    void DrawDepth(Shader& shader, const glm::mat4& modelMatrix);
};
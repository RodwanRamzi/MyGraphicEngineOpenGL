#pragma once

#include <json/json.h>
#include "Mesh.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

using json = nlohmann::json;

// ============================================================
//              FORWARD DECLARATIONS
// ============================================================
class Shader;
class Camera;

class Model {
public:
    Model() = default;
    Model(const char* file);

    void Draw(Shader& shader,
        Camera& camera,
        glm::mat4 globalTransform = glm::mat4(1.0f),
        bool      bindTextures = true);

    void DrawDepth(Shader& shader, const glm::mat4& modelMatrix);

    std::vector<Mesh> meshes;

    float GetBoundingRadius() const;

    const std::vector<std::string>& GetTextureNames() const { return loadedTexName; }
    bool IsValid() const { return !meshes.empty(); }

private:
    std::vector<unsigned char> data;
    json JSON;

    std::string file;
    std::string mDirectory;

    std::vector<glm::vec3> translationsMeshes;
    std::vector<glm::quat> rotationsMeshes;
    std::vector<glm::vec3> scalesMeshes;
    std::vector<glm::mat4> matricesMeshes;

    std::vector<std::string> loadedTexName;
    std::vector<Texture>     loadedTex;

    void loadMesh(unsigned int indMesh);
    void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));
    std::vector<unsigned char> getData();
    std::vector<float>  getFloats(json accessor);
    std::vector<GLuint> getIndices(json accessor);
    std::vector<Texture> getTextures();

    std::vector<Vertex> assembleVertices(
        std::vector<glm::vec3> positions,
        std::vector<glm::vec3> normals,
        std::vector<glm::vec2> texUVs,
        std::vector<glm::vec3> tangents,
        std::vector<glm::vec3> bitangents);

    std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
    std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
    std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
};
#pragma once

#include <json/json.h>
#include "Mesh.h"
#include "Texture.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Shader;
class Camera;

using json = nlohmann::json;

class Model
{
public:
    Model() = default;
    Model(const char* filePath);
    ~Model() = default;

    void Draw(Shader& shader,
              Camera& camera,
              glm::mat4 globalTransform = glm::mat4(1.0f),
              bool bindTextures = true);

    void DrawDepth(Shader& shader, const glm::mat4& modelMatrix);

    float GetBoundingRadius() const;

    std::string file;
    std::vector<Mesh> meshes;
    std::string mDirectory;

    static std::vector<Texture>&     GetLoadedTex();
    static std::vector<std::string>& GetLoadedTexName();

private:
    json JSON;
    std::vector<unsigned char> data;
    std::vector<unsigned char> mGLBBuffer;
    bool mIsGLB = false;

    std::vector<glm::vec3> translationsMeshes;
    std::vector<glm::quat> rotationsMeshes;
    std::vector<glm::vec3> scalesMeshes;
    std::vector<glm::mat4> matricesMeshes;

    void loadMesh(unsigned int indMesh);
    void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));

    std::vector<unsigned char> getDataFromURI();

    std::vector<float>    getFloats(json accessor);
    std::vector<GLuint>   getIndices(json accessor);

    std::vector<Texture>  getTexturesForMaterial(int materialIndex);
    std::vector<Texture>  getFallbackWhiteTexture();
    void loadTextureFromImageRef(json texRef,
                                 const std::string& typeName,
                                 const std::string& baseDirectory,
                                 std::vector<Texture>& out);

    std::vector<Vertex>   assembleVertices(std::vector<glm::vec3> positions,
                                           std::vector<glm::vec3> normals,
                                           std::vector<glm::vec2> texUVs,
                                           std::vector<glm::vec3> tangents,
                                           std::vector<glm::vec3> bitangents);

    std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
    std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
    std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
};
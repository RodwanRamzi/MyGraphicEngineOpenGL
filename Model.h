#ifndef MODEL_CLASS_H
#define MODEL_CLASS_H

#include<json/json.h>
#include"Mesh.h"

using json = nlohmann::json;

class Model
{
public:
    Model(const char* file);
    void Draw(Shader& shader, Camera& camera, glm::mat4 globalTransform = glm::mat4(1.0f));

    void DrawDepth(Shader& shader, const glm::mat4& modelMatrix);

    std::vector<Mesh> meshes;

    float GetBoundingRadius() const;

private:
    const char* file;
    std::vector<unsigned char> data;
    json JSON;

    std::vector<glm::vec3> translationsMeshes;
    std::vector<glm::quat> rotationsMeshes;
    std::vector<glm::vec3> scalesMeshes;
    std::vector<glm::mat4> matricesMeshes;

    std::vector<std::string> loadedTexName;
    std::vector<Texture> loadedTex;

    void loadMesh(unsigned int indMesh);
    void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));
    std::vector<unsigned char> getData();
    std::vector<float> getFloats(json accessor);
    std::vector<GLuint> getIndices(json accessor);
    std::vector<Texture> getTextures();

    // ✅ UPDATE THIS FUNCTION SIGNATURE
    std::vector<Vertex> assembleVertices
    (
        std::vector<glm::vec3> positions,
        std::vector<glm::vec3> normals,
        std::vector<glm::vec2> texUVs,
        std::vector<glm::vec3> tangents,      // ✅ ADD
        std::vector<glm::vec3> bitangents     // ✅ ADD
    );

    std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
    std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
    std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
};
#endif
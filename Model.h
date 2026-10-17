#pragma once

#include <json/json.h>
#include "Mesh.h"
#include "Texture.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>  

class Shader;
class Camera;

using json = nlohmann::json;

// ── Animation structures ──────────────────────────────
struct AnimationChannel {
    int node = -1;
    std::string path;               // "translation" | "rotation" | "scale"
    std::string interp = "LINEAR";
    std::vector<float> times;
    std::vector<glm::vec4> values;
};

struct Animation {
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannel> channels;
};

struct AnimationPlayer {
    int currentClip = 0;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool loop = true;
    bool playing = true;
};

struct NodeData {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    int parent = -1;
};

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

    const std::vector<glm::mat4>& GetMatrices() const { return matricesMeshes; }

    glm::mat4 GetMeshMatrix(size_t index) const {
        if (index < matricesMeshes.size()) return matricesMeshes[index];
        return glm::mat4(1.0f);
    }

    std::vector<glm::mat4> matricesMeshes;

    // ---- Skins (skeletal animation) ----
    struct SkinData {
        std::vector<int>         jointNodes;        // node indices in JSON["nodes"]
        std::vector<glm::mat4>   inverseBindMatrices;
        int                      skeletonRoot = -1; // optional
    };

    std::vector<SkinData> skins;

    // ---- Animation clips ----
    std::vector<Animation> animations;             
    AnimationPlayer        animPlayer;    // (per-model player)

    // ---- Bone matrices for skinning ----
    std::vector<glm::mat4> nodeGlobalTransforms;   // global transform for each node
    std::vector<glm::mat4> nodeLocalTransforms;

    // Local TRS per node (used by animation)
    std::vector<glm::vec3> nodeLocalPositions;
    std::vector<glm::quat> nodeLocalRotations;
    std::vector<glm::vec3> nodeLocalScales;

    std::vector<int> nodeParents;   // parent index per node, -1 for root
    std::vector<NodeData> nodeData;   // parallel to nodes

    void updateAnimation(float t);

    std::vector<glm::mat4> computeBoneMatrices(int skinIndex);

    void rebuildLocalMatrices();    

    std::string file;
    std::vector<Mesh> meshes;
    std::string mDirectory;

    static std::vector<Texture>&     GetLoadedTex();
    static std::vector<std::string>& GetLoadedTexName();

    static GLuint getWhiteTexture1x1();

private:
    json JSON;
    std::vector<unsigned char> data;
    std::vector<unsigned char> mGLBBuffer;
    bool mIsGLB = false;

    std::vector<glm::vec3> translationsMeshes;
    std::vector<glm::quat> rotationsMeshes;
    std::vector<glm::vec3> scalesMeshes;
    
    void decodeGLBImages();
    std::unordered_map<int, GLuint> mEmbeddedTextures;

    void ensureNodeCapacity(unsigned int nodeIndex);

    void loadSkins();

    void loadMesh(unsigned int indMesh);
    void traverseNode(unsigned int nextNode,
        glm::mat4 matrix = glm::mat4(1.0f),
        int parentIdx = -1);

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
                                           std::vector<glm::vec3> bitangents,
                                           std::vector<glm::ivec4> joints,      
                                           std::vector<glm::vec4> weights       
    );

    std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
    std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
    std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);

    std::vector<float>   readScalarAccessor(int accessorIdx);
    std::vector<glm::vec4> readVec4Accessor(int accessorIdx);
};
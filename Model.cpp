#include "Model.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cfloat>
#include <cstdint>
#include <stdexcept>

// ============================================================
//              FUNCTION-LOCAL STATIC CACHES
// ============================================================
std::vector<Texture>& Model::GetLoadedTex()
{
    static std::vector<Texture> cache;
    return cache;
}

std::vector<std::string>& Model::GetLoadedTexName()
{
    static std::vector<std::string> cache;
    return cache;
}

// ============================================================
//                     LOG LEVEL
// ============================================================
// 0 = silent, 1 = errors only, 2 = info, 3 = verbose
static int gLogLevel = 0;

#define MLOG(x)  do { if (gLogLevel >= 2) { std::cerr << x << std::endl; } } while(0)
#define MERR(x)  do { if (gLogLevel >= 1) { std::cerr << x << std::endl; } } while(0)

// ============================================================
//                  LITTLE-ENDIAN READERS
// ============================================================
static uint32_t readU32LE(const unsigned char* p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

// ============================================================
//                       FILE HELPER
// ============================================================
static std::string get_file_contents(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Model] Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ============================================================
//         DECODE EMBEDDED IMAGES FROM GLB BINARY CHUNK
// ============================================================
void Model::decodeGLBImages()
{
    if (!JSON.contains("images") || !JSON.contains("bufferViews"))
        return;

    for (size_t i = 0; i < JSON["images"].size(); ++i) {
        auto& img = JSON["images"][i];

        // Skip images that already have a URI
        if (img.contains("uri")) continue;

        // Must have a bufferView reference
        if (!img.contains("bufferView")) continue;

        int bvIndex = img["bufferView"];
        if (bvIndex < 0 || bvIndex >= (int)JSON["bufferViews"].size()) {
            std::cerr << "[GLB] Invalid bufferView " << bvIndex << " for image " << i << std::endl;
            continue;
        }

        auto& bv = JSON["bufferViews"][bvIndex];
        size_t byteOffset = bv.value("byteOffset", 0);
        size_t byteLength = bv["byteLength"];

        if (byteOffset + byteLength > mGLBBuffer.size()) {
            std::cerr << "[GLB] Image " << i << " out of bounds" << std::endl;
            continue;
        }

        // Decode with stb_image
        const unsigned char* imgData = mGLBBuffer.data() + byteOffset;
        int w, h, ch;
        unsigned char* decoded = stbi_load_from_memory(imgData, (int)byteLength,
                                                       &w, &h, &ch, 0);

        if (!decoded) {
            std::cerr << "[GLB] Failed to decode image " << i
                      << " (size=" << byteLength << " bytes)" << std::endl;
            continue;
        }

        // Upload to GPU
        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        GLenum format = (ch == 4) ? GL_RGBA : (ch == 3) ? GL_RGB : GL_RED;
        GLenum internal = (ch == 4) ? GL_SRGB_ALPHA : (ch == 3) ? GL_SRGB : GL_RED;

        glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, GL_UNSIGNED_BYTE, decoded);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        stbi_image_free(decoded);

        mEmbeddedTextures[i] = texID;

        std::cout << "[GLB] Decoded image " << i
                  << " → " << w << "x" << h
                  << " (ch=" << ch << ", ID=" << texID << ")" << std::endl;
    }
}

std::vector<glm::mat4> Model::computeBoneMatrices(int skinIndex)
{
    std::vector<glm::mat4> result;

    if (skinIndex < 0 || skinIndex >= (int)skins.size())
        return result;

    const SkinData& skin = skins[skinIndex];
    if (skin.jointNodes.empty())
        return result;

    result.reserve(skin.jointNodes.size());

    MLOG("[Bone] skin=" << skinIndex
        << " joints=" << skin.jointNodes.size()
        << " nodeGlobals=" << nodeGlobalTransforms.size());

    for (size_t i = 0; i < skin.jointNodes.size(); ++i) {
        int nodeIdx = skin.jointNodes[i];

        glm::mat4 jointWorld = glm::mat4(1.0f);
        if (nodeIdx >= 0 && nodeIdx < (int)nodeGlobalTransforms.size()) {
            jointWorld = nodeGlobalTransforms[nodeIdx];
        } else {
            MLOG("[Bone] WARNING: joint node " << nodeIdx << " out of range!");
        }

        glm::mat4 invBind = (i < skin.inverseBindMatrices.size())
                            ? skin.inverseBindMatrices[i]
                            : glm::mat4(1.0f);

        if (i == 0) {
        MLOG("[Bone] jointWorld[0] row3 = ("
            << jointWorld[3][0] << ", "
            << jointWorld[3][1] << ", "
            << jointWorld[3][2] << ")");
        MLOG("[Bone] skin[0]    row3 = ("
            << (jointWorld * invBind)[3][0] << ", "
            << (jointWorld * invBind)[3][1] << ", "
            << (jointWorld * invBind)[3][2] << ")");

        
}

        result.push_back(jointWorld * invBind);
    }

    return result;
}

static void computeGlobalsRecursive(
    int node,
    const std::vector<glm::mat4>& local,
    const std::vector<int>& parents,
    std::vector<glm::mat4>& globals,
    std::vector<bool>& visited)
{
    if (node < 0 || node >= (int)local.size()) return;
    if (visited[node]) return;
    visited[node] = true;

    int p = parents[node];
    if (p < 0) {
        globals[node] = local[node];
    } else {
        if (!visited[p]) computeGlobalsRecursive(p, local, parents, globals, visited);
        globals[node] = globals[p] * local[node];
    }
}

// DFS is required — glTF does NOT guarantee parents come before children
// in node index order. CesiumMan.glb has node 2 (mesh) whose parent is 1,
// but other files place parents at higher indices.
void computeGlobalTransformsDFS(
    const std::vector<glm::mat4>& local,
    const std::vector<int>& parents,
    std::vector<glm::mat4>& globals)
{
    globals.assign(local.size(), glm::mat4(1.0f));
    std::vector<bool> visited(local.size(), false);
    for (int i = 0; i < (int)local.size(); ++i)
        computeGlobalsRecursive(i, local, parents, globals, visited);
}

void Model::rebuildLocalMatrices()
{
    size_t n = nodeLocalPositions.size();
    nodeLocalTransforms.resize(n);

    for (size_t i = 0; i < n; ++i) {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), nodeLocalPositions[i]);
        glm::mat4 R = glm::mat4_cast(nodeLocalRotations[i]);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), nodeLocalScales[i]);
        nodeLocalTransforms[i] = T * R * S;
    }
}

void applyAnimation(const Animation& anim, float t,
    std::vector<glm::vec3>& positions,
    std::vector<glm::quat>& rotations,
    std::vector<glm::vec3>& scales)
{
    for (const auto& ch : anim.channels) {
    if (ch.times.empty()) continue;
    if (ch.node < 0)      continue;
    if ((size_t)ch.node >= positions.size()) continue;

    // ---- find surrounding keyframes ----
    size_t k = 0;
    while (k + 1 < ch.times.size() && ch.times[k + 1] < t) k++;

    float t0 = ch.times[k];
    float t1 = (k + 1 < ch.times.size()) ? ch.times[k + 1] : t0;
    float u  = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;

    glm::vec4 v0 = ch.values[k];
    glm::vec4 v1 = ch.values[std::min(k + 1, ch.values.size() - 1)];

    if (ch.path == "translation") {
    glm::vec3 v = glm::mix(glm::vec3(v0), glm::vec3(v1), u);
    positions[ch.node] = v;
    }
    else if (ch.path == "scale") {
    glm::vec3 v = glm::mix(glm::vec3(v0), glm::vec3(v1), u);
    scales[ch.node] = v;
    }
    else if (ch.path == "rotation") {
    glm::quat q0(v0.w, v0.x, v0.y, v0.z);
    glm::quat q1(v1.w, v1.x, v1.y, v1.z);
    rotations[ch.node] = glm::normalize(glm::slerp(q0, q1, u));
    }
    }
}

void Model::updateAnimation(float t)
{
    MLOG("[updateAnimation] t=" << t 
        << " animations=" << animations.size()
        << " channels[0]=" << (animations.empty() ? 0 : animations[0].channels.size()));

    if (animations.empty()) return;

    // 1) Sample into TRS
    int clipIdx = 0;   // or pass in
    applyAnimation(animations[clipIdx], t,
                   nodeLocalPositions,
                   nodeLocalRotations,
                   nodeLocalScales);

    // 2) Rebuild local matrices
    rebuildLocalMatrices();

    // 3) Recompute world transforms
    computeGlobalTransformsDFS(nodeLocalTransforms,
                            nodeParents,
                            nodeGlobalTransforms);

    // 4) Rebuild bone matrices
    for (auto& mesh : meshes) {
        if (mesh.isSkinned && mesh.skinIndex >= 0) {
            mesh.boneMatrices = computeBoneMatrices(mesh.skinIndex);

            if (!mesh.boneMatrices.empty()) {
                static int animCounter = 0;
                if (animCounter++ < 5) {
                    std::cout << "[Anim Debug] mesh.boneMatrices[0] row3 = ("
                              << mesh.boneMatrices[0][3][0] << ", "
                              << mesh.boneMatrices[0][3][1] << ", "
                              << mesh.boneMatrices[0][3][2] << ")" << std::endl;
                }
            }
        }
    }
}

// ============================================================
//                     CONSTRUCTOR
// ============================================================
Model::Model(const char* filePath)
{
    MLOG("=== MODEL CTOR START: " << filePath << " ===");

    this->file = filePath;

    std::string text = get_file_contents(filePath);
    MLOG("=== STAGE 1: file read, size=" << text.size() << " ===");

    if (text.empty()) {
        std::cerr << "[Model] Empty file: " << filePath << std::endl;
        return;
    }

    bool isGLB = (text.size() >= 12
                  && text[0] == 'g' && text[1] == 'l'
                  && text[2] == 'T' && text[3] == 'F');

    MLOG("=== STAGE 2: isGLB=" << isGLB << " ===");

    try
    {
        if (isGLB)
        {
            mIsGLB = true;
            const unsigned char* p = reinterpret_cast<const unsigned char*>(text.data());

            uint32_t version     = readU32LE(p + 4);
            uint32_t totalLength = readU32LE(p + 8);

            if (version != 2) {
                std::cerr << "[Model] Unsupported GLB version: " << version << std::endl;
                return;
            }
            if (totalLength > text.size()) {
                std::cerr << "[Model] GLB totalLength > file size!" << std::endl;
                return;
            }

            size_t offset = 12;
            bool jsonFound = false;

            while (offset + 8 <= text.size())
            {
                uint32_t chunkLength = readU32LE(p + offset);
                uint32_t chunkType   = readU32LE(p + offset + 4);
                const unsigned char* chunkData = p + offset + 8;

                if (offset + 8 + chunkLength > text.size()) {
                    std::cerr << "[Model] GLB chunk out of bounds" << std::endl;
                    break;
                }

                if (chunkType == 0x4E4F534A) // 'JSON'
                {
                    std::string jsonStr(reinterpret_cast<const char*>(chunkData), chunkLength);
                    JSON = json::parse(jsonStr);
                    jsonFound = true;
                }
                else if (chunkType == 0x004E4942) // 'BIN\0'
                {
                    mGLBBuffer.assign(chunkData, chunkData + chunkLength);
                }

                offset += 8 + chunkLength;
            }

            if (!jsonFound) {
                std::cerr << "[Model] No JSON chunk in GLB" << std::endl;
                return;
            }

            data = mGLBBuffer;
            MLOG("=== STAGE 3a: GLB parsed, bin=" << mGLBBuffer.size() << " ===");

            // Decode embedded images from the binary chunk
            decodeGLBImages();


            loadSkins();
        }
        else
        {
            JSON = json::parse(text);
            MLOG("=== STAGE 3b: JSON parsed ===");
            data = getDataFromURI();
            MLOG("=== STAGE 3c: URI buffer size=" << data.size() << " ===");

            loadSkins();
        }

        MLOG("=== STAGE 4: about to traverseNode(0) ===");
        // Use scene's root nodes if available
        if (JSON.contains("scenes") && JSON.contains("scene")) {
            int sceneIdx = JSON["scene"];
            if (sceneIdx >= 0 && sceneIdx < (int)JSON["scenes"].size()) {
                auto& scene = JSON["scenes"][sceneIdx];
                if (scene.contains("nodes")) {
                    MLOG("[Model] Scene " << sceneIdx
                        << " has " << scene["nodes"].size() << " root node(s)");                   
                    for (auto& rootNodeIdx : scene["nodes"]) {
                        traverseNode(rootNodeIdx.get<unsigned int>());
                    }
                } else {
                    traverseNode(0);
                }
            } else {
                traverseNode(0);
            }
        } else {
            traverseNode(0);
        }
        MLOG("=== STAGE 5: traverseNode DONE, meshes=" << meshes.size() << " ===");
        // Compute bone matrices AFTER full traversal
        for (auto& mesh : meshes) {
            if (mesh.isSkinned && mesh.skinIndex >= 0) {
                mesh.boneMatrices = computeBoneMatrices(mesh.skinIndex);
                MLOG("[Model] Final: " << mesh.boneMatrices.size()
                    << " bone matrices for skinned mesh");
            }
        }

    }
    catch (const std::exception& e)
    {
        std::cerr << "\n!!!!! EXCEPTION in Model constructor !!!!!\n";
        std::cerr << "  file: " << filePath << "\n";
        std::cerr << "  what(): " << e.what() << "\n";
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
        std::cin.get();
        std::exit(1);
    }

    MLOG("=== MODEL CTOR DONE ===");
}

// ============================================================
//                       DRAW
// ============================================================
void Model::Draw(Shader& shader,
                 Camera& camera,
                 glm::mat4 globalTransform,
                 bool bindTextures)
{
    size_t n = std::min(meshes.size(), matricesMeshes.size());
    for (size_t i = 0; i < n; i++)
    {
        glm::mat4 finalMatrix = globalTransform * matricesMeshes[i];

        meshes[i].Mesh::Draw(shader,
                             camera,
                             finalMatrix,
                             glm::vec3(0.0f),
                             glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                             glm::vec3(1.0f),
                             bindTextures);
    }
}

// ============================================================
//                     LOAD MESH (multi-primitive)
// ============================================================
void Model::loadMesh(unsigned int indMesh)
{
    MLOG("    [LM] enter mesh=" << indMesh);

    try
    {
        if (!JSON.contains("meshes")) {
            std::cerr << "    [LM] JSON has no 'meshes' key!" << std::endl;
            return;
        }

        if (indMesh >= JSON["meshes"].size()) {
            std::cerr << "    [LM] mesh " << indMesh << " out of range!" << std::endl;
            return;
        }

        json meshJson = JSON["meshes"][indMesh];
        MLOG("    [LM] got mesh json");

        if (!meshJson.contains("primitives")) {
            std::cerr << "    [LM] mesh has no 'primitives' key!" << std::endl;
            return;
        }

        if (meshJson["primitives"].empty()) {
            std::cerr << "    [LM] primitives array is EMPTY!" << std::endl;
            return;
        }

        MLOG("    [LM] primitives count=" << meshJson["primitives"].size());

        for (size_t pIdx = 0; pIdx < meshJson["primitives"].size(); ++pIdx)
        {
            MLOG("    [LM] === prim " << pIdx << " start ===");

            json primitive = meshJson["primitives"][pIdx];

            if (!primitive.contains("attributes")) {
                std::cerr << "    [LM] prim " << pIdx << " has no attributes!" << std::endl;
                continue;
            }

            json attrs = primitive["attributes"];
            MLOG("    [LM] prim " << pIdx << " got attrs");

            // ---- POSITION is REQUIRED ----
            if (!attrs.contains("POSITION")) {
                std::cerr << "    [LM] prim " << pIdx << " no POSITION" << std::endl;
                continue;
            }

            // ---- Read POSITIONS first ----
            unsigned int posAccInd = attrs["POSITION"];
            MLOG("    [LM] prim " << pIdx << " reading POSITION accessor " << posAccInd);
            std::vector<glm::vec3> positions = groupFloatsVec3(getFloats(JSON["accessors"][posAccInd]));

            if (positions.empty()) {
                std::cerr << "    [LM] prim " << pIdx << " positions empty" << std::endl;
                continue;
            }

            // ---- NORMAL (optional) ----
            std::vector<glm::vec3> normals;
            if (attrs.contains("NORMAL")) {
                unsigned int normalAccInd = attrs["NORMAL"];
                MLOG("    [LM] prim " << pIdx << " reading NORMAL accessor " << normalAccInd);
                normals = groupFloatsVec3(getFloats(JSON["accessors"][normalAccInd]));
            }
            if (normals.size() != positions.size()) {
                normals.assign(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
                MLOG("    [LM] prim " << pIdx << " using default normals");
            }

            // ---- TEXCOORD_0 (optional) ----
            std::vector<glm::vec2> texUVs;
            if (attrs.contains("TEXCOORD_0")) {
                unsigned int texAccInd = attrs["TEXCOORD_0"];
                MLOG("    [LM] prim " << pIdx << " reading TEXCOORD_0 accessor " << texAccInd);
                texUVs = groupFloatsVec2(getFloats(JSON["accessors"][texAccInd]));
            }
            if (texUVs.size() != positions.size()) {
                texUVs.assign(positions.size(), glm::vec2(0.0f));
                MLOG("    [LM] prim " << pIdx << " using default UVs");
            }

            // ---- TANGENT (optional) ----
            std::vector<glm::vec3> tangents, bitangents;

            // ---- Read JOINTS_0 and WEIGHTS_0 (for skinning) ----
            std::vector<glm::ivec4> joints;
            std::vector<glm::vec4> weights;

            if (attrs.contains("JOINTS_0")) {
                unsigned int jointsAccInd = attrs["JOINTS_0"];
                // JOINTS_0 is typically VEC4 UNSIGNED_BYTE or UNSIGNED_SHORT
                std::vector<float> jointsRaw = getFloats(JSON["accessors"][jointsAccInd]);
                joints.reserve(jointsRaw.size() / 4);
                for (size_t i = 0; i + 4 <= jointsRaw.size(); i += 4) {
                    joints.emplace_back(
                        (int)jointsRaw[i],
                        (int)jointsRaw[i + 1],
                        (int)jointsRaw[i + 2],
                        (int)jointsRaw[i + 3]
                    );
                }
            }

            if (attrs.contains("WEIGHTS_0")) {
                unsigned int weightsAccInd = attrs["WEIGHTS_0"];
                std::vector<float> weightsRaw = getFloats(JSON["accessors"][weightsAccInd]);
                weights.reserve(weightsRaw.size() / 4);
                for (size_t i = 0; i + 4 <= weightsRaw.size(); i += 4) {
                    weights.emplace_back(
                        weightsRaw[i],
                        weightsRaw[i + 1],
                        weightsRaw[i + 2],
                        weightsRaw[i + 3]
                    );
                }
            }

            if (!joints.empty() && !weights.empty()) {
                MLOG("[Mesh] vert[0] joints=(" << joints[0].x << "," << joints[0].y
                     << "," << joints[0].z << "," << joints[0].w << ")"
                     << " weights=(" << weights[0].x << "," << weights[0].y
                     << "," << weights[0].z << "," << weights[0].w << ")");
            } else {
                MLOG("[Mesh] no skin data (not skinned)");
            }
            

            if (attrs.contains("TANGENT")) {
                unsigned int tanAccInd = attrs["TANGENT"];
                std::vector<float> tanVec = getFloats(JSON["accessors"][tanAccInd]);
                std::vector<glm::vec4> tanVec4 = groupFloatsVec4(tanVec);
                tangents.resize(tanVec4.size());
                for (size_t i = 0; i < tanVec4.size(); i++)
                    tangents[i] = glm::vec3(tanVec4[i].x, tanVec4[i].y, tanVec4[i].z);

                if (tangents.size() != positions.size()) {
                    tangents.assign(positions.size(), glm::vec3(1.0f, 0.0f, 0.0f));
                }

                bitangents.resize(tangents.size());
                for (size_t i = 0; i < tangents.size(); i++) {
                    if (i < normals.size())
                        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]));
                    else
                        bitangents[i] = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            } else {
                tangents.assign(positions.size(),   glm::vec3(1.0f, 0.0f, 0.0f));
                bitangents.assign(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // ---- Assemble vertices ----
            std::vector<Vertex> vertices = assembleVertices(
                positions, normals, texUVs, tangents, bitangents,
                joints, weights);

            if (vertices.empty()) {
                std::cerr << "    [LM] prim " << pIdx << ": no vertices, skipping" << std::endl;
                continue;
            }

            // ---- Indices (optional) ----
            std::vector<GLuint> indices;
            if (primitive.contains("indices")) {
                MLOG("    [LM] prim " << pIdx << " reading indices");
                unsigned int indAccInd = primitive["indices"].get<unsigned int>();
                indices = getIndices(JSON["accessors"][indAccInd]);
            } else {
                indices.resize(vertices.size());
                for (size_t i = 0; i < vertices.size(); ++i) indices[i] = (GLuint)i;
            }

                        // ---- Material ----
            MLOG("    [LM] prim " << pIdx << " calling getTexturesForMaterial");
            int matIndex = primitive.value("material", -1);
            std::vector<Texture> textures = getTexturesForMaterial(matIndex);

            // ---- Read material factors from JSON ----
            glm::vec4  baseColorFactor(1.0f);
            float      metallicFactor  = 1.0f;
            float      roughnessFactor = 1.0f;
            glm::vec3  emissiveFactor(0.0f);
            float      emissiveStrength = 1.0f;

            if (matIndex >= 0
                && JSON.contains("materials")
                && matIndex < (int)JSON["materials"].size())
            {
                json mat = JSON["materials"][matIndex];

                if (mat.contains("pbrMetallicRoughness")) {
                    json pbr = mat["pbrMetallicRoughness"];

                    if (pbr.contains("baseColorFactor") && pbr["baseColorFactor"].is_array()) {
                        auto& bcf = pbr["baseColorFactor"];
                        baseColorFactor = glm::vec4(
                            bcf[0].get<float>(),
                            bcf[1].get<float>(),
                            bcf[2].get<float>(),
                            bcf.size() > 3 ? bcf[3].get<float>() : 1.0f
                        );
                    }

                    metallicFactor  = pbr.value("metallicFactor",  1.0f);
                    roughnessFactor = pbr.value("roughnessFactor", 1.0f);
                }

                if (mat.contains("emissiveFactor") && mat["emissiveFactor"].is_array()) {
                    auto& ef = mat["emissiveFactor"];
                    emissiveFactor = glm::vec3(
                        ef[0].get<float>(),
                        ef[1].get<float>(),
                        ef[2].get<float>()
                    );
                }

                if (mat.contains("extensions")
                    && mat["extensions"].contains("KHR_materials_emissive_strength"))
                {
                    auto& ext = mat["extensions"]["KHR_materials_emissive_strength"];
                    emissiveStrength = ext.value("emissiveStrength", 1.0f);
                }

                MLOG("[LM] prim " << pIdx 
                    << " emissive=(" << emissiveFactor.x << "," << emissiveFactor.y << "," << emissiveFactor.z
                    << ") strength=" << emissiveStrength);               
            }

            // ---- Detect which textures are present ----
            bool hasDiffuse  = false;
            bool hasNormal   = false;
            bool hasMR       = false;
            bool hasEmissive = false;
            bool hasAO       = false;

            for (const auto& tex : textures) {
                if (tex.type == "diffuse")           hasDiffuse  = true;
                if (tex.type == "normal")            hasNormal   = true;
                if (tex.type == "metallicRoughness") hasMR       = true;
                if (tex.type == "emissive")          hasEmissive = true;
                if (tex.type == "ao")                hasAO       = true;
            }

            // ---- Create Mesh with all fields ----
            Mesh m(vertices, indices, textures);
            m.baseColorFactor     = baseColorFactor;
            m.metallicFactor      = metallicFactor;
            m.roughnessFactor     = roughnessFactor;
            m.emissiveFactor      = emissiveFactor;
            m.emissiveStrength    = emissiveStrength;

            m.hasBaseColorTexture = hasDiffuse;
            m.hasMRTexture        = hasMR;
            m.hasNormalTexture    = hasNormal;
            m.hasEmissiveTexture  = hasEmissive;
            m.hasAOTexture        = hasAO;

            MLOG("    [LM] prim " << pIdx << " pushing Mesh: "
                 << vertices.size() << " verts, "
                 << indices.size() << " idx, "
                 << textures.size() << " tex");

            meshes.push_back(m);
            MLOG("    [LM] prim " << pIdx << " done");
        }   // ← هذا يقفل حلقة for (pIdx)

        MLOG("    [LM] exit mesh=" << indMesh);
        
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n!!!!! EXCEPTION in loadMesh !!!!!\n";
        std::cerr << "  mesh=" << indMesh << "\n";
        std::cerr << "  what(): " << e.what() << "\n";
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
    }
    
}

// ============================================================
//                     TRAVERSE NODE
// ============================================================
void Model::traverseNode(unsigned int nextNode, glm::mat4 matrix, int parentIdx)
{
    MLOG("  [TN] enter node=" << nextNode);

    if (!JSON.contains("nodes") || nextNode >= JSON["nodes"].size()) {
        std::cerr << "  [TN] node " << nextNode << " out of range!" << std::endl;
        return;
    }

    // Record parent index
    if (nextNode >= nodeParents.size())
        nodeParents.resize(nextNode + 1, -1);
    nodeParents[nextNode] = parentIdx;

    json node = JSON["nodes"][nextNode];

    glm::vec3 translation(0.0f);
    if (node.find("translation") != node.end()) {
        float t[3] = { 0, 0, 0 };
        for (unsigned int i = 0; i < node["translation"].size() && i < 3; i++)
            t[i] = node["translation"][i];
        translation = glm::make_vec3(t);
    }

    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    if (node.find("rotation") != node.end()) {
        float r[4] = {
            node["rotation"][3],
            node["rotation"][0],
            node["rotation"][1],
            node["rotation"][2]
        };
        rotation = glm::make_quat(r);
    }

    glm::vec3 scale(1.0f);
    if (node.find("scale") != node.end()) {
        float s[3] = { 1, 1, 1 };
        for (unsigned int i = 0; i < node["scale"].size() && i < 3; i++)
            s[i] = node["scale"][i];
        scale = glm::make_vec3(s);
    }

    glm::mat4 matNode(1.0f);
    bool hasMatrix = (node.find("matrix") != node.end());
    if (hasMatrix) {
        float m[16];
        for (unsigned int i = 0; i < node["matrix"].size() && i < 16; i++)
            m[i] = node["matrix"][i];
        matNode = glm::make_mat4(m);
    }

    glm::mat4 trans = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 rot   = glm::mat4_cast(rotation);
    glm::mat4 sca   = glm::scale(glm::mat4(1.0f), scale);

    glm::mat4 local;
    if (hasMatrix) local = matNode;
    else           local = trans * rot * sca;

    glm::mat4 matNextNode = matrix * local;

    // Store global transform for this node (for skinning)
    if (nextNode >= nodeGlobalTransforms.size())
        nodeGlobalTransforms.resize(nextNode + 1, glm::mat4(1.0f));
    nodeGlobalTransforms[nextNode] = matNextNode;

    // Store local transform (for animation)
    if (nextNode >= nodeLocalTransforms.size())
        nodeLocalTransforms.resize(nextNode + 1, glm::mat4(1.0f));
    nodeLocalTransforms[nextNode] = local;

    // Ensure the TRS vectors are large enough
    if (nextNode >= nodeLocalPositions.size()) {
        size_t newSize = nextNode + 1;
        nodeLocalPositions.resize(newSize, glm::vec3(0.0f));
        nodeLocalRotations.resize(newSize, glm::quat(1, 0, 0, 0));
        nodeLocalScales.resize(newSize, glm::vec3(1.0f));
        nodeLocalTransforms.resize(newSize, glm::mat4(1.0f));
    }

    nodeLocalPositions[nextNode] = translation;
    nodeLocalRotations[nextNode] = rotation;
    nodeLocalScales[nextNode]    = scale;
    nodeLocalTransforms[nextNode] = local;

    if (node.find("mesh") != node.end())
    {
        MLOG("  [TN] node has mesh=" << node["mesh"]);

        int skinIndex = node.value("skin", -1);
        MLOG("  [TN] node skin=" << skinIndex);

        size_t before = meshes.size();
        loadMesh(node["mesh"]);
        size_t added = meshes.size() - before;

        MLOG("  [TN] loadMesh done, added " << added << " meshes");

        for (size_t k = 0; k < added; ++k) {
            translationsMeshes.push_back(translation);
            rotationsMeshes.push_back(rotation);
            scalesMeshes.push_back(scale);
            matricesMeshes.push_back(matNextNode);

            if (skinIndex >= 0 && skinIndex < (int)skins.size()) {
                meshes[before + k].isSkinned = true;
                meshes[before + k].skinIndex = skinIndex;
            }
        }
    }

    if (node.find("children") != node.end())
    {
        MLOG("  [TN] node has " << node["children"].size() << " children");
        for (unsigned int i = 0; i < node["children"].size(); i++)
            traverseNode(node["children"][i], matNextNode, nextNode);
    }

    MLOG("  [TN] exit node=" << nextNode);
}

// ============================================================
//              GET DATA FROM URI (.gltf path)
// ============================================================
std::vector<unsigned char> Model::getDataFromURI()
{
    if (!JSON.contains("buffers") || JSON["buffers"].empty())
        return {};

    json buf = JSON["buffers"][0];
    if (!buf.contains("uri")) {
        std::cerr << "[Model] Buffer has no URI in .gltf file" << std::endl;
        return {};
    }

    std::string uri = buf["uri"];

    if (uri.find("data:") == 0) {
        std::cerr << "[Model] data: URIs not supported" << std::endl;
        return {};
    }

    std::string fileStr = std::string(file);
    size_t lastSlash = fileStr.find_last_of("/\\");
    std::string fileDirectory = (lastSlash != std::string::npos)
                                ? fileStr.substr(0, lastSlash + 1)
                                : "";

    std::string fullBufferPath = fileDirectory + uri;
    MLOG("[Model] Loading buffer: " << fullBufferPath);

    std::string bytesText = get_file_contents(fullBufferPath.c_str());
    if (bytesText.empty()) {
        std::cerr << "[Model] Buffer empty/not found: " << fullBufferPath << std::endl;
        return {};
    }

    return std::vector<unsigned char>(bytesText.begin(), bytesText.end());
}

// ============================================================
//              GET FLOATS (stride-aware)
// ============================================================
std::vector<float> Model::getFloats(json accessor)
{
    std::vector<float> out;
    if (data.empty()) return out;

    if (!accessor.contains("componentType") || !accessor.contains("count")
        || !accessor.contains("type")) {
        std::cerr << "[Model] Malformed accessor" << std::endl;
        return out;
    }

    unsigned int buffViewInd   = accessor.value("bufferView", 0);
    unsigned int count         = accessor["count"];
    unsigned int accByteOffset = accessor.value("byteOffset", 0);
    unsigned int componentType = accessor["componentType"];
    std::string  type          = accessor["type"];

    if (!JSON.contains("bufferViews") || buffViewInd >= JSON["bufferViews"].size()) {
        std::cerr << "[Model] Bad bufferView index: " << buffViewInd << std::endl;
        return out;
    }

    json bufferView = JSON["bufferViews"][buffViewInd];
    unsigned int byteOffset = bufferView.value("byteOffset", 0);
    unsigned int byteStride = bufferView.value("byteStride", 0);

    unsigned int numPerVert = 1;
    if      (type == "SCALAR") numPerVert = 1;
    else if (type == "VEC2")   numPerVert = 2;
    else if (type == "VEC3")   numPerVert = 3;
    else if (type == "VEC4")   numPerVert = 4;
    else if (type == "MAT2")   numPerVert = 4;   // ← NEW
    else if (type == "MAT3")   numPerVert = 9;   // ← NEW
    else if (type == "MAT4")   numPerVert = 16;  // ← NEW
    else { std::cerr << "[Model] Bad type: " << type << std::endl; return out; }

    unsigned int componentSize = 4;
    switch (componentType) {
        case 5120: componentSize = 1; break;
        case 5121: componentSize = 1; break;
        case 5122: componentSize = 2; break;
        case 5123: componentSize = 2; break;
        case 5125: componentSize = 4; break;
        case 5126: componentSize = 4; break;
        default:
            std::cerr << "[Model] Unknown componentType " << componentType << std::endl;
            return out;
    }

    unsigned int elemSize = componentSize * numPerVert;
    unsigned int stride   = (byteStride != 0) ? byteStride : elemSize;
    if (byteStride != 0 && byteStride < elemSize) {
        std::cerr << "[Model] byteStride < element size — malformed\n";
        return out;
    }
    unsigned int beginning = byteOffset + accByteOffset;

    if (count == 0) return out;
    if ((uint64_t)beginning + (uint64_t)stride * (count - 1) + elemSize > data.size()) {
        std::cerr << "[Model] getFloats OOB" << std::endl;
        return out;
    }

    out.reserve((size_t)count * numPerVert);
    const unsigned char* base = data.data() + beginning;

    for (unsigned int i = 0; i < count; ++i) {
        const unsigned char* elem = base + (size_t)i * stride;
        for (unsigned int c = 0; c < numPerVert; ++c) {
            const unsigned char* e = elem + c * componentSize;
            float v = 0.0f;
            switch (componentType) {
                case 5126: { float    t; std::memcpy(&t, e, 4); v = t; break; }
                case 5125: { uint32_t t; std::memcpy(&t, e, 4); v = (float)t; break; }
                case 5123: { uint16_t t; std::memcpy(&t, e, 2); v = (float)t; break; }
                case 5122: { int16_t  t; std::memcpy(&t, e, 2); v = (float)t; break; }
                case 5121: { v = (float)(*e); break; }
                case 5120: { v = (float)((int8_t)*e); break; }
            }
            out.push_back(v);
        }
    }

    return out;
}

// ============================================================
//              GET INDICES (stride-aware)
// ============================================================
std::vector<GLuint> Model::getIndices(json accessor)
{
    std::vector<GLuint> out;
    if (data.empty()) return out;

    unsigned int buffViewInd   = accessor.value("bufferView", 0);
    unsigned int count         = accessor.value("count", 0);
    unsigned int accByteOffset = accessor.value("byteOffset", 0);
    unsigned int componentType = accessor.value("componentType", 5123);

    if (!JSON.contains("bufferViews") || buffViewInd >= JSON["bufferViews"].size()) {
        std::cerr << "[Model] Bad bufferView for indices" << std::endl;
        return out;
    }

    json bufferView = JSON["bufferViews"][buffViewInd];
    unsigned int byteOffset = bufferView.value("byteOffset", 0);

    unsigned int componentSize = 0;
    switch (componentType) {
        case 5121: componentSize = 1; break;
        case 5123: componentSize = 2; break;
        case 5125: componentSize = 4; break;
        default:
            std::cerr << "[Model] Bad index componentType " << componentType << std::endl;
            return out;
    }

    unsigned int beginning = byteOffset + accByteOffset;
    if (count == 0) return out;
    if ((uint64_t)beginning + (uint64_t)count * componentSize > data.size()) {
        std::cerr << "[Model] getIndices OOB" << std::endl;
        return out;
    }

    out.reserve(count);
    const unsigned char* base = data.data() + beginning;

    for (unsigned int i = 0; i < count; ++i) {
        const unsigned char* e = base + (size_t)i * componentSize;
        GLuint v = 0;
        if      (componentType == 5121) v = *e;
        else if (componentType == 5123) { uint16_t t; std::memcpy(&t, e, 2); v = t; }
        else if (componentType == 5125) { uint32_t t; std::memcpy(&t, e, 4); v = t; }
        out.push_back(v);
    }

    return out;
}

// HELPERS
std::vector<float> Model::readScalarAccessor(int accessorIdx) {
    std::vector<float> out;
    auto& acc = JSON["accessors"][accessorIdx];
    int bvIdx = acc["bufferView"].get<int>();
    auto& bv  = JSON["bufferViews"][bvIdx];

    size_t offset = bv.value("byteOffset", 0) + acc.value("byteOffset", 0);
    size_t count  = acc["count"].get<size_t>();
    int compType  = acc["componentType"].get<int>();  // 5126 = FLOAT
    if (compType != 5126) { std::cerr << "[Anim] non-float scalar!\n"; return out; }

    out.resize(count);
    std::memcpy(out.data(), data.data() + offset, count * sizeof(float));
    return out;
}

std::vector<glm::vec4> Model::readVec4Accessor(int accessorIdx) {
    std::vector<glm::vec4> out;
    auto& acc = JSON["accessors"][accessorIdx];
    int bvIdx = acc["bufferView"].get<int>();
    auto& bv  = JSON["bufferViews"][bvIdx];

    size_t offset = bv.value("byteOffset", 0) + acc.value("byteOffset", 0);
    size_t count  = acc["count"].get<size_t>();
    std::string type = acc["type"].get<std::string>();  // "VEC3" or "VEC4"
    int comps = (type == "VEC3") ? 3 : (type == "VEC4") ? 4 : 0;
    if (comps == 0) { std::cerr << "[Anim] bad type " << type << "\n"; return out; }

    const float* src = reinterpret_cast<const float*>(data.data() + offset);
    out.resize(count);
    for (size_t i = 0; i < count; i++) {
        glm::vec4 v(0, 0, 0, 0);
        v.x = src[i * comps + 0];
        v.y = src[i * comps + 1];
        v.z = src[i * comps + 2];
        if (comps == 4) v.w = src[i * comps + 3];
        out[i] = v;
    }
    return out;
}

// ============================================================
//              FALLBACK WHITE TEXTURE
// ============================================================
std::vector<Texture> Model::getFallbackWhiteTexture()
{
    std::vector<Texture> textures;

    unsigned int whiteTexID = 0;
    glGenTextures(1, &whiteTexID);
    glBindTexture(GL_TEXTURE_2D, whiteTexID);
    unsigned char whitePixel[] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    Texture whiteTex(whiteTexID, "diffuse", (int)GetLoadedTex().size());
    textures.push_back(whiteTex);
    GetLoadedTex().push_back(whiteTex);
    GetLoadedTexName().push_back("__fallback_white__");
    return textures;
}

// ============================================================
//              LOAD TEXTURE FROM IMAGE REF
// ============================================================
void Model::loadTextureFromImageRef(json texRef,
    const std::string& typeName,
    const std::string& baseDirectory,
    std::vector<Texture>& out)
{
if (!texRef.contains("index")) return;
int texIndex = texRef["index"];

if (!JSON.contains("textures") || texIndex >= (int)JSON["textures"].size()) return;
json tex = JSON["textures"][texIndex];
if (!tex.contains("source")) return;
int imgIndex = tex["source"];

if (!JSON.contains("images") || imgIndex >= (int)JSON["images"].size()) return;
json img = JSON["images"][imgIndex];

auto& texCache  = GetLoadedTex();
auto& nameCache = GetLoadedTexName();

// ============================================
//  Case 1: External URI (.gltf)
// ============================================
if (img.contains("uri")) {
std::string texPath = img["uri"];

if (texPath.find("data:") == 0) {
std::cerr << "[Texture] data: URI not supported: " << typeName << std::endl;
return;
}

// Cache check
for (size_t j = 0; j < nameCache.size(); ++j) {
if (nameCache[j] == texPath) {
out.push_back(texCache[j]);
return;
}
}

std::string fullPath = baseDirectory + texPath;
if (!std::filesystem::exists(fullPath)) fullPath = texPath;
if (!std::filesystem::exists(fullPath)) {
std::cerr << "[Texture] Missing file: " << fullPath << std::endl;
return;
}

Texture t(fullPath.c_str(), typeName.c_str(), (int)texCache.size());
t.path = fullPath;
out.push_back(t);
texCache.push_back(t);
nameCache.push_back(texPath);

std::cerr << "[Texture] Loaded: " << fullPath
<< " (type=" << typeName << ", ID=" << t.ID << ")" << std::endl;
return;
}

// ============================================
//  Case 2: Embedded in GLB (bufferView)
//  Placeholder: white 1x1 texture
// ============================================
if (img.contains("bufferView")) {
    // Check if we decoded this image already
    auto it = mEmbeddedTextures.find((int)imgIndex);
    if (it != mEmbeddedTextures.end() && it->second != 0) {
        GLuint texID = it->second;
        std::string cacheKey = "__glb_" + file + "_decoded_" +
                       std::to_string(imgIndex) + "_" + typeName;

        // Cache check
        for (size_t j = 0; j < nameCache.size(); ++j) {
            if (nameCache[j] == cacheKey) {
                out.push_back(texCache[j]);
                return;
            }
        }

        Texture t(texID, typeName.c_str(), (int)texCache.size());
        t.path = cacheKey;
        out.push_back(t);
        texCache.push_back(t);
        nameCache.push_back(cacheKey);

        std::cerr << "[Texture] Using decoded GLB image " << imgIndex
                  << " (" << typeName << ") ID=" << texID << std::endl;
        return;
    }

    // Fallback: white placeholder
    int bvIdx = img["bufferView"];
    std::string cacheKey = "__glb_embedded_" + std::to_string(bvIdx) + "_" + typeName;

    for (size_t j = 0; j < nameCache.size(); ++j) {
        if (nameCache[j] == cacheKey) {
            out.push_back(texCache[j]);
            return;
        }
    }

    unsigned int whiteID = 0;
    glGenTextures(1, &whiteID);
    glBindTexture(GL_TEXTURE_2D, whiteID);
    unsigned char whitePixel[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    Texture t(whiteID, typeName.c_str(), (int)texCache.size());
    t.path = cacheKey;
    out.push_back(t);
    texCache.push_back(t);
    nameCache.push_back(cacheKey);

    std::cerr << "[Texture] Embedded image " << bvIdx
              << " (" << typeName << ") — white placeholder (decode failed)" << std::endl;
    return;
}


std::cerr << "[Texture] Image " << imgIndex << " has no uri or bufferView" << std::endl;
}

// ============================================================
//              GET TEXTURES FOR MATERIAL
// ============================================================
std::vector<Texture> Model::getTexturesForMaterial(int materialIndex)
{
    std::vector<Texture> textures;

    std::string baseDirectory = mDirectory;
    if (baseDirectory.empty() && !file.empty()) {
        std::string fileStr = std::string(file);
        size_t lastSlash = fileStr.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            baseDirectory = fileStr.substr(0, lastSlash + 1);
    }

    if (materialIndex < 0
        || !JSON.contains("materials")
        || materialIndex >= (int)JSON["materials"].size())
    {
        return getFallbackWhiteTexture();
    }

    json mat = JSON["materials"][materialIndex];

    if (mat.contains("pbrMetallicRoughness")) {
        json pbr = mat["pbrMetallicRoughness"];
        if (pbr.contains("baseColorTexture"))
            loadTextureFromImageRef(pbr["baseColorTexture"], "diffuse", baseDirectory, textures);
        if (pbr.contains("metallicRoughnessTexture"))
            loadTextureFromImageRef(pbr["metallicRoughnessTexture"], "metallicRoughness", baseDirectory, textures);
    }

    if (mat.contains("normalTexture"))
        loadTextureFromImageRef(mat["normalTexture"], "normal", baseDirectory, textures);

    if (mat.contains("occlusionTexture"))
        loadTextureFromImageRef(mat["occlusionTexture"], "ao", baseDirectory, textures);

    if (mat.contains("emissiveTexture"))
        loadTextureFromImageRef(mat["emissiveTexture"], "emissive", baseDirectory, textures);

    if (textures.empty())
        return getFallbackWhiteTexture();

    return textures;
}

// ============================================================
//                 VERTEX ASSEMBLY
// ============================================================
std::vector<Vertex> Model::assembleVertices(
    std::vector<glm::vec3> positions,
    std::vector<glm::vec3> normals,
    std::vector<glm::vec2> texUVs,
    std::vector<glm::vec3> tangents,
    std::vector<glm::vec3> bitangents,
    std::vector<glm::ivec4> joints,
    std::vector<glm::vec4> weights)
{
    std::vector<Vertex> vertices;
    unsigned int n = (unsigned int)positions.size();
    n = std::min(n, (unsigned int)normals.size());
    n = std::min(n, (unsigned int)texUVs.size());
    n = std::min(n, (unsigned int)tangents.size());
    n = std::min(n, (unsigned int)bitangents.size());

    if (n == 0) return vertices;

    vertices.reserve(n);
    for (unsigned int i = 0; i < n; ++i) {
        Vertex v;
        v.position  = positions[i];
        v.normal    = normals[i];
        v.color     = glm::vec3(1.0f);
        v.texUV     = texUVs[i];
        v.tangent   = tangents[i];
        v.bitangent = bitangents[i];

        // Skinning data (or zero if not present)
        v.joints  = (i < joints.size())  ? joints[i]  : glm::ivec4(0);
        v.weights = (i < weights.size()) ? weights[i] : glm::vec4(0.0f);

        vertices.push_back(v);
    }
    return vertices;
}


// ============================================================
//                    GROUP FLOATS
// ============================================================
std::vector<glm::vec2> Model::groupFloatsVec2(std::vector<float> v)
{
    std::vector<glm::vec2> out;
    out.reserve(v.size() / 2);
    for (size_t i = 0; i + 2 <= v.size(); i += 2)
        out.emplace_back(v[i], v[i + 1]);
    return out;
}

std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> v)
{
    std::vector<glm::vec3> out;
    out.reserve(v.size() / 3);
    for (size_t i = 0; i + 3 <= v.size(); i += 3)
        out.emplace_back(v[i], v[i + 1], v[i + 2]);
    return out;
}

std::vector<glm::vec4> Model::groupFloatsVec4(std::vector<float> v)
{
    std::vector<glm::vec4> out;
    out.reserve(v.size() / 4);
    for (size_t i = 0; i + 4 <= v.size(); i += 4)
        out.emplace_back(v[i], v[i + 1], v[i + 2], v[i + 3]);
    return out;
}

void Model::loadSkins()
{
    for (size_t s = 0; s < JSON["skins"].size(); ++s) {
        auto& skinJson = JSON["skins"][s];
        SkinData skin;

        // Read joint node indices
        if (skinJson.contains("joints")) {
            for (auto& j : skinJson["joints"]) {
                skin.jointNodes.push_back(j.get<int>());
            }
        }

        // Read inverseBindMatrices (optional)
        if (skinJson.contains("inverseBindMatrices")) {
            int accIdx = skinJson["inverseBindMatrices"];
            std::vector<float> floats = getFloats(JSON["accessors"][accIdx]);

            // MAT4 = 16 floats per matrix
            skin.inverseBindMatrices.reserve(floats.size() / 16);
            for (size_t i = 0; i + 16 <= floats.size(); i += 16) {
                glm::mat4 m;
                std::memcpy(glm::value_ptr(m), &floats[i], 16 * sizeof(float));
                skin.inverseBindMatrices.push_back(m);
            }
        } else {
            // No inverse bind matrices → use identity for all joints
            skin.inverseBindMatrices.assign(skin.jointNodes.size(), glm::mat4(1.0f));
        }

        MLOG("[Skin] IBM[0][3] = (" 
            << skin.inverseBindMatrices[0][3][0] << ", "
            << skin.inverseBindMatrices[0][3][1] << ", "
            << skin.inverseBindMatrices[0][3][2] << ")");
       

        // Read skeleton root (optional)
        if (skinJson.contains("skeleton")) {
            skin.skeletonRoot = skinJson["skeleton"].get<int>();
        }

        skins.push_back(skin);

        std::cout << "[Model] Skin " << s << ": "
                  << skin.jointNodes.size() << " joints, "
                  << skin.inverseBindMatrices.size() << " IBMs"
                  << std::endl;
    }

    for (auto& animJson : JSON["animations"]) {
        Animation anim;
        anim.name = animJson.value("name", "anim");
    
        for (auto& ch : animJson["channels"]) {
            std::cout << "[Anim] animJson is_array() = " << animJson["channels"].is_array()
            << " is_null() = " << animJson["channels"].is_null()
            << " size() = " << animJson["channels"].size() << std::endl;
            try {
                AnimationChannel c;
                c.node = ch["target"]["node"].get<int>();
                c.path = ch["target"]["path"].get<std::string>();
    
                int samplerIdx = ch["sampler"].get<int>();
                if (samplerIdx < 0 || samplerIdx >= (int)animJson["samplers"].size()) {
                    std::cerr << "[Anim] sampler idx out of range: " << samplerIdx << std::endl;
                    continue;
                }
                auto& sampler = animJson["samplers"][samplerIdx];
                c.interp = sampler.value("interpolation", "LINEAR");
    
                int inAcc  = sampler["input"].get<int>();
                int outAcc = sampler["output"].get<int>();
    
                c.times  = readScalarAccessor(inAcc);
                c.values = readVec4Accessor(outAcc);
    
                std::cout << "[Anim] channel: node=" << c.node
                          << " path=" << c.path
                          << " times=" << c.times.size()
                          << " values=" << c.values.size() << std::endl;
    
                if (!c.times.empty())
                    anim.duration = std::max(anim.duration, c.times.back());
                anim.channels.push_back(std::move(c));
            }
            catch (const std::exception& e) {
                std::cerr << "[Anim] EXCEPTION in channel: " << e.what() << std::endl;
            }
        }
    
        animations.push_back(std::move(anim));
        std::cout << "[Anim] Loaded '" << anim.name
                  << "' duration=" << anim.duration
                  << " channels=" << anim.channels.size() << "\n";
    }
}

// ============================================================
//                  BOUNDING RADIUS
// ============================================================
float Model::GetBoundingRadius() const
{
    glm::vec3 minV( FLT_MAX);
    glm::vec3 maxV(-FLT_MAX);
    for (const auto& mesh : meshes) {
        for (const auto& v : mesh.vertices) {
            minV = glm::min(minV, v.position);
            maxV = glm::max(maxV, v.position);
        }
    }
    glm::vec3 center = (minV + maxV) * 0.5f;
    return glm::length(maxV - center);
}

// ============================================================
//                    DRAW DEPTH
// ============================================================
void Model::DrawDepth(Shader& shader, const glm::mat4& worldMatrix) {
    for (size_t i = 0; i < meshes.size(); ++i) {
        glm::mat4 finalMat = worldMatrix;
        if (i < matricesMeshes.size())
            finalMat = worldMatrix * matricesMeshes[i];
        meshes[i].DrawDepth(shader, finalMat);   // ← mesh sets model itself
    }
}

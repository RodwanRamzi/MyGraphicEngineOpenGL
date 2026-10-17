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
//                     DEBUG MACRO
// ============================================================
#define MODEL_DEBUG  // ← comment this out for silent mode

#ifdef MODEL_DEBUG
    #define MLOG(x) do { std::cerr << x << std::endl; } while(0)
#else
    #define MLOG(x) do {} while(0)
#endif

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
        }
        else
        {
            JSON = json::parse(text);
            MLOG("=== STAGE 3b: JSON parsed ===");
            data = getDataFromURI();
            MLOG("=== STAGE 3c: URI buffer size=" << data.size() << " ===");
        }

        MLOG("=== STAGE 4: about to traverseNode(0) ===");
        traverseNode(0);
        MLOG("=== STAGE 5: traverseNode DONE, meshes=" << meshes.size() << " ===");
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

        MLOG("    [LM] meshes array size=" << JSON["meshes"].size());

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

        MLOG("    [LM] primitives count=" << meshJson["primitives"].size());

        if (meshJson["primitives"].empty()) {
            std::cerr << "    [LM] primitives array is EMPTY!" << std::endl;
            return;
        }

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

            if (!attrs.contains("POSITION")) {
                std::cerr << "    [LM] prim " << pIdx << " no POSITION" << std::endl;
                continue;
            }
            if (!attrs.contains("NORMAL")) {
                std::cerr << "    [LM] prim " << pIdx << " no NORMAL" << std::endl;
                continue;
            }
            if (!attrs.contains("TEXCOORD_0")) {
                std::cerr << "    [LM] prim " << pIdx << " no TEXCOORD_0" << std::endl;
                continue;
            }

            unsigned int posAccInd    = attrs["POSITION"];
            unsigned int normalAccInd = attrs["NORMAL"];
            unsigned int texAccInd    = attrs["TEXCOORD_0"];

            MLOG("    [LM] prim " << pIdx << " reading POSITION accessor " << posAccInd);
            std::vector<glm::vec3> positions = groupFloatsVec3(getFloats(JSON["accessors"][posAccInd]));

            MLOG("    [LM] prim " << pIdx << " reading NORMAL accessor " << normalAccInd);
            std::vector<glm::vec3> normals = groupFloatsVec3(getFloats(JSON["accessors"][normalAccInd]));

            MLOG("    [LM] prim " << pIdx << " reading TEXCOORD_0 accessor " << texAccInd);
            std::vector<glm::vec2> texUVs = groupFloatsVec2(getFloats(JSON["accessors"][texAccInd]));

            std::vector<glm::vec3> tangents, bitangents;

            if (attrs.contains("TANGENT"))
            {
                unsigned int tanAccInd = attrs["TANGENT"];
                std::vector<float> tanVec = getFloats(JSON["accessors"][tanAccInd]);
                std::vector<glm::vec4> tanVec4 = groupFloatsVec4(tanVec);
                tangents.resize(tanVec4.size());
                for (size_t i = 0; i < tanVec4.size(); i++)
                    tangents[i] = glm::vec3(tanVec4[i].x, tanVec4[i].y, tanVec4[i].z);

                bitangents.resize(tangents.size());
                for (size_t i = 0; i < tangents.size(); i++) {
                    if (i < normals.size())
                        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]));
                    else
                        bitangents[i] = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }
            else
            {
                tangents.assign(positions.size(),   glm::vec3(1.0f, 0.0f, 0.0f));
                bitangents.assign(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            std::vector<Vertex> vertices = assembleVertices(positions, normals, texUVs, tangents, bitangents);
            if (vertices.empty()) {
                MLOG("    [LM] prim " << pIdx << ": no vertices, skipping");
                continue;
            }

            std::vector<GLuint> indices;
            if (primitive.contains("indices")) {
                json& indicesRef = primitive["indices"];
                unsigned int indAccInd = 0;
                if (indicesRef.is_number_integer()) {
                    indAccInd = indicesRef.get<unsigned int>();
                } else if (indicesRef.is_string()) {
                    indAccInd = std::stoul(indicesRef.get<std::string>());
                }
                //unsigned int indAccInd = primitive["indices"].get<unsigned int>();
                indices = getIndices(JSON["accessors"][indAccInd]);
            } else {
                indices.resize(vertices.size());
                for (size_t i = 0; i < vertices.size(); ++i) indices[i] = (GLuint)i;
            }

            MLOG("    [LM] prim " << pIdx << " calling getTexturesForMaterial");
            int matIndex = primitive.value("material", -1);
            std::vector<Texture> textures = getTexturesForMaterial(matIndex);

            MLOG("    [LM] prim " << pIdx << " pushing Mesh: "
                 << vertices.size() << " verts, "
                 << indices.size() << " idx, "
                 << textures.size() << " tex");

            meshes.push_back(Mesh(vertices, indices, textures));

            MLOG("    [LM] prim " << pIdx << " done");
        }

        MLOG("    [LM] exit mesh=" << indMesh);
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n!!!!! EXCEPTION in loadMesh !!!!!\n";
        std::cerr << "  mesh=" << indMesh << "\n";
        std::cerr << "  what(): " << e.what() << "\n";
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
        std::cin.get();
        std::exit(1);
    }
}

// ============================================================
//                     TRAVERSE NODE
// ============================================================
void Model::traverseNode(unsigned int nextNode, glm::mat4 matrix)
{
    MLOG("  [TN] enter node=" << nextNode);

    if (!JSON.contains("nodes") || nextNode >= JSON["nodes"].size()) {
        std::cerr << "  [TN] node " << nextNode << " out of range!" << std::endl;
        return;
    }

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

    if (node.find("mesh") != node.end())
    {
        MLOG("  [TN] node has mesh=" << node["mesh"]);

        size_t before = meshes.size();
        loadMesh(node["mesh"]);
        size_t added = meshes.size() - before;

        MLOG("  [TN] loadMesh done, added " << added << " meshes");

        for (size_t k = 0; k < added; ++k) {
            translationsMeshes.push_back(translation);
            rotationsMeshes.push_back(rotation);
            scalesMeshes.push_back(scale);
            matricesMeshes.push_back(matNextNode);
        }
    }

    if (node.find("children") != node.end())
    {
        MLOG("  [TN] node has " << node["children"].size() << " children");
        for (unsigned int i = 0; i < node["children"].size(); i++)
            traverseNode(node["children"][i], matNextNode);
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
    if (!img.contains("uri")) return;

    std::string texPath = img["uri"];
    if (texPath.find("data:") == 0) return;

    auto& texCache  = GetLoadedTex();
    auto& nameCache = GetLoadedTexName();

    for (size_t j = 0; j < nameCache.size(); ++j) {
        if (nameCache[j] == texPath) {
            out.push_back(texCache[j]);
            return;
        }
    }

    std::string fullPath = baseDirectory + texPath;
    if (!std::filesystem::exists(fullPath)) fullPath = texPath;
    if (!std::filesystem::exists(fullPath)) {
        MLOG("[Model] Texture missing: " << texPath);
        return;
    }

    Texture t(fullPath.c_str(), typeName.c_str(), (int)texCache.size());
    t.path = fullPath;
    out.push_back(t);
    texCache.push_back(t);
    nameCache.push_back(texPath);
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
std::vector<Vertex> Model::assembleVertices(std::vector<glm::vec3> positions,
                                            std::vector<glm::vec3> normals,
                                            std::vector<glm::vec2> texUVs,
                                            std::vector<glm::vec3> tangents,
                                            std::vector<glm::vec3> bitangents)
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
        vertices.push_back(Vertex{
            positions[i],
            normals[i],
            glm::vec3(1.0f, 1.0f, 1.0f),
            texUVs[i],
            tangents[i],
            bitangents[i]
        });
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
void Model::DrawDepth(Shader& shader, const glm::mat4& modelMatrix)
{
    for (auto& mesh : meshes)
        mesh.DrawDepth(shader, modelMatrix);
}
#include "Model.h"
#include "Camera.h"

// Required for glm::make_vec3, make_quat, make_mat4, value_ptr
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ... then your existing includes below:
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <sstream>


// ============================================================
//                      HELPER FUNCTION
// ============================================================
static std::string get_file_contents(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ============================================================
//                     CONSTRUCTORS
// ============================================================
Model::Model(const char* filePath)
{
    this->file = filePath;
    std::string text = get_file_contents(filePath);
    JSON = json::parse(text);
    data = getData();
    traverseNode(0);
}

// ============================================================
//                         DRAW
// ============================================================
void Model::Draw(Shader& shader,
    Camera& camera,
    glm::mat4 globalTransform,
    bool      bindTextures)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        if (i < matricesMeshes.size())
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
}

// ============================================================
//                       LOAD MESH
// ============================================================
void Model::loadMesh(unsigned int indMesh)
{
    if (!JSON["meshes"][indMesh]["primitives"][0]["attributes"].contains("POSITION"))
    {
        std::cerr << "ERROR: Mesh " << indMesh << " missing POSITION attribute!" << std::endl;
        return;
    }
    if (!JSON["meshes"][indMesh]["primitives"][0]["attributes"].contains("NORMAL"))
    {
        std::cerr << "ERROR: Mesh " << indMesh << " missing NORMAL attribute!" << std::endl;
        return;
    }
    if (!JSON["meshes"][indMesh]["primitives"][0]["attributes"].contains("TEXCOORD_0"))
    {
        std::cerr << "ERROR: Mesh " << indMesh << " missing TEXCOORD_0 attribute!" << std::endl;
        return;
    }

    unsigned int posAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["POSITION"];
    unsigned int normalAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["NORMAL"];
    unsigned int texAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TEXCOORD_0"];
    unsigned int indAccInd = JSON["meshes"][indMesh]["primitives"][0]["indices"];

    std::vector<float> posVec = getFloats(JSON["accessors"][posAccInd]);
    std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
    std::vector<float> normalVec = getFloats(JSON["accessors"][normalAccInd]);
    std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
    std::vector<float> texVec = getFloats(JSON["accessors"][texAccInd]);
    std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

    if (JSON["meshes"][indMesh]["primitives"][0]["attributes"].contains("TANGENT"))
    {
        unsigned int tanAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TANGENT"];
        std::vector<float> tanVec = getFloats(JSON["accessors"][tanAccInd]);
        tangents = groupFloatsVec3(tanVec);
        bitangents.resize(tangents.size());
        for (size_t i = 0; i < tangents.size(); i++)
        {
            if (i < normals.size())
                bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]));
            else
                bitangents[i] = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
    else
    {
        tangents.resize(positions.size(), glm::vec3(1.0f, 0.0f, 0.0f));
        bitangents.resize(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    std::vector<Vertex> vertices = assembleVertices(positions, normals, texUVs, tangents, bitangents);
    std::vector<GLuint> indices = getIndices(JSON["accessors"][indAccInd]);
    std::vector<Texture> textures = getTextures();
    std::cout << "[Model] Creating Mesh with " << vertices.size() << " vertices, "
        << indices.size() << " indices, " << textures.size() << " textures" << std::endl;

    meshes.push_back(Mesh(vertices, indices, textures));
}

// ============================================================
//                      TRAVERSE NODE
// ============================================================
void Model::traverseNode(unsigned int nextNode, glm::mat4 matrix)
{
    json node = JSON["nodes"][nextNode];

    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
    if (node.find("translation") != node.end())
    {
        float transValues[3];
        for (unsigned int i = 0; i < node["translation"].size(); i++)
            transValues[i] = (node["translation"][i]);
        translation = glm::make_vec3(transValues);
    }

    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (node.find("rotation") != node.end())
    {
        float rotValues[4] =
        {
            node["rotation"][3],
            node["rotation"][0],
            node["rotation"][1],
            node["rotation"][2]
        };
        rotation = glm::make_quat(rotValues);
    }

    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    if (node.find("scale") != node.end())
    {
        float scaleValues[3];
        for (unsigned int i = 0; i < node["scale"].size(); i++)
            scaleValues[i] = (node["scale"][i]);
        scale = glm::make_vec3(scaleValues);
    }

    glm::mat4 matNode = glm::mat4(1.0f);
    if (node.find("matrix") != node.end())
    {
        float matValues[16];
        for (unsigned int i = 0; i < node["matrix"].size(); i++)
            matValues[i] = (node["matrix"][i]);
        matNode = glm::make_mat4(matValues);
    }

    glm::mat4 trans = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

    glm::mat4 matNextNode = matrix * matNode * trans * rot * sca;

    if (node.find("mesh") != node.end())
    {
        translationsMeshes.push_back(translation);
        rotationsMeshes.push_back(rotation);
        scalesMeshes.push_back(scale);
        matricesMeshes.push_back(matNextNode);
        loadMesh(node["mesh"]);
    }

    if (node.find("children") != node.end())
    {
        for (unsigned int i = 0; i < node["children"].size(); i++)
            traverseNode(node["children"][i], matNextNode);
    }
}

// ============================================================
//                         GET DATA
// ============================================================
std::vector<unsigned char> Model::getData()
{
    std::string bytesText;
    std::string uri = JSON["buffers"][0]["uri"];

    std::string fileStr = std::string(file);
    std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);
    std::string fullBufferPath = fileDirectory + uri;

    std::cout << "[Model] Loading buffer from: " << fullBufferPath << std::endl;

    bytesText = get_file_contents(fullBufferPath.c_str());

    if (bytesText.empty()) {
        std::cerr << "[Model] ERROR: Buffer file empty or not found: " << fullBufferPath << std::endl;
    }

    std::vector<unsigned char> data(bytesText.begin(), bytesText.end());
    std::cout << "[Model] Buffer size: " << data.size() << " bytes" << std::endl;
    return data;
}

// ============================================================
//                        GET FLOATS
// ============================================================
std::vector<float> Model::getFloats(json accessor)
{
    std::vector<float> floatVec;

    if (data.empty()) {
        std::cerr << "[Model] getFloats: data vector is empty!" << std::endl;
        return floatVec;
    }

    unsigned int buffViewInd = accessor.value("bufferView", 1);
    unsigned int count = accessor["count"];
    unsigned int accByteOffset = accessor.value("byteOffset", 0);
    std::string type = accessor["type"];

    // Validate bufferView index
    if (!JSON.contains("bufferViews") || buffViewInd >= JSON["bufferViews"].size()) {
        std::cerr << "[Model] getFloats: Invalid bufferView index " << buffViewInd << std::endl;
        return floatVec;
    }

    json bufferView = JSON["bufferViews"][buffViewInd];
    if (!bufferView.contains("byteOffset")) {
        std::cerr << "[Model] getFloats: bufferView missing byteOffset" << std::endl;
        return floatVec;
    }
    unsigned int byteOffset = bufferView["byteOffset"];

    unsigned int numPerVert;
    if (type == "SCALAR") numPerVert = 1;
    else if (type == "VEC2") numPerVert = 2;
    else if (type == "VEC3") numPerVert = 3;
    else if (type == "VEC4") numPerVert = 4;
    else throw std::invalid_argument("Type is invalid");

    unsigned int beginningOfData = byteOffset + accByteOffset;
    unsigned int lengthOfData = count * 4 * numPerVert;

    std::cout << "[Model] getFloats: count=" << count << ", type=" << type
        << ", numPerVert=" << numPerVert << ", byteOffset=" << byteOffset
        << ", accByteOffset=" << accByteOffset
        << ", beginning=" << beginningOfData << ", length=" << lengthOfData
        << ", buffer size=" << data.size() << std::endl;

    if (beginningOfData + lengthOfData > data.size())
    {
        std::cerr << "[Model] getFloats: OUT OF BOUNDS! Requested bytes "
            << beginningOfData << " to " << beginningOfData + lengthOfData
            << " but buffer size is " << data.size() << std::endl;
        return floatVec;
    }

    for (unsigned int i = beginningOfData; i < beginningOfData + lengthOfData; i += 4)
    {
        unsigned char bytes[] = { data[i], data[i + 1], data[i + 2], data[i + 3] };
        float value;
        std::memcpy(&value, bytes, sizeof(float));
        floatVec.push_back(value);
    }

    return floatVec;
}

// ============================================================
//                       GET INDICES
// ============================================================
std::vector<GLuint> Model::getIndices(json accessor)
{
    std::vector<GLuint> indices;

    if (data.empty()) {
        std::cerr << "[Model] getIndices: data vector is empty!" << std::endl;
        return indices;
    }

    unsigned int buffViewInd = accessor.value("bufferView", 0);
    unsigned int count = accessor["count"];
    unsigned int accByteOffset = accessor.value("byteOffset", 0);
    unsigned int componentType = accessor["componentType"];

    if (!JSON.contains("bufferViews") || buffViewInd >= JSON["bufferViews"].size()) {
        std::cerr << "[Model] getIndices: Invalid bufferView index " << buffViewInd << std::endl;
        return indices;
    }

    json bufferView = JSON["bufferViews"][buffViewInd];
    if (!bufferView.contains("byteOffset")) {
        std::cerr << "[Model] getIndices: bufferView missing byteOffset" << std::endl;
        return indices;
    }
    unsigned int byteOffset = bufferView["byteOffset"];

    unsigned int beginningOfData = byteOffset + accByteOffset;
    unsigned int bytesPerIndex = (componentType == 5125) ? 4 : 2;
    unsigned int totalBytes = count * bytesPerIndex;

    std::cout << "[Model] getIndices: count=" << count << ", componentType=" << componentType
        << ", bytesPerIndex=" << bytesPerIndex << ", byteOffset=" << byteOffset
        << ", accByteOffset=" << accByteOffset
        << ", beginning=" << beginningOfData << ", totalBytes=" << totalBytes
        << ", buffer size=" << data.size() << std::endl;

    if (beginningOfData + totalBytes > data.size())
    {
        std::cerr << "[Model] getIndices: OUT OF BOUNDS! Requested bytes "
            << beginningOfData << " to " << beginningOfData + totalBytes
            << " but buffer size is " << data.size() << std::endl;
        return indices;
    }

    if (componentType == 5125) // unsigned int
    {
        for (unsigned int i = beginningOfData; i < beginningOfData + count * 4; i += 4)
        {
            unsigned char bytes[] = { data[i], data[i + 1], data[i + 2], data[i + 3] };
            unsigned int value;
            std::memcpy(&value, bytes, sizeof(unsigned int));
            indices.push_back((GLuint)value);
        }
    }
    else if (componentType == 5123) // unsigned short
    {
        for (unsigned int i = beginningOfData; i < beginningOfData + count * 2; i += 2)
        {
            unsigned char bytes[] = { data[i], data[i + 1] };
            unsigned short value;
            std::memcpy(&value, bytes, sizeof(unsigned short));
            indices.push_back((GLuint)value);
        }
    }
    else if (componentType == 5122) // short
    {
        for (unsigned int i = beginningOfData; i < beginningOfData + count * 2; i += 2)
        {
            unsigned char bytes[] = { data[i], data[i + 1] };
            short value;
            std::memcpy(&value, bytes, sizeof(short));
            indices.push_back((GLuint)value);
        }
    }
    else
    {
        std::cerr << "ERROR: Unknown component type for indices: " << componentType << std::endl;
    }

    return indices;
}

// ============================================================
//                    GET TEXTURES (FIXED)
// ============================================================
std::vector<Texture> Model::getTextures()
{
    std::vector<Texture> textures;

    std::string baseDirectory;
    if (!mDirectory.empty()) {
        baseDirectory = mDirectory;
    }
    else if (!file.empty()) {
        std::string fileStr = std::string(file);
        size_t lastSlash = fileStr.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            baseDirectory = fileStr.substr(0, lastSlash + 1);
        }
        else {
            baseDirectory = "";
        }
    }
    else {
        baseDirectory = "";
    }

    if (!JSON.contains("images") || JSON["images"].size() == 0)
    {
        std::cout << "[Model] No images found in JSON. Creating white texture." << std::endl;
        unsigned int whiteTexID;
        glGenTextures(1, &whiteTexID);
        glBindTexture(GL_TEXTURE_2D, whiteTexID);
        unsigned char whitePixel[] = { 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        Texture whiteTex(whiteTexID, "diffuse", (int)loadedTex.size());
        textures.push_back(whiteTex);
        loadedTex.push_back(whiteTex);
        loadedTexName.push_back("default_white_mem");
        return textures;
    }

    for (unsigned int i = 0; i < JSON["images"].size(); i++)
    {
        if (!JSON["images"][i].contains("uri"))
        {
            std::cout << "[Model] Image " << i << " has no URI. Skipping." << std::endl;
            continue;
        }

        std::string texPath = JSON["images"][i]["uri"];

        if (texPath.find("data:") == 0) {
            std::cout << "[Model] Skipping embedded image (data URI)." << std::endl;
            continue;
        }

        bool skip = false;
        for (unsigned int j = 0; j < loadedTexName.size(); j++)
        {
            if (loadedTexName[j] == texPath)
            {
                textures.push_back(loadedTex[j]);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            std::string fullPath = baseDirectory + texPath;

            if (!std::filesystem::exists(fullPath)) {
                fullPath = texPath;
            }

            std::cout << "[Model] Attempting to load texture: " << fullPath << std::endl;

            if (!std::filesystem::exists(fullPath)) {
                std::cout << "[Model] Texture not found: " << fullPath << ". Using default white." << std::endl;
                Texture defaultTex("default_white", "diffuse", (int)loadedTex.size());
                textures.push_back(defaultTex);
                loadedTex.push_back(defaultTex);
                loadedTexName.push_back("default_white");
                continue;
            }

            std::string type = "diffuse";
            std::string lowerPath = texPath;
            std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
            if (lowerPath.find("metallicroughness") != std::string::npos ||
                lowerPath.find("metallic_roughness") != std::string::npos) {
                type = "specular";
            }
            else if (lowerPath.find("normal") != std::string::npos) {
                type = "normal";
            }

            // ✅ FIX: use type.c_str() to convert std::string to const char*
            Texture tex(fullPath.c_str(), type.c_str(), (int)loadedTex.size());
            std::cout << "[Model] Texture loaded: " << fullPath << " (type=" << type << ", ID=" << tex.ID << ")" << std::endl;
            tex.path = fullPath;
            textures.push_back(tex);
            loadedTex.push_back(tex);
            loadedTexName.push_back(texPath);
        }
    }

    if (textures.empty()) {
        std::cout << "[Model] No textures loaded. Adding white fallback." << std::endl;
        unsigned int whiteTexID;
        glGenTextures(1, &whiteTexID);
        glBindTexture(GL_TEXTURE_2D, whiteTexID);
        unsigned char whitePixel[] = { 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        Texture whiteTex(whiteTexID, "diffuse", (int)loadedTex.size());
        textures.push_back(whiteTex);
        loadedTex.push_back(whiteTex);
        loadedTexName.push_back("fallback_white");
    }

    std::cout << "[Model] getTextures() completed. Total textures: " << textures.size() << std::endl;
    return textures;
}

// ============================================================
//                 VERTEX ASSEMBLY HELPERS
// ============================================================
std::vector<Vertex> Model::assembleVertices
(
    std::vector<glm::vec3> positions,
    std::vector<glm::vec3> normals,
    std::vector<glm::vec2> texUVs,
    std::vector<glm::vec3> tangents,
    std::vector<glm::vec3> bitangents
)
{
    std::vector<Vertex> vertices;

    // ✅ SAFETY: Print sizes for debugging
    std::cout << "[Model] assembleVertices: positions=" << positions.size()
        << ", normals=" << normals.size()
        << ", texUVs=" << texUVs.size()
        << ", tangents=" << tangents.size()
        << ", bitangents=" << bitangents.size() << std::endl;

    // ✅ SAFETY: Ensure all vectors have the same size
    unsigned int numVertices = positions.size();
    numVertices = std::min(numVertices, (unsigned int)normals.size());
    numVertices = std::min(numVertices, (unsigned int)texUVs.size());
    numVertices = std::min(numVertices, (unsigned int)tangents.size());
    numVertices = std::min(numVertices, (unsigned int)bitangents.size());

    std::cout << "[Model] assembleVertices: numVertices=" << numVertices << std::endl;

    if (numVertices == 0) {
        std::cerr << "[Model] ERROR: No vertices to assemble!" << std::endl;
        return vertices;
    }

    vertices.reserve(numVertices);
    for (unsigned int i = 0; i < numVertices; i++)
    {
        vertices.push_back(
            Vertex{
                positions[i],
                normals[i],
                glm::vec3(1.0f, 1.0f, 1.0f),
                texUVs[i],
                tangents[i],
                bitangents[i]
            }
        );
    }
    return vertices;
}

std::vector<glm::vec2> Model::groupFloatsVec2(std::vector<float> floatVec)
{
    const unsigned int floatsPerVector = 2;
    std::vector<glm::vec2> vectors;
    for (unsigned int i = 0; i + floatsPerVector <= floatVec.size(); i += floatsPerVector)
    {
        vectors.push_back(glm::vec2(0, 0));
        for (unsigned int j = 0; j < floatsPerVector; j++)
            vectors.back()[j] = floatVec[i + j];
    }
    return vectors;
}

std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> floatVec)
{
    const unsigned int floatsPerVector = 3;
    std::vector<glm::vec3> vectors;
    for (unsigned int i = 0; i + floatsPerVector <= floatVec.size(); i += floatsPerVector)
    {
        vectors.push_back(glm::vec3(0, 0, 0));
        for (unsigned int j = 0; j < floatsPerVector; j++)
            vectors.back()[j] = floatVec[i + j];
    }
    return vectors;
}

std::vector<glm::vec4> Model::groupFloatsVec4(std::vector<float> floatVec)
{
    const unsigned int floatsPerVector = 4;
    std::vector<glm::vec4> vectors;
    for (unsigned int i = 0; i + floatsPerVector <= floatVec.size(); i += floatsPerVector)
    {
        vectors.push_back(glm::vec4(0, 0, 0, 0));
        for (unsigned int j = 0; j < floatsPerVector; j++)
            vectors.back()[j] = floatVec[i + j];
    }
    return vectors;
}

// ============================================================
//                     BOUNDING RADIUS
// ============================================================
float Model::GetBoundingRadius() const {
    glm::vec3 minV = glm::vec3(FLT_MAX);
    glm::vec3 maxV = glm::vec3(-FLT_MAX);
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
//                      DRAW DEPTH
// ============================================================
void Model::DrawDepth(Shader& shader, const glm::mat4& modelMatrix) {
    for (auto& mesh : meshes) {
        mesh.DrawDepth(shader, modelMatrix);
    }
}
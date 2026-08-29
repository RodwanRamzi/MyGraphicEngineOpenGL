#include "Model.h"
#include <algorithm>   // For std::min
#include <iostream>    // For error messages

Model::Model(const char* file)
{
    std::string text = get_file_contents(file);
    JSON = json::parse(text);

    Model::file = file;
    data = getData();

    traverseNode(0);
}

void Model::Draw(Shader& shader, Camera& camera, glm::mat4 globalTransform)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        // Only draw if we have a corresponding matrix
        if (i < matricesMeshes.size())
        {
            // Combine the global transform with the node's local matrix
            glm::mat4 finalMatrix = globalTransform * matricesMeshes[i];
            meshes[i].Mesh::Draw(shader, camera, finalMatrix);
        }
    }
}

void Model::loadMesh(unsigned int indMesh)
{
    // Check if required attributes exist
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

    // Load tangents and bitangents (if available)
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

    if (JSON["meshes"][indMesh]["primitives"][0]["attributes"].contains("TANGENT"))
    {
        unsigned int tanAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TANGENT"];
        std::vector<float> tanVec = getFloats(JSON["accessors"][tanAccInd]);
        tangents = groupFloatsVec3(tanVec);

        // Compute bitangents from normals and tangents
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
        // Default tangents/bitangents (not correct but safe)
        tangents.resize(positions.size(), glm::vec3(1.0f, 0.0f, 0.0f));
        bitangents.resize(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    std::vector<Vertex> vertices = assembleVertices(positions, normals, texUVs, tangents, bitangents);
    std::vector<GLuint> indices = getIndices(JSON["accessors"][indAccInd]);
    std::vector<Texture> textures = getTextures();

    meshes.push_back(Mesh(vertices, indices, textures));
}

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

    glm::mat4 trans = glm::mat4(1.0f);
    glm::mat4 rot = glm::mat4(1.0f);
    glm::mat4 sca = glm::mat4(1.0f);

    trans = glm::translate(trans, translation);
    rot = glm::mat4_cast(rotation);
    sca = glm::scale(sca, scale);

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

std::vector<unsigned char> Model::getData()
{
    std::string bytesText;
    std::string uri = JSON["buffers"][0]["uri"];

    std::string fileStr = std::string(file);
    std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);
    bytesText = get_file_contents((fileDirectory + uri).c_str());

    std::vector<unsigned char> data(bytesText.begin(), bytesText.end());

    // Debug: print size
    // std::cout << "Buffer size: " << data.size() << " bytes" << std::endl;
    return data;
}

std::vector<float> Model::getFloats(json accessor)
{
    std::vector<float> floatVec;

    unsigned int buffViewInd = accessor.value("bufferView", 1);
    unsigned int count = accessor["count"];
    unsigned int accByteOffset = accessor.value("byteOffset", 0);
    std::string type = accessor["type"];

    json bufferView = JSON["bufferViews"][buffViewInd];
    unsigned int byteOffset = bufferView["byteOffset"];

    unsigned int numPerVert;
    if (type == "SCALAR") numPerVert = 1;
    else if (type == "VEC2") numPerVert = 2;
    else if (type == "VEC3") numPerVert = 3;
    else if (type == "VEC4") numPerVert = 4;
    else throw std::invalid_argument("Type is invalid");

    unsigned int beginningOfData = byteOffset + accByteOffset;
    unsigned int lengthOfData = count * 4 * numPerVert;

    // 🛡️ SAFETY CHECK: Ensure the range is within the data vector
    if (beginningOfData + lengthOfData > data.size())
    {
        std::cerr << "ERROR: Accessor out of bounds! Requested bytes " 
                  << beginningOfData << " to " << beginningOfData + lengthOfData 
                  << " but buffer size is " << data.size() << std::endl;
        return floatVec; // Return empty to avoid crash
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

std::vector<GLuint> Model::getIndices(json accessor)
{
    std::vector<GLuint> indices;

    unsigned int buffViewInd = accessor.value("bufferView", 0);
    unsigned int count = accessor["count"];
    unsigned int accByteOffset = accessor.value("byteOffset", 0);
    unsigned int componentType = accessor["componentType"];

    json bufferView = JSON["bufferViews"][buffViewInd];
    unsigned int byteOffset = bufferView["byteOffset"];

    unsigned int beginningOfData = byteOffset + accByteOffset;
    
    // Calculate total bytes needed based on component type
    unsigned int bytesPerIndex = (componentType == 5125) ? 4 : 2;
    unsigned int totalBytes = count * bytesPerIndex;

    // 🛡️ SAFETY CHECK
    if (beginningOfData + totalBytes > data.size())
    {
        std::cerr << "ERROR: Indices out of bounds! Requested bytes " 
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

std::vector<Texture> Model::getTextures()
{
    std::vector<Texture> textures;

    std::string fileStr = std::string(file);
    std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);

    for (unsigned int i = 0; i < JSON["images"].size(); i++)
    {
        std::string texPath = JSON["images"][i]["uri"];

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
            if (texPath.find("baseColor") != std::string::npos)
            {
                Texture diffuse = Texture((fileDirectory + texPath).c_str(), "diffuse", loadedTex.size());
                textures.push_back(diffuse);
                loadedTex.push_back(diffuse);
                loadedTexName.push_back(texPath);
            }
            else if (texPath.find("metallicRoughness") != std::string::npos)
            {
                Texture specular = Texture((fileDirectory + texPath).c_str(), "specular", loadedTex.size());
                textures.push_back(specular);
                loadedTex.push_back(specular);
                loadedTexName.push_back(texPath);
            }
        }
    }

    return textures;
}

// ✅ UPDATED assembleVertices WITH Tangent and Bitangent
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
    // Use the minimum size among all vectors to avoid out-of-bounds
    unsigned int numVertices = std::min({ positions.size(), normals.size(), texUVs.size(), tangents.size(), bitangents.size() });

    for (unsigned int i = 0; i < numVertices; i++)
    {
        vertices.push_back
        (
            Vertex
            {
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
    for (unsigned int i = 0; i + floatsPerVector <= floatVec.size(); i += floatsPerVector) // fixed condition
    {
        vectors.push_back(glm::vec2(0, 0));

        for (unsigned int j = 0; j < floatsPerVector; j++)
        {
            vectors.back()[j] = floatVec[i + j];
        }
    }
    return vectors;
}

std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> floatVec)
{
    const unsigned int floatsPerVector = 3;

    std::vector<glm::vec3> vectors;
    for (unsigned int i = 0; i + floatsPerVector <= floatVec.size(); i += floatsPerVector) // fixed condition
    {
        vectors.push_back(glm::vec3(0, 0, 0));

        for (unsigned int j = 0; j < floatsPerVector; j++)
        {
            vectors.back()[j] = floatVec[i + j];
        }
    }
    return vectors;
}

std::vector<glm::vec4> Model::groupFloatsVec4(std::vector<float> floatVec)
{
    const unsigned int floatsPerVector = 4;

    std::vector<glm::vec4> vectors;
    for (unsigned int i = 0; i + floatsPerVector <= floatVec.size(); i += floatsPerVector) // fixed condition
    {
        vectors.push_back(glm::vec4(0, 0, 0, 0));

        for (unsigned int j = 0; j < floatsPerVector; j++)
        {
            vectors.back()[j] = floatVec[i + j];
        }
    }
    return vectors;
}

float Model::GetBoundingRadius() const {
    // Compute min/max of all vertices in all meshes
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

void Model::DrawDepth(Shader& shader, const glm::mat4& modelMatrix) {
    for (auto& mesh : meshes) {
        mesh.DrawDepth(shader, modelMatrix);
    }
}
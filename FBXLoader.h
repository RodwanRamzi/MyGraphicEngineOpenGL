#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include "Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declaration – you must define this function elsewhere
// (e.g., in a TextureManager class or a global helper)
GLuint LoadTextureFromFile(const std::string& path);

class FBXLoader {
public:
    static Model* Load(const std::string& filepath) {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(filepath,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace |
            aiProcess_GenNormals |
            aiProcess_OptimizeMeshes |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType |
            aiProcess_FindInvalidData |
            aiProcess_ValidateDataStructure
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "[FBX] Failed to load: " << importer.GetErrorString() << std::endl;
            return nullptr;
        }

        std::cout << "[FBX] Loaded: " << filepath << std::endl;
        std::cout << "[FBX] Meshes: " << scene->mNumMeshes << std::endl;
        std::cout << "[FBX] Materials: " << scene->mNumMaterials << std::endl;

        // ⚠️ Your Model class must have a public default constructor.
        // If it doesn't, add: Model() = default; inside Model.h
        Model* model = new Model();
        ProcessNode(scene->mRootNode, scene, model);
        return model;
    }

private:
    static void ProcessNode(aiNode* node, const aiScene* scene, Model* model) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene, model);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, model);
        }
    }

    static void ProcessMesh(aiMesh* mesh, const aiScene* scene, Model* model) {
        std::vector<Vertex> vertices;
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            vertex.position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            if (mesh->HasNormals()) {
                vertex.normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            }

            if (mesh->HasTextureCoords(0)) {
                vertex.texUV = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            }

            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = glm::vec3(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                );
                vertex.bitangent = glm::vec3(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z
                );
            }

            vertices.push_back(vertex);
        }

        std::vector<unsigned int> indices;
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        std::vector<Texture> textures;
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            textures = LoadMaterialTextures(material, scene);
        }

        Mesh newMesh(vertices, indices, textures);
        // Inside ProcessMesh, after creating Mesh newMesh(...)
        std::cout << "[FBX] ProcessMesh DEBUG: vertices=" << vertices.size()
            << ", indices=" << indices.size()
            << ", textures=" << textures.size() << std::endl;

        if (textures.empty()) {
            std::cout << "[FBX] WARNING: This mesh has NO textures!" << std::endl;
        }
        else {
            for (size_t t = 0; t < textures.size(); ++t) {
                std::cout << "[FBX]   Texture " << t << ": type=" << textures[t].type
                    << ", ID=" << textures[t].ID
                    << ", path=" << textures[t].path << std::endl;
            }
        }

        model->meshes.push_back(newMesh);
    }

    static std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, const aiScene* scene) {
        std::vector<Texture> textures;

        aiTextureType types[] = {
            aiTextureType_DIFFUSE,
            aiTextureType_NORMALS,
            aiTextureType_METALNESS,
            aiTextureType_SPECULAR,
            aiTextureType_EMISSIVE,
            aiTextureType_AMBIENT_OCCLUSION
        };

        std::string typeNames[] = {
            "diffuse",
            "normal",
            "metallic",
            "specular",
            "emissive",
            "ao"
        };

        for (int i = 0; i < 6; i++) {
            Texture texture;
            texture.type = typeNames[i];

            for (unsigned int j = 0; j < mat->GetTextureCount(types[i]); j++) {
                aiString path;
                if (mat->GetTexture(types[i], j, &path) == AI_SUCCESS) {
                    std::string fullPath = path.C_Str();
                    texture.path = fullPath;
                    texture.ID = LoadTextureFromFile(texture.path);   // Now defined via forward declaration
                    textures.push_back(texture);
                    std::cout << "[FBX] Loaded texture: " << texture.path << std::endl;
                }
            }
        }

        return textures;
    }
};
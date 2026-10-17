#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include "Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// Must be declared somewhere (you already have it in main.cpp)
GLuint LoadTextureFromFile(const std::string& path);

/*
class FBXLoader {
public:
    static Model* Load(const std::string& filepath) {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(filepath,
            aiProcess_Triangulate |
            aiProcess_FlipWindingOrder |
            aiProcess_CalcTangentSpace |
            aiProcess_GenNormals |
            aiProcess_GenSmoothNormals |   // ← ولّد normals لو مش موجودة
            aiProcess_OptimizeMeshes |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_FindInvalidData |
            aiProcess_ValidateDataStructure
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cerr << "[FBX] Failed to load: " << filepath << "\n"
                      << "      Reason: " << importer.GetErrorString() << std::endl;
            return nullptr;
        }

        // Directory of the FBX file — used to resolve texture paths
        std::string fbxDir = fs::path(filepath).parent_path().string();
        std::replace(fbxDir.begin(), fbxDir.end(), '\\', '/');

        std::cout << "[FBX] Loaded: " << filepath << "\n"
                  << "[FBX]   Meshes:    " << scene->mNumMeshes << "\n"
                  << "[FBX]   Materials: " << scene->mNumMaterials << "\n"
                  << "[FBX]   Directory: " << fbxDir << std::endl;

        Model* model = new Model();
        ProcessNode(scene->mRootNode, scene, model, fbxDir);
        return model;
    }

private:
    static void ProcessNode(aiNode* node, const aiScene* scene, Model* model,
                            const std::string& fbxDir) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene, model, fbxDir);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, model, fbxDir);
        }
    }

    static void ProcessMesh(aiMesh* mesh, const aiScene* scene, Model* model,
                            const std::string& fbxDir) {
        std::vector<Vertex> vertices;
        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};   // ← zero-init (fixes garbage for missing attributes)

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

            // في ProcessMesh، بعد بناء vertices:
            bool hasValidNormals = false;
            for (const auto& v : vertices) {
                if (glm::length(v.normal) > 0.01f) { hasValidNormals = true; break; }
            }
            if (!hasValidNormals) {
                std::cout << "[FBX] ⚠️ Mesh has NO valid normals! Computing..." << std::endl;
                // حساب يدوي: متوسط normals الوجوه
                // أو اتركه للـ aiProcess_GenSmoothNormals
            }

            vertices.push_back(vertex);
        }

        std::vector<unsigned int> indices;
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        std::vector<Texture> textures;
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            textures = LoadMaterialTextures(mat, fbxDir);
        }

        std::cout << "[FBX]   Mesh: verts=" << vertices.size()
                  << " idx=" << indices.size()
                  << " tex=" << textures.size() << std::endl;

        // DEBUG: تحقق من البيانات
        std::cout << "[FBX-DEBUG] Mesh " << mesh->mNumVertices << " verts:" << std::endl;
        if (vertices.size() > 0) {
            const auto& v = vertices[0];
            std::cout << "  v[0].position  = (" << v.position.x << ", " << v.position.y << ", " << v.position.z << ")" << std::endl;
            std::cout << "  v[0].normal    = (" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z << ")" << std::endl;
            std::cout << "  v[0].texUV     = (" << v.texUV.x << ", " << v.texUV.y << ")" << std::endl;
            std::cout << "  v[0].tangent   = (" << v.tangent.x << ", " << v.tangent.y << ", " << v.tangent.z << ")" << std::endl;
            std::cout << "  v[0].bitangent = (" << v.bitangent.x << ", " << v.bitangent.y << ", " << v.bitangent.z << ")" << std::endl;
            std::cout << "  |normal| = " << glm::length(v.normal) << std::endl;
            std::cout << "  |tangent| = " << glm::length(v.tangent) << std::endl;
        }

        if (vertices.size() > 0) {
            glm::vec3 mn(FLT_MAX), mx(-FLT_MAX);
            for (const auto& v : vertices) {
                mn = glm::min(mn, v.position);
                mx = glm::max(mx, v.position);
            }
            std::cout << "[FBX-INFO] mesh bounds: min=(" << mn.x << "," << mn.y << "," << mn.z
                      << ") max=(" << mx.x << "," << mx.y << "," << mx.z << ")" << std::endl;
            std::cout << "[FBX-INFO] |v[0]|=" << glm::length(vertices[0].position) << std::endl;
        }

        std::cout << "\n===== FBX MESH DUMP =====" << std::endl;
        std::cout << "verts=" << vertices.size() 
                << " indices=" << indices.size() << std::endl;

        for (int k = 0; k < std::min(3, (int)vertices.size()); k++) {
            const auto& v = vertices[k];
            std::cout << "v[" << k << "]:" << std::endl
                    << "  pos       = (" << v.position.x << ", " << v.position.y << ", " << v.position.z << ")" << std::endl
                    << "  normal    = (" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z << ") |len|=" << glm::length(v.normal) << std::endl
                    << "  color     = (" << v.color.x << ", " << v.color.y << ", " << v.color.z << ")" << std::endl
                    << "  texUV     = (" << v.texUV.x << ", " << v.texUV.y << ")" << std::endl
                    << "  tangent   = (" << v.tangent.x << ", " << v.tangent.y << ", " << v.tangent.z << ") |len|=" << glm::length(v.tangent) << std::endl
                    << "  bitangent = (" << v.bitangent.x << ", " << v.bitangent.y << ", " << v.bitangent.z << ") |len|=" << glm::length(v.bitangent) << std::endl;
        }
        std::cout << "========================\n" << std::endl;

        // ✅ Force-compute tangents if missing
        for (auto& v : vertices) {
            if (glm::length(v.tangent) < 0.01f) {
                glm::vec3 n = glm::length(v.normal) > 0.01f ? glm::normalize(v.normal) : glm::vec3(0,1,0);
                glm::vec3 up = std::abs(n.y) < 0.99f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
                v.tangent   = glm::normalize(glm::cross(up, n));
                v.bitangent = glm::normalize(glm::cross(n, v.tangent));
            }
            if (glm::length(v.normal) < 0.01f) {
                v.normal = glm::vec3(0, 1, 0);   // احتياطي
            }
        }


        Mesh newMesh(vertices, indices, textures);
        model->meshes.push_back(newMesh);
    }

    // Attempt to resolve a relative FBX texture path (e.g. "textures\\wood.png")
    // to a path that works from the current working directory.
    static std::string ResolveTexturePath(const std::string& rawPath,
                                          const std::string& fbxDir) {
        if (rawPath.empty()) return "";

        std::string p = rawPath;
        std::replace(p.begin(), p.end(), '\\', '/');

        // Already absolute (has drive letter or starts with /)
        if (p.size() > 1 && (p[1] == ':' || p[0] == '/'))
            return p;

        // Try relative to the FBX file's folder first
        std::string candidate = fbxDir + "/" + p;
        if (fs::exists(candidate)) return candidate;

        // Fallback: relative to working directory
        if (fs::exists(p)) return p;

        // Nothing found — return the best guess so the log is informative
        return candidate;
    }

    static std::vector<Texture> LoadMaterialTextures(aiMaterial* mat,
                                                     const std::string& fbxDir) {
        std::vector<Texture> textures;

        struct Slot { aiTextureType type; const char* name; };
        static const Slot slots[] = {
            { aiTextureType_DIFFUSE,  "diffuse"  },
            { aiTextureType_NORMALS,  "normal"   },
            { aiTextureType_METALNESS,"metallic" },
            { aiTextureType_SPECULAR, "specular" },
            { aiTextureType_EMISSIVE, "emissive" },
            { aiTextureType_AMBIENT_OCCLUSION, "ao" },
        };

        for (const auto& slot : slots) {
            const unsigned int count = mat->GetTextureCount(slot.type);
            for (unsigned int j = 0; j < count; j++) {
                aiString aiPath;
                if (mat->GetTexture(slot.type, j, &aiPath) != AI_SUCCESS) continue;

                Texture texture;
                texture.type = slot.name;
                texture.path = ResolveTexturePath(aiPath.C_Str(), fbxDir);

                if (fs::exists(texture.path)) {
                    texture.ID = LoadTextureFromFile(texture.path);
                    std::cout << "[FBX]   Tex[" << slot.name << "] "
                              << texture.path << "  (ID=" << texture.ID << ")" << std::endl;
                } else {
                    std::cerr << "[FBX]   Tex[" << slot.name << "] NOT FOUND: "
                              << texture.path << std::endl;
                    texture.ID = 0;
                }

                textures.push_back(texture);
            }
        }

        return textures;
    }
};*/
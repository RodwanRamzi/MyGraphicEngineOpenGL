#define NOMINMAX

#include <filesystem>
namespace fs = std::filesystem;

#include "Model.h"
#include "FBXLoader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "shaderClass.h"
#include "Camera.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <float.h>
#include <cmath>
#include <unordered_map>
#include <thread>
#include <random>

#include <windows.h>
#include <commdlg.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include "ScriptComponent.h"

// JOLT PHYSICS
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/RegisterTypes.h>  
#include <Jolt/Core/Factory.h>   

using namespace JPH;
using namespace JPH::literals;

// ============================================================
//                   SCAN SCRIPTS FOLDER
// ============================================================
std::vector<std::string> scanScriptsFolder(const std::string& folderPath) {
    std::vector<std::string> scripts;
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cout << "[Scripts] Folder not found: " << folderPath << std::endl;
        return scripts;
    }
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".lua") {
                std::string fullPath = entry.path().string();
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                scripts.push_back(fullPath);
            }
        }
    }
    return scripts;
}

// ============================================================
//                     RENDER CUBE (for IBL generation)
// ============================================================
void renderCube() {
    static GLuint cubeVAO = 0, cubeVBO = 0;
    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
             // front face
             -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
              1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
              // left face
              -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
              -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,
              // right face
               1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
               1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
               // bottom face
               -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
                1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
                // top face
                -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderSkybox() {
    static GLuint skyboxVAO = 0, skyboxVBO = 0, skyboxEBO = 0;
    if (skyboxVAO == 0) {
        float skyboxVertices[] = {
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f
        };
        unsigned int skyboxIndices[] = {
            1, 2, 6, 6, 5, 1,  // Right
            0, 4, 7, 7, 3, 0,  // Left
            4, 5, 6, 6, 7, 4,  // Top
            0, 3, 2, 2, 1, 0,  // Bottom
            0, 1, 5, 5, 4, 0,  // Back
            3, 7, 6, 6, 2, 3   // Front
        };
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glGenBuffers(1, &skyboxEBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    glBindVertexArray(skyboxVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ============================================================
//             SKYBOX FILE FINDER 
// ============================================================
std::string FindSkyboxFace(const std::string& basePath, const std::vector<std::string>& possibleNames) {
    for (const auto& name : possibleNames) {
        std::string fullPath = basePath + "/" + name;
        if (fs::exists(fullPath)) {
            return fullPath;
        }
    }
    return "";  
}

// ============================================================
//             SKYBOX SCANNER & LOADER (NEW)
// ============================================================

int envCubemap = 0;

// ============================================================
//             SKYBOX SCANNER 
// ============================================================
void ScanSkyboxFolders(const std::string& basePath, std::vector<std::string>& outFolders) {
    outFolders.clear();

    if (!fs::exists(basePath) || !fs::is_directory(basePath)) {
        std::cout << "[Skybox] Base folder not found: " << basePath << std::endl;
        return;
    }

    // الكلمات المفتاحية لكل وجه (نفسها المستخدمة في التحميل)
    struct FaceKeywords {
        std::string faceName;
        std::vector<std::string> keywords;
    };

    std::vector<FaceKeywords> faceKeywords = {
        {"right", {"right", "posx", "px", "_x", "x_pos"}},
        {"left",  {"left", "negx", "nx", "_x_neg"}},
        {"top",   {"top", "posy", "py", "_y", "y_pos"}},
        {"bottom",{"bottom", "negy", "ny", "_y_neg"}},
        {"front", {"front", "posz", "pz", "_z", "z_pos"}},
        {"back",  {"back", "negz", "nz", "_z_neg"}}
    };

    // البحث في المجلدات الفرعية
    for (const auto& entry : fs::directory_iterator(basePath)) {
        if (!entry.is_directory()) continue;

        std::string folderName = entry.path().filename().string();
        std::string folderPath = entry.path().string();

        // جمع جميع ملفات الصور في المجلد
        std::vector<std::string> imageFiles;
        for (const auto& fileEntry : fs::directory_iterator(folderPath)) {
            if (fileEntry.is_regular_file()) {
                std::string ext = fileEntry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga") {
                    imageFiles.push_back(fileEntry.path().filename().string());
                }
            }
        }

        // التحقق من وجود كل وجه
        bool allFacesExist = true;
        for (int face = 0; face < 6; face++) {
            bool faceFound = false;
            for (const auto& file : imageFiles) {
                std::string lowerFile = file;
                std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
                for (const auto& keyword : faceKeywords[face].keywords) {
                    if (lowerFile.find(keyword) != std::string::npos) {
                        faceFound = true;
                        break;
                    }
                }
                if (faceFound) break;
            }
            if (!faceFound) {
                allFacesExist = false;
                break;
            }
        }

        if (allFacesExist && !imageFiles.empty()) {
            outFolders.push_back(folderName);
            std::cout << "[Skybox] Found valid skybox folder: " << folderName << std::endl;
        }
    }

    // التحقق من المجلد الأساسي
    std::vector<std::string> baseImages;
    for (const auto& entry : fs::directory_iterator(basePath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga") {
                baseImages.push_back(entry.path().filename().string());
            }
        }
    }

    bool baseExists = true;
    for (int face = 0; face < 6; face++) {
        bool faceFound = false;
        for (const auto& file : baseImages) {
            std::string lowerFile = file;
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
            for (const auto& keyword : faceKeywords[face].keywords) {
                if (lowerFile.find(keyword) != std::string::npos) {
                    faceFound = true;
                    break;
                }
            }
            if (faceFound) break;
        }
        if (!faceFound) {
            baseExists = false;
            break;
        }
    }

    if (baseExists && !baseImages.empty()) {
        outFolders.insert(outFolders.begin(), ".");
        std::cout << "[Skybox] Base folder contains valid skybox images." << std::endl;
    }

    if (outFolders.empty()) {
        std::cout << "[Skybox] No valid skybox folders found in: " << basePath << std::endl;
    }
    else {
        std::cout << "[Skybox] Found " << outFolders.size() << " skybox set(s)." << std::endl;
    }
}

void LoadSkyboxFromFolder(const std::string& folderPath, GLuint& outCubemap) {
    stbi_set_flip_vertically_on_load(false);

    struct FaceKeywords {
        std::string faceName;
        std::vector<std::string> keywords;
    };

    std::vector<FaceKeywords> faceKeywords = {
        {"right", {"right", "posx", "px", "_x", "x_pos"}},
        {"left",  {"left", "negx", "nx", "_x_neg"}},
        {"top",   {"top", "posy", "py", "_y", "y_pos"}},
        {"bottom",{"bottom", "negy", "ny", "_y_neg"}},
        {"front", {"front", "posz", "pz", "_z", "z_pos"}},
        {"back",  {"back", "negz", "nz", "_z_neg"}}
    };

    std::vector<std::string> loadedFaces(6);
    std::vector<std::string> allFiles;

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            std::string ext = entry.path().extension().string();
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga") {
                allFiles.push_back(filename);
            }
        }
    }

    for (int face = 0; face < 6; face++) {
        loadedFaces[face] = "";
        for (const auto& file : allFiles) {
            std::string lowerFile = file;
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
            for (const auto& keyword : faceKeywords[face].keywords) {
                if (lowerFile.find(keyword) != std::string::npos) {
                    loadedFaces[face] = folderPath + "/" + file;
                    std::cout << "[Skybox] Found: " << file << " for face: " << faceKeywords[face].faceName << std::endl;
                    break;
                }
            }
            if (!loadedFaces[face].empty()) break;
        }
        if (loadedFaces[face].empty()) {
            std::cout << "[Skybox] Warning: No file found for face " << face << " in " << folderPath << std::endl;
        }
    }

    GLuint newCubemap;
    glGenTextures(1, &newCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, newCubemap);

    int w, h, channels;
    for (unsigned int i = 0; i < 6; i++) {
        if (!loadedFaces[i].empty() && fs::exists(loadedFaces[i])) {
            unsigned char* data = stbi_load(loadedFaces[i].c_str(), &w, &h, &channels, 0);
            if (data) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
                std::cout << "[Skybox] Loaded: " << loadedFaces[i] << " (" << w << "x" << h << ")" << std::endl;
            }
            else {
                unsigned char fallback[] = { 255, 0, 255 };
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
                std::cout << "[Skybox] Failed to load: " << loadedFaces[i] << std::endl;
            }
        }
        else {
            unsigned char fallback[] = { 255, 0, 255 };
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
            std::cout << "[Skybox] Using fallback for face " << i << std::endl;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    if (outCubemap != 0) {
        glDeleteTextures(1, &outCubemap);
    }
    outCubemap = newCubemap;
    std::cout << "[Skybox] Cubemap loaded from: " << folderPath << std::endl;
}

// ============================================================
//                     FILE DIALOG HELPER
// ============================================================
std::string OpenFileDialog(const std::string& title, const std::string& filter = "All Files\0*.*\0", GLFWwindow* win = NULL) {
    OPENFILENAMEA ofn = {};
    char fileName[MAX_PATH] = "";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = title.c_str();
    if (GetOpenFileNameA(&ofn)) {
        return std::string(fileName);
    }
    return "";
}

#include <unordered_map>
std::unordered_map<std::string, GLuint> textureCache;

GLuint LoadTextureFromFile(const std::string& path) {
    if (textureCache.find(path) != textureCache.end()) {
        return textureCache[path];
    }
    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);

    if (!data) {
        std::cout << "Failed to load texture: " << path << std::endl;
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(data);
    textureCache[path] = tex;
    std::cout << "Loaded texture: " << path << " (ID: " << tex << ")" << std::endl;
    return tex;
}

// ============================================================
//                  AABB & COLLISION HELPERS
// ============================================================
#undef min
#undef max

bool ComputeModelAABB(Model* model, glm::vec3& outMin, glm::vec3& outMax) {
    if (!model || model->meshes.empty()) return false;
    outMin = glm::vec3(FLT_MAX);
    outMax = glm::vec3(-FLT_MAX);
    for (const auto& mesh : model->meshes) {
        for (const auto& vertex : mesh.vertices) {
            outMin = glm::min(outMin, vertex.position);
            outMax = glm::max(outMax, vertex.position);
        }
    }
    return true;
}

ShapeRefC CreateCollisionShapeFromAABB(const glm::vec3& min, const glm::vec3& max) {
    glm::vec3 size = max - min;
    float x = size.x, y = size.y, z = size.z;
    if (y > x * 1.5f && y > z * 1.5f) {
        float radius = std::max(x, z) * 0.5f;
        float halfHeight = y * 0.5f - radius;
        if (halfHeight < 0.1f) halfHeight = 0.1f;
        return new CapsuleShape(halfHeight, radius);
    }
    else if (std::abs(x - y) < 0.1f && std::abs(x - z) < 0.1f) {
        float radius = (x + y + z) / 6.0f;
        return new SphereShape(radius);
    }
    else {
        return new BoxShape(Vec3(x * 0.5f, y * 0.5f, z * 0.5f));
    }
}

// ============================================================
//                  JOLT CALLBACKS
// ============================================================
static void TraceImpl(const char* inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
    return true;
};
#endif

namespace Layers {
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
};

class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
        switch (inObject1) {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

namespace BroadPhaseLayers {
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS = 2;
};

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }
    virtual uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
        if (inLayer == BroadPhaseLayers::NON_MOVING) return "NON_MOVING";
        if (inLayer == BroadPhaseLayers::MOVING)     return "MOVING";
        JPH_ASSERT(false);
        return "INVALID";
    }
private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

GLuint LoadSimpleCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int w, h, channels;
    for (unsigned int i = 0; i < 6; i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &channels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Failed to load cubemap face: " << faces[i] << std::endl;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// ============================================================
//               IMGUI DARK PRO THEME
// ============================================================
void SetDarkProTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);
}

// ============================================================
//                     RESET HELPER
// ============================================================
void ResetSliderButton(const char* label, float* value, float defaultValue, const char* tooltip = "") {
    ImGui::SameLine();
    if (ImGui::SmallButton(("R##" + std::string(label)).c_str())) {
        *value = defaultValue;
    }
    if (!tooltip || strlen(tooltip) > 0) {
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Reset to default: %.2f", defaultValue);
        }
    }
}


// ============================================================
//                     WINDOW SETUP
// ============================================================
const unsigned int width = 1500;
const unsigned int height = 900;

enum class EntityType {
    Static,
    Player,
    Ball,
    Camera,
    Empty,
    PointLight,      
    SpotLight,       
    DirectionalLight,
    AmbientLight
};

struct Material {
    glm::vec3 albedo = glm::vec3(1.0f, 1.0f, 1.0f);
    float metallic = 0.0f;
    float roughness = 0.95f;
    float ao = 1.0f;
    float emissiveIntensity = 0.0f;
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    int useAlbedoMap = 1;
    int useNormalMap = 0;
    int useMetallicRoughnessMap = 0;
    int useAOMap = 0;
    int useHeightMap = 0;
    float heightScale = 0.05f;
    int useEmissiveMap = 0;

    // texture paths
    std::string albedoMapPath = "";
    std::string normalMapPath = "";
    std::string metallicRoughnessMapPath = "";
    std::string aoMapPath = "";
    std::string heightMapPath = "";
    std::string emissiveMapPath = "";

    // Texture IDs
    GLuint customAlbedoID = 0;
    GLuint customNormalID = 0;
    GLuint customMRID = 0;
    GLuint customAOID = 0;
    GLuint customHeightID = 0;
    GLuint customEmissiveID = 0;
};

struct ScriptParam {
    std::string name;
    float value = 0.0f;
};

struct EditorEntity {
    Model* model;
    std::string modelPath;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    EntityType type = EntityType::Static;
    glm::vec3 velocity = glm::vec3(0.0f);
    std::shared_ptr<ScriptComponent> script = nullptr;
    Material material;
    bool isCameraPossessed = false;
    bool simulatePhysics = false;
    BodyID physicsBodyID;

    struct LightProperties {
        bool enabled = true;
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        float intensity = 1.0f;
        float range = 10.0f;        
        float innerConeAngle = 15.0f;   
        float outerConeAngle = 30.0f;  
        float attenuation = 1.0f;       
    } light;

    // ===== Script Parameters =====
    std::vector<ScriptParam> scriptParams;
};

void SpawnRandomLights(std::vector<EditorEntity>& entities, int count = 20, float range = 10.0f) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-range / 2.0f, range / 2.0f);
    std::uniform_real_distribution<float> colorDist(0.2f, 1.0f);
    std::uniform_real_distribution<float> intensityDist(0.5f, 3.0f);
    std::uniform_real_distribution<float> rangeDist(3.0f, 12.0f);

    for (int i = 0; i < count; ++i) {
        EditorEntity lightEntity;
        lightEntity.model = nullptr;         
        lightEntity.modelPath = "";
        lightEntity.position = glm::vec3(posDist(gen), 1.0f, posDist(gen));
        lightEntity.rotation = glm::vec3(0.0f);
        lightEntity.scale = glm::vec3(1.0f);
        lightEntity.type = EntityType::PointLight; 
        lightEntity.velocity = glm::vec3(0.0f);
        lightEntity.script = nullptr;
        lightEntity.isCameraPossessed = false;
        lightEntity.simulatePhysics = false;

        lightEntity.light.enabled = true;
        lightEntity.light.color = glm::vec3(colorDist(gen), colorDist(gen), colorDist(gen));
        lightEntity.light.intensity = intensityDist(gen);
        lightEntity.light.range = rangeDist(gen);
        lightEntity.light.innerConeAngle = 15.0f;   
        lightEntity.light.outerConeAngle = 30.0f;
        lightEntity.light.attenuation = 1.0f;

        entities.push_back(lightEntity);
    }

    std::cout << "[Lights] Spawned " << count << " random point lights." << std::endl;
}

struct DebugLine {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
    float life;
};

struct Prefab {
    std::string name;
    std::string modelPath;
    glm::vec3 defaultPosition;
    glm::vec3 defaultRotation;
    glm::vec3 defaultScale;
    EntityType type;
    std::string scriptPath;
};

// Helpers
std::vector<EditorEntity> copyEntities(const std::vector<EditorEntity>& src) { return src; }
std::string normalizePath(const std::string& path) { std::string result = path; std::replace(result.begin(), result.end(), '\\', '/'); return result; }

std::vector<std::string> scanModelsFolder(const std::string& folderPath) {
    std::vector<std::string> modelFiles;
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cout << "Folder not found: " << folderPath << std::endl;
        return modelFiles;
    }
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".gltf" || ext == ".glb") {
                std::string fullPath = entry.path().string();
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                size_t pos = fullPath.find("models/");
                if (pos != std::string::npos) {
                    fullPath = fullPath.substr(pos);
                }
                modelFiles.push_back(fullPath);
            }
        }
    }
    return modelFiles;
}

struct Frustum {
    glm::vec4 planes[6];
};

Frustum extractFrustum(const glm::mat4& viewProj) {
    Frustum frustum;
    frustum.planes[0] = glm::vec4(viewProj[0][3] + viewProj[0][0],
        viewProj[1][3] + viewProj[1][0],
        viewProj[2][3] + viewProj[2][0],
        viewProj[3][3] + viewProj[3][0]);
    frustum.planes[1] = glm::vec4(viewProj[0][3] - viewProj[0][0],
        viewProj[1][3] - viewProj[1][0],
        viewProj[2][3] - viewProj[2][0],
        viewProj[3][3] - viewProj[3][0]);
    frustum.planes[2] = glm::vec4(viewProj[0][3] + viewProj[0][1],
        viewProj[1][3] + viewProj[1][1],
        viewProj[2][3] + viewProj[2][1],
        viewProj[3][3] + viewProj[3][1]);
    frustum.planes[3] = glm::vec4(viewProj[0][3] - viewProj[0][1],
        viewProj[1][3] - viewProj[1][1],
        viewProj[2][3] - viewProj[2][1],
        viewProj[3][3] - viewProj[3][1]);
    frustum.planes[4] = glm::vec4(viewProj[0][3] + viewProj[0][2],
        viewProj[1][3] + viewProj[1][2],
        viewProj[2][3] + viewProj[2][2],
        viewProj[3][3] + viewProj[3][2]);
    frustum.planes[5] = glm::vec4(viewProj[0][3] - viewProj[0][2],
        viewProj[1][3] - viewProj[1][2],
        viewProj[2][3] - viewProj[2][2],
        viewProj[3][3] - viewProj[3][2]);
    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        frustum.planes[i] /= length;
    }
    return frustum;
}

bool isSphereInFrustum(const Frustum& frustum, const glm::vec3& center, float radius) {
    for (int i = 0; i < 6; i++) {
        float distance = glm::dot(frustum.planes[i], glm::vec4(center, 1.0f));
        if (distance < -radius) return false;
    }
    return true;
}

// ============================================================
//                     FILE READING
// ============================================================
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ---- Debug View ----
int debugView = 0;
const char* debugViewNames[] = {
    "Off",
    "Position",
    "Normal",
    "Albedo",
    "Metallic/Roughness",
    "Emissive",
    "SSAO",
    "Depth"
};

// ============================================================
//                     MAIN
// ============================================================
int main()
{
    // 1. GLFW Init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    /*glfwWindowHint(GLFW_SAMPLES, 4);*/

    GLFWwindow* window = glfwCreateWindow(width, height, "Rodwan Engine - Level Editor", NULL, NULL);
    if (window == NULL) return -1;
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, width, height);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    // 2. Shaders
    Shader gBufferShader("gBuffer.vert", "gBuffer.frag");
    Shader deferredLightingShader("deferred_lighting.vert", "deferred_lighting.frag");
    Shader depthShader("depth.vert", "depth.frag");
    Shader fxaaShader("fxaa.vert", "fxaa.frag");
    Shader ssaoShader("ssao.vert", "ssao.frag");
    Shader lineShader("line.vert", "line.frag");
    Shader volumetricFogShader("volumetric_fog.vert", "volumetric_fog.frag");
    Shader skyboxShader("skybox.vert", "skybox.frag");

    // ---- Skybox variables ----
    bool showSkybox = true;
    bool useCustomSky = false;
    std::string currentSkyboxFolder = "cubemap";
    std::vector<std::string> skyboxFolders;
    int selectedSkyboxIndex = 0;
    GLuint envCubemap = 0;


    // Pre-cached uniforms
    static GLint uAlbedoLoc = glGetUniformLocation(gBufferShader.ID, "uAlbedo");
    static GLint uMetallicLoc = glGetUniformLocation(gBufferShader.ID, "uMetallic");
    static GLint uRoughnessLoc = glGetUniformLocation(gBufferShader.ID, "uRoughness");
    static GLint uAOLoc = glGetUniformLocation(gBufferShader.ID, "uAO");
    static GLint uUseAlbedoMapLoc = glGetUniformLocation(gBufferShader.ID, "uUseAlbedoMap");
    static GLint uUseNormalMapLoc = glGetUniformLocation(gBufferShader.ID, "uUseNormalMap");
    static GLint uUseMetallicRoughnessMapLoc = glGetUniformLocation(gBufferShader.ID, "uUseMetallicRoughnessMap");
    static GLint uUseAOMapLoc = glGetUniformLocation(gBufferShader.ID, "uUseAOMap");
    static GLint uUseHeightMapLoc = glGetUniformLocation(gBufferShader.ID, "useHeightMap");
    static GLint uHeightScaleLoc = glGetUniformLocation(gBufferShader.ID, "heightScale");
    static GLint uHeightMapLoc = glGetUniformLocation(gBufferShader.ID, "heightMap");
    static GLint uEmissiveColorLoc = glGetUniformLocation(gBufferShader.ID, "uEmissiveColor");
    static GLint uEmissiveIntensityLoc = glGetUniformLocation(gBufferShader.ID, "uEmissiveIntensity");

    static GLint uGEmissiveLoc = glGetUniformLocation(deferredLightingShader.ID, "gEmissive");
    static GLint uEmissiveMapLoc = glGetUniformLocation(deferredLightingShader.ID, "emissiveMap");
    static GLint uUseEmissiveMapLoc = glGetUniformLocation(deferredLightingShader.ID, "uUseEmissiveMap");

    static GLint uCameraMatrixLoc = glGetUniformLocation(deferredLightingShader.ID, "cameraMatrix");
    static GLint uCamPosLoc = glGetUniformLocation(deferredLightingShader.ID, "camPos");
    static GLint uViewMatrixLoc = glGetUniformLocation(deferredLightingShader.ID, "viewMatrix");
    static GLint uInverseViewMatrixLoc = glGetUniformLocation(deferredLightingShader.ID, "inverseViewMatrix");

    static GLint uVolLightPosLoc = glGetUniformLocation(volumetricFogShader.ID, "lightPos");
    static GLint uVolLightIntensityLoc = glGetUniformLocation(volumetricFogShader.ID, "volumetricLightIntensity");

    // ---- Script cache ----
    std::vector<std::string> scriptFiles;
    bool scriptsScanned = false;

    // 3. Camera
    Camera camera(width, height, glm::vec3(3.0f, 2.0f, 6.0f));
    camera.Orientation = glm::normalize(glm::vec3(-3.0f, -2.0f, -6.0f));

    // 4. ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 1.0f;

    SetDarkProTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // ---- Gizmos ----
    enum class GizmoType { None, Translate, Rotate, Scale };
    GizmoType currentGizmo = GizmoType::Translate;
    bool showGizmos = true;
    int gizmoSelectedAxis = -1;  // -1 = none, 0 = X, 1 = Y, 2 = Z
    float gizmoSize = 0.5f;

    // 5. Fullscreen Quad
    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    float rect[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 6. G-Buffer
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    unsigned int gPosition, gNormal, gColor, gMetallicRoughness, gEmissive;
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gColor);
    glBindTexture(GL_TEXTURE_2D, gColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColor, 0);

    glGenTextures(1, &gMetallicRoughness);
    glBindTexture(GL_TEXTURE_2D, gMetallicRoughness);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gMetallicRoughness, 0);

    glGenTextures(1, &gEmissive);
    glBindTexture(GL_TEXTURE_2D, gEmissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gEmissive, 0);

    // Depth texture
    unsigned int gDepth;
    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "G-Buffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Final Color FBO
    unsigned int finalColorTexture, finalColorFBO;
    glGenTextures(1, &finalColorTexture);
    glBindTexture(GL_TEXTURE_2D, finalColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &finalColorFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, finalColorFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalColorTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Final Color FBO not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Copy texture for fog
    unsigned int litCopyTexture;
    glGenTextures(1, &litCopyTexture);
    glBindTexture(GL_TEXTURE_2D, litCopyTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Cubemap
    {
        stbi_set_flip_vertically_on_load(false);

        std::vector<std::vector<std::string>> faceNames = {
            {"px.png", "right.jpg", "right.png", "posx.jpg", "posx.png"},
            {"nx.png", "left.jpg", "left.png", "negx.jpg", "negx.png"},
            {"py.png", "top.jpg", "top.png", "posy.jpg", "posy.png"},
            {"ny.png", "bottom.jpg", "bottom.png", "negy.jpg", "negy.png"},
            {"pz.png", "front.jpg", "front.png", "posz.jpg", "posz.png"},
            {"nz.png", "back.jpg", "back.png", "negz.jpg", "negz.png"}
        };

        std::vector<std::string> loadedFaces(6);
        bool allFound = true;

        for (int i = 0; i < 6; i++) {
            std::string foundPath = FindSkyboxFace("cubemap", faceNames[i]);
            if (!foundPath.empty()) {
                loadedFaces[i] = foundPath;
                std::cout << "[Skybox] Found: " << foundPath << std::endl;
            }
            else {
                std::cout << "[Skybox] Warning: No file found for face " << i << std::endl;
                allFound = false;
            }
        }

        glGenTextures(1, &envCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        int w, h, channels;
        for (unsigned int i = 0; i < 6; i++) {
            if (!loadedFaces[i].empty() && fs::exists(loadedFaces[i])) {
                unsigned char* data = stbi_load(loadedFaces[i].c_str(), &w, &h, &channels, 0);
                if (data) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    stbi_image_free(data);
                    std::cout << "[Skybox] Loaded: " << loadedFaces[i] << " (" << w << "x" << h << ")" << std::endl;
                }
                else {
                    unsigned char fallback[] = { 255, 0, 255 };
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
                    std::cout << "[Skybox] Failed to load: " << loadedFaces[i] << std::endl;
                }
            }
            else {
                unsigned char fallback[] = { 255, 0, 255 };
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
                std::cout << "[Skybox] Using fallback color for face " << i << std::endl;
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        std::cout << "[Skybox] Cubemap loaded successfully!" << std::endl;

        // مسح المجلدات الفرعية
        ScanSkyboxFolders("cubemap", skyboxFolders);
        if (!skyboxFolders.empty()) {
            currentSkyboxFolder = skyboxFolders[0];
            selectedSkyboxIndex = 0;
            std::cout << "[Skybox] Found " << skyboxFolders.size() << " skybox sets." << std::endl;
            for (const auto& f : skyboxFolders) {
                std::cout << "  - " << f << std::endl;
            }
        }
        else {
            currentSkyboxFolder = "cubemap";
            skyboxFolders.push_back(".");
            selectedSkyboxIndex = 0;
            std::cout << "[Skybox] No subfolders found. Using base folder." << std::endl;
        }
    }
    

    // 7. Shadow Map
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 8. White Texture
    unsigned int whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whitePixel[] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 9. SSAO Setup (reduced samples for performance)
    unsigned int ssaoFBO, ssaoColorBuffer;
    glGenFramebuffers(1, &ssaoFBO);
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO FBO not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int noiseTexture;
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    std::vector<glm::vec3> noiseData(16);
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 randomVec = glm::vec3(glm::linearRand(-1.0f, 1.0f), glm::linearRand(-1.0f, 1.0f), 0.0f);
        randomVec = glm::normalize(randomVec);
        noiseData[i] = randomVec;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &noiseData[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Reduced SSAO samples from 64 to 32
    std::vector<glm::vec3> ssaoSamples(32);
    for (unsigned int i = 0; i < 32; i++) {
        glm::vec3 sample = glm::vec3(glm::linearRand(-1.0f, 1.0f), glm::linearRand(-1.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
        sample = glm::normalize(sample);
        float scale = (float)i / 32.0f;
        scale = 0.1f + 0.9f * scale * scale;
        sample *= scale;
        ssaoSamples[i] = sample;
    }

    // 10. Line VAO/VBO
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // 11. Grid
    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    std::vector<glm::vec3> gridVerts;
    for (int i = -50; i <= 50; i++) {
        gridVerts.push_back({ i, 0, -50 });
        gridVerts.push_back({ i, 0, 50 });
        gridVerts.push_back({ -50, 0, i });
        gridVerts.push_back({ 50, 0, i });
    }
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(glm::vec3), gridVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glm::vec3 customTopColor = glm::vec3(0.2f, 0.4f, 0.8f);
    glm::vec3 customHorizonColor = glm::vec3(0.6f, 0.7f, 0.9f);
    glm::vec3 customBottomColor = glm::vec3(0.4f, 0.5f, 0.6f);
    glm::vec3 customSunColor = glm::vec3(1.0f, 0.9f, 0.6f);
    glm::vec3 customSunDirection = glm::vec3(0.5f, -0.2f, 0.3f);
    float customSunIntensity = 1.0f;
    float customCloudDensity = 1.5f;
    float customCloudOpacity = 0.3f;
    static int selectedSky = 0;

    // 13. Post-processing (defaults)
    bool enableSSAO = true;
    float ssaoRadius = 0.5f;
    float ssaoBias = 0.025f;
    float ssaoPower = 2.0f;
    bool enableBloom = true;
    float bloomThreshold = 0.5f;
    float bloomIntensity = 0.4f;
    float saturation = 1.0f;
    float contrast = 1.0f;
    float gamma = 2.2f;
    float exposure = 1.5f;

    // ---- Volumetric Fog ----
    bool enableVolumetricFog = false;
    float fogDensity = 0.05f;
    float fogHeight = 0.5f;
    float fogFalloff = 2.0f;
    int fogSteps = 16;
    float fogMaxDistance = 200.0f;
    glm::vec3 fogColor = glm::vec3(0.6f, 0.7f, 0.8f);

    // ---- Cubemap Reflection ----
    bool enableEnvReflections = true;
    float envReflectionIntensity = 0.5f;

    // ---- NEW TOGGLES ----
    bool enableContactShadows = false;   // default off
    bool enableFXAA = false;             // disabled by default

    // ---- Entity List & Properties UI ----
    static bool showEntityList = true;
    static bool showEntityProperties = true;
    static int selectedEntityIndex = -1;
    static char entityNameBuffer[128] = "";

    // 14. Performance
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
    float fps = 0.0f;
    float fpsCounter = 0.0f;
    float fpsTime = 0.0f;
    bool showFPS = true;
    bool enableFrustumCulling = true;

    // 15. Content browser
    std::vector<std::string> modelFiles;
    std::string selectedModelPath = "";
    float editorFOV = 75.0f;
    float gameFOV = 65.0f;

    // 16. Pre‑loaded models
    std::vector<std::string> modelPaths = {
        "models/Geometries/Cube.gltf"
    };
    std::vector<Model*> loadedModels;
    for (const auto& path : modelPaths) {
        Model* model = new Model(path.c_str());
        loadedModels.push_back(model);
        std::cout << "Loaded: " << path << std::endl;
    }

    // 17. Entities
    std::vector<EditorEntity> entities;
    entities.reserve(256);
    std::vector<DebugLine> debugLines;
    debugLines.reserve(1024);
    std::vector<Prefab> prefabs;
    prefabs.reserve(32);
    int selectedEntity = -1;

    Material defaultMat;
    defaultMat.roughness = 0.95f;
    defaultMat.useNormalMap = 0;
    defaultMat.useMetallicRoughnessMap = 0;
    defaultMat.emissiveIntensity = 0.0f;

    entities.push_back({
        loadedModels[0], modelPaths[0],
        glm::vec3(0.0f, 1.5f, 0.0f),
        glm::vec3(0.0f, 90.0f, 0.0f),
        glm::vec3(0.5f),
        EntityType::Static,
        glm::vec3(0),
        nullptr,
        defaultMat,
        false,
        false
        });

    // 18. Game state
    int gameScore = 0;
    int gameHealth = 100;
    bool gameOver = false;
    bool gameWon = false;

    struct GPULight {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float range;
        int type; // 0 = Point, 1 = Spot, 2 = Directional
        glm::vec3 direction;
        float innerCone;
        float outerCone;
    };
    std::vector<GPULight> gpuLights;

    struct CameraSettings {
        int targetId = -1;
        float distance = 4.0f;
        float height = 2.0f;
        float yaw = 0.0f;
        float pitch = -20.0f;
        float smoothSpeed = 5.0f;
        glm::vec3 offset = glm::vec3(0);
        bool useSmooth = true;
        bool lookAtTarget = true;
        glm::vec3 currentPos = glm::vec3(0);
        glm::vec3 currentRot = glm::vec3(0);
    } camSettings;

    float yaw = -90.0f;
    float pitch = 0.0f;
    float mouseSensitivity = 0.1f;
    bool firstMouse = true;
    double lastMouseX = width / 2.0;
    double lastMouseY = height / 2.0;

    bool useEntityCamera = false;
    int cameraEntityId = -1;

    // ============================================================
    // 19. JOLT PHYSICS
    // ============================================================
    std::cout << "[Jolt] Initializing Physics System..." << std::endl;

    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

        Factory::sInstance = new Factory();
    RegisterTypes();

    TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
    JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    PhysicsSystem physics_system;
    const uint cMaxBodies = 1024;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 1024;
    const uint cMaxContactConstraints = 1024;
    physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
        broad_phase_layer_interface, object_vs_broadphase_layer_filter,
        object_vs_object_layer_filter);

    BodyInterface& body_interface = physics_system.GetBodyInterface();

    // Floor
    BoxShapeSettings floor_shape_settings(Vec3(50.0f, 0.5f, 50.0f));
    ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    ShapeRefC floor_shape = floor_shape_result.Get();
    BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -0.5_r, 0.0_r),
        Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
    Body* floor = body_interface.CreateBody(floor_settings);
    body_interface.AddBody(floor->GetID(), EActivation::DontActivate);

    // Player
    BodyID playerBodyID;
    {
        CapsuleShapeSettings capsule_settings(0.5f, 0.5f);
        ShapeSettings::ShapeResult capsule_result = capsule_settings.Create();
        ShapeRefC capsule_shape = capsule_result.Get();
        BodyCreationSettings player_settings(
            capsule_shape,
            RVec3(0.0_r, 2.0_r, 0.0_r),
            Quat::sIdentity(),
            EMotionType::Dynamic,
            Layers::MOVING
        );
        player_settings.mAllowSleeping = false;
        playerBodyID = body_interface.CreateAndAddBody(player_settings, EActivation::Activate);
        std::cout << "[Jolt] Player body created with ID: " << playerBodyID.GetIndex() << std::endl;
    }

    std::cout << "[Jolt] Physics System initialized successfully!" << std::endl;

    // Generate physics lambda
    auto GeneratePhysicsBody = [&](EditorEntity& entity) {
        if (entity.model == nullptr) return;
        glm::vec3 min, max;
        if (!ComputeModelAABB(entity.model, min, max)) {
            std::cout << "[Physics] Could not compute AABB for " << entity.modelPath << std::endl;
            return;
        }
        ShapeRefC shape = CreateCollisionShapeFromAABB(min, max);
        if (shape == nullptr) return;
        RVec3 pos(entity.position.x, entity.position.y, entity.position.z);
        BodyCreationSettings body_settings(
            shape,
            pos,
            Quat::sIdentity(),
            EMotionType::Dynamic,
            Layers::MOVING
        );
        body_settings.mAllowSleeping = true;
        BodyID body_id = body_interface.CreateAndAddBody(body_settings, EActivation::Activate);
        entity.physicsBodyID = body_id;
        entity.simulatePhysics = true;
        std::cout << "[Physics] Generated collision for " << entity.modelPath << std::endl;
        };

    // ----- Interpolation for smooth camera -----
    RVec3 prevPlayerPos;
    RVec3 currPlayerPos;
    float physicsAccumulator = 0.0f;
    float fixedDeltaTime = 1.0f / 60.0f;

    // ============================================================
    //                     LUA API (FULL DEFINITIONS)
    // ============================================================
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    
    // Logging and Time
    lua.set_function("log", [](const std::string& msg) { std::cout << "[Lua]: " << msg << std::endl; });
    lua.set_function("get_time", []() -> float { return (float)glfwGetTime(); });

    // Position, Rotation, Scale
    lua.set_function("get_pos", [&entities](int idx) -> std::tuple<float, float, float> {
        if (idx < 0 || idx >= (int)entities.size()) return { 0.0f, 0.0f, 0.0f };
        auto& e = entities[idx];
        return { e.position.x, e.position.y, e.position.z };
        });
    lua.set_function("set_pos", [&entities](int idx, float x, float y, float z) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        entities[idx].position = glm::vec3(x, y, z);
        });
    lua.set_function("get_rot", [&entities](int idx) -> std::tuple<float, float, float> {
        if (idx < 0 || idx >= (int)entities.size()) return { 0.0f, 0.0f, 0.0f };
        auto& e = entities[idx];
        return { e.rotation.x, e.rotation.y, e.rotation.z };
        });
    lua.set_function("set_rot", [&entities](int idx, float x, float y, float z) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        entities[idx].rotation = glm::vec3(x, y, z);
        });
    lua.set_function("get_scale", [&entities](int idx) -> std::tuple<float, float, float> {
        if (idx < 0 || idx >= (int)entities.size()) return { 1.0f, 1.0f, 1.0f };
        auto& e = entities[idx];
        return { e.scale.x, e.scale.y, e.scale.z };
        });
    lua.set_function("set_scale", [&entities](int idx, float x, float y, float z) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        entities[idx].scale = glm::vec3(x, y, z);
        });

    // ============================================================
    //                   SCRIPT PARAMETERS API
    // ============================================================
    lua.set_function("get_param", [&entities](int idx, const std::string& name) -> float {
        if (idx < 0 || idx >= (int)entities.size()) return 0.0f;
        for (const auto& p : entities[idx].scriptParams) {
            if (p.name == name) return p.value;
        }
        return 0.0f;
        });

    lua.set_function("set_param", [&entities](int idx, const std::string& name, float value) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        for (auto& p : entities[idx].scriptParams) {
            if (p.name == name) { p.value = value; return; }
        }
        // Auto-add if not exists
        entities[idx].scriptParams.push_back({ name, value });
        });

    lua.set_function("has_param", [&entities](int idx, const std::string& name) -> bool {
        if (idx < 0 || idx >= (int)entities.size()) return false;
        for (const auto& p : entities[idx].scriptParams) {
            if (p.name == name) return true;
        }
        return false;
        });

    lua.set_function("get_param_names", [&entities](int idx) -> std::vector<std::string> {
        std::vector<std::string> names;
        if (idx < 0 || idx >= (int)entities.size()) return names;
        for (const auto& p : entities[idx].scriptParams) {
            names.push_back(p.name);
        }
        return names;
        });

    // Material
    lua.set_function("set_material", [&entities](int idx, float r, float g, float b, float metallic, float roughness) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        auto& mat = entities[idx].material;
        mat.albedo = glm::vec3(r, g, b);
        mat.metallic = metallic;
        mat.roughness = roughness;
        });
    lua.set_function("get_material", [&entities](int idx) -> std::tuple<float, float, float, float, float> {
        if (idx < 0 || idx >= (int)entities.size()) return { 1,1,1,0,0.5 };
        auto& mat = entities[idx].material;
        return { mat.albedo.x, mat.albedo.y, mat.albedo.z, mat.metallic, mat.roughness };
        });
    lua.set_function("set_emissive", [&entities](int idx, float r, float g, float b, float intensity) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        auto& mat = entities[idx].material;
        mat.emissiveColor = glm::vec3(r, g, b);
        mat.emissiveIntensity = intensity;
        });

    // Physics
    lua.set_function("simulate_physics", [&entities](int idx, bool enable) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        entities[idx].simulatePhysics = enable;
        });
    lua.set_function("is_physics_simulating", [&entities](int idx) -> bool {
        if (idx < 0 || idx >= (int)entities.size()) return false;
        return entities[idx].simulatePhysics;
        });
    lua.set_function("apply_force", [&entities, &body_interface](int idx, float fx, float fy, float fz) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        if (entities[idx].simulatePhysics) {
            body_interface.AddForce(entities[idx].physicsBodyID, Vec3(fx, fy, fz));
        }
        });
    lua.set_function("apply_impulse", [&entities, &body_interface](int idx, float ix, float iy, float iz) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        if (entities[idx].simulatePhysics) {
            body_interface.AddImpulse(entities[idx].physicsBodyID, Vec3(ix, iy, iz));
        }
        });
    lua.set_function("get_velocity", [&entities, &body_interface](int idx) -> std::tuple<float, float, float> {
        if (idx < 0 || idx >= (int)entities.size()) return { 0.0f, 0.0f, 0.0f };
        if (entities[idx].simulatePhysics) {
            Vec3 vel = body_interface.GetLinearVelocity(entities[idx].physicsBodyID);
            return { vel.GetX(), vel.GetY(), vel.GetZ() };
        }
        return { 0.0f, 0.0f, 0.0f };
        });

    // Camera Possession
    lua.set_function("possess_camera", [&entities](int idx) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        for (auto& e : entities) e.isCameraPossessed = false;
        entities[idx].isCameraPossessed = true;
        });
    lua.set_function("release_camera", [&entities]() {
        for (auto& e : entities) e.isCameraPossessed = false;
        });
    lua.set_function("is_camera_possessed", [&entities](int idx) -> bool {
        if (idx < 0 || idx >= (int)entities.size()) return false;
        return entities[idx].isCameraPossessed;
        });

    // Key Input
    lua.set_function("is_key_pressed", [window](const std::string& key) -> bool {
        static const std::unordered_map<std::string, int> keyMap = {
            {"W", GLFW_KEY_W}, {"A", GLFW_KEY_A}, {"S", GLFW_KEY_S}, {"D", GLFW_KEY_D},
            {"SPACE", GLFW_KEY_SPACE}, {"LEFT_SHIFT", GLFW_KEY_LEFT_SHIFT},
            {"ESCAPE", GLFW_KEY_ESCAPE}, {"LEFT", GLFW_KEY_LEFT}, {"RIGHT", GLFW_KEY_RIGHT},
            {"UP", GLFW_KEY_UP}, {"DOWN", GLFW_KEY_DOWN}, {"E", GLFW_KEY_E}, {"Q", GLFW_KEY_Q},
            {"R", GLFW_KEY_R}, {"F", GLFW_KEY_F}, {"G", GLFW_KEY_G}, {"T", GLFW_KEY_T},
            {"Y", GLFW_KEY_Y}, {"U", GLFW_KEY_U}, {"I", GLFW_KEY_I}, {"O", GLFW_KEY_O},
            {"P", GLFW_KEY_P}, {"L", GLFW_KEY_L}, {"K", GLFW_KEY_K}, {"J", GLFW_KEY_J},
            {"H", GLFW_KEY_H}, {"Z", GLFW_KEY_Z}, {"X", GLFW_KEY_X}, {"C", GLFW_KEY_C},
            {"V", GLFW_KEY_V}, {"B", GLFW_KEY_B}, {"N", GLFW_KEY_N}, {"M", GLFW_KEY_M}
        };
        auto it = keyMap.find(key);
        return (it != keyMap.end()) ? (glfwGetKey(window, it->second) == GLFW_PRESS) : false;
        });

    // Camera Settings
    lua.set_function("camera_get_target", [&camSettings]() -> int { return camSettings.targetId; });
    lua.set_function("camera_get_distance", [&camSettings]() -> float { return camSettings.distance; });
    lua.set_function("camera_get_height", [&camSettings]() -> float { return camSettings.height; });
    lua.set_function("camera_get_yaw", [&camSettings]() -> float { return camSettings.yaw; });
    lua.set_function("camera_get_pitch", [&camSettings]() -> float { return camSettings.pitch; });
    lua.set_function("camera_get_smooth_speed", [&camSettings]() -> float { return camSettings.smoothSpeed; });
    lua.set_function("camera_get_offset", [&camSettings]() -> std::tuple<float, float, float> {
        return { camSettings.offset.x, camSettings.offset.y, camSettings.offset.z };
        });
    lua.set_function("camera_get_mode", [&camSettings]() -> bool { return camSettings.lookAtTarget; });
    lua.set_function("get_camera_yaw", [&camSettings]() -> float { return camSettings.yaw; });
    lua.set_function("get_camera_pitch", [&camSettings]() -> float { return camSettings.pitch; });

    // Spawn Entity
    lua.set_function("spawn_entity", [&entities, &loadedModels, &modelPaths](const std::string& path, float x, float y, float z) -> int {
        int modelIndex = -1;
        for (int i = 0; i < (int)modelPaths.size(); ++i) {
            if (modelPaths[i] == path) { modelIndex = i; break; }
        }
        if (modelIndex == -1) {
            if (!fs::exists(path)) {
                std::cerr << "[Lua] Model not found: " << path << std::endl;
                return -1;
            }
            Model* newModel = new Model(path.c_str());
            loadedModels.push_back(newModel);
            modelPaths.push_back(path);
            modelIndex = loadedModels.size() - 1;
        }
        entities.push_back({ loadedModels[modelIndex], path, glm::vec3(x, y, z), glm::vec3(0,0,0), glm::vec3(1,1,1) });
        int newIdx = entities.size() - 1;
        return newIdx;
        });
    lua.set_function("spawn_physics_entity", [&entities, &loadedModels, &modelPaths, &body_interface](const std::string& path, float x, float y, float z, float radius) -> int {
        int modelIndex = -1;
        for (int i = 0; i < (int)modelPaths.size(); ++i) {
            if (modelPaths[i] == path) { modelIndex = i; break; }
        }
        if (modelIndex == -1) {
            if (!fs::exists(path)) {
                std::cerr << "[Lua] Model not found: " << path << std::endl;
                return -1;
            }
            Model* newModel = new Model(path.c_str());
            loadedModels.push_back(newModel);
            modelPaths.push_back(path);
            modelIndex = loadedModels.size() - 1;
        }
        entities.push_back({ loadedModels[modelIndex], path, glm::vec3(x, y, z), glm::vec3(0,0,0), glm::vec3(1,1,1) });
        int newIdx = entities.size() - 1;
        BodyCreationSettings sphere_settings(new SphereShape(radius), RVec3(x, y, z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
        BodyID body_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
        entities[newIdx].physicsBodyID = body_id;
        entities[newIdx].simulatePhysics = true;
        std::cout << "[Lua] Spawned physics entity " << newIdx << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
        return newIdx;
        });
    lua.set_function("spawn_empty_entity", [&entities, &lua](float x, float y, float z, float rx, float ry, float rz, const std::string& typeStr, const std::string& scriptPath) -> int {
        EntityType type = EntityType::Empty;
        if (typeStr == "Static") type = EntityType::Static;
        else if (typeStr == "Player") type = EntityType::Player;
        else if (typeStr == "Ball") type = EntityType::Ball;
        else if (typeStr == "Camera") type = EntityType::Camera;
        entities.push_back({ nullptr, "", glm::vec3(x, y, z), glm::vec3(rx, ry, rz), glm::vec3(1,1,1), type, glm::vec3(0), nullptr });
        int idx = entities.size() - 1;
        if (!scriptPath.empty()) {
            entities[idx].script = std::make_shared<ScriptComponent>(lua, scriptPath, idx);
            if (entities[idx].script->isLoaded()) entities[idx].script->start();
        }
        std::cout << "[Lua] Spawned empty entity " << idx << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
        return idx;
        });

    // Visibility
    lua.set_function("set_entity_visible", [&entities](int idx, bool visible) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        if (visible) entities[idx].scale = glm::vec3(1.0f);
        else entities[idx].scale = glm::vec3(0.0f);
        });
    lua.set_function("set_visible", [&entities](int idx, bool visible) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        if (visible) entities[idx].scale = glm::vec3(1.0f);
        else entities[idx].scale = glm::vec3(0.0f);
        });

    // Spawn Camera Entity
    lua.set_function("spawn_camera_entity", [&entities, &lua, &cameraEntityId, &useEntityCamera](float x, float y, float z, const std::string& scriptPath) -> int {
        entities.push_back({ nullptr, "", glm::vec3(x, y, z), glm::vec3(0), glm::vec3(1), EntityType::Camera, glm::vec3(0), nullptr });
        int idx = entities.size() - 1;
        if (!scriptPath.empty()) {
            entities[idx].script = std::make_shared<ScriptComponent>(lua, scriptPath, idx);
            if (entities[idx].script->isLoaded()) entities[idx].script->start();
        }
        cameraEntityId = idx;
        useEntityCamera = true;
        std::cout << "[Lua] Spawned camera entity " << idx << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
        return idx;
        });
    lua.set_function("spawn_camera", [&entities, &loadedModels, &modelPaths, &camera, &cameraEntityId, &useEntityCamera](const std::string& path, float x, float y, float z) -> int {
        int modelIndex = -1;
        for (int i = 0; i < (int)modelPaths.size(); ++i) {
            if (modelPaths[i] == path) { modelIndex = i; break; }
        }
        if (modelIndex == -1) {
            if (!fs::exists(path)) {
                std::cerr << "[Lua] Camera model not found: " << path << std::endl;
                return -1;
            }
            Model* newModel = new Model(path.c_str());
            loadedModels.push_back(newModel);
            modelPaths.push_back(path);
            modelIndex = loadedModels.size() - 1;
        }
        entities.push_back({ loadedModels[modelIndex], path, glm::vec3(x, y, z), glm::vec3(0,0,0), glm::vec3(0.5f), EntityType::Camera, glm::vec3(0), nullptr });
        int newIdx = entities.size() - 1;
        cameraEntityId = newIdx;
        useEntityCamera = true;
        std::cout << "[Lua] Spawned camera entity " << newIdx << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
        return newIdx;
        });

    // Camera Control
    lua.set_function("set_active_camera", [&cameraEntityId](int idx) { cameraEntityId = idx; });
    lua.set_function("camera_set_target", [&camSettings](int entityId) { camSettings.targetId = entityId; });
    lua.set_function("camera_set_distance", [&camSettings](float dist) { camSettings.distance = dist; });
    lua.set_function("camera_set_height", [&camSettings](float h) { camSettings.height = h; });
    lua.set_function("camera_set_offset", [&camSettings](float x, float y, float z) { camSettings.offset = glm::vec3(x, y, z); });
    lua.set_function("camera_set_smooth_speed", [&camSettings](float speed) { camSettings.smoothSpeed = speed; });
    lua.set_function("camera_set_pitch", [&camSettings](float pitch) { camSettings.pitch = pitch; });
    lua.set_function("camera_set_yaw", [&camSettings](float yaw) { camSettings.yaw = yaw; });
    lua.set_function("camera_set_mode", [&camSettings](bool lookAt) { camSettings.lookAtTarget = lookAt; });

    lua.set_function("camera_get_position", [&camera]() -> std::tuple<float, float, float> {
        return { camera.Position.x, camera.Position.y, camera.Position.z };
        });
    lua.set_function("camera_get_orientation", [&camera]() -> std::tuple<float, float, float> {
        return { camera.Orientation.x, camera.Orientation.y, camera.Orientation.z };
        });

    // Destroy Entity
    lua.set_function("destroy_entity", [&entities, &selectedEntity, &body_interface](int idx) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        if (entities[idx].script && entities[idx].script->isLoaded()) entities[idx].script->destroy();
        if (entities[idx].simulatePhysics) {
            body_interface.RemoveBody(entities[idx].physicsBodyID);
            body_interface.DestroyBody(entities[idx].physicsBodyID);
        }
        entities.erase(entities.begin() + idx);
        if (selectedEntity == idx) selectedEntity = -1;
        else if (selectedEntity > idx) selectedEntity--;
        });

    // Entity Type
    lua.set_function("get_entity_type", [&entities](int idx) -> std::string {
        if (idx < 0 || idx >= (int)entities.size()) return "Invalid";
        switch (entities[idx].type) {
        case EntityType::Static: return "Static";
        case EntityType::Player: return "Player";
        case EntityType::Ball:   return "Ball";
        case EntityType::Camera: return "Camera";
        default: return "Unknown";
        }
        });
    lua.set_function("set_entity_type", [&entities](int idx, const std::string& type) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        if (type == "Static") entities[idx].type = EntityType::Static;
        else if (type == "Player") entities[idx].type = EntityType::Player;
        else if (type == "Ball") entities[idx].type = EntityType::Ball;
        });

    // Raycast & Debug
    lua.set_function("draw_debug_line", [&debugLines](float sx, float sy, float sz, float ex, float ey, float ez, float r, float g, float b, float life) {
        debugLines.push_back({ glm::vec3(sx, sy, sz), glm::vec3(ex, ey, ez), glm::vec3(r, g, b), life });
        });
    lua.set_function("raycast", [&entities, &debugLines](float ox, float oy, float oz, float dx, float dy, float dz) -> int {
        glm::vec3 origin(ox, oy, oz);
        glm::vec3 dir(dx, dy, dz);
        dir = glm::normalize(dir);
        float maxDist = 20.0f;
        glm::vec3 end = origin + dir * maxDist;
        debugLines.push_back({ origin, end, glm::vec3(1.0f, 0.0f, 0.0f), 2.0f });
        float radius = 0.5f;
        int hitIdx = -1;
        float minDist = FLT_MAX;
        for (int i = 0; i < (int)entities.size(); ++i) {
            const auto& entity = entities[i];
            glm::vec3 center = entity.position;
            glm::vec3 diff = center - origin;
            float t = glm::dot(diff, dir);
            if (t < 0.0f) continue;
            glm::vec3 closest = origin + t * dir;
            float distSq = glm::distance2(closest, center);
            if (distSq < radius * radius) {
                if (t < minDist) {
                    minDist = t;
                    hitIdx = i;
                    debugLines.push_back({ origin, center, glm::vec3(0.0f, 1.0f, 0.0f), 2.0f });
                }
            }
        }
        return hitIdx;
        });

    // Prefabs
    lua.set_function("register_prefab", [&prefabs](const std::string& name, const std::string& model,
        float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz,
        const std::string& typeStr, const std::string& script) {
            EntityType type = EntityType::Static;
            if (typeStr == "Player") type = EntityType::Player;
            else if (typeStr == "Ball") type = EntityType::Ball;
            prefabs.push_back({ name, model, glm::vec3(px,py,pz), glm::vec3(rx,ry,rz), glm::vec3(sx,sy,sz), type, script });
        });
    lua.set_function("spawn_prefab", [&prefabs, &entities, &loadedModels, &modelPaths, &lua](const std::string& name, float x, float y, float z) -> int {
        for (const auto& prefab : prefabs) {
            if (prefab.name == name) {
                int modelIndex = -1;
                for (int i = 0; i < (int)modelPaths.size(); ++i) {
                    if (modelPaths[i] == prefab.modelPath) { modelIndex = i; break; }
                }
                if (modelIndex == -1) {
                    if (!fs::exists(prefab.modelPath)) {
                        std::cerr << "[Lua] Prefab model not found: " << prefab.modelPath << std::endl;
                        return -1;
                    }
                    Model* newModel = new Model(prefab.modelPath.c_str());
                    loadedModels.push_back(newModel);
                    modelPaths.push_back(prefab.modelPath);
                    modelIndex = loadedModels.size() - 1;
                }
                entities.push_back({ loadedModels[modelIndex], prefab.modelPath,
                    glm::vec3(x, y, z) + prefab.defaultPosition,
                    prefab.defaultRotation,
                    prefab.defaultScale,
                    prefab.type,
                    glm::vec3(0,0,0),
                    nullptr });
                int idx = entities.size() - 1;
                if (!prefab.scriptPath.empty()) {
                    entities[idx].script = std::make_shared<ScriptComponent>(lua, prefab.scriptPath, idx);
                    if (entities[idx].script->isLoaded()) entities[idx].script->start();
                }
                return idx;
            }
        }
        return -1;
        });

    // Camera Attach
    lua.set_function("attach_camera_to_entity", [&useEntityCamera, &cameraEntityId](int entityId) {
        cameraEntityId = entityId;
        useEntityCamera = true;
        });
    lua.set_function("detach_camera", [&useEntityCamera]() { useEntityCamera = false; });
    lua.set_function("set_camera_rotation", [&entities](int idx, float x, float y, float z) {
        if (idx < 0 || idx >= (int)entities.size()) return;
        entities[idx].rotation = glm::vec3(x, y, z);
        });
    lua.set_function("set_camera_orientation", [&camera](float pitch, float yaw) {
        camera.Orientation = glm::vec3(
            glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
            glm::sin(glm::radians(pitch)),
            glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
        );
        camera.Orientation = glm::normalize(camera.Orientation);
        });

    lua.set_function("set_as_player", [&entities](int idx) {
        if (idx >= 0 && idx < (int)entities.size()) {
            entities[idx].type = EntityType::Player;
        }
        });

    // Game State
    lua.set_function("get_score", [&gameScore]() -> int { return gameScore; });
    lua.set_function("add_score", [&gameScore](int pts) { gameScore += pts; });
    lua.set_function("get_health", [&gameHealth]() -> int { return gameHealth; });
    lua.set_function("set_health", [&gameHealth](int hp) { gameHealth = hp; });
    lua.set_function("is_game_over", [&gameOver]() -> bool { return gameOver; });
    lua.set_function("is_game_won", [&gameWon]() -> bool { return gameWon; });
    lua.set_function("quit_game", [&window]() { glfwSetWindowShouldClose(window, true); });

    // ============================================================
    // 22. Undo/Redo & Level management
    // ============================================================
    bool gameMode = false;
    std::vector<EditorEntity> savedEntities;
    std::vector<std::vector<EditorEntity>> undoStack;
    std::vector<std::vector<EditorEntity>> redoStack;

    auto pushUndo = [&]() {
        undoStack.push_back(copyEntities(entities));
        redoStack.clear();
        };
    auto applyUndo = [&]() {
        if (!undoStack.empty()) {
            redoStack.push_back(copyEntities(entities));
            entities = undoStack.back();
            undoStack.pop_back();
            selectedEntity = -1;
        }
        };
    auto applyRedo = [&]() {
        if (!redoStack.empty()) {
            undoStack.push_back(copyEntities(entities));
            entities = redoStack.back();
            redoStack.pop_back();
            selectedEntity = -1;
        }
        };

    std::string levelFolder = "Levels";
    fs::create_directories(levelFolder);
    char levelNameBuffer[128] = "MyLevel";
    std::string levelName = "MyLevel";
    std::string levelPath = levelFolder + "/" + levelName + ".txt";
    bool showOverwriteWarning = false;
    bool showRenamePopup = false;
    char newLevelNameBuffer[128] = "";

    auto saveLevel = [&]() {
        std::ostringstream oss;
        for (const auto& entity : entities) {
            oss << entity.modelPath << '|'
                << entity.position.x << '|' << entity.position.y << '|' << entity.position.z << '|'
                << entity.rotation.x << '|' << entity.rotation.y << '|' << entity.rotation.z << '|'
                << entity.scale.x << '|' << entity.scale.y << '|' << entity.scale.z << '|'
                << static_cast<int>(entity.type) << '|'
                << (entity.script && entity.script->isLoaded() ? entity.script->scriptPath : "") << '|'
                << entity.isCameraPossessed << '|'
                << entity.material.albedo.x << '|' << entity.material.albedo.y << '|' << entity.material.albedo.z << '|'
                << entity.material.metallic << '|' << entity.material.roughness << '|' << entity.material.ao << '|'
                << entity.material.emissiveColor.x << '|' << entity.material.emissiveColor.y << '|' << entity.material.emissiveColor.z << '|'
                << entity.material.emissiveIntensity << '|'
                << entity.material.useAlbedoMap << '|'
                << entity.material.useNormalMap << '|'
                << entity.material.useMetallicRoughnessMap << '|'
                << entity.material.useAOMap << '|'
                << entity.material.albedoMapPath << '|'
                << entity.material.normalMapPath << '|'
                << entity.material.metallicRoughnessMapPath << '|'
                << entity.material.aoMapPath << '|'
                << entity.material.heightMapPath << '|'
                << entity.material.useEmissiveMap << '|'
                << entity.material.emissiveMapPath << '|'
                << entity.simulatePhysics << '|';
            // Serialize script params as: name1=val1;name2=val2;...
            for (size_t i = 0; i < entity.scriptParams.size(); ++i) {
                if (i > 0) oss << ';';
                oss << entity.scriptParams[i].name << '=' << entity.scriptParams[i].value;
            }
            oss << '\n';


        }
        std::ofstream file(levelPath);
        if (file.is_open()) {
            file << oss.str();
            file.close();
            std::cout << "Saved level to " << levelPath << std::endl;
        }
        else {
            std::cout << "Failed to open: " << levelPath << std::endl;
        }
        };

    auto loadLevel = [&]() {
        std::ifstream file(levelPath);
        if (!file.is_open()) {
            std::cout << "Failed to open: " << levelPath << std::endl;
            return;
        }
        pushUndo();
        entities.clear();
        selectedEntity = -1;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string path;
            std::getline(ss, path, '|');
            float px, py, pz, rx, ry, rz, sx, sy, sz;
            ss >> px; ss.ignore(); ss >> py; ss.ignore(); ss >> pz;
            ss.ignore();
            ss >> rx; ss.ignore(); ss >> ry; ss.ignore(); ss >> rz;
            ss.ignore();
            ss >> sx; ss.ignore(); ss >> sy; ss.ignore(); ss >> sz;
            ss.ignore();

            int typeInt = 0;
            ss >> typeInt;
            ss.ignore();

            std::string scriptPath;
            std::getline(ss, scriptPath, '|');

            bool isPossessed = false;
            ss >> isPossessed;
            ss.ignore();

            float albedoR, albedoG, albedoB, metallic, roughness, ao;
            float emissiveR, emissiveG, emissiveB, emissiveIntensity;
            bool useAlbedoMap, useNormalMap, useMetallicRoughnessMap, useAOMap;
            int useEmissiveMap;
            std::string emissiveMapPath;

            ss >> albedoR; ss.ignore();
            ss >> albedoG; ss.ignore();
            ss >> albedoB; ss.ignore();
            ss >> metallic; ss.ignore();
            ss >> roughness; ss.ignore();
            ss >> ao; ss.ignore();
            ss >> emissiveR; ss.ignore();
            ss >> emissiveG; ss.ignore();
            ss >> emissiveB; ss.ignore();
            ss >> emissiveIntensity; ss.ignore();
            ss >> useAlbedoMap; ss.ignore();
            ss >> useNormalMap; ss.ignore();
            ss >> useMetallicRoughnessMap; ss.ignore();
            ss >> useAOMap; ss.ignore();
            ss >> useEmissiveMap; ss.ignore();

            std::string albedoMapPath, normalMapPath, metallicRoughnessMapPath, aoMapPath, heightMapPath;
            std::getline(ss, albedoMapPath, '|');
            std::getline(ss, normalMapPath, '|');
            std::getline(ss, metallicRoughnessMapPath, '|');
            std::getline(ss, aoMapPath, '|');
            std::getline(ss, heightMapPath, '|');
            std::getline(ss, emissiveMapPath, '|');

            bool simulatePhysics = false;
            ss >> simulatePhysics;
            ss.ignore();  // consume '|'

            // Read params string (rest of line)
            std::string paramsStr;
            std::getline(ss, paramsStr);

            std::vector<ScriptParam> loadedParams;
            if (!paramsStr.empty()) {
                std::stringstream ps(paramsStr);
                std::string pair;
                while (std::getline(ps, pair, ';')) {
                    size_t eq = pair.find('=');
                    if (eq != std::string::npos) {
                        ScriptParam p;
                        p.name = pair.substr(0, eq);
                        p.value = std::stof(pair.substr(eq + 1));
                        loadedParams.push_back(p);
                    }
                }
            }

            int modelIndex = -1;
            for (int i = 0; i < (int)modelPaths.size(); ++i) {
                if (modelPaths[i] == path) { modelIndex = i; break; }
            }
            if (modelIndex == -1) {
                if (fs::exists(path)) {
                    Model* newModel = new Model(path.c_str());
                    loadedModels.push_back(newModel);
                    modelPaths.push_back(path);
                    modelIndex = loadedModels.size() - 1;
                    std::cout << "[Load] Loaded model: " << path << std::endl;
                }
                else {
                    std::cerr << "[Load] Model not found: " << path << std::endl;
                    continue;
                }
            }

            if (modelIndex != -1) {
                EditorEntity ent;
                ent.model = loadedModels[modelIndex];
                ent.modelPath = path;
                ent.position = glm::vec3(px, py, pz);
                ent.rotation = glm::vec3(rx, ry, rz);
                ent.scale = glm::vec3(sx, sy, sz);
                ent.type = static_cast<EntityType>(typeInt);
                ent.isCameraPossessed = isPossessed;
                ent.simulatePhysics = simulatePhysics;
                ent.scriptParams = loadedParams;

                ent.material.albedo = glm::vec3(albedoR, albedoG, albedoB);
                ent.material.metallic = metallic;
                ent.material.roughness = roughness;
                ent.material.ao = ao;
                ent.material.emissiveColor = glm::vec3(emissiveR, emissiveG, emissiveB);
                ent.material.emissiveIntensity = emissiveIntensity;
                ent.material.useAlbedoMap = useAlbedoMap;
                ent.material.useNormalMap = useNormalMap;
                ent.material.useMetallicRoughnessMap = useMetallicRoughnessMap;
                ent.material.useAOMap = useAOMap;

                ent.material.albedoMapPath = albedoMapPath;
                ent.material.normalMapPath = normalMapPath;
                ent.material.metallicRoughnessMapPath = metallicRoughnessMapPath;
                ent.material.aoMapPath = aoMapPath;
                ent.material.heightMapPath = heightMapPath;

                ent.material.useEmissiveMap = useEmissiveMap;
                ent.material.emissiveMapPath = emissiveMapPath;
                if (!emissiveMapPath.empty() && fs::exists(emissiveMapPath)) {
                    ent.material.customEmissiveID = LoadTextureFromFile(emissiveMapPath);
                }

                if (!albedoMapPath.empty() && fs::exists(albedoMapPath)) {
                    ent.material.customAlbedoID = LoadTextureFromFile(albedoMapPath);
                    ent.material.useAlbedoMap = 1;
                }
                if (!normalMapPath.empty() && fs::exists(normalMapPath)) {
                    ent.material.customNormalID = LoadTextureFromFile(normalMapPath);
                    ent.material.useNormalMap = 1;
                }
                if (!metallicRoughnessMapPath.empty() && fs::exists(metallicRoughnessMapPath)) {
                    ent.material.customMRID = LoadTextureFromFile(metallicRoughnessMapPath);
                    ent.material.useMetallicRoughnessMap = 1;
                }
                if (!aoMapPath.empty() && fs::exists(aoMapPath)) {
                    ent.material.customAOID = LoadTextureFromFile(aoMapPath);
                    ent.material.useAOMap = 1;
                }
                if (!heightMapPath.empty() && fs::exists(heightMapPath)) {
                    ent.material.customHeightID = LoadTextureFromFile(heightMapPath);
                    ent.material.useHeightMap = 1;
                }

                entities.push_back(ent);

                if (simulatePhysics) {
                    GeneratePhysicsBody(entities.back());
                }
                if (!scriptPath.empty()) {
                    int idx = entities.size() - 1;
                    entities[idx].script = std::make_shared<ScriptComponent>(lua, scriptPath, idx);
                    if (entities[idx].script->isLoaded()) {
                        entities[idx].script->start();
                    }
                }
            }
        }
        file.close();
        std::cout << "Loaded level from " << levelPath << std::endl;
        };

    // ============================================================
    // 23. Lighting defaults
    // ============================================================
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(2.0f, 3.0f, 2.0f);
    glm::vec3 lightPos2 = glm::vec3(-2.0f, 2.0f, -1.0f);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    SpawnRandomLights(entities, 1, 10.0f);


    // ============================================================
    // 24. MAIN LOOP (with updated UI)
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        float deltaTime = 0.016f;

        // ---- Player Update ----
        if (gameMode && playerBodyID != BodyID()) {
            // mouse look, movement, jump (same as before)
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            if (firstMouse) {
                lastMouseX = mouseX;
                lastMouseY = mouseY;
                firstMouse = false;
            }
            float deltaX = (float)(mouseX - lastMouseX);
            float deltaY = (float)(lastMouseY - mouseY);
            lastMouseX = mouseX;
            lastMouseY = mouseY;

            yaw += deltaX * mouseSensitivity;
            pitch += deltaY * mouseSensitivity;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);

            camera.Orientation = glm::normalize(glm::vec3(
                glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
                glm::sin(glm::radians(pitch)),
                glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
            ));

            float forward = 0.0f, right = 0.0f;
            bool jump = false;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward = 1.0f;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward = -1.0f;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right = -1.0f;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right = 1.0f;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) jump = true;

            Vec3 current_velocity = body_interface.GetLinearVelocity(playerBodyID);
            glm::vec3 forwardDir = glm::normalize(glm::vec3(camera.Orientation.x, 0.0f, camera.Orientation.z));
            glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0.0f, 1.0f, 0.0f)));

            float speed = 5.0f;
            Vec3 desired_horizontal_velocity = Vec3(0.0f, 0.0f, 0.0f);
            desired_horizontal_velocity += Vec3(forwardDir.x, 0.0f, forwardDir.z) * forward * speed;
            desired_horizontal_velocity += Vec3(rightDir.x, 0.0f, rightDir.z) * right * speed;

            Vec3 new_velocity = Vec3(desired_horizontal_velocity.GetX(), current_velocity.GetY(), desired_horizontal_velocity.GetZ());
            body_interface.SetLinearVelocity(playerBodyID, new_velocity);

            if (jump && std::abs(current_velocity.GetY()) < 0.1f) {
                body_interface.AddImpulse(playerBodyID, Vec3(0.0f, 6.0f, 0.0f));
            }


            RVec3 player_pos = body_interface.GetPosition(playerBodyID);


            for (auto& entity : entities) {
                if (entity.type == EntityType::Player) {
                    entity.position = glm::vec3(player_pos.GetX(), player_pos.GetY(), player_pos.GetZ());
                    entity.rotation.y = yaw;
                    break;
                }
            }
        }

        // ---- Game Mode Logic ----
        if (gameMode) {
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                if (playerBodyID != BodyID()) {
                    body_interface.SetPosition(playerBodyID, RVec3(0.0_r, 2.0_r, 0.0_r), EActivation::Activate);
                    body_interface.SetLinearVelocity(playerBodyID, Vec3(0.0f, 0.0f, 0.0f));
                    body_interface.SetAngularVelocity(playerBodyID, Vec3(0.0f, 0.0f, 0.0f));
                }
                camera.Position = glm::vec3(0.0f, 3.7f, 0.0f);
                camera.Orientation = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
                yaw = -90.0f;
                pitch = 0.0f;
                for (auto& entity : entities) {
                    if (entity.type == EntityType::Player) {
                        entity.position = glm::vec3(0.0f, 2.0f, 0.0f);
                        entity.rotation.y = 0.0f;
                        break;
                    }
                }
            }

            for (int i = 0; i < (int)entities.size(); ++i) {
                if (entities[i].script && entities[i].script->isLoaded()) {
                    entities[i].script->update(deltaTime);
                }
            }

            // ---- Physics with fixed timestep & interpolation ----
            const float cDeltaTime = 1.0f / 60.0f;
            const int cCollisionSteps = 1;

            // Accumulate time
            physicsAccumulator += deltaTime;

            // While we have enough time, step physics and store previous/current positions
            while (physicsAccumulator >= cDeltaTime) {
                // Store previous position before stepping
                prevPlayerPos = body_interface.GetPosition(playerBodyID);

                // Step physics
                physics_system.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);

                // Store current position after stepping
                currPlayerPos = body_interface.GetPosition(playerBodyID);

                physicsAccumulator -= cDeltaTime;
            }

            // Interpolation factor (0..1) between previous and current
            float alpha = physicsAccumulator / cDeltaTime;

            // Interpolate player position
            glm::vec3 interpPos = glm::mix(
                glm::vec3(prevPlayerPos.GetX(), prevPlayerPos.GetY(), prevPlayerPos.GetZ()),
                glm::vec3(currPlayerPos.GetX(), currPlayerPos.GetY(), currPlayerPos.GetZ()),
                alpha
            );

            // Update camera position with interpolated position
            camera.Position = interpPos + glm::vec3(0.0f, 1.7f, 0.0f);

            for (auto& entity : entities) {
                if (entity.simulatePhysics) {
                    RVec3 pos = body_interface.GetPosition(entity.physicsBodyID);
                    entity.position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
                }
            }

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                gameMode = false;
                entities = copyEntities(savedEntities);
            }
        }
        else {
            camera.Inputs(window);
            for (auto& entity : entities) {
                if (entity.type == EntityType::Player) {
                    entity.scale = glm::vec3(0.5f);
                }
            }
        }

        // ---- Camera Possession ----
        int possessedEntityId = -1;
        for (int i = 0; i < (int)entities.size(); ++i) {
            if (entities[i].isCameraPossessed) {
                possessedEntityId = i;
                break;
            }
        }

        if (possessedEntityId != -1 && !gameMode) {
            const auto& target = entities[possessedEntityId];
            glm::vec3 targetPos = target.position + glm::vec3(0, 1.7f, 0);
            float smoothFactor = 1.0f - exp(-8.0f * deltaTime);
            camera.Position += (targetPos - camera.Position) * smoothFactor;

            float yawRad = glm::radians(target.rotation.y);
            float pitchRad = glm::radians(target.rotation.x);
            glm::vec3 front;
            front.x = cos(yawRad) * cos(pitchRad);
            front.y = sin(pitchRad);
            front.z = sin(yawRad) * cos(pitchRad);
            glm::vec3 targetOrient = glm::normalize(front);
            camera.Orientation += (targetOrient - camera.Orientation) * smoothFactor * 2.0f;
            camera.Orientation = glm::normalize(camera.Orientation);
        }

        // ---- Matrices ----
        glm::mat4 proj = glm::perspective(glm::radians(gameMode ? gameFOV : editorFOV), (float)width / height, 0.1f, 1000.0f);
        glm::mat4 viewMatrix = camera.GetViewMatrix();
        glm::mat4 cameraMatrix = proj * viewMatrix;
        glm::mat4 inverseView = glm::inverse(viewMatrix);
        glm::mat4 inverseProj = glm::inverse(proj);

        camera.cameraMatrix = cameraMatrix;
        Frustum frustum = extractFrustum(cameraMatrix);

        // FPS
        float currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        fpsCounter++;
        fpsTime += deltaTime;
        if (fpsTime >= 1.0f) {
            fps = fpsCounter;
            fpsCounter = 0;
            fpsTime = 0.0f;
        }

        // ---- ImGui ----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ---- تهيئة ImGuizmo ----
        ImGuizmo::BeginFrame();

        // ربط بيانات الإدخال
        ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
        ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());

        // ربط مصفوفات العرض والإسقاط
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(0, 0, (float)width, (float)height);

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) applyUndo();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) applyRedo();

        // ---- Window Visibility ----
        static bool showLevelEditor = true;
        static bool showContentBrowser = true;
        static bool showSkyboxWindow = true;
        static bool showPerformanceWindow = true;
        static bool showPostProcessingWindow = true;

        // ============================================================
        //           MENU BAR (No Emojis, Clean)
        // ============================================================
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Level", "Ctrl+N")) {
                    pushUndo();
                    entities.clear();
                    selectedEntity = -1;
                }
                if (ImGui::MenuItem("Save Level", "Ctrl+S")) {
                    if (fs::exists(levelPath)) showOverwriteWarning = true;
                    else saveLevel();
                }
                if (ImGui::MenuItem("Load Level", "Ctrl+O")) {
                    if (fs::exists(levelPath)) loadLevel();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Level Editor", nullptr, &showLevelEditor);
                ImGui::MenuItem("Content Browser", nullptr, &showContentBrowser);
                ImGui::MenuItem("Skybox Settings", nullptr, &showSkyboxWindow);
                ImGui::MenuItem("Performance Monitor", nullptr, &showPerformanceWindow);
                ImGui::MenuItem("Post-Processing", nullptr, &showPostProcessingWindow);
                ImGui::MenuItem("Entity List", nullptr, &showEntityList);
                ImGui::MenuItem("Entity Properties", nullptr, &showEntityProperties);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Play Mode", "F5")) {
                    if (!gameMode) {
                        savedEntities = copyEntities(entities);
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
                        firstMouse = true;
                        yaw = glm::degrees(atan2(camera.Orientation.z, camera.Orientation.x));
                        pitch = glm::degrees(asin(camera.Orientation.y));
                    }
                    else {
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        entities = copyEntities(savedEntities);
                    }
                    gameMode = !gameMode;
                    if (!gameMode) entities = copyEntities(savedEntities);
                }
                if (ImGui::MenuItem("Generate Physics", "Ctrl+P")) {
                    for (auto& entity : entities) {
                        if (entity.model != nullptr && !entity.simulatePhysics) {
                            GeneratePhysicsBody(entity);
                        }
                    }
                    pushUndo();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("Controls", nullptr, false, false);
                ImGui::MenuItem("About", nullptr, false, false);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // ---- Level Editor Window ----
        if (showLevelEditor) {
            ImGui::Begin("Level Editor", &showLevelEditor);

            ImGui::Text("Level Name:");
            ImGui::SameLine();
            ImGui::InputText("##LevelName", levelNameBuffer, sizeof(levelNameBuffer));
            levelName = levelNameBuffer;
            levelPath = levelFolder + "/" + levelName + ".txt";
            ImGui::Text("Path: %s", levelPath.c_str());
            ImGui::Separator();

            if (ImGui::Button("Save")) {
                if (fs::exists(levelPath)) showOverwriteWarning = true;
                else saveLevel();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                if (fs::exists(levelPath)) loadLevel();
                else std::cout << "Level not found: " << levelPath << std::endl;
            }
            ImGui::SameLine();
            if (ImGui::Button("New")) {
                pushUndo();
                entities.clear();
                selectedEntity = -1;
            }

            ImGui::Separator();
            if (ImGui::Button("Undo")) applyUndo();
            ImGui::SameLine();
            if (ImGui::Button("Redo")) applyRedo();

            ImGui::Separator();
            if (ImGui::Button(gameMode ? "Stop" : "Play")) {
                if (!gameMode) {
                    savedEntities = copyEntities(entities);
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
                    firstMouse = true;
                    yaw = glm::degrees(atan2(camera.Orientation.z, camera.Orientation.x));
                    pitch = glm::degrees(asin(camera.Orientation.y));
                }
                else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    entities = copyEntities(savedEntities);
                }
                gameMode = !gameMode;
                if (!gameMode) entities = copyEntities(savedEntities);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Engine Ready");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "|");
            ImGui::SameLine();
            ImGui::Text("FPS: %.1f  |  Entities: %zu  |  Resolution: %dx%d",
                fps, entities.size(), width, height);

            ImGui::Separator();
            ImGui::Text("Camera FOV");
            if (gameMode) {
                if (ImGui::SliderFloat("Game FOV", &gameFOV, 10.0f, 120.0f)) {
                    camera.updateMatrix(gameFOV, 0.1f, 1000.0f);
                }
            }
            else {
                if (ImGui::SliderFloat("Editor FOV", &editorFOV, 10.0f, 120.0f)) {
                    camera.updateMatrix(editorFOV, 0.1f, 1000.0f);
                }
                else (ImGui::SliderFloat("Game FOV", &gameFOV, 10.0f, 120.0f));
            }

            ImGui::Separator();
            ImGui::Text("Debug View");
            if (ImGui::Combo("##DebugView", &debugView, debugViewNames, 8)) {
                std::cout << "[Debug] View changed to: " << debugViewNames[debugView] << std::endl;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##Debug")) {
                debugView = 0;
            }

            ImGui::Separator();
            ImGui::Text("Lights");

            if (ImGui::Button("Ambient")) {
                EditorEntity lightEntity;
                lightEntity.model = nullptr;
                lightEntity.modelPath = "";
                lightEntity.position = glm::vec3(0.0f, 10.0f, 0.0f);
                lightEntity.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                lightEntity.scale = glm::vec3(1.0f);
                lightEntity.type = EntityType::AmbientLight;
                lightEntity.velocity = glm::vec3(0.0f);
                lightEntity.script = nullptr;
                lightEntity.isCameraPossessed = false;
                lightEntity.simulatePhysics = false;

                // Light defaults
                lightEntity.light.enabled = true;
                lightEntity.light.color = glm::vec3(1.0f, 0.95f, 0.85f); // Slight warm
                lightEntity.light.intensity = 1.0f;
                lightEntity.light.range = 0.0f;              // Not used for directional
                lightEntity.light.innerConeAngle = 0.0f;     // Not used
                lightEntity.light.outerConeAngle = 0.0f;     // Not used
                lightEntity.light.attenuation = 1.0f;

                pushUndo();
                entities.push_back(lightEntity);
                std::cout << "[Editor] Created Ambient Light entity." << std::endl;
            }
            ImGui::SameLine();

            if (ImGui::Button("Directional")) {
                EditorEntity lightEntity;
                lightEntity.model = nullptr;
                lightEntity.modelPath = "";
                lightEntity.position = glm::vec3(0.0f, 10.0f, 0.0f);
                lightEntity.rotation = glm::vec3(0.0f, 0.0f, 0.0f); 
                lightEntity.scale = glm::vec3(1.0f);
                lightEntity.type = EntityType::DirectionalLight;
                lightEntity.velocity = glm::vec3(0.0f);
                lightEntity.script = nullptr;
                lightEntity.isCameraPossessed = false;
                lightEntity.simulatePhysics = false;

                // Light defaults
                lightEntity.light.enabled = true;
                lightEntity.light.color = glm::vec3(1.0f, 0.95f, 0.85f); // Slight warm
                lightEntity.light.intensity = 1.0f;
                lightEntity.light.range = 0.0f;              // Not used for directional
                lightEntity.light.innerConeAngle = 0.0f;     // Not used
                lightEntity.light.outerConeAngle = 0.0f;     // Not used
                lightEntity.light.attenuation = 1.0f;

                pushUndo();
                entities.push_back(lightEntity);
                std::cout << "[Editor] Created Directional Light entity." << std::endl;
            }
            ImGui::SameLine();

            if (ImGui::Button("Point")) {
                EditorEntity lightEntity;
                lightEntity.model = nullptr;
                lightEntity.modelPath = "";
                lightEntity.position = glm::vec3(0.0f, 3.0f, 0.0f);
                lightEntity.rotation = glm::vec3(0.0f);
                lightEntity.scale = glm::vec3(1.0f);
                lightEntity.type = EntityType::PointLight;
                lightEntity.velocity = glm::vec3(0.0f);
                lightEntity.script = nullptr;
                lightEntity.isCameraPossessed = false;
                lightEntity.simulatePhysics = false;

                lightEntity.light.enabled = true;
                lightEntity.light.color = glm::vec3(0.0f, 0.9f, 0.7f);
                lightEntity.light.intensity = 5.0f;
                lightEntity.light.range = 15.0f;
                lightEntity.light.innerConeAngle = 0.0f;
                lightEntity.light.outerConeAngle = 0.0f;
                lightEntity.light.attenuation = 1.0f;

                pushUndo();
                entities.push_back(lightEntity);
                std::cout << "[Editor] Created Point Light entity." << std::endl;
            }
            ImGui::SameLine();

            if (ImGui::Button("Spot")) {
                EditorEntity lightEntity;
                lightEntity.model = nullptr;
                lightEntity.modelPath = "";
                lightEntity.position = glm::vec3(0.0f, 5.0f, 0.0f);
                lightEntity.rotation = glm::vec3(0.0f, 0.0f, 0.0f); 
                lightEntity.scale = glm::vec3(1.0f);
                lightEntity.type = EntityType::SpotLight;
                lightEntity.velocity = glm::vec3(0.0f);
                lightEntity.script = nullptr;
                lightEntity.isCameraPossessed = false;
                lightEntity.simulatePhysics = false;

                lightEntity.light.enabled = true;
                lightEntity.light.color = glm::vec3(1.0f, 1.0f, 1.0f);
                lightEntity.light.intensity = 8.0f;
                lightEntity.light.range = 25.0f;
                lightEntity.light.innerConeAngle = 15.0f;
                lightEntity.light.outerConeAngle = 30.0f;
                lightEntity.light.attenuation = 1.0f;

                pushUndo();
                entities.push_back(lightEntity);
                std::cout << "[Editor] Created Spot Light entity." << std::endl;
            }
            ImGui::Separator();

            ImGui::Text("Gizmo Controls");

            // اختيار الوضع
            const char* modes[] = { "Translate (T)", "Rotate (R)", "Scale (S)" };
            int modeIndex = (int)currentGizmo - 1;
            if (modeIndex < 0) modeIndex = 0;
            if (ImGui::Combo("Mode", &modeIndex, modes, 3)) {
                currentGizmo = (GizmoType)(modeIndex + 1);
            }

            // اختيار النظام (محلي / عالمي)
            static bool useLocal = true;
            ImGui::Checkbox("Local Space", &useLocal);
            ImGuizmo::Enable(useLocal);


            // اختصارات لوحة المفاتيح
            if (ImGui::IsKeyPressed('T')) currentGizmo = GizmoType::Translate;
            if (ImGui::IsKeyPressed('R')) currentGizmo = GizmoType::Rotate;
            if (ImGui::IsKeyPressed('S')) currentGizmo = GizmoType::Scale;
            ImGui::Text("Hotkeys: T=Translate, R=Rotate, S=Scale");

            ImGui::Separator();
            if (ImGui::Button("Reset Player & Camera")) {
                if (playerBodyID != BodyID()) {
                    body_interface.SetPosition(playerBodyID, RVec3(0.0_r, 2.0_r, 0.0_r), EActivation::Activate);
                    body_interface.SetLinearVelocity(playerBodyID, Vec3(0.0f, 0.0f, 0.0f));
                    body_interface.SetAngularVelocity(playerBodyID, Vec3(0.0f, 0.0f, 0.0f));
                }
                camera.Position = glm::vec3(0.0f, 3.7f, 0.0f);
                camera.Orientation = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
                yaw = -90.0f;
                pitch = 0.0f;
                for (auto& entity : entities) {
                    if (entity.type == EntityType::Player) {
                        entity.position = glm::vec3(0.0f, 2.0f, 0.0f);
                        entity.rotation.y = 0.0f;
                        break;
                    }
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(or press R in Play Mode)");

            ImGui::Separator();
            if (ImGui::Button("Generate Physics for All Entities")) {
                for (auto& entity : entities) {
                    if (entity.model != nullptr && !entity.simulatePhysics) {
                        GeneratePhysicsBody(entity);
                    }
                }
                pushUndo();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(creates collisions from models)");

            ImGui::Separator();
            ImGui::Checkbox("Frustum Culling", &enableFrustumCulling);

            // Entities list
            if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {

                // Helper: get readable type name
                auto GetTypeName = [](EntityType t) -> const char* {
                    switch (t) {
                    case EntityType::Static:           return "Static";
                    case EntityType::Player:           return "Player";
                    case EntityType::Ball:             return "Ball";
                    case EntityType::Camera:           return "Camera";
                    case EntityType::Empty:            return "Empty";
                    case EntityType::PointLight:       return "Point Light";
                    case EntityType::SpotLight:        return "Spot Light";
                    case EntityType::DirectionalLight: return "Directional Light";
                    case EntityType::AmbientLight:     return "Ambient Light";
                    }
                    return "Unknown";
                    };

                // Helper: get icon/prefix
                auto GetTypeIcon = [](EntityType t) -> const char* {
                    switch (t) {
                    case EntityType::Static:           return "[M]"; // Mesh
                    case EntityType::Player:           return "[P]";
                    case EntityType::Ball:             return "[B]";
                    case EntityType::Camera:           return "[C]";
                    case EntityType::Empty:            return "[ ]";
                    case EntityType::PointLight:       return "[o]";
                    case EntityType::SpotLight:        return "[/]";
                    case EntityType::DirectionalLight: return "[|]";
                    case EntityType::AmbientLight:     return "[*]";
                    }
                    return "[?]";
                    };

                // Helper: get display name for an entity
                auto GetEntityLabel = [&](int i) -> std::string {
                    const auto& e = entities[i];

                    // Start with icon + index
                    std::string label = std::string(GetTypeIcon(e.type)) + " " + std::to_string(i) + ": ";

                    // Determine the "name" part
                    if (e.type == EntityType::PointLight ||
                        e.type == EntityType::SpotLight ||
                        e.type == EntityType::DirectionalLight ||
                        e.type == EntityType::AmbientLight) {
                        // Lights: show type + color
                        label += GetTypeName(e.type);
                    }
                    else if (e.type == EntityType::Empty) {
                        label += "Empty";
                    }
                    else if (e.type == EntityType::Camera && e.modelPath.empty()) {
                        label += "Camera";
                    }
                    else if (!e.modelPath.empty()) {
                        // Show only filename, not full path
                        std::string filename = fs::path(e.modelPath).filename().string();
                        label += filename;

                        // Add type suffix if not "Static"
                        if (e.type != EntityType::Static) {
                            label += std::string(" (") + GetTypeName(e.type) + ")";
                        }
                    }
                    else {
                        label += GetTypeName(e.type);
                    }

                    return label;
                    };

                // Render the list
                for (int i = 0; i < (int)entities.size(); ++i) {
                    bool isSelected = (selectedEntity == i);
                    std::string label = GetEntityLabel(i);

                    ImGui::PushID(i);

                    // Color-code the label based on type
                    ImVec4 color;
                    switch (entities[i].type) {
                    case EntityType::Player:           color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f); break;
                    case EntityType::Camera:           color = ImVec4(0.6f, 0.7f, 1.0f, 1.0f); break;
                    case EntityType::PointLight:       color = ImVec4(1.0f, 0.9f, 0.4f, 1.0f); break;
                    case EntityType::SpotLight:        color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f); break;
                    case EntityType::DirectionalLight: color = ImVec4(1.0f, 0.95f, 0.5f, 1.0f); break;
                    case EntityType::AmbientLight:     color = ImVec4(0.9f, 0.7f, 1.0f, 1.0f); break;
                    case EntityType::Ball:             color = ImVec4(1.0f, 0.6f, 0.6f, 1.0f); break;
                    case EntityType::Empty:            color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break;
                    default:                            color = ImVec4(0.85f, 0.85f, 0.9f, 1.0f); break;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        selectedEntity = i;
                    }
                    ImGui::PopStyleColor();

                    // Tooltip with full info
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Index: %d", i);
                        ImGui::Text("Type: %s", GetTypeName(entities[i].type));
                        if (!entities[i].modelPath.empty()) {
                            ImGui::Text("Path: %s", entities[i].modelPath.c_str());
                        }
                        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                            entities[i].position.x,
                            entities[i].position.y,
                            entities[i].position.z);
                        ImGui::EndTooltip();
                    }

                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem("EntityContextMenu")) {
                        if (ImGui::MenuItem("Delete")) {
                            pushUndo();
                            entities.erase(entities.begin() + i);
                            if (selectedEntity == i) selectedEntity = -1;
                            else if (selectedEntity > i) selectedEntity--;
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break; // Exit loop after deletion
                        }
                        if (ImGui::MenuItem("Duplicate")) {
                            pushUndo();
                            EditorEntity copy = entities[i];
                            copy.position += glm::vec3(1.0f, 0.0f, 0.0f); // Offset
                            entities.push_back(copy);
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break;
                        }
                        if (ImGui::MenuItem("Focus Camera")) {
                            // Optional: move camera to look at entity
                            camera.Position = entities[i].position + glm::vec3(0, 2, 5);
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                // Show empty message
                if (entities.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No entities in scene");
                }

                // Summary line
                ImGui::Separator();
                int lightCount = 0, meshCount = 0;
                for (const auto& e : entities) {
                    if (e.type == EntityType::PointLight ||
                        e.type == EntityType::SpotLight ||
                        e.type == EntityType::DirectionalLight ||
                        e.type == EntityType::AmbientLight) lightCount++;
                    else if (!e.modelPath.empty()) meshCount++;
                }
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                    "Total: %zu  |  Meshes: %d  |  Lights: %d",
                    entities.size(), meshCount, lightCount);
            }

            // Transform controls
            if (selectedEntity >= 0 && selectedEntity < entities.size()) {
                auto& entity = entities[selectedEntity];

                // بناء مصفوفة التحويل
                glm::mat4 matrix = glm::mat4(1.0f);
                matrix = glm::translate(matrix, entity.position);
                matrix = glm::rotate(matrix, glm::radians(entity.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                matrix = glm::rotate(matrix, glm::radians(entity.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                matrix = glm::rotate(matrix, glm::radians(entity.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                matrix = glm::scale(matrix, entity.scale);

                // اختيار وضع Gizmo
                ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
                if (currentGizmo == GizmoType::Translate) operation = ImGuizmo::TRANSLATE;
                else if (currentGizmo == GizmoType::Rotate) operation = ImGuizmo::ROTATE;
                else if (currentGizmo == GizmoType::Scale) operation = ImGuizmo::SCALE;

                // ✅ استخدم التوقيع الصحيح (8 معاملات)
                if (ImGuizmo::Manipulate(
                    glm::value_ptr(viewMatrix),              // view
                    glm::value_ptr(proj),                    // projection
                    operation,                               // OPERATION
                    useLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD,  // MODE
                    glm::value_ptr(matrix)                   // matrix (مدخل/مخرج)
                    // يمكنك إضافة معاملات إضافية اختيارية:
                    // NULL,  // deltaMatrix
                    // NULL,  // snap
                    // NULL,  // localBounds
                    // NULL   // boundsSnap
                )) {
                    // استخراج المصفوفة الناتجة
                    glm::vec3 translation, scale;
                    glm::quat rotation;
                    glm::vec3 skew;
                    glm::vec4 perspective;

                    if (glm::decompose(matrix, scale, rotation, translation, skew, perspective)) {
                        entity.position = translation;
                        entity.rotation = glm::degrees(glm::eulerAngles(rotation));
                        entity.scale = scale;
                        pushUndo();
                        std::cout << "Entity transformed!" << std::endl;
                    }
                }

                ImGui::Separator();
                ImGui::Text("Selected: %d", selectedEntity);

                bool changed = false;
                if (ImGui::DragFloat3("Position", &entity.position.x, 0.1f)) changed = true;
                if (ImGui::DragFloat3("Rotation (deg)", &entity.rotation.x, 1.0f)) changed = true;
                if (ImGui::DragFloat3("Scale", &entity.scale.x, 0.05f)) changed = true;

                if (changed && ImGui::IsItemDeactivatedAfterEdit()) {
                    pushUndo();
                }

                const char* typeNames[] = { "Static", "Player", "Ball", "Camera", "Empty", "Point Light", "Spot Light", "Directional Light", "Ambient Light" };
                int typeIndex = static_cast<int>(entity.type);
                if (ImGui::Combo("Type", &typeIndex, typeNames, 9)) {
                    pushUndo();
                    entity.type = static_cast<EntityType>(typeIndex);
                }

                // ============================================================
                //                      SCRIPT SELECTOR
                // ============================================================
                // Only for non-light entities
                if (entity.type != EntityType::PointLight &&
                    entity.type != EntityType::SpotLight &&
                    entity.type != EntityType::DirectionalLight &&
                    entity.type != EntityType::AmbientLight)
                {
                    if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen)) {

                        // Lazy-scan on first open
                        if (!scriptsScanned) {
                            scriptFiles = scanScriptsFolder("Scripts/");
                            scriptsScanned = true;
                            std::cout << "[Scripts] Found " << scriptFiles.size() << " script(s)." << std::endl;
                        }

                        // ---- Current script display ----
                        std::string currentScript = "";
                        bool hasScript = (entity.script && entity.script->isLoaded());
                        if (hasScript) currentScript = entity.script->scriptPath;

                        std::string displayName = hasScript
                            ? fs::path(currentScript).filename().string()
                            : "(none)";

                        ImGui::Text("Current:");
                        ImGui::SameLine();
                        ImGui::TextColored(
                            hasScript ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                            "%s", displayName.c_str());

                        // ---- Combo to pick a script ----
                        if (ImGui::BeginCombo("##ScriptCombo", displayName.c_str())) {

                            // ---- "(none)" option ----
                            bool noneSelected = !hasScript;
                            if (ImGui::Selectable("(none)", noneSelected)) {
                                pushUndo();
                                if (entity.script) entity.script->destroy();
                                entity.script = nullptr;
                                std::cout << "[Script] Cleared script from entity " << selectedEntity << std::endl;
                            }
                            if (noneSelected) ImGui::SetItemDefaultFocus();

                            // ---- Script list ----
                            for (const auto& scriptPath : scriptFiles) {
                                std::string filename = fs::path(scriptPath).filename().string();
                                bool isSelected = (scriptPath == currentScript);

                                ImGui::PushID(scriptPath.c_str());
                                if (ImGui::Selectable(filename.c_str(), isSelected)) {
                                    pushUndo();
                                    // Destroy previous
                                    if (entity.script) entity.script->destroy();

                                    // Create new
                                    entity.script = std::make_shared<ScriptComponent>(
                                        lua, scriptPath, selectedEntity);

                                    if (entity.script->isLoaded()) {
                                        entity.script->start();
                                        std::cout << "[Script] Loaded and started: " << filename
                                            << " on entity " << selectedEntity << std::endl;
                                    }
                                    else {
                                        std::cout << "[Script] Failed to load: " << scriptPath << std::endl;
                                    }
                                }
                                if (isSelected) ImGui::SetItemDefaultFocus();

                                // Tooltip with full path
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("%s", scriptPath.c_str());
                                }
                                ImGui::PopID();
                            }

                            ImGui::EndCombo();
                        }

                        // ---- Buttons row ----
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Refresh")) {
                            scriptFiles = scanScriptsFolder("Scripts/");
                            std::cout << "[Scripts] Rescanned: " << scriptFiles.size() << " script(s)." << std::endl;
                        }

                        if (hasScript) {
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Reload")) {
                                pushUndo();
                                std::string path = entity.script->scriptPath;
                                entity.script->destroy();
                                entity.script = std::make_shared<ScriptComponent>(
                                    lua, path, selectedEntity);
                                if (entity.script->isLoaded()) {
                                    entity.script->start();
                                    std::cout << "[Script] Reloaded: " << path << std::endl;
                                }
                            }

                            if (ImGui::SmallButton("Clear")) {
                                pushUndo();
                                entity.script->destroy();
                                entity.script = nullptr;
                                std::cout << "[Script] Cleared from entity " << selectedEntity << std::endl;
                            }
                        }

                        // ---- Info line ----
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                            "%zu script(s) available in Scripts/", scriptFiles.size());
                    }


                }

                // ============================================================
//                   SCRIPT PARAMETERS UI
// ============================================================
// Only show if the entity has a script OR already has params
                if (entity.script || !entity.scriptParams.empty()) {
                    if (ImGui::CollapsingHeader("Script Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& params = entity.scriptParams;
                        int toDelete = -1;

                        // ---- Table ----
                        if (ImGui::BeginTable("ScriptParamTable", 3,
                            ImGuiTableFlags_Borders |
                            ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_SizingStretchProp))
                        {
                            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.4f);
                            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                            ImGui::TableSetupColumn("##act", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                            ImGui::TableHeadersRow();

                            for (int i = 0; i < (int)params.size(); ++i) {
                                ImGui::PushID(i);
                                ImGui::TableNextRow();

                                // ---- Name ----
                                ImGui::TableNextColumn();
                                char nameBuf[64];
                                strncpy_s(nameBuf, sizeof(nameBuf), params[i].name.c_str(), _TRUNCATE);
                                ImGui::SetNextItemWidth(-1);
                                if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf))) {
                                    params[i].name = nameBuf;
                                }

                                // ---- Value ----
                                ImGui::TableNextColumn();
                                ImGui::SetNextItemWidth(-1);
                                ImGui::DragFloat("##value", &params[i].value, 0.01f);

                                // ---- Delete ----
                                ImGui::TableNextColumn();
                                if (ImGui::SmallButton("X")) {
                                    toDelete = i;
                                }
                                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete parameter");

                                ImGui::PopID();
                            }

                            ImGui::EndTable();
                        }

                        // Handle deferred deletion (safe after table)
                        if (toDelete >= 0) {
                            pushUndo();
                            params.erase(params.begin() + toDelete);
                        }

                        // ---- Action buttons ----
                        if (ImGui::Button("+ Add Parameter")) {
                            pushUndo();
                            params.push_back({ "new_param", 0.0f });
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear All")) {
                            pushUndo();
                            params.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reload Script")) {
                            // Convenience: reload script after adding params
                            if (entity.script && entity.script->isLoaded()) {
                                std::string path = entity.script->scriptPath;
                                entity.script->destroy();
                                entity.script = std::make_shared<ScriptComponent>(
                                    lua, path, selectedEntity);
                                if (entity.script->isLoaded()) entity.script->start();
                            }
                        }

                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                            "%zu parameter(s)", params.size());
                    }
                }

                if (ImGui::Button("Delete")) {
                    pushUndo();
                    entities.erase(entities.begin() + selectedEntity);
                    selectedEntity = -1;
                }

                // ---- Light Properties (للإضاءات فقط) ----
                if (entity.type == EntityType::PointLight ||
                    entity.type == EntityType::SpotLight ||
                    entity.type == EntityType::DirectionalLight ||
                    entity.type == EntityType::AmbientLight) {

                    if (ImGui::CollapsingHeader("Light Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& light = entity.light;
                        bool lightChanged = false;

                        lightChanged |= ImGui::Checkbox("Enabled", &light.enabled);
                        lightChanged |= ImGui::ColorEdit3("Color", &light.color[0]);
                        lightChanged |= ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);

                        // خصائص خاصة حسب نوع الإضاءة
                        if (entity.type == EntityType::PointLight || entity.type == EntityType::SpotLight) {
                            lightChanged |= ImGui::SliderFloat("Range", &light.range, 0.1f, 50.0f);
                        }

                        if (entity.type == EntityType::SpotLight) {
                            lightChanged |= ImGui::SliderFloat("Inner Cone", &light.innerConeAngle, 0.0f, 45.0f);
                            lightChanged |= ImGui::SliderFloat("Outer Cone", &light.outerConeAngle, 1.0f, 90.0f);
                        }

                        if (lightChanged && ImGui::IsItemDeactivatedAfterEdit()) {
                            pushUndo();
                        }
                    }
                }
                else if (entity.type != EntityType::Empty &&
                    entity.type != EntityType::Camera)  {
                    // ---- Material Overrides ----
                    if (ImGui::CollapsingHeader("Material Overrides", ImGuiTreeNodeFlags_DefaultOpen)) {
                    Material& mat = entity.material;
                    bool matChanged = false;

                    // 1. Albedo
                    ImGui::Checkbox("Use Albedo Map", (bool*)&mat.useAlbedoMap);
                    if (mat.useAlbedoMap) {
                        if (ImGui::Button("Load Albedo Map##Albedo")) {
                            std::string path = OpenFileDialog("Select Albedo Texture", "Image Files\0*.jpg;*.png;*.bmp;*.tga\0", window);
                            if (!path.empty()) {
                                pushUndo();
                                mat.albedoMapPath = path;
                                mat.customAlbedoID = LoadTextureFromFile(path);
                                mat.useAlbedoMap = 1;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##Albedo")) {
                            pushUndo();
                            mat.albedoMapPath = "";
                            mat.customAlbedoID = 0;
                            mat.useAlbedoMap = 0;
                        }
                        ImGui::Text("Path: %s", mat.albedoMapPath.empty() ? "(none)" : mat.albedoMapPath.c_str());
                    }
                    else {
                        if (ImGui::ColorEdit3("Albedo Color", &mat.albedo[0])) matChanged = true;
                    }

                    // 2. Normal Map
                    ImGui::Checkbox("Use Normal Map", (bool*)&mat.useNormalMap);
                    if (mat.useNormalMap) {
                        if (ImGui::Button("Load Normal Map##Normal")) {
                            std::string path = OpenFileDialog("Select Normal Map", "Image Files\0*.jpg;*.png;*.bmp;*.tga\0", window);
                            if (!path.empty()) {
                                pushUndo();
                                mat.normalMapPath = path;
                                mat.customNormalID = LoadTextureFromFile(path);
                                mat.useNormalMap = 1;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##Normal")) {
                            pushUndo();
                            mat.normalMapPath = "";
                            mat.customNormalID = 0;
                            mat.useNormalMap = 0;
                        }
                        ImGui::Text("Path: %s", mat.normalMapPath.empty() ? "(none)" : mat.normalMapPath.c_str());
                    }

                    // 3. Metallic/Roughness
                    ImGui::Checkbox("Use Metallic/Roughness Map", (bool*)&mat.useMetallicRoughnessMap);
                    if (mat.useMetallicRoughnessMap) {
                        if (ImGui::Button("Load MR Map##MR")) {
                            std::string path = OpenFileDialog("Select Metallic/Roughness Map", "Image Files\0*.jpg;*.png;*.bmp;*.tga\0", window);
                            if (!path.empty()) {
                                pushUndo();
                                mat.metallicRoughnessMapPath = path;
                                mat.customMRID = LoadTextureFromFile(path);
                                mat.useMetallicRoughnessMap = 1;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##MR")) {
                            pushUndo();
                            mat.metallicRoughnessMapPath = "";
                            mat.customMRID = 0;
                            mat.useMetallicRoughnessMap = 0;
                        }
                        ImGui::Text("Path: %s", mat.metallicRoughnessMapPath.empty() ? "(none)" : mat.metallicRoughnessMapPath.c_str());
                    }
                    else {
                        if (ImGui::SliderFloat("Metallic", &mat.metallic, 0.0f, 1.0f)) matChanged = true;
                        if (ImGui::SliderFloat("Roughness", &mat.roughness, 0.0f, 1.0f)) matChanged = true;
                    }

                    // 4. AO Map
                    ImGui::Checkbox("Use AO Map", (bool*)&mat.useAOMap);
                    if (mat.useAOMap) {
                        if (ImGui::Button("Load AO Map##AO")) {
                            std::string path = OpenFileDialog("Select AO Map", "Image Files\0*.jpg;*.png;*.bmp;*.tga\0", window);
                            if (!path.empty()) {
                                pushUndo();
                                mat.aoMapPath = path;
                                mat.customAOID = LoadTextureFromFile(path);
                                mat.useAOMap = 1;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##AO")) {
                            pushUndo();
                            mat.aoMapPath = "";
                            mat.customAOID = 0;
                            mat.useAOMap = 0;
                        }
                        ImGui::Text("Path: %s", mat.aoMapPath.empty() ? "(none)" : mat.aoMapPath.c_str());
                    }
                    else {
                        if (ImGui::SliderFloat("AO", &mat.ao, 0.0f, 1.0f)) matChanged = true;
                    }

                    // 5. Height Map
                    ImGui::Checkbox("Use Height Map", (bool*)&mat.useHeightMap);
                    if (mat.useHeightMap) {
                        if (ImGui::Button("Load Height Map##Height")) {
                            std::string path = OpenFileDialog("Select Height Map", "Image Files\0*.jpg;*.png;*.bmp;*.tga\0", window);
                            if (!path.empty()) {
                                pushUndo();
                                mat.heightMapPath = path;
                                mat.customHeightID = LoadTextureFromFile(path);
                                mat.useHeightMap = 1;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##Height")) {
                            pushUndo();
                            mat.heightMapPath = "";
                            mat.customHeightID = 0;
                            mat.useHeightMap = 0;
                        }
                        ImGui::Text("Path: %s", mat.heightMapPath.empty() ? "(none)" : mat.heightMapPath.c_str());
                        if (ImGui::SliderFloat("Height Scale", &mat.heightScale, 0.0f, 0.2f)) matChanged = true;
                    }

                    // 6. Emissive Map
                    ImGui::Checkbox("Use Emissive Map", (bool*)&mat.useEmissiveMap);
                    if (mat.useEmissiveMap) {
                        if (ImGui::Button("Load Emissive Map##Emissive")) {
                            std::string path = OpenFileDialog("Select Emissive Map", "Image Files\0*.jpg;*.png;*.bmp;*.tga\0", window);
                            if (!path.empty()) {
                                pushUndo();
                                mat.emissiveMapPath = path;
                                mat.customEmissiveID = LoadTextureFromFile(path);
                                mat.useEmissiveMap = 1;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##Emissive")) {
                            pushUndo();
                            mat.emissiveMapPath = "";
                            mat.customEmissiveID = 0;
                            mat.useEmissiveMap = 0;
                        }
                        ImGui::Text("Path: %s", mat.emissiveMapPath.empty() ? "(none)" : mat.emissiveMapPath.c_str());
                        if (ImGui::SliderFloat("Emissive Intensity", &mat.emissiveIntensity, 0.0f, 10.0f)) matChanged = true;
                    }
                    else {
                        if (ImGui::ColorEdit3("Emissive Color", &mat.emissiveColor[0])) matChanged = true;
                        if (ImGui::SliderFloat("Emissive Intensity", &mat.emissiveIntensity, 0.0f, 10.0f)) matChanged = true;
                    }

                    if (matChanged && ImGui::IsItemDeactivatedAfterEdit()) {
                        pushUndo();
                    }
                }
                }
            }

            ImGui::End();
        }

        // ---- Post-Processing Window ----
        if (showPostProcessingWindow) {
            ImGui::Begin("Post-Processing", &showPostProcessingWindow);

            if (ImGui::BeginTabBar("PostProcessingTabs")) {
                // Color Grading
                if (ImGui::BeginTabItem("Color Grading")) {
                    ImGui::SliderFloat("Saturation", &saturation, 0.0f, 2.0f);
                    ResetSliderButton("Saturation", &saturation, 1.0f);
                    ImGui::SliderFloat("Contrast", &contrast, 0.5f, 2.0f);
                    ResetSliderButton("Contrast", &contrast, 1.0f);
                    ImGui::SliderFloat("Gamma", &gamma, 1.0f, 3.0f);
                    ResetSliderButton("Gamma", &gamma, 2.2f);
                    ImGui::SliderFloat("Exposure", &exposure, 0.0f, 3.0f);
                    ResetSliderButton("Exposure", &exposure, 1.5f);
                    ImGui::EndTabItem();
                }

                // Bloom
                if (ImGui::BeginTabItem("Bloom")) {
                    ImGui::Checkbox("Enable Bloom", &enableBloom);
                    ImGui::SliderFloat("Threshold", &bloomThreshold, 0.0f, 2.0f);
                    ResetSliderButton("BloomThreshold", &bloomThreshold, 0.5f);
                    ImGui::SliderFloat("Intensity", &bloomIntensity, 0.0f, 2.0f);
                    ResetSliderButton("BloomIntensity", &bloomIntensity, 0.4f);
                    ImGui::EndTabItem();
                }

                // SSAO
                if (ImGui::BeginTabItem("SSAO")) {
                    ImGui::Checkbox("Enable SSAO", &enableSSAO);
                    ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.8f, 1.0f),
                        "Radius: %.2f  |  Bias: %.3f  |  Power: %.1f",
                        ssaoRadius, ssaoBias, ssaoPower);
                    ImGui::EndTabItem();
                }

                // Volumetric Fog
                if (ImGui::BeginTabItem("Fog")) {
                    ImGui::Checkbox("Enable Fog", &enableVolumetricFog);
                    ImGui::SliderFloat("Density", &fogDensity, 0.0f, 0.5f);
                    ResetSliderButton("FogDensity", &fogDensity, 0.05f);
                    ImGui::SliderFloat("Height", &fogHeight, -10.0f, 10.0f);
                    ResetSliderButton("FogHeight", &fogHeight, 0.5f);
                    ImGui::SliderFloat("Falloff", &fogFalloff, 0.1f, 5.0f);
                    ResetSliderButton("FogFalloff", &fogFalloff, 2.0f);
                    ImGui::SliderInt("Steps", &fogSteps, 16, 128);
                    ResetSliderButton("FogSteps", (float*)&fogSteps, 64.0f);
                    ImGui::SliderFloat("Max Distance", &fogMaxDistance, 50.0f, 500.0f);
                    ResetSliderButton("FogMaxDist", &fogMaxDistance, 200.0f);
                    ImGui::ColorEdit3("Fog Color", &fogColor[0]);
                    ImGui::EndTabItem();
                }

                // Shadows (Contact Shadows)
                if (ImGui::BeginTabItem("Shadows")) {
                    ImGui::Checkbox("Contact Shadows", &enableContactShadows);
                    ImGui::EndTabItem();
                }

                // Anti-Aliasing (FXAA)
                if (ImGui::BeginTabItem("Anti-Aliasing")) {
                    ImGui::Checkbox("FXAA", &enableFXAA);
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        // ---- Content Browser ----
        if (showContentBrowser) {
            ImGui::Begin("Content Browser", &showContentBrowser, ImGuiWindowFlags_NoCollapse);

            ImGui::Text("Models Folder");
            if (ImGui::Button("Refresh Models")) {
                modelFiles = scanModelsFolder("models/");
                std::cout << "Found " << modelFiles.size() << " model files." << std::endl;
            }

            if (ImGui::BeginChild("ModelList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), 0)) {
                if (modelFiles.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No models found. Click Refresh.");
                }
                else {
                    for (int i = 0; i < (int)modelFiles.size(); ++i) {
                        const auto& filePath = modelFiles[i];
                        std::string fileName = fs::path(filePath).filename().string();
                        ImGui::PushID(i);
                        bool isSelected = (selectedModelPath == filePath);
                        if (ImGui::Selectable(fileName.c_str(), isSelected)) {
                            selectedModelPath = filePath;
                            std::cout << " Selected: " << fileName << std::endl;
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", filePath.c_str());
                        }
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChild();

            if (!selectedModelPath.empty()) {
                ImGui::Text("Selected: %s", fs::path(selectedModelPath).filename().string().c_str());
                if (ImGui::Button(" Import Selected Model")) {
                    std::string normalizedPath = selectedModelPath;
                    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

                    if (!fs::exists(normalizedPath)) {
                        std::cout << "[Import] File not found: " << normalizedPath << std::endl;
                        std::string filename = fs::path(normalizedPath).filename().string();
                        std::cout << "[Import] Searching for: " << filename << " in models/..." << std::endl;

                        bool found = false;
                        try {
                            for (const auto& entry : fs::recursive_directory_iterator(
                                "models/",
                                fs::directory_options::skip_permission_denied))
                            {
                                if (!entry.is_regular_file()) continue;

                                std::string entryPath = entry.path().string();
                                std::replace(entryPath.begin(), entryPath.end(), '\\', '/');

                                if (entryPath.find(filename) == std::string::npos) continue;

                                size_t pos = entryPath.find("models/");
                                if (pos != std::string::npos) {
                                    normalizedPath = entryPath.substr(pos);
                                    std::cout << "[Import] Found alternative path: "
                                        << normalizedPath << std::endl;
                                    found = true;
                                    break;
                                }
                            }
                        }
                        catch (const std::exception& e) {
                            std::cout << "[Import] Directory scan error: " << e.what() << std::endl;
                        }

                        if (!found) {
                            std::cout << "[Import] Could not locate file: " << filename << std::endl;
                            selectedModelPath = "";
                            goto import_skip;
                        }
                    }

                import_skip:

                    int existingIndex = -1;
                    for (int i = 0; i < (int)modelPaths.size(); ++i) {
                        if (modelPaths[i] == normalizedPath) {
                            existingIndex = i;
                            break;
                        }
                    }

                    Model* modelToUse = nullptr;
                    bool success = true;

                    if (existingIndex != -1) {
                        modelToUse = loadedModels[existingIndex];
                        std::cout << "[Import] Using already loaded model: " << normalizedPath << std::endl;
                    }
                    else {
                        if (!fs::exists(normalizedPath)) {
                            std::cout << "[Import] File still not found: " << normalizedPath << std::endl;
                            selectedModelPath = "";
                            success = false;
                        }
                        else {
                            try {
                                // ---------- FBX detection added here ----------
                                Model* newModel = nullptr;
                                std::string ext = fs::path(normalizedPath).extension().string();
                                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                                if (ext == ".fbx" || ext == ".FBX") {
                                    newModel = FBXLoader::Load(normalizedPath);
                                    if (newModel) {
                                        std::cout << "[Import] FBX loaded via FBXLoader" << std::endl;
                                    }
                                }
                                else {
                                    newModel = new Model(normalizedPath.c_str());
                                }
                                // ---------- end of added block ----------

                                if (newModel && !newModel->meshes.empty()) {
                                    loadedModels.push_back(newModel);
                                    modelPaths.push_back(normalizedPath);
                                    modelToUse = newModel;
                                    std::cout << "[Import] Model loaded successfully: " << normalizedPath << std::endl;
                                }
                                else {
                                    std::cout << "[Import] Model has no meshes (corrupted?): " << normalizedPath << std::endl;
                                    delete newModel;
                                    success = false;
                                }
                            }
                            catch (const std::exception& e) {
                                std::cout << "[Import] Exception loading model: " << e.what() << std::endl;
                                success = false;
                            }
                            catch (...) {
                                std::cout << "[Import] Unknown exception loading model: " << normalizedPath << std::endl;
                                success = false;
                            }
                        }
                    }

                    if (success && modelToUse != nullptr && !modelToUse->meshes.empty()) {
                        pushUndo();
                        Material defaultMat;
                        defaultMat.roughness = 0.5f;
                        defaultMat.useAlbedoMap = 1;
                        defaultMat.useNormalMap = 0;
                        defaultMat.useMetallicRoughnessMap = 0;
                        defaultMat.useAOMap = 0;
                        defaultMat.useHeightMap = 0;
                        defaultMat.useEmissiveMap = 0;
                        defaultMat.emissiveIntensity = 1.0f;



                        if (modelToUse && !modelToUse->meshes.empty()) {
                            for (const auto& mesh : modelToUse->meshes) {
                                for (const auto& tex : mesh.textures) {
                                    std::string texPath = tex.path;
                                    std::string texName = fs::path(texPath).filename().string();
                                    std::string lowerName = texName;
                                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                                    std::string detectedType = tex.type;
                                    if (detectedType == "diffuse" || detectedType == "baseColor" || detectedType == "albedo") {
                                        if (lowerName.find("emissive") != std::string::npos) {
                                            detectedType = "emissive";
                                        }
                                        else if (lowerName.find("metallic") != std::string::npos ||
                                            lowerName.find("roughness") != std::string::npos ||
                                            lowerName.find("metalness") != std::string::npos) {
                                            detectedType = "metallicRoughness";
                                        }
                                        else if (lowerName.find("ao") != std::string::npos ||
                                            lowerName.find("ambient") != std::string::npos ||
                                            lowerName.find("occlusion") != std::string::npos) {
                                            detectedType = "ao";
                                        }
                                        else if (lowerName.find("height") != std::string::npos ||
                                            lowerName.find("displacement") != std::string::npos) {
                                            detectedType = "height";
                                        }
                                        else {
                                            detectedType = "albedo";
                                        }
                                    }
                                    else if (detectedType == "normal" || detectedType == "normalMap") {
                                        detectedType = "normal";
                                    }

                                    if (detectedType == "albedo" || detectedType == "diffuse" || detectedType == "baseColor") {
                                        if (tex.ID != 0 && defaultMat.customAlbedoID == 0) {
                                            defaultMat.customAlbedoID = tex.ID;
                                            defaultMat.useAlbedoMap = 1;
                                            defaultMat.albedoMapPath = texPath;
                                            std::cout << "[Import] Assigned albedo texture: " << texName << " (ID=" << tex.ID << ")" << std::endl;
                                        }
                                    }
                                    else if (detectedType == "normal") {
                                        if (tex.ID != 0 && defaultMat.customNormalID == 0) {
                                            defaultMat.customNormalID = tex.ID;
                                            defaultMat.useNormalMap = 1;
                                            defaultMat.normalMapPath = texPath;
                                            std::cout << "[Import] Assigned normal texture: " << texName << " (ID=" << tex.ID << ")" << std::endl;
                                        }
                                    }
                                    else if (detectedType == "metallicRoughness" || detectedType == "metallic" || detectedType == "roughness") {
                                        if (tex.ID != 0 && defaultMat.customMRID == 0) {
                                            defaultMat.customMRID = tex.ID;
                                            defaultMat.useMetallicRoughnessMap = 1;
                                            defaultMat.metallicRoughnessMapPath = texPath;
                                            std::cout << "[Import] Assigned metallicRoughness texture: " << texName << " (ID=" << tex.ID << ")" << std::endl;
                                        }
                                    }
                                    else if (detectedType == "ao") {
                                        if (tex.ID != 0 && defaultMat.customAOID == 0) {
                                            defaultMat.customAOID = tex.ID;
                                            defaultMat.useAOMap = 1;
                                            defaultMat.aoMapPath = texPath;
                                            std::cout << "[Import] Assigned AO texture: " << texName << " (ID=" << tex.ID << ")" << std::endl;
                                        }
                                    }
                                    else if (detectedType == "height" || detectedType == "displacement") {
                                        if (tex.ID != 0 && defaultMat.customHeightID == 0) {
                                            defaultMat.customHeightID = tex.ID;
                                            defaultMat.useHeightMap = 1;
                                            defaultMat.heightMapPath = texPath;
                                            std::cout << "[Import] Assigned height texture: " << texName << " (ID=" << tex.ID << ")" << std::endl;
                                        }
                                    }
                                    else if (detectedType == "emissive") {
                                        if (tex.ID != 0 && defaultMat.customEmissiveID == 0) {
                                            defaultMat.customEmissiveID = tex.ID;
                                            defaultMat.useEmissiveMap = 1;
                                            defaultMat.emissiveMapPath = texPath;
                                            std::cout << "[Import] Assigned emissive texture: " << texName << " (ID=" << tex.ID << ")" << std::endl;
                                        }
                                    }
                                }
                            }
                        }

                        entities.push_back(EditorEntity{
                            modelToUse, normalizedPath,
                            glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(1,1,1),
                            EntityType::Static,
                            glm::vec3(0),
                            nullptr,
                            defaultMat,
                            false,
                            false
                            });
                        std::cout << "[Import] Added new entity with model: " << normalizedPath << std::endl;
                        selectedModelPath = "";
                    }
                    else {
                        if (!success) {
                            std::cout << "[Import] Import failed for: " << normalizedPath << std::endl;
                        }
                        selectedModelPath = "";
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    selectedModelPath = "";
                }
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a model from the list above");
            }

            ImGui::End();
        }

        // ---- Skybox Window ----
        if (showSkyboxWindow) {
            ImGui::Begin("Skybox", &showSkyboxWindow);

            ImGui::Checkbox("Show Skybox", &showSkybox);

            if (!skyboxFolders.empty()) {
                if (ImGui::Combo("Skybox Set", &selectedSkyboxIndex, [](void* data, int idx, const char** out_text) {
                    auto* vec = (std::vector<std::string>*)data;
                    *out_text = (*vec)[idx].c_str();
                    return true;
                    }, &skyboxFolders, (int)skyboxFolders.size())) {
                    std::string selected = skyboxFolders[selectedSkyboxIndex];
                    std::string folderPath = (selected == ".") ? "cubemap" : "cubemap/" + selected;
                    LoadSkyboxFromFolder(folderPath, envCubemap);
                    currentSkyboxFolder = selected;
                    std::cout << "[Skybox] Switched to: " << selected << std::endl;
                }
            }


            // ===== انعكاسات البيئة =====
            ImGui::Checkbox("Enable Reflections", &enableEnvReflections);
            ImGui::SliderFloat("Reflection Intensity", &envReflectionIntensity, 0.0f, 2.0f);

            ImGui::Separator();

            // ===== ألوان السماء (تظهر فقط عند إخفاء الـ Skybox) =====
            if (!showSkybox) {
                ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Gradient Sky (when Skybox is off)");
                ImGui::ColorEdit3("Top Color", &customTopColor[0]);
                ImGui::ColorEdit3("Horizon Color", &customHorizonColor[0]);
                ImGui::ColorEdit3("Bottom Color", &customBottomColor[0]);

                if (ImGui::CollapsingHeader("Sun Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::ColorEdit3("Sun Color", &customSunColor[0]);
                    ImGui::DragFloat3("Sun Direction", &customSunDirection[0], 0.05f, -1.0f, 1.0f);
                    ImGui::SliderFloat("Sun Intensity", &customSunIntensity, 0.0f, 2.0f);
                }
                if (ImGui::CollapsingHeader("Cloud Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderFloat("Cloud Density", &customCloudDensity, 0.0f, 4.0f);
                    ImGui::SliderFloat("Cloud Opacity", &customCloudOpacity, 0.0f, 1.0f);
                }
            }
            else {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Skybox Cube is active");
                ImGui::Text("(Uses cubemap images from 'cubemap/' folder)");
            }

            ImGui::End();
        }

        // ---- Performance Overlay ----
        if (showPerformanceWindow) {
            ImGui::Begin("Performance Monitor", &showPerformanceWindow, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.2f ms", deltaTime * 1000.0f);
            ImGui::Text("Entities: %zu", entities.size());
            ImGui::End();
        }

        // ---- Shadow Map Pass ----
        glm::vec3 lightPosWorld = -lightDir * 20.0f;
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 30.0f);
        glm::mat4 lightView = glm::lookAt(lightPosWorld, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        for (auto& entity : entities) {
            if (entity.type == EntityType::PointLight ||
                entity.type == EntityType::SpotLight ||
                entity.type == EntityType::DirectionalLight ||
                entity.type == EntityType::AmbientLight) {
                continue;
            }

            if (entity.model == nullptr) continue;
            if (enableFrustumCulling) {
                float radius = 1.0f;
                if (!isSphereInFrustum(frustum, entity.position, radius)) continue;
            }
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.x), glm::vec3(1, 0, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.y), glm::vec3(0, 1, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.z), glm::vec3(0, 0, 1));
            modelMat = glm::translate(modelMat, entity.position);
            modelMat = glm::scale(modelMat, entity.scale);
            glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
            entity.model->DrawDepth(depthShader, modelMat);
        }

        // ---- G-Buffer Pass ----
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gBufferShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(gBufferShader.ID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
        glUniform3f(glGetUniformLocation(gBufferShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useNormalMap"), 1);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useMetallicRoughness"), 1);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useHeightMap"), 1);

        for (auto& entity : entities) {
            if (entity.type == EntityType::PointLight ||
                entity.type == EntityType::SpotLight ||
                entity.type == EntityType::DirectionalLight ||
                entity.type == EntityType::AmbientLight) {
                continue;
            }

            if (enableFrustumCulling) {
                float radius = 1.0f;
                if (!isSphereInFrustum(frustum, entity.position, radius)) continue;
            }

            glUniform3f(uAlbedoLoc, entity.material.albedo.x, entity.material.albedo.y, entity.material.albedo.z);
            glUniform1f(uMetallicLoc, entity.material.metallic);
            glUniform1f(uRoughnessLoc, entity.material.roughness);
            glUniform1f(uAOLoc, entity.material.ao);

            GLuint albedoTexID = whiteTexture;
            bool useAlbedo = false;
            if (entity.material.useAlbedoMap && entity.material.customAlbedoID != 0) {
                albedoTexID = entity.material.customAlbedoID;
                useAlbedo = true;
            }
            else if (entity.model && !entity.model->meshes.empty()) {
                for (const auto& mesh : entity.model->meshes) {
                    for (const auto& tex : mesh.textures) {
                        if (tex.type == "diffuse" && tex.ID != 0) {
                            albedoTexID = tex.ID;
                            useAlbedo = true;
                            break;
                        }
                    }
                    if (useAlbedo) break;
                }
            }
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, albedoTexID);
            glUniform1i(uUseAlbedoMapLoc, useAlbedo ? 1 : 0);

            bool useNormal = (entity.material.useNormalMap && entity.material.customNormalID != 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, useNormal ? entity.material.customNormalID : whiteTexture);
            glUniform1i(uUseNormalMapLoc, useNormal ? 1 : 0);

            bool useMR = (entity.material.useMetallicRoughnessMap && entity.material.customMRID != 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, useMR ? entity.material.customMRID : whiteTexture);
            glUniform1i(uUseMetallicRoughnessMapLoc, useMR ? 1 : 0);

            glUniform1i(uUseAOMapLoc, 0);

            bool useHeight = (entity.material.useHeightMap && entity.material.customHeightID != 0);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, useHeight ? entity.material.customHeightID : whiteTexture);
            glUniform1i(uUseHeightMapLoc, useHeight ? 1 : 0);
            glUniform1f(uHeightScaleLoc, entity.material.heightScale);

            bool useEmissive = (entity.material.useEmissiveMap && entity.material.customEmissiveID != 0);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, useEmissive ? entity.material.customEmissiveID : whiteTexture);
            glUniform1i(glGetUniformLocation(gBufferShader.ID, "emissiveMap0"), 5);
            glUniform1i(glGetUniformLocation(gBufferShader.ID, "uUseEmissiveMap"), useEmissive ? 1 : 0);
            glUniform3f(uEmissiveColorLoc, entity.material.emissiveColor.x, entity.material.emissiveColor.y, entity.material.emissiveColor.z);
            glUniform1f(uEmissiveIntensityLoc, entity.material.emissiveIntensity);

            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.x), glm::vec3(1, 0, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.y), glm::vec3(0, 1, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.z), glm::vec3(0, 0, 1));
            modelMat = glm::translate(modelMat, entity.position);
            modelMat = glm::scale(modelMat, entity.scale);

            entity.model->Draw(gBufferShader, camera, modelMat, /*bindTextures=*/false);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ---- SSAO Pass ----
        if (enableSSAO) {
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);

            ssaoShader.Activate();
            glUniform1f(glGetUniformLocation(ssaoShader.ID, "radius"), ssaoRadius);
            glUniform1f(glGetUniformLocation(ssaoShader.ID, "bias"), ssaoBias);

            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, noiseTexture);

            glUniform1i(glGetUniformLocation(ssaoShader.ID, "gPosition"), 0);
            glUniform1i(glGetUniformLocation(ssaoShader.ID, "gNormal"), 1);
            glUniform1i(glGetUniformLocation(ssaoShader.ID, "noiseTexture"), 2);

            glm::mat4 proj = glm::perspective(glm::radians(editorFOV), (float)width / height, 0.1f, 1000.0f);
            glUniformMatrix4fv(glGetUniformLocation(ssaoShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
            glUniform2f(glGetUniformLocation(ssaoShader.ID, "noiseScale"), (float)width / 4.0f, (float)height / 4.0f);

            for (unsigned int i = 0; i < ssaoSamples.size(); i++) {
                std::string name = "samples[" + std::to_string(i) + "]";
                glUniform3fv(glGetUniformLocation(ssaoShader.ID, name.c_str()), 1, glm::value_ptr(ssaoSamples[i]));
            }

            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // ---- Lighting Pass ----

        gpuLights.clear();

        for (const auto& entity : entities) {
            if (entity.type == EntityType::PointLight ||
                entity.type == EntityType::SpotLight ||
                entity.type == EntityType::DirectionalLight ||
                entity.type == EntityType::AmbientLight)   // ← جديد
            {
                if (!entity.light.enabled) continue;

                GPULight light;
                light.position = entity.position;
                light.color = entity.light.color;
                light.intensity = entity.light.intensity;
                light.range = entity.light.range;

                // ---- تحديد النوع ----
                if (entity.type == EntityType::PointLight)            light.type = 0;
                else if (entity.type == EntityType::SpotLight)        light.type = 1;
                else if (entity.type == EntityType::DirectionalLight) light.type = 2;
                else if (entity.type == EntityType::AmbientLight)     light.type = 3;  // ← جديد

                // ---- الاتجاه (يُستخدم فقط لـ Spot / Directional) ----
                glm::vec3 dir(0.0f);
                if (entity.type == EntityType::DirectionalLight || entity.type == EntityType::SpotLight) {
                    glm::mat4 rotMat = glm::mat4(1.0f);
                    rotMat = glm::rotate(rotMat, glm::radians(entity.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    rotMat = glm::rotate(rotMat, glm::radians(entity.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotMat = glm::rotate(rotMat, glm::radians(entity.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                    dir = -glm::vec3(rotMat[2]);
                    dir = glm::normalize(dir);
                }
                light.direction = dir;

                light.innerCone = glm::radians(entity.light.innerConeAngle);
                light.outerCone = glm::radians(entity.light.outerConeAngle);

                gpuLights.push_back(light);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, finalColorFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        deferredLightingShader.Activate();

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "numLights"), (int)gpuLights.size());

        for (int i = 0; i < (int)gpuLights.size() && i < 128; i++) {
            const auto& light = gpuLights[i];

            std::string posName = "lightPositions[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, posName.c_str()), 1, glm::value_ptr(light.position));

            std::string colName = "lightColors[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, colName.c_str()), 1, glm::value_ptr(light.color));

            std::string intName = "lightIntensities[" + std::to_string(i) + "]";
            glUniform1f(glGetUniformLocation(deferredLightingShader.ID, intName.c_str()), light.intensity);

            std::string rangeName = "lightRanges[" + std::to_string(i) + "]";
            glUniform1f(glGetUniformLocation(deferredLightingShader.ID, rangeName.c_str()), light.range);

            std::string typeName = "lightTypes[" + std::to_string(i) + "]";
            glUniform1i(glGetUniformLocation(deferredLightingShader.ID, typeName.c_str()), light.type);

            std::string dirName = "lightDirections[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, dirName.c_str()), 1, glm::value_ptr(light.direction));

            std::string innerName = "lightInnerCone[" + std::to_string(i) + "]";
            glUniform1f(glGetUniformLocation(deferredLightingShader.ID, innerName.c_str()), light.innerCone);

            std::string outerName = "lightOuterCone[" + std::to_string(i) + "]";
            glUniform1f(glGetUniformLocation(deferredLightingShader.ID, outerName.c_str()), light.outerCone);
        }

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gPosition"), 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gNormal"), 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gColor);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gColor"), 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gMetallicRoughness);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gMetallicRoughness"), 3);

        if (enableSSAO) {
            glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        }
        else {
            glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, whiteTexture);
        }
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "ssao"), 4);

        glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "shadowMap"), 5);

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableContactShadows"),
            enableContactShadows ? 1 : 0);


        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "environmentMap"), 6);
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableEnvReflections"), enableEnvReflections ? 1 : 0);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "envReflectionIntensity"), envReflectionIntensity);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, gEmissive);
        glUniform1i(uGEmissiveLoc, 7);

        glUniform3f(uCamPosLoc, camera.Position.x, camera.Position.y, camera.Position.z);
        glUniformMatrix4fv(uCameraMatrixLoc, 1, GL_FALSE, glm::value_ptr(cameraMatrix));
        glUniformMatrix4fv(uViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(uInverseViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(inverseView));

        glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform4fv(glGetUniformLocation(deferredLightingShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, "lightPos2"), 1, glm::value_ptr(lightPos2));
        glUniformMatrix4fv(glGetUniformLocation(deferredLightingShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "showSkybox"), showSkybox ? 1 : 0);

        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "skyTopColor"), customTopColor.x, customTopColor.y, customTopColor.z);
        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "skyHorizonColor"), customHorizonColor.x, customHorizonColor.y, customHorizonColor.z);
        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "skyBottomColor"), customBottomColor.x, customBottomColor.y, customBottomColor.z);
        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "sunColor"), customSunColor.x, customSunColor.y, customSunColor.z);
        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "sunDirection"), customSunDirection.x, customSunDirection.y, customSunDirection.z);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "sunIntensity"), customSunIntensity);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "cloudDensity"), customCloudDensity);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "cloudOpacity"), customCloudOpacity);

        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "saturation"), saturation);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "contrast"), contrast);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "gamma"), gamma);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "exposure"), exposure);

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableSSAO"), enableSSAO ? 1 : 0);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "ssaoRadius"), ssaoRadius);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "ssaoBias"), ssaoBias);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "ssaoPower"), ssaoPower);

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableBloom"), enableBloom ? 1 : 0);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "bloomThreshold"), bloomThreshold);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "bloomIntensity"), bloomIntensity);

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableFXAA"), enableFXAA ? 1 : 0);

        // ---- Debug View ----
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "debugView"), debugView);

        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ---- Copy for fog ----
        glCopyImageSubData(finalColorTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
            litCopyTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
            width, height, 1);

        // ---- Skybox ----
        if (showSkybox) {
            glDepthFunc(GL_LEQUAL);

            skyboxShader.Activate();

           
            glm::mat4 skyView = glm::mat4(glm::mat3(viewMatrix));

           
            float aspect = (float)width / (float)height;
            glm::mat4 skyProj = glm::perspective(glm::radians(editorFOV), aspect, 0.1f, 100.0f);

            float scaleFactor = 1.5f;  
            glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));

            glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
            glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(skyProj));

            glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
            glUniform1i(glGetUniformLocation(skyboxShader.ID, "skybox"), 0);

            renderSkybox();

            glDepthFunc(GL_LESS);
        }

        // ---- Volumetric Fog ----
        if (enableVolumetricFog) {
            glBindFramebuffer(GL_FRAMEBUFFER, finalColorFBO);

            volumetricFogShader.Activate();

            glUniform1i(glGetUniformLocation(volumetricFogShader.ID, "depthTexture"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gDepth);

            glUniform1i(glGetUniformLocation(volumetricFogShader.ID, "colorTexture"), 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, litCopyTexture);

            glUniformMatrix4fv(glGetUniformLocation(volumetricFogShader.ID, "inverseViewMatrix"), 1, GL_FALSE, glm::value_ptr(inverseView));
            glUniformMatrix4fv(glGetUniformLocation(volumetricFogShader.ID, "inverseProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(inverseProj));
            glUniform3f(glGetUniformLocation(volumetricFogShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
            glUniform1f(glGetUniformLocation(volumetricFogShader.ID, "fogDensity"), fogDensity);
            glUniform1f(glGetUniformLocation(volumetricFogShader.ID, "fogHeight"), fogHeight);
            glUniform1f(glGetUniformLocation(volumetricFogShader.ID, "fogFalloff"), fogFalloff);
            glUniform1i(glGetUniformLocation(volumetricFogShader.ID, "fogSteps"), fogSteps);
            glUniform1f(glGetUniformLocation(volumetricFogShader.ID, "fogMaxDistance"), fogMaxDistance);
            glUniform3f(glGetUniformLocation(volumetricFogShader.ID, "fogColor"), fogColor.x, fogColor.y, fogColor.z);
            glUniform3fv(uVolLightPosLoc, 1, glm::value_ptr(lightPos));
            glUniform1f(uVolLightIntensityLoc, 0.5f);

            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // ---- FXAA (only if enabled) ----
        if (enableFXAA) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            fxaaShader.Activate();
            glUniform1i(glGetUniformLocation(fxaaShader.ID, "screenTexture"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, finalColorTexture);
            glUniform2f(glGetUniformLocation(fxaaShader.ID, "screenSize"), (float)width, (float)height);

            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            fxaaShader.Activate();
            glUniform1i(glGetUniformLocation(fxaaShader.ID, "screenTexture"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, finalColorTexture);
            glUniform2f(glGetUniformLocation(fxaaShader.ID, "screenSize"), (float)width, (float)height);
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
        }

        // ---- Debug Lines ----
#ifdef _DEBUG
        if (!debugLines.empty()) {
            std::vector<float> vertices;
            vertices.reserve(debugLines.size() * 6 * 2);
            glm::mat4 MVP = camera.cameraMatrix;
            for (const auto& line : debugLines) {
                vertices.push_back(line.start.x);
                vertices.push_back(line.start.y);
                vertices.push_back(line.start.z);
                vertices.push_back(line.color.x);
                vertices.push_back(line.color.y);
                vertices.push_back(line.color.z);
                vertices.push_back(line.end.x);
                vertices.push_back(line.end.y);
                vertices.push_back(line.end.z);
                vertices.push_back(line.color.x);
                vertices.push_back(line.color.y);
                vertices.push_back(line.color.z);
            }
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

            lineShader.Activate();
            glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "uMVP"), 1, GL_FALSE, glm::value_ptr(MVP));

            glBindVertexArray(lineVAO);
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_LINES, 0, (GLsizei)(vertices.size() / 6));
            glEnable(GL_DEPTH_TEST);
            glBindVertexArray(0);

            for (auto it = debugLines.begin(); it != debugLines.end(); ) {
                if (it->life > 0) {
                    it->life -= deltaTime;
                    if (it->life <= 0) {
                        it = debugLines.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }
#endif

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---- Cleanup ----
    for (auto* model : loadedModels) delete model;

    if (playerBodyID != BodyID()) {
        body_interface.RemoveBody(playerBodyID);
        body_interface.DestroyBody(playerBodyID);
    }
    body_interface.RemoveBody(floor->GetID());
    body_interface.DestroyBody(floor->GetID());

    JPH::UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;

    return 0;
}
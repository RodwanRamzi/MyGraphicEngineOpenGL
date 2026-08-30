#include <filesystem>
namespace fs = std::filesystem;

#include "Model.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shaderClass.h"
#include "Camera.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"



const unsigned int width = 1480;
const unsigned int height = 800;

enum class EntityType { Static, Player, Ball };

struct EditorEntity {
    Model* model;
    std::string modelPath;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    EntityType type = EntityType::Static;
    glm::vec3 velocity = glm::vec3(0.0f);
};

std::vector<EditorEntity> copyEntities(const std::vector<EditorEntity>& src) {
    return src;
}

// ==================== NORMALIZE PATH ====================
std::string normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

// ==================== SCAN MODELS FOLDER ====================
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
                modelFiles.push_back(fullPath);
            }
        }
    }

    return modelFiles;
}

// ==================== FRUSTUM CULLING ====================
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



int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(width, height, "Rodwan Engine - Level Editor", NULL, NULL);
    if (window == NULL) return -1;
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, width, height);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    // ==================== SHADERS ====================
    Shader gBufferShader("gBuffer.vert", "gBuffer.frag");
    Shader deferredLightingShader("deferred_lighting.vert", "deferred_lighting.frag");
    Shader depthShader("depth.vert", "depth.frag");
    Shader ssaoShader("ssao.vert", "ssao.frag");

    // ==================== CAMERA ====================
    Camera camera(width, height, glm::vec3(3.0f, 2.0f, 6.0f));
    camera.Orientation = glm::normalize(glm::vec3(-3.0f, -2.0f, -6.0f));

    // ==================== IMGUI ====================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // ==================== FULLSCREEN QUAD ====================
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

    // ==================== G-BUFFER ====================
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    unsigned int gPosition, gNormal, gColor, gMetallicRoughness;

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

    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);

    unsigned int gDepth;
    glGenRenderbuffers(1, &gDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, gDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "G-Buffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ==================== SHADOW MAP FBO ====================
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

    // ==================== WHITE TEXTURE (fallback) ====================
    unsigned int whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whitePixel[] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ==================== SSAO SETUP ====================
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

    // Noise texture
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

    // ==================== SAMPLE KERNEL (STATIC, NOT SCALED BY RADIUS) ====================
    std::vector<glm::vec3> ssaoSamples(64);
    for (unsigned int i = 0; i < 64; i++) {
        glm::vec3 sample = glm::vec3(glm::linearRand(-1.0f, 1.0f), glm::linearRand(-1.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
        sample = glm::normalize(sample);
        float scale = (float)i / 64.0f;
        scale = 0.1f + 0.9f * scale * scale;
        sample *= scale;
        ssaoSamples[i] = sample;
    }

    // ==================== GRID ====================
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

    // ==================== SKY SETTINGS ====================
    bool showSkybox = true;
    bool useCustomSky = false;
    glm::vec3 customTopColor = glm::vec3(0.2f, 0.4f, 0.8f);
    glm::vec3 customHorizonColor = glm::vec3(0.6f, 0.7f, 0.9f);
    glm::vec3 customBottomColor = glm::vec3(0.4f, 0.5f, 0.6f);
    glm::vec3 customSunColor = glm::vec3(1.0f, 0.9f, 0.6f);
    glm::vec3 customSunDirection = glm::vec3(0.5f, -0.2f, 0.3f);
    float customSunIntensity = 1.0f;
    float customCloudDensity = 1.5f;
    float customCloudOpacity = 0.3f;
    static int selectedSky = 0;

    // ==================== POST-PROCESSING SETTINGS ====================
    bool enableSSAO = true;
    float ssaoRadius = 0.5f;      // kept but not exposed
    float ssaoBias = 0.025f;      // kept but not exposed
    float ssaoPower = 2.0f;       // kept but not exposed

    bool enableBloom = true;
    float bloomThreshold = 0.5f;
    float bloomIntensity = 0.4f;

    // Color grading controls (with default values for reset)
    float saturation = 1.0f;
    float contrast = 1.0f;
    float gamma = 2.2f;
    float exposure = 1.5f;

    // ==================== FPS COUNTER ====================
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
    float fps = 0.0f;
    float fpsCounter = 0.0f;
    float fpsTime = 0.0f;
    bool showFPS = true;
    bool enableFrustumCulling = true;

    // ==================== CONTENT BROWSER ====================
    std::vector<std::string> modelFiles;
    std::string selectedModelPath = "";
    float editorFOV = 75.0f;
    float gameFOV = 65.0f;

    // ==================== MODEL LIST ====================
    std::vector<std::string> modelPaths = {
        "models/map/scene.gltf",
        "models/sword/scene.gltf"
    };
    std::vector<Model*> loadedModels;
    for (const auto& path : modelPaths) {
        Model* model = new Model(path.c_str());
        loadedModels.push_back(model);
        std::cout << "Loaded: " << path << std::endl;
    }

    // ==================== ENTITY LIST ====================
    std::vector<EditorEntity> entities;
    int selectedEntity = -1;
    entities.push_back({ loadedModels[0], modelPaths[0], glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.5f) });

    // ==================== GAME MODE ====================
    bool gameMode = false;
    std::vector<EditorEntity> savedEntities;

    // ==================== UNDO / REDO ====================
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

    // ==================== LEVEL FOLDER ====================
    std::string levelFolder = "Levels";
    fs::create_directories(levelFolder);
    char levelNameBuffer[128] = "MyLevel";
    std::string levelName = "MyLevel";
    std::string levelPath = levelFolder + "/" + levelName + ".txt";
    bool showOverwriteWarning = false;
    bool showRenamePopup = false;
    char newLevelNameBuffer[128] = "";

    auto saveLevel = [&]() {
        std::ofstream file(levelPath);
        if (!file.is_open()) {
            std::cout << "Failed to open: " << levelPath << std::endl;
            return;
        }
        for (const auto& entity : entities) {
            file << entity.modelPath << '|'
                << entity.position.x << '|' << entity.position.y << '|' << entity.position.z << '|'
                << entity.rotation.x << '|' << entity.rotation.y << '|' << entity.rotation.z << '|'
                << entity.scale.x << '|' << entity.scale.y << '|' << entity.scale.z << '|'
                << static_cast<int>(entity.type) << '\n';
        }
        file.close();
        std::cout << "Saved level to " << levelPath << std::endl;
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

            int modelIndex = -1;
            for (int i = 0; i < modelPaths.size(); ++i) {
                if (modelPaths[i] == path) { modelIndex = i; break; }
            }
            if (modelIndex != -1) {
                EditorEntity ent;
                ent.model = loadedModels[modelIndex];
                ent.modelPath = path;
                ent.position = glm::vec3(px, py, pz);
                ent.rotation = glm::vec3(rx, ry, rz);
                ent.scale = glm::vec3(sx, sy, sz);
                int typeInt;
                if (ss >> typeInt) ent.type = static_cast<EntityType>(typeInt);
                else ent.type = EntityType::Static;
                entities.push_back(ent);
            }
        }
        file.close();
        std::cout << "Loaded level from " << levelPath << std::endl;
        };

    // ==================== LIGHT SETUP ====================
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(2.0f, 3.0f, 2.0f);
    glm::vec3 lightPos2 = glm::vec3(-2.0f, 2.0f, -1.0f);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // ==================== MAIN LOOP ====================
    while (!glfwWindowShouldClose(window)) {
        float deltaTime = 0.016f;

        // ==================== GAME MODE LOGIC ====================
        if (gameMode) {
            for (auto& entity : entities) {
                if (entity.type == EntityType::Player) {
                    float speed = 5.0f;
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) entity.position.z -= speed * deltaTime;
                    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) entity.position.z += speed * deltaTime;
                    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) entity.position.x -= speed * deltaTime;
                    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) entity.position.x += speed * deltaTime;
                    camera.Position = entity.position + glm::vec3(0, 1.5f, 0);
                    camera.Orientation = glm::vec3(0, 0, -1);
                }
                else if (entity.type == EntityType::Ball) {
                    entity.velocity.y -= 9.8f * deltaTime;
                    entity.position += entity.velocity * deltaTime;
                    if (entity.position.y < 0.0f) {
                        entity.position.y = 0.0f;
                        entity.velocity.y = -entity.velocity.y * 0.8f;
                        entity.velocity.x *= 0.99f;
                        entity.velocity.z *= 0.99f;
                    }
                }
            }
            camera.updateMatrix(gameFOV, 0.1f, 1000.0f);
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) gameMode = false;
        }
        else {
            camera.Inputs(window);
            camera.updateMatrix(editorFOV, 0.1f, 1000.0f);
        }

        // ==================== FPS CALCULATION ====================
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

        // ==================== VIEW MATRICES ====================
        glm::mat4 viewMatrix = camera.GetViewMatrix();
        glm::mat4 inverseView = glm::inverse(viewMatrix);

        // ==================== IMGUI ====================
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) applyUndo();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) applyRedo();

        // ==================== LEVEL EDITOR WINDOW ====================
        ImGui::Begin("Level Editor");

        ImGui::Checkbox("Show FPS", &showFPS);
        ImGui::InputText("Level Name", levelNameBuffer, sizeof(levelNameBuffer));
        levelName = levelNameBuffer;
        levelPath = levelFolder + "/" + levelName + ".txt";
        ImGui::Text("Path: %s", levelPath.c_str());

        if (ImGui::Button("Save Level")) {
            if (fs::exists(levelPath)) showOverwriteWarning = true;
            else saveLevel();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Level")) {
            if (fs::exists(levelPath)) loadLevel();
            else std::cout << "Level not found: " << levelPath << std::endl;
        }
        ImGui::SameLine();
        if (ImGui::Button("New Level")) {
            pushUndo();
            entities.clear();
            selectedEntity = -1;
        }

        ImGui::Separator();
        if (ImGui::Button("Undo")) applyUndo();
        ImGui::SameLine();
        if (ImGui::Button("Redo")) applyRedo();
        ImGui::Separator();

        if (ImGui::Button(gameMode ? "Back to Editor" : "Play Mode")) {
            if (!gameMode) savedEntities = copyEntities(entities);
            gameMode = !gameMode;
            if (!gameMode) entities = copyEntities(savedEntities);
        }
        ImGui::Separator();

        ImGui::Checkbox("Enable Frustum Culling", &enableFrustumCulling);
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
        }
        ImGui::Separator();

        // ==================== POST-PROCESSING CONTROLS ====================
        if (ImGui::CollapsingHeader("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("🎨 Post-Processing Settings");
            ImGui::Separator();

            // SSAO – only enable/disable
            ImGui::Checkbox("Enable SSAO", &enableSSAO);

            ImGui::Separator();

            // Color Grading with Reset buttons
            ImGui::Text("Color Grading");

            // Saturation
            ImGui::SliderFloat("Saturation", &saturation, 0.0f, 2.0f);
            ImGui::SameLine();
            if (ImGui::Button("R##Saturation")) { saturation = 1.0f; }

            // Contrast
            ImGui::SliderFloat("Contrast", &contrast, 0.5f, 2.0f);
            ImGui::SameLine();
            if (ImGui::Button("R##Contrast")) { contrast = 1.0f; }

            // Gamma
            ImGui::SliderFloat("Gamma", &gamma, 1.0f, 3.0f);
            ImGui::SameLine();
            if (ImGui::Button("R##Gamma")) { gamma = 2.2f; }

            // Exposure
            ImGui::SliderFloat("Exposure", &exposure, 0.0f, 3.0f);
            ImGui::SameLine();
            if (ImGui::Button("R##Exposure")) { exposure = 1.5f; }

            ImGui::Separator();

            // Bloom with Reset buttons
            if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Enable Bloom", &enableBloom);

                ImGui::SliderFloat("Threshold", &bloomThreshold, 0.0f, 2.0f);
                ImGui::SameLine();
                if (ImGui::Button("R##BloomThreshold")) { bloomThreshold = 0.5f; }

                ImGui::SliderFloat("Intensity", &bloomIntensity, 0.0f, 2.0f);
                ImGui::SameLine();
                if (ImGui::Button("R##BloomIntensity")) { bloomIntensity = 0.4f; }
            }

            ImGui::Separator();

        }

        // Add Object
        if (ImGui::CollapsingHeader("Add Object")) {
            for (int i = 0; i < modelPaths.size(); ++i) {
                if (ImGui::Button(modelPaths[i].c_str())) {
                    pushUndo();
                    entities.push_back({ loadedModels[i], modelPaths[i], glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(1.0f) });
                    selectedEntity = (int)entities.size() - 1;
                }
            }
        }

        // Entities list
        if (ImGui::CollapsingHeader("Entities")) {
            for (int i = 0; i < entities.size(); ++i) {
                bool isSelected = (selectedEntity == i);
                std::string label = "Entity " + std::to_string(i) + " (" + entities[i].modelPath + ")";
                if (ImGui::Selectable(label.c_str(), isSelected)) selectedEntity = i;
            }
        }

        // Transform controls
        if (selectedEntity >= 0 && selectedEntity < entities.size()) {
            auto& entity = entities[selectedEntity];
            ImGui::Separator();
            ImGui::Text("Selected: %d", selectedEntity);

            bool changed = false;
            if (ImGui::DragFloat3("Position", &entity.position.x, 0.1f)) changed = true;
            if (ImGui::DragFloat3("Rotation (deg)", &entity.rotation.x, 1.0f)) changed = true;
            if (ImGui::DragFloat3("Scale", &entity.scale.x, 0.05f)) changed = true;

            if (changed && ImGui::IsItemDeactivatedAfterEdit()) {
                pushUndo();
            }

            if (ImGui::Button("Delete")) {
                pushUndo();
                entities.erase(entities.begin() + selectedEntity);
                selectedEntity = -1;
            }

            const char* typeNames[] = { "Static", "Player", "Ball" };
            int typeIndex = static_cast<int>(entity.type);
            if (ImGui::Combo("Type", &typeIndex, typeNames, 3)) {
                pushUndo();
                entity.type = static_cast<EntityType>(typeIndex);
            }
        }

        ImGui::End();

        // ==================== CONTENT BROWSER ====================
        ImGui::Begin("Content Browser", nullptr, ImGuiWindowFlags_NoCollapse);
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
                std::cout << " Importing: " << normalizedPath << std::endl;

                int existingIndex = -1;
                for (int i = 0; i < modelPaths.size(); ++i) {
                    if (modelPaths[i] == normalizedPath) {
                        existingIndex = i;
                        break;
                    }
                }

                Model* modelToUse = nullptr;
                bool success = true;

                if (existingIndex != -1) {
                    modelToUse = loadedModels[existingIndex];
                    std::cout << " Reusing existing model: " << normalizedPath << std::endl;
                }
                else {
                    if (!fs::exists(normalizedPath)) {
                        std::cout << "❌ File not found: " << normalizedPath << std::endl;
                        selectedModelPath = "";
                        success = false;
                    }
                    else {
                        Model* newModel = new Model(normalizedPath.c_str());
                        if (newModel) {
                            loadedModels.push_back(newModel);
                            modelPaths.push_back(normalizedPath);
                            modelToUse = newModel;
                            std::cout << " Loaded new model: " << normalizedPath << std::endl;
                        }
                        else {
                            std::cout << "❌ Failed to load model: " << normalizedPath << std::endl;
                            selectedModelPath = "";
                            success = false;
                        }
                    }
                }

                if (success && modelToUse != nullptr) {
                    pushUndo();
                    entities.push_back(EditorEntity{ modelToUse, normalizedPath, glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(1,1,1), EntityType::Static, glm::vec3(0) });
                    std::cout << " Added new entity with model: " << normalizedPath << std::endl;
                    selectedModelPath = "";
                }
                else {
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

        // ==================== SKYBOX WINDOW ====================
        ImGui::Begin("Skybox");
        ImGui::Text("Sky Settings");
        ImGui::Separator();
        ImGui::Checkbox("Show Skybox", &showSkybox);
        ImGui::SameLine();
        if (ImGui::Button("Reset Sky")) {
            selectedSky = 0;
            useCustomSky = false;
            customTopColor = glm::vec3(0.2f, 0.4f, 0.8f);
            customHorizonColor = glm::vec3(0.6f, 0.7f, 0.9f);
            customBottomColor = glm::vec3(0.4f, 0.5f, 0.6f);
            customSunColor = glm::vec3(1.0f, 0.9f, 0.6f);
            customSunDirection = glm::vec3(0.5f, -0.2f, 0.3f);
            customSunIntensity = 1.0f;
            customCloudDensity = 1.5f;
            customCloudOpacity = 0.3f;
        }

        if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char* skyNames[] = { "Day (Blue Sky)", "Sunset", "Golden Hour", "Night", "Space/Sci-Fi", "Cyberpunk" };
            struct SkyPreset {
                glm::vec3 top, horizon, bottom, sunColor, sunDir;
                float sunIntensity, cloudDensity, cloudOpacity;
            };
            static std::vector<SkyPreset> skyPresets = {
                { glm::vec3(0.2f,0.4f,0.8f), glm::vec3(0.6f,0.7f,0.9f), glm::vec3(0.4f,0.5f,0.6f), glm::vec3(1.0f,0.9f,0.6f), glm::vec3(0.5f,-0.2f,0.3f), 1.0f, 1.5f, 0.3f },
                { glm::vec3(0.1f,0.2f,0.5f), glm::vec3(0.9f,0.5f,0.2f), glm::vec3(0.3f,0.2f,0.1f), glm::vec3(1.0f,0.6f,0.2f), glm::vec3(0.3f,-0.4f,0.2f), 1.2f, 2.0f, 0.4f },
                { glm::vec3(0.3f,0.5f,0.8f), glm::vec3(0.9f,0.7f,0.3f), glm::vec3(0.5f,0.3f,0.1f), glm::vec3(1.0f,0.8f,0.3f), glm::vec3(0.4f,-0.3f,0.2f), 1.5f, 1.0f, 0.2f },
                { glm::vec3(0.02f,0.02f,0.05f), glm::vec3(0.1f,0.1f,0.15f), glm::vec3(0.05f,0.05f,0.08f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,-0.5f,0.0f), 0.0f, 0.5f, 0.1f },
                { glm::vec3(0.01f,0.01f,0.05f), glm::vec3(0.2f,0.1f,0.3f), glm::vec3(0.05f,0.02f,0.1f), glm::vec3(0.8f,0.6f,0.4f), glm::vec3(0.2f,-0.6f,0.1f), 0.8f, 0.3f, 0.05f },
                { glm::vec3(0.8f,0.1f,0.5f), glm::vec3(0.1f,0.2f,0.8f), glm::vec3(0.0f,0.0f,0.1f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,-0.3f,0.0f), 0.0f, 0.8f, 0.2f }
            };
            if (ImGui::Combo("Preset", &selectedSky, skyNames, IM_ARRAYSIZE(skyNames))) {
                customTopColor = skyPresets[selectedSky].top;
                customHorizonColor = skyPresets[selectedSky].horizon;
                customBottomColor = skyPresets[selectedSky].bottom;
                customSunColor = skyPresets[selectedSky].sunColor;
                customSunDirection = skyPresets[selectedSky].sunDir;
                customSunIntensity = skyPresets[selectedSky].sunIntensity;
                customCloudDensity = skyPresets[selectedSky].cloudDensity;
                customCloudOpacity = skyPresets[selectedSky].cloudOpacity;
            }
            ImGui::Checkbox("Custom Mode", &useCustomSky);
        }

        if (useCustomSky) {
            if (ImGui::CollapsingHeader("Custom Sky Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Top Color", &customTopColor[0]);
                ImGui::ColorEdit3("Horizon Color", &customHorizonColor[0]);
                ImGui::ColorEdit3("Bottom Color", &customBottomColor[0]);
            }
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
        ImGui::End();

        // ==================== OVERWRITE / RENAME POPUPS ====================
        if (showOverwriteWarning) {
            ImGui::OpenPopup("Overwrite Warning");
            showOverwriteWarning = false;
        }
        if (ImGui::BeginPopupModal("Overwrite Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Level '%s' already exists.", levelName.c_str());
            ImGui::Text("Do you want to overwrite it?");
            ImGui::Separator();
            if (ImGui::Button("Overwrite", ImVec2(120, 0))) {
                saveLevel();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Rename", ImVec2(120, 0))) {
                strcpy_s(newLevelNameBuffer, levelName.c_str());
                showRenamePopup = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (showRenamePopup) {
            ImGui::OpenPopup("Rename Level");
            showRenamePopup = false;
        }
        if (ImGui::BeginPopupModal("Rename Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("New Name", newLevelNameBuffer, sizeof(newLevelNameBuffer));
            ImGui::Separator();
            if (ImGui::Button("Rename & Save", ImVec2(120, 0))) {
                levelName = newLevelNameBuffer;
                levelPath = levelFolder + "/" + levelName + ".txt";
                saveLevel();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== FPS OVERLAY ====================
        if (showFPS) {
            ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", fps);
            float frameTimeMS = deltaTime * 1000.0f;
            ImGui::Text("Frame Time: %.2f ms", frameTimeMS);
            ImGui::Text("Entities: %zu", entities.size());
            ImGui::End();
        }

        // ==================== COMPUTE LIGHT SPACE MATRIX ====================
        glm::vec3 lightPosWorld = -lightDir * 20.0f;
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 30.0f);
        glm::mat4 lightView = glm::lookAt(lightPosWorld, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // ==================== DEPTH PASS (SHADOW MAP) ====================
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);

        Frustum frustum = extractFrustum(camera.cameraMatrix);
        depthShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        for (auto& entity : entities) {
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

        // ==================== GEOMETRY PASS (G-BUFFER) ====================
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Frustum frustum2 = extractFrustum(camera.cameraMatrix);
        gBufferShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(gBufferShader.ID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(camera.cameraMatrix));
        glUniform3f(glGetUniformLocation(gBufferShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useNormalMap"), 1);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useMetallicRoughness"), 1);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useHeightMap"), 1);

        for (auto& entity : entities) {
            if (enableFrustumCulling) {
                float radius = 1.0f;
                if (!isSphereInFrustum(frustum2, entity.position, radius)) continue;
            }
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.x), glm::vec3(1, 0, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.y), glm::vec3(0, 1, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.z), glm::vec3(0, 0, 1));
            modelMat = glm::translate(modelMat, entity.position);
            modelMat = glm::scale(modelMat, entity.scale);
            entity.model->Draw(gBufferShader, camera, modelMat);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ==================== SSAO PASS (run every frame if enabled) ====================
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

            for (unsigned int i = 0; i < 64; i++) {
                std::string name = "samples[" + std::to_string(i) + "]";
                glUniform3fv(glGetUniformLocation(ssaoShader.ID, name.c_str()), 1, glm::value_ptr(ssaoSamples[i]));
            }

            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // ==================== LIGHTING PASS ====================
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        deferredLightingShader.Activate();

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gPosition"), 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gNormal"), 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gColor); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gColor"), 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gMetallicRoughness); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gMetallicRoughness"), 3);

        if (enableSSAO) {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        }
        else {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, whiteTexture);
        }
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "ssao"), 4);

        glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, depthMap); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "shadowMap"), 5);

        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(deferredLightingShader.ID, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(deferredLightingShader.ID, "inverseViewMatrix"), 1, GL_FALSE, glm::value_ptr(inverseView));

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

        // Post-processing uniforms
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "saturation"), saturation);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "contrast"), contrast);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "gamma"), gamma);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "exposure"), exposure);

        // SSAO & Bloom
        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableSSAO"), enableSSAO ? 1 : 0);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "ssaoRadius"), ssaoRadius);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "ssaoBias"), ssaoBias);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "ssaoPower"), ssaoPower);

        glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "enableBloom"), enableBloom ? 1 : 0);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "bloomThreshold"), bloomThreshold);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "bloomIntensity"), bloomIntensity);

        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto* model : loadedModels) delete model;
    return 0;
}
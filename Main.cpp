#include<filesystem>
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

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const unsigned int width = 1024;
const unsigned int height = 720;

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
    Shader depthShader("depth.vert", "depth.frag");  // NEW

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

    // ==================== SAVE / LOAD ====================
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
                if (ss >> typeInt) {
                    ent.type = static_cast<EntityType>(typeInt);
                }
                else {
                    ent.type = EntityType::Static;
                }

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

    float saturation = 1.0f;
    float exposure = 1.5f;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // ==================== MAIN LOOP ====================
    while (!glfwWindowShouldClose(window))
    {
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
            camera.updateMatrix(45.0f, 0.1f, 1000.0f);
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) gameMode = false;
        }
        else {
            camera.Inputs(window);
            camera.updateMatrix(45.0f, 0.1f, 1000.0f);
        }

        // ==================== IMGUI ====================
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) applyUndo();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) applyRedo();

        ImGui::Begin("Level Editor");

        // File management
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

        // Play mode
        if (ImGui::Button(gameMode ? "Back to Editor" : "Play Mode")) {
            if (!gameMode) savedEntities = copyEntities(entities);
            gameMode = !gameMode;
            if (!gameMode) entities = copyEntities(savedEntities);
        }
        ImGui::Separator();

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
            ImGui::DragFloat3("Position", &entity.position.x, 0.1f);
            ImGui::DragFloat3("Rotation (deg)", &entity.rotation.x, 1.0f);
            ImGui::DragFloat3("Scale", &entity.scale.x, 0.05f);
            if (ImGui::Button("Delete")) {
                pushUndo();
                entities.erase(entities.begin() + selectedEntity);
                selectedEntity = -1;
            }

            const char* typeNames[] = { "Static", "Player", "Ball" };
            int typeIndex = static_cast<int>(entity.type);
            if (ImGui::Combo("Type", &typeIndex, typeNames, 3)) {
                entity.type = static_cast<EntityType>(typeIndex);
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

        // ==================== COMPUTE LIGHT SPACE MATRIX ====================
        glm::vec3 lightPosWorld = -lightDir * 20.0f;
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 30.0f);
        glm::mat4 lightView = glm::lookAt(lightPosWorld, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // ==================== DEPTH PASS (SHADOW MAP) ====================
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        for (auto& entity : entities) {
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

        gBufferShader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(gBufferShader.ID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(camera.cameraMatrix));
        glUniform3f(glGetUniformLocation(gBufferShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useNormalMap"), 1);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useMetallicRoughness"), 1);
        glUniform1i(glGetUniformLocation(gBufferShader.ID, "useHeightMap"), 1);

        for (auto& entity : entities) {
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.x), glm::vec3(1, 0, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.y), glm::vec3(0, 1, 0));
            modelMat = glm::rotate(modelMat, glm::radians(entity.rotation.z), glm::vec3(0, 0, 1));
            modelMat = glm::translate(modelMat, entity.position);
            modelMat = glm::scale(modelMat, entity.scale);
            entity.model->Draw(gBufferShader, camera, modelMat);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ==================== LIGHTING PASS ====================
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        deferredLightingShader.Activate();

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gPosition"), 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gNormal"), 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gColor); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gColor"), 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gMetallicRoughness); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "gMetallicRoughness"), 3);
        glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, whiteTexture); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "ssao"), 4);
        // Bind real shadow map
        glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, depthMap); glUniform1i(glGetUniformLocation(deferredLightingShader.ID, "shadowMap"), 5);

        glUniform3f(glGetUniformLocation(deferredLightingShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform4fv(glGetUniformLocation(deferredLightingShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(deferredLightingShader.ID, "lightPos2"), 1, glm::value_ptr(lightPos2));
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "saturation"), saturation);
        glUniform1f(glGetUniformLocation(deferredLightingShader.ID, "exposure"), exposure);
        // Pass real light space matrix
        glUniformMatrix4fv(glGetUniformLocation(deferredLightingShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);

        // ==================== IMGUI RENDER ====================
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (auto* model : loadedModels) delete model;

    return 0;
}

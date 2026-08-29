#include "Editor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <json/json.h>

using json = nlohmann::json;

Editor::Editor(int width, int height, const std::vector<std::string>& paths)
    : modelPaths(paths)
{
    // Load all models once
    for (const auto& path : modelPaths) {
        Model* model = new Model(path.c_str());
        modelInstances.push_back(model);
        std::cout << "Loaded model: " << path << std::endl;
    }
    CreateGrid();

    // Create white texture for SSAO/ShadowMap
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whitePixel[] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Add a default object so you see something
    if (!modelInstances.empty()) {
        AddEntity(0, glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f));
        std::cout << "Added default entity" << std::endl;
    }
}

Editor::~Editor() {
    for (auto* model : modelInstances) delete model;
    if (gridVAO) glDeleteVertexArrays(1, &gridVAO);
    if (gridVBO) glDeleteBuffers(1, &gridVBO);
    if (whiteTexture) glDeleteTextures(1, &whiteTexture);
}

void Editor::CreateGrid() {
    std::vector<glm::vec3> vertices;
    for (int i = -50; i <= 50; i++) {
        vertices.push_back(glm::vec3(i, 0, -50));
        vertices.push_back(glm::vec3(i, 0, 50));
        vertices.push_back(glm::vec3(-50, 0, i));
        vertices.push_back(glm::vec3(50, 0, i));
    }
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Editor::DrawGrid(Camera& camera, Shader& gridShader) {
    gridShader.Activate();
    glUniformMatrix4fv(glGetUniformLocation(gridShader.ID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(camera.cameraMatrix));
    glUniform3f(glGetUniformLocation(gridShader.ID, "gridColor"), 0.5f, 0.5f, 0.5f);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, 4 * 101 * 2);
    glBindVertexArray(0);
}

void Editor::AddEntity(int modelIndex, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale) {
    if (modelIndex < 0 || modelIndex >= modelInstances.size()) return;
    Entity entity = world.CreateEntity();
    world.AddComponent(entity, TransformComponent{ pos, glm::quat(glm::vec3(glm::radians(rot.x), glm::radians(rot.y), glm::radians(rot.z))), scale });
    world.AddComponent(entity, MeshComponent{ modelInstances[modelIndex] });
    entityModelIndex[entity] = modelIndex;
    if (selectedEntity == -1) selectedEntity = entity;
}

void Editor::RemoveEntity(Entity entity) {
    if (entity == -1) return;
    world.meshes.erase(entity);
    world.transforms.erase(entity);
    world.velocities.erase(entity);
    entityModelIndex.erase(entity);
    if (selectedEntity == entity) selectedEntity = -1;
}

void Editor::Update(float deltaTime, Camera& camera) {
    ImGui::Begin("Level Editor");

    if (ImGui::CollapsingHeader("Add Object")) {
        for (int i = 0; i < modelPaths.size(); ++i) {
            if (ImGui::Button(modelPaths[i].c_str())) {
                AddEntity(i, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1));
            }
        }
    }
    if (ImGui::CollapsingHeader("Entities")) {
        for (const auto& [entity, mesh] : world.meshes) {
            bool isSelected = (selectedEntity == entity);
            std::string label = "Entity " + std::to_string(entity);
            if (ImGui::Selectable(label.c_str(), isSelected)) selectedEntity = entity;
        }
    }
    if (selectedEntity != -1 && world.transforms.find(selectedEntity) != world.transforms.end()) {
        auto& transform = world.transforms[selectedEntity];
        ImGui::Separator();
        ImGui::Text("Selected: %d", selectedEntity);
        ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation));
        if (ImGui::DragFloat3("Rotation (deg)", &euler.x, 1.0f)) {
            transform.rotation = glm::quat(glm::radians(euler));
        }
        ImGui::DragFloat3("Scale", &transform.scale.x, 0.05f);
        if (ImGui::Button("Delete")) RemoveEntity(selectedEntity);
    }
    ImGui::Separator();
    if (ImGui::Button("Save Level")) SaveLevel("level_editor.json");
    if (ImGui::Button("Load Level")) LoadLevel("level_editor.json");
    ImGui::End();
}

void Editor::Draw(Camera& camera, Shader& gBufferShader, Shader& lightingShader,
    unsigned int gBufferFBO, unsigned int gPosition, unsigned int gNormal,
    unsigned int gColor, unsigned int gMetallicRoughness, unsigned int rectVAO) {
    // G-Buffer pass
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gBufferShader.Activate();
    glUniformMatrix4fv(glGetUniformLocation(gBufferShader.ID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(camera.cameraMatrix));
    world.Draw(gBufferShader, camera);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Lighting pass
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    lightingShader.Activate();

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition); glUniform1i(glGetUniformLocation(lightingShader.ID, "gPosition"), 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal); glUniform1i(glGetUniformLocation(lightingShader.ID, "gNormal"), 1);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gColor); glUniform1i(glGetUniformLocation(lightingShader.ID, "gColor"), 2);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gMetallicRoughness); glUniform1i(glGetUniformLocation(lightingShader.ID, "gMetallicRoughness"), 3);

    // Bind white textures for SSAO and ShadowMap
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, whiteTexture); glUniform1i(glGetUniformLocation(lightingShader.ID, "ssao"), 4);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, whiteTexture); glUniform1i(glGetUniformLocation(lightingShader.ID, "shadowMap"), 5);

    // Set light uniforms
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
    glm::vec4 lightColor = glm::vec4(1.0f);
    glm::vec3 lightPos = glm::vec3(2.0f, 3.0f, 2.0f);
    glm::vec3 lightPos2 = glm::vec3(-2.0f, 2.0f, -1.0f);
    glUniform3f(glGetUniformLocation(lightingShader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
    glUniform3fv(glGetUniformLocation(lightingShader.ID, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform4fv(glGetUniformLocation(lightingShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(lightingShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(lightingShader.ID, "lightPos2"), 1, glm::value_ptr(lightPos2));
    glUniform1f(glGetUniformLocation(lightingShader.ID, "saturation"), 1.0f);
    glUniform1f(glGetUniformLocation(lightingShader.ID, "exposure"), 1.5f);
    glm::mat4 identity = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lightingShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(identity));

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(rectVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Editor::SaveLevel(const std::string& path) {
    json j;
    j["entities"] = json::array();
    for (const auto& [entity, mesh] : world.meshes) {
        json entityData;
        entityData["name"] = "Entity " + std::to_string(entity);
        if (world.transforms.find(entity) != world.transforms.end()) {
            auto& t = world.transforms[entity];
            entityData["position"] = { t.position.x, t.position.y, t.position.z };
            glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
            entityData["rotation"] = { euler.x, euler.y, euler.z };
            entityData["scale"] = { t.scale.x, t.scale.y, t.scale.z };
        }
        if (entityModelIndex.find(entity) != entityModelIndex.end()) {
            entityData["model"] = modelPaths[entityModelIndex[entity]];
        }
        j["entities"].push_back(entityData);
    }
    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
        std::cout << "Saved level to " << path << std::endl;
    }
    else {
        std::cerr << "Failed to save level to " << path << std::endl;
    }
}

void Editor::LoadLevel(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        return;
    }
    world = ECSWorld();
    entityModelIndex.clear();
    selectedEntity = -1;
    std::stringstream ss;
    ss << file.rdbuf();
    json j;
    try {
        j = json::parse(ss.str());
    }
    catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return;
    }
    for (const auto& entityData : j["entities"]) {
        std::string modelPath = entityData["model"];
        int modelIndex = -1;
        for (int i = 0; i < modelPaths.size(); ++i) {
            if (modelPaths[i] == modelPath) {
                modelIndex = i;
                break;
            }
        }
        if (modelIndex == -1) continue;
        glm::vec3 pos = { 0,0,0 };
        glm::vec3 rot = { 0,0,0 };
        glm::vec3 scl = { 1,1,1 };
        if (entityData.contains("position")) {
            auto p = entityData["position"];
            pos = glm::vec3(p[0], p[1], p[2]);
        }
        if (entityData.contains("rotation")) {
            auto r = entityData["rotation"];
            rot = glm::vec3(r[0], r[1], r[2]);
        }
        if (entityData.contains("scale")) {
            auto sc = entityData["scale"];
            scl = glm::vec3(sc[0], sc[1], sc[2]);
        }
        AddEntity(modelIndex, pos, rot, scl);
    }
    std::cout << "Loaded level from " << path << std::endl;
}
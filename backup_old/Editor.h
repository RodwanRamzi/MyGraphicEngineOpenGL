#pragma once
#include "ECS.h"
#include "Model.h"
#include "Camera.h"
#include "shaderClass.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Editor {
public:
    Editor(int width, int height, const std::vector<std::string>& modelPaths);
    ~Editor();

    void Update(float deltaTime, Camera& camera);
    void Draw(Camera& camera, Shader& gBufferShader, Shader& lightingShader,
        unsigned int gBufferFBO, unsigned int gPosition, unsigned int gNormal,
        unsigned int gColor, unsigned int gMetallicRoughness, unsigned int rectVAO);
    void DrawGrid(Camera& camera, Shader& gridShader);

    void SaveLevel(const std::string& path);
    void LoadLevel(const std::string& path);

private:
    ECSWorld world;
    std::vector<std::string> modelPaths;
    std::vector<Model*> modelInstances;
    std::unordered_map<Entity, int> entityModelIndex;
    int selectedEntity = -1;

    // Grid VAO/VBO
    unsigned int gridVAO = 0, gridVBO = 0;

    // White texture for SSAO / ShadowMap
    unsigned int whiteTexture = 0;

    void AddEntity(int modelIndex, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
    void RemoveEntity(Entity entity);
    void CreateGrid();
};
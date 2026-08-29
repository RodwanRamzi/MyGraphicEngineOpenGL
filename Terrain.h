#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "shaderClass.h"
#include "Camera.h"

class Terrain {
public:
    Terrain(int size = 100, float scale = 50.0f, float heightScale = 2.0f);
    void Draw(Shader& shader, Camera& camera);
    void DrawShadow(Shader& shader, glm::mat4 shadowMatrices[6], int faceIndex);
    glm::mat4 GetModelMatrix() { return modelMatrix; }
    float GetHeightAt(float x, float z);

private:
    Mesh mesh;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    int size;
    float scale;
    float heightScale;
    std::vector<float> heights;
};
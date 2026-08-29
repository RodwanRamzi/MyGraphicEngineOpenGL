#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Model.h"
#include "shaderClass.h"
#include "Camera.h"
#include <string>

class GameObject {
public:
    GameObject(Model* model = nullptr, const std::string& name = "GameObject");
    void Draw(Shader& shader, Camera& camera);
    glm::mat4 GetModelMatrix() const;

    // Transform properties
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    std::string name;
    Model* model;
    bool isActive = true;

    // Collision box
    glm::vec3 collisionBoxSize = glm::vec3(1.0f);

    bool disableCull = false;  // تعطيل Culling لهذا الكائن

    void UpdatePosition(const glm::vec3& newPosition) {
        position = newPosition;
    }

    void UpdateRotation(const glm::vec3& newRotation) {
        rotation = newRotation;
    }

};
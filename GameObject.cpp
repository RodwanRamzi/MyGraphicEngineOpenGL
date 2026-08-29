#include "GameObject.h"

GameObject::GameObject(Model* model, const std::string& name) {
    this->model = model;
    this->name = name;
}

void GameObject::Draw(Shader& shader, Camera& camera) {
    if (!isActive || model == nullptr) return;
    model->Draw(shader, camera);
}

glm::mat4 GameObject::GetModelMatrix() const {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, scale);
    return modelMatrix;
}
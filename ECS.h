#pragma once
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Model.h"
#include "shaderClass.h"
#include "Camera.h"

using Entity = uint32_t;

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct MeshComponent {
    Model* model = nullptr;
};

struct VelocityComponent {
    float speed = 5.0f;
    float y = 0.0f;
    float JumpVelocity = 7.5f;
    glm::vec3 velocity = glm::vec3(0.0f);
};

class ECSWorld {
public:
    Entity CreateEntity() {
        Entity id = nextEntityID++;
        entities.push_back(id);
        return id;
    }

    void AddComponent(Entity entity, TransformComponent component) {
        transforms[entity] = component;
    }

    void AddComponent(Entity entity, MeshComponent component) {
        meshes[entity] = component;
    }

    void AddComponent(Entity entity, VelocityComponent component) {
        velocities[entity] = component;
    }

    TransformComponent& GetTransform(Entity entity) {
        return transforms[entity];
    }

    void Update(float deltaTime) {
        for (auto& [entity, velocity] : velocities) {
            if (transforms.find(entity) != transforms.end()) {
                auto& transform = transforms[entity];
                transform.position += velocity.velocity * deltaTime;
            }
        }
    }

    void Draw(Shader& shader, Camera& camera) {
        for (auto& [entity, mesh] : meshes) {
            if (mesh.model != nullptr && transforms.find(entity) != transforms.end()) {
                auto& transform = transforms[entity];
                glm::mat4 modelMat = glm::mat4(1.0f);
                modelMat = glm::translate(modelMat, transform.position);
                modelMat = modelMat * glm::mat4_cast(transform.rotation);
                modelMat = glm::scale(modelMat, transform.scale);
                mesh.model->Draw(shader, camera, modelMat);
            }
        }
    }

    void DrawShadow(Shader& shader, Camera& camera) {
        for (auto& [entity, mesh] : meshes) {
            if (mesh.model != nullptr && transforms.find(entity) != transforms.end()) {
                auto& transform = transforms[entity];
                glm::mat4 modelMat = glm::mat4(1.0f);
                modelMat = glm::translate(modelMat, transform.position);
                modelMat = modelMat * glm::mat4_cast(transform.rotation);
                modelMat = glm::scale(modelMat, transform.scale);
                mesh.model->Draw(shader, camera, modelMat);
            }
        }
    }

    // PUBLIC maps for Editor access
    std::unordered_map<Entity, TransformComponent> transforms;
    std::unordered_map<Entity, MeshComponent> meshes;
    std::unordered_map<Entity, VelocityComponent> velocities;

private:
    Entity nextEntityID = 0;
    std::vector<Entity> entities;
};
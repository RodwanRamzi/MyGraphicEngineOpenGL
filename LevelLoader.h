#pragma once
#include "ECS.h"
#include "Model.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <json/json.h>

using json = nlohmann::json;

class LevelLoader {
public:
    static bool LoadLevel(const char* filepath, ECSWorld& world) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "ERROR: Could not open level file: " << filepath << std::endl;
            return false;
        }

        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        try {
            json j = json::parse(content);

            if (!j.contains("entities")) {
                std::cerr << "ERROR: No entities array in level file." << std::endl;
                return false;
            }

            for (const auto& entityData : j["entities"]) {
                Entity newEntity = world.CreateEntity();

                if (entityData.contains("position")) {
                    auto pos = entityData["position"];
                    world.AddComponent(newEntity, TransformComponent{
                        glm::vec3(pos[0], pos[1], pos[2]),
                        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                        glm::vec3(1.0f)
                        });
                }

                if (entityData.contains("model")) {
                    std::string modelPath = entityData["model"];
                    Model* loadedModel = new Model(modelPath.c_str());
                    world.AddComponent(newEntity, MeshComponent{ loadedModel });
                }

                if (entityData.contains("velocity")) {
                    auto vel = entityData["velocity"];
                    world.AddComponent(newEntity, VelocityComponent{
                        vel.value("speed", 5.0f),
                        vel.value("y", 0.0f),
                        vel.value("jumpVelocity", 7.5f),
                        glm::vec3(0.0f)
                        });
                }
            }
        }
        catch (const json::exception& e) {
            std::cerr << "JSON Parse Error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }
};
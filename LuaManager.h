#pragma once
#include <sol/sol.hpp>
#include <unordered_map>
#include "ScriptComponent.h"
#include "EditorEntity.h" // if you have the entity struct

class LuaManager {
public:
    LuaManager();
    ~LuaManager();

    bool loadScript(const std::string& path, ScriptComponent& comp);
    bool callFunction(ScriptComponent& comp, const std::string& funcName, float dt = 0.0f);
    void update(float dt, std::vector<EditorEntity>& entities); // call all scripts' Update(dt)
    void exposeFunctions(sol::state& lua); // bind C++ functions

private:
    sol::state lua;
    std::unordered_map<std::string, sol::function> loadedFunctions;
};
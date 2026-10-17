#include "ScriptComponent.h"
#include <iostream>

ScriptComponent::ScriptComponent(sol::state& state, const std::string& path, int idx)
    : luaState(&state), scriptPath(path), entityIndex(idx) {
    if (!luaState) return;
    reload(state);
}

void ScriptComponent::reload(sol::state& state) {
    luaState = &state;
    loaded = false;
    onStartFunc = sol::protected_function();
    onUpdateFunc = sol::protected_function();
    onDestroyFunc = sol::protected_function();

    // تنفيذ ملف Lua
    sol::protected_function_result result = state.script_file(scriptPath);

    if (result.valid()) {
        // جلب الدوال العامة من Lua
        onStartFunc = state["onStart"];
        onUpdateFunc = state["onUpdate"];
        onDestroyFunc = state["onDestroy"];
        loaded = true;
        std::cout << "[Lua] ✅ Script loaded: " << scriptPath << std::endl;
    }
    else {
        sol::error err = result;
        std::cerr << "[Lua] ❌ Error loading script: " << err.what() << std::endl;
        loaded = false;
    }
}

void ScriptComponent::start() {
    if (loaded && onStartFunc.valid()) {
        // استدعاء onStart() مع تمرير رقم الكيان كمعامل اختياري
        sol::protected_function_result result = onStartFunc(entityIndex);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[Lua] Error in onStart: " << err.what() << std::endl;
        }
    }
}

void ScriptComponent::update(float deltaTime) {
    if (loaded && onUpdateFunc.valid()) {
        sol::protected_function_result result = onUpdateFunc(deltaTime, entityIndex);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[Lua] Error in onUpdate: " << err.what() << std::endl;
        }
    }
}

void ScriptComponent::destroy() {
    if (loaded && onDestroyFunc.valid()) {
        sol::protected_function_result result = onDestroyFunc(entityIndex);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[Lua] Error in onDestroy: " << err.what() << std::endl;
        }
    }
}
#pragma once
#include <sol/sol.hpp>
#include <string>
#include <memory>

struct EditorEntity; // تعريف مسبق

class ScriptComponent {
private:
    sol::state* luaState = nullptr;          // مؤشر لمفسر Lua
    sol::protected_function onStartFunc;     // دالة البداية
    sol::protected_function onUpdateFunc;    // دالة التحديث
    sol::protected_function onDestroyFunc;   // دالة التدمير
    bool loaded = false;
    int entityIndex = -1;                    // رقم الكيان في المتجه (للأمان)

public:
    std::string scriptPath;                  // مسار الملف (لإعادة التحميل)

    ScriptComponent() = default;
    ScriptComponent(sol::state& state, const std::string& path, int idx);

    void start();
    void update(float deltaTime);
    void destroy();
    void setEntityIndex(int idx) { entityIndex = idx; }
    bool isLoaded() const { return loaded; }

    // إعادة تحميل السكريبت
    void reload(sol::state& state);
};
#include "Theme.h"
#include <string>
#include <cstring>

void SetDarkProTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(10,10); s.FramePadding = ImVec2(8,6);
    s.ItemSpacing = ImVec2(8,6);     s.ItemInnerSpacing = ImVec2(4,4);
    s.IndentSpacing = 20.0f; s.ScrollbarSize = 14.0f; s.GrabMinSize = 12.0f;
    s.WindowRounding = 4.0f; s.FrameRounding = 4.0f; s.GrabRounding = 4.0f; s.PopupRounding = 4.0f;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.10f,0.10f,0.12f,1);
    c[ImGuiCol_ChildBg]  = ImVec4(0.08f,0.08f,0.10f,1);
    c[ImGuiCol_PopupBg]  = ImVec4(0.10f,0.10f,0.12f,1);
    c[ImGuiCol_Border]   = ImVec4(0.20f,0.20f,0.25f,1);
    c[ImGuiCol_FrameBg]  = ImVec4(0.15f,0.15f,0.18f,1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f,0.22f,0.28f,1);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.18f,0.18f,0.22f,1);
    c[ImGuiCol_TitleBg]        = ImVec4(0.12f,0.12f,0.15f,1);
    c[ImGuiCol_TitleBgActive]  = ImVec4(0.18f,0.18f,0.22f,1);
    c[ImGuiCol_MenuBarBg]      = ImVec4(0.08f,0.08f,0.10f,1);
    c[ImGuiCol_ScrollbarBg]    = ImVec4(0.05f,0.05f,0.07f,1);
    c[ImGuiCol_ScrollbarGrab]  = ImVec4(0.20f,0.20f,0.25f,1);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f,0.30f,0.35f,1);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.40f,0.40f,0.45f,1);
    c[ImGuiCol_CheckMark]      = ImVec4(0.20f,0.60f,0.90f,1);
    c[ImGuiCol_SliderGrab]     = ImVec4(0.20f,0.60f,0.90f,1);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.30f,0.70f,1.00f,1);
    c[ImGuiCol_Button]         = ImVec4(0.15f,0.15f,0.18f,1);
    c[ImGuiCol_ButtonHovered]  = ImVec4(0.22f,0.22f,0.28f,1);
    c[ImGuiCol_ButtonActive]   = ImVec4(0.28f,0.28f,0.34f,1);
    c[ImGuiCol_Header]         = ImVec4(0.15f,0.15f,0.18f,1);
    c[ImGuiCol_HeaderHovered]  = ImVec4(0.22f,0.22f,0.28f,1);
    c[ImGuiCol_HeaderActive]   = ImVec4(0.28f,0.28f,0.34f,1);
    c[ImGuiCol_Separator]      = ImVec4(0.20f,0.20f,0.25f,1);
    c[ImGuiCol_Text]           = ImVec4(0.85f,0.85f,0.90f,1);
    c[ImGuiCol_TextDisabled]   = ImVec4(0.40f,0.40f,0.45f,1);
    c[ImGuiCol_PlotLines]      = ImVec4(0.20f,0.60f,0.90f,1);
    c[ImGuiCol_PlotHistogram]  = ImVec4(0.20f,0.60f,0.90f,1);
}
void ResetSliderButton(const char* label, float* value, float defaultValue, const char* tooltip) {
    ImGui::SameLine();
    if (ImGui::SmallButton(("R##" + std::string(label)).c_str())) *value = defaultValue;
    if (!tooltip || std::strlen(tooltip) > 0)
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset to default: %.2f", defaultValue);
}
#pragma once
#include "GameState.h"
#include "imgui.h"

class MainMenu {
public:
    void Show(GameStateMachine& stateMachine, bool& shouldClose) {
        ImGui::Begin("Main Menu");
        ImGui::SetWindowSize(ImVec2(300, 200));
        ImGui::Text("Rodwan Engine");
        if (ImGui::Button("Level Editor")) {
            stateMachine.ChangeState(GameState::EDITOR);
        }
        if (ImGui::Button("Play", ImVec2(250, 40))) {
            stateMachine.ChangeState(GameState::LOADING);
        }
        if (ImGui::Button("Quit", ImVec2(250, 40))) {
            shouldClose = true;
        }
        ImGui::End();
    }

    void ShowPauseMenu(GameStateMachine& stateMachine) {
        ImGui::Begin("Paused");
        if (ImGui::Button("Resume", ImVec2(250, 40))) {
            stateMachine.ChangeState(GameState::GAMEPLAY);
        }
        if (ImGui::Button("Back to Menu", ImVec2(250, 40))) {
            stateMachine.ChangeState(GameState::MAIN_MENU);
        }
        ImGui::End();
    }
};
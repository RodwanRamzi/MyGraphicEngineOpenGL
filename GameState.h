#pragma once

enum class GameState {
    MAIN_MENU,
    LOADING,
    GAMEPLAY,
    PAUSED,
    GAME_OVER,
    EDITOR
};

class GameStateMachine {
public:
    GameState currentState = GameState::MAIN_MENU;

    void ChangeState(GameState newState) {
        currentState = newState;
    }
};
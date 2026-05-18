#pragma once

enum class SceneId { MainMenu, Playing, Paused, GameOver, Settings };

enum class MenuAction {
    None,
    NewGame,
    Continue,
    Resume,
    ToSettings,
    ToMain,
    Retry,
    BackFromSettings,
    Quit
};

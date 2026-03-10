#pragma once

struct FGameContext {
    class URenderer* Renderer;

    FGameContext(URenderer* renderer) :
        Renderer(renderer) {
    }
};

enum class SceneType {
    Init,
    InGame,
    Outro,
};

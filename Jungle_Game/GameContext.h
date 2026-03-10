#pragma once

struct FGameContext {
    class URenderer* Renderer;
    class UTimer* Timer;

    FGameContext(URenderer* renderer, UTimer* timer) :
        Renderer(renderer), Timer(timer){
    }
};

enum class SceneType {
    Init,
    InGame,
    Outro,
};

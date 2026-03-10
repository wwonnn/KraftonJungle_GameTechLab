#pragma once

struct FGameContext {
    class URenderer* Renderer;
    class UAudioSystem* AudioSystem;

    FGameContext(URenderer* renderer, UAudioSystem* audioSystem) :
        Renderer(renderer), AudioSystem(audioSystem) {
    }
};

enum class SceneType {
    Init,
    InGame,
    Outro,
};

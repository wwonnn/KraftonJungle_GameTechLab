#pragma once

class Scene {
public:
    virtual ~Scene() {}
    virtual void Initialize() = 0;
    virtual void Update(float dt) = 0;
    virtual void Render() = 0;
    virtual void Release() = 0;
};

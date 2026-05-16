#pragma once

#include "Animation/AnimInstance.h"
#include "ThirdParty/sol/sol.hpp"

class ULuaAnimInstance : public UAnimInstance
{
public:
    DECLARE_CLASS(ULuaAnimInstance, UAnimInstance)

public:
    void SetScriptName(const FString& InScriptName) { ScriptName = InScriptName; }
    const FString& GetScriptName() const { return ScriptName; }

protected:
    void NativeInitializeAnimation() override;
    void NativeUpdateAnimation(float DeltaTime) override;

private:
    void ClearLuaStateReferences();
    bool LoadScript();
    bool CreateLuaInstance(const sol::table& ScriptClass);
    void CallLuaFunction(const char* FunctionName);
    void CallLuaUpdate(float DeltaTime);

private:
    FString ScriptName = "LuaAnimation";
    sol::environment ScriptEnv;
    sol::table ScriptClass;
    sol::table ScriptInstance;
};

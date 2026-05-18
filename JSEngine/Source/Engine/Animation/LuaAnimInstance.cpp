#include "LuaAnimInstance.h"

#include "Animation/AnimationStateMachine.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Object/ObjectFactory.h"
#include "Runtime/Script/ScriptManager.h"

#include <filesystem>
#include <fstream>
#include <sstream>

DEFINE_CLASS(ULuaAnimInstance, UAnimInstance)
REGISTER_FACTORY(ULuaAnimInstance)

namespace
{
    bool ReadLuaFile(const FString& ScriptPath, FString& OutSource)
    {
        std::ifstream File(std::filesystem::path(FPaths::ToWide(ScriptPath)), std::ios::binary);
        if (!File.is_open())
        {
            return false;
        }

        std::ostringstream Stream;
        Stream << File.rdbuf();
        OutSource = Stream.str();
        return true;
    }
}

void ULuaAnimInstance::NativeInitializeAnimation()
{
    LoadScript();
    CallLuaFunction("NativeInitializeAnimation");
}

void ULuaAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    CallLuaUpdate(DeltaTime);
}

void ULuaAnimInstance::NativeAnimNotify(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent)
{
    (void)Sequence;
    CallLuaNotify("AnimNotify", NotifyEvent);
    CallNamedLuaNotify("AnimNotify_", NotifyEvent);
}

void ULuaAnimInstance::NativeAnimNotifyBegin(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent)
{
    (void)Sequence;
    CallLuaNotify("AnimNotifyBegin", NotifyEvent);
    CallNamedLuaNotify("AnimNotifyBegin_", NotifyEvent);
}

void ULuaAnimInstance::NativeAnimNotifyTick(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent, float DeltaTime)
{
    (void)Sequence;
    CallLuaNotifyTick("AnimNotifyTick", NotifyEvent, DeltaTime);
    CallNamedLuaNotifyTick("AnimNotifyTick_", NotifyEvent, DeltaTime);
}

void ULuaAnimInstance::NativeAnimNotifyEnd(const UAnimSequence* Sequence, const FAnimNotifyEvent& NotifyEvent)
{
    (void)Sequence;
    CallLuaNotify("AnimNotifyEnd", NotifyEvent);
    CallNamedLuaNotify("AnimNotifyEnd_", NotifyEvent);
}

void ULuaAnimInstance::ClearLuaStateReferences()
{
    ScriptEnv = sol::environment{};
    ScriptClass = sol::table{};
    ScriptInstance = sol::table{};
}

bool ULuaAnimInstance::LoadScript()
{
    if (ScriptName.empty())
    {
        ClearLuaStateReferences();
        return false;
    }

    sol::state* Lua = FScriptManager::Get().GetGlobalLuaState();
    if (!Lua)
    {
        UE_LOG_ERROR("[LuaAnimInstance] Lua state is null");
        ClearLuaStateReferences();
        return false;
    }

    FString ScriptPath;
    if (!FScriptManager::Get().ResolveScriptPath(ScriptName, ScriptPath))
    {
        UE_LOG_WARNING("[LuaAnimInstance] Script not found: %s", ScriptName.c_str());
        ClearLuaStateReferences();
        return false;
    }

    FString ScriptSource;
    if (!ReadLuaFile(ScriptPath, ScriptSource))
    {
        UE_LOG_ERROR("[LuaAnimInstance] Failed to read script: %s", ScriptPath.c_str());
        ClearLuaStateReferences();
        return false;
    }

    sol::environment Env(*Lua, sol::create, Lua->globals());
    Env["AnimInstance"] = this;
    Env["Owner"] = GetOwningComponent();

    sol::protected_function_result Result = Lua->safe_script(ScriptSource, Env);
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG_ERROR("[LuaAnimInstance] Lua load error: %s", Err.what());
        ClearLuaStateReferences();
        return false;
    }

    sol::object ReturnObj = Result;
    if (!ReturnObj.valid() || ReturnObj.get_type() != sol::type::table)
    {
        UE_LOG_ERROR("[LuaAnimInstance] Script must return table: %s", ScriptName.c_str());
        ClearLuaStateReferences();
        return false;
    }

    ScriptEnv = std::move(Env);
    ScriptClass = ReturnObj.as<sol::table>();

    if (!CreateLuaInstance(ScriptClass))
    {
        ClearLuaStateReferences();
        return false;
    }

    return true;
}

bool ULuaAnimInstance::CreateLuaInstance(const sol::table& InScriptClass)
{
    sol::object NewObj = InScriptClass["new"];
    if (!NewObj.valid() || NewObj.get_type() != sol::type::function)
    {
        ScriptInstance = InScriptClass;
        ScriptInstance["AnimInstance"] = this;
        ScriptInstance["StateMachine"] = GetStateMachine();
        return true;
    }

    sol::protected_function NewFunc = NewObj.as<sol::protected_function>();
    sol::protected_function_result NewResult = NewFunc(this);
    if (!NewResult.valid())
    {
        sol::error Err = NewResult;
        UE_LOG_ERROR("[LuaAnimInstance] Script.new failed: %s", Err.what());
        return false;
    }

    sol::object InstanceObj = NewResult;
    if (!InstanceObj.valid() || InstanceObj.get_type() != sol::type::table)
    {
        UE_LOG_ERROR("[LuaAnimInstance] Script.new must return table: %s", ScriptName.c_str());
        return false;
    }

    ScriptInstance = InstanceObj.as<sol::table>();
    return true;
}

void ULuaAnimInstance::CallLuaFunction(const char* FunctionName)
{
    if (!ScriptInstance.valid())
    {
        return;
    }

    sol::object FuncObj = ScriptInstance[FunctionName];
    if (!FuncObj.valid() || FuncObj.get_type() != sol::type::function)
    {
        return;
    }

    sol::protected_function Func = FuncObj.as<sol::protected_function>();
    sol::protected_function_result Result = Func(ScriptInstance);
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG_ERROR("[LuaAnimInstance] Lua Error in %s: %s", FunctionName, Err.what());
    }
}

void ULuaAnimInstance::CallLuaUpdate(float DeltaTime)
{
    if (!ScriptInstance.valid())
    {
        return;
    }

    sol::object FuncObj = ScriptInstance["NativeUpdateAnimation"];
    if (!FuncObj.valid() || FuncObj.get_type() != sol::type::function)
    {
        return;
    }

    sol::protected_function Func = FuncObj.as<sol::protected_function>();
    sol::protected_function_result Result = Func(ScriptInstance, DeltaTime);
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG_ERROR("[LuaAnimInstance] Lua Error in NativeUpdateAnimation: %s", Err.what());
    }
}

void ULuaAnimInstance::CallLuaNotify(const char* FunctionName, const FAnimNotifyEvent& NotifyEvent)
{
    if (!ScriptInstance.valid())
    {
        return;
    }

    sol::object FuncObj = ScriptInstance[FunctionName];
    if (!FuncObj.valid() || FuncObj.get_type() != sol::type::function)
    {
        return;
    }

    sol::protected_function Func = FuncObj.as<sol::protected_function>();
    sol::protected_function_result Result = Func(ScriptInstance, NotifyEvent);
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG_ERROR("[LuaAnimInstance] Lua Error in %s: %s", FunctionName, Err.what());
    }
}

void ULuaAnimInstance::CallLuaNotifyTick(const char* FunctionName, const FAnimNotifyEvent& NotifyEvent, float DeltaTime)
{
    if (!ScriptInstance.valid())
    {
        return;
    }

    sol::object FuncObj = ScriptInstance[FunctionName];
    if (!FuncObj.valid() || FuncObj.get_type() != sol::type::function)
    {
        return;
    }

    sol::protected_function Func = FuncObj.as<sol::protected_function>();
    sol::protected_function_result Result = Func(ScriptInstance, NotifyEvent, DeltaTime);
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG_ERROR("[LuaAnimInstance] Lua Error in %s: %s", FunctionName, Err.what());
    }
}

void ULuaAnimInstance::CallNamedLuaNotify(const char* Prefix, const FAnimNotifyEvent& NotifyEvent)
{
    if (!NotifyEvent.Name.IsValid())
    {
        return;
    }

    const FString FunctionName = FString(Prefix) + NotifyEvent.Name.ToString();
    CallLuaNotify(FunctionName.c_str(), NotifyEvent);
}

void ULuaAnimInstance::CallNamedLuaNotifyTick(const char* Prefix, const FAnimNotifyEvent& NotifyEvent, float DeltaTime)
{
    if (!NotifyEvent.Name.IsValid())
    {
        return;
    }

    const FString FunctionName = FString(Prefix) + NotifyEvent.Name.ToString();
    CallLuaNotifyTick(FunctionName.c_str(), NotifyEvent, DeltaTime);
}

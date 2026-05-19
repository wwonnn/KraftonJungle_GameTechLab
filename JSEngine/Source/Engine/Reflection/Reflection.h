#pragma once

#include "Core/Containers/Array.h"
#include "Reflection/Property.h"
#include "Reflection/Enum.h"
#include "Reflection/Struct.h"
#include "Reflection/Class.h"

#include <utility>

class UObject;
struct FArchive;

#define UPROPERTY(...)
#define USTRUCT(...)
#define UCLASS(...)
#define UENUM(...)
#define UMETA(...)

#define REFLECTION_BODY_MACRO_COMBINE_INNER(A, B, C, D) A##B##C##D
#define REFLECTION_BODY_MACRO_COMBINE(A, B, C, D) REFLECTION_BODY_MACRO_COMBINE_INNER(A, B, C, D)
#define GENERATED_BODY(...) REFLECTION_BODY_MACRO_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY)

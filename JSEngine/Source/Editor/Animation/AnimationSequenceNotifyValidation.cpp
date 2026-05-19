#include "Editor/Animation/AnimationSequenceNotifyValidation.h"

#include "Animation/AnimNotify.h"
#include "Animation/AnimNotifyPayloadParser.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Editor/Animation/AnimationSequenceNotifyPayloadSchema.h"
#include "Editor/Animation/AnimationSequencePreviewController.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"
#include "Object/ObjectFactory.h"

#include <algorithm>
#include <string>

namespace
{
    const UClass* FindRegisteredClass(const FString& TypeName)
    {
        if (TypeName.empty())
        {
            return nullptr;
        }

        TArray<const UClass*> RegisteredClasses;
        FObjectFactory::Get().GetRegisteredClasses(RegisteredClasses);
        for (const UClass* Class : RegisteredClasses)
        {
            if (Class && Class->GetName() && TypeName == Class->GetName())
            {
                return Class;
            }
        }

        return nullptr;
    }

    bool IsNotifyClassCompatible(const UClass* Class, EAnimNotifyEventType EventType)
    {
        if (!Class)
        {
            return false;
        }

        if (EventType == EAnimNotifyEventType::NotifyState)
        {
            return Class->IsA(UAnimNotifyState::StaticClass());
        }

        return Class->IsA(UAnimNotify::StaticClass()) && !Class->IsA(UAnimNotifyState::StaticClass());
    }

    FString GetNotifyTypeLabel(EAnimNotifyEventType EventType)
    {
        return EventType == EAnimNotifyEventType::NotifyState ? "Notify State" : "Notify";
    }

    void AddRequiredPayloadIssues(
        FAnimNotifyValidationReport& Report,
        const FAnimNotifyPayloadSchema& Schema,
        const FAnimNotifyPayloadParser& Payload,
        int32 TrackIndex,
        int32 EventIndex,
        const FGuid& StableId)
    {
        const char* PayloadExample = GetAnimNotifyPayloadExample(Schema.NotifyClassName);
        const FString PayloadExampleHint = PayloadExample ? FString("Payload Example: ") + PayloadExample : FString();
        for (const FAnimNotifyPayloadFieldDefinition& Field : Schema.Fields)
        {
            if (!Field.bRequired)
            {
                continue;
            }

            const TArray<FString> LookupKeys = GetAnimNotifyPayloadFieldLookupKeys(Field);
            if (!Payload.HasAnyValue(LookupKeys) || Payload.GetStringAny(LookupKeys).empty())
            {
                Report.AddIssue(
                    EAnimNotifyValidationSeverity::Error,
                    EAnimNotifyValidationField::Payload,
                    Field.Label + " is required for " + Schema.NotifyClassName + ".",
                    PayloadExampleHint,
                    TrackIndex,
                    EventIndex,
                    StableId);
            }
        }
    }

    void AddMalformedPayloadIssues(
        FAnimNotifyValidationReport& Report,
        const FAnimNotifyPayloadSchema& Schema,
        const FAnimNotifyPayloadParser& Payload,
        int32 TrackIndex,
        int32 EventIndex,
        const FGuid& StableId)
    {
        const char* PayloadExample = GetAnimNotifyPayloadExample(Schema.NotifyClassName);
        const FString PayloadExampleHint = PayloadExample ? FString("Payload Example: ") + PayloadExample : FString();
        for (const FAnimNotifyPayloadFieldDefinition& Field : Schema.Fields)
        {
            const TArray<FString> LookupKeys = GetAnimNotifyPayloadFieldLookupKeys(Field);
            if (Field.Type == EAnimNotifyPayloadFieldType::Float)
            {
                float ParsedValue = 0.0f;
                if (Payload.HasAnyValue(LookupKeys) && !Payload.TryGetFloatAny(LookupKeys, ParsedValue))
                {
                    Report.AddIssue(
                        EAnimNotifyValidationSeverity::Warning,
                        EAnimNotifyValidationField::Payload,
                        Field.Label + " value is invalid. Expected a numeric value.",
                        PayloadExampleHint,
                        TrackIndex,
                        EventIndex,
                        StableId);
                }
            }
            else if (Field.Type == EAnimNotifyPayloadFieldType::Bool)
            {
                bool ParsedValue = false;
                if (Payload.HasAnyValue(LookupKeys) && !Payload.TryGetBoolAny(LookupKeys, ParsedValue))
                {
                    Report.AddIssue(
                        EAnimNotifyValidationSeverity::Warning,
                        EAnimNotifyValidationField::Payload,
                        Field.Label + " value is invalid. Expected true/false.",
                        PayloadExampleHint,
                        TrackIndex,
                        EventIndex,
                        StableId);
                }
            }
        }
    }

    void AddPreviewContextIssues(
        FAnimNotifyValidationReport& Report,
        const FAnimNotifyPayloadSchema& Schema,
        const FAnimNotifyPayloadParser& Payload,
        const FAnimNotifyValidationContext& Context,
        int32 TrackIndex,
        int32 EventIndex,
        const FGuid& StableId)
    {
        if (!Context.PreviewController || !Context.PreviewController->HasValidPreview())
        {
            return;
        }

        if (const FAnimNotifyPayloadFieldDefinition* SocketField =
            FindAnimNotifyPayloadFieldBySemanticId(Schema, EAnimNotifySemanticFieldId::SocketName))
        {
            const FName SocketName = Payload.GetNameAny(GetAnimNotifyPayloadFieldLookupKeys(*SocketField));
            if (SocketName != FName::None && !Context.PreviewController->HasPreviewSocket(SocketName))
            {
                Report.AddIssue(
                    EAnimNotifyValidationSeverity::Warning,
                    EAnimNotifyValidationField::Payload,
                    "Socket \"" + SocketName.ToString() + "\" is not present on the current preview mesh.",
                    "Verify the socket name or pick a different preview mesh.",
                    TrackIndex,
                    EventIndex,
                    StableId);
            }
        }

        if (Schema.NotifyClassName == "UAnimNotifyState_AttackWindow")
        {
            const FAnimNotifyPayloadFieldDefinition* ComponentField =
                FindAnimNotifyPayloadFieldBySemanticId(Schema, EAnimNotifySemanticFieldId::ComponentName);
            const FString ComponentName = ComponentField
                ? Payload.GetStringAny(GetAnimNotifyPayloadFieldLookupKeys(*ComponentField))
                : FString();
            if (!ComponentName.empty() && !Context.PreviewController->HasPreviewPrimitiveComponent(ComponentName))
            {
                Report.AddIssue(
                    EAnimNotifyValidationSeverity::Warning,
                    EAnimNotifyValidationField::Payload,
                    "Component \"" + ComponentName + "\" was not found on the current preview actor.",
                    "AttackWindow payloads target primitive components by name.",
                    TrackIndex,
                    EventIndex,
                    StableId);
            }
        }
    }
}

void FAnimNotifyValidationReport::AddIssue(const FAnimNotifyValidationIssue& Issue)
{
    Issues.push_back(Issue);

    switch (Issue.Severity)
    {
    case EAnimNotifyValidationSeverity::Error:
        ++ErrorCount;
        break;
    case EAnimNotifyValidationSeverity::Warning:
        ++WarningCount;
        break;
    case EAnimNotifyValidationSeverity::Info:
    default:
        ++InfoCount;
        break;
    }
}

void FAnimNotifyValidationReport::AddIssue(
    EAnimNotifyValidationSeverity Severity,
    EAnimNotifyValidationField Field,
    const FString& Message,
    const FString& Hint,
    int32 TrackIndex,
    int32 EventIndex,
    const FGuid& StableId)
{
    FAnimNotifyValidationIssue Issue;
    Issue.Severity = Severity;
    Issue.Field = Field;
    Issue.Message = Message;
    Issue.Hint = Hint;
    Issue.TrackIndex = TrackIndex;
    Issue.EventIndex = EventIndex;
    Issue.StableId = StableId;
    AddIssue(Issue);
}

void FAnimNotifyValidationReport::Append(const FAnimNotifyValidationReport& Other)
{
    for (const FAnimNotifyValidationIssue& Issue : Other.Issues)
    {
        AddIssue(Issue);
    }
}

int32 FAnimNotifyValidationReport::GetCount(EAnimNotifyValidationSeverity Severity) const
{
    switch (Severity)
    {
    case EAnimNotifyValidationSeverity::Error:
        return ErrorCount;
    case EAnimNotifyValidationSeverity::Warning:
        return WarningCount;
    case EAnimNotifyValidationSeverity::Info:
    default:
        return InfoCount;
    }
}

bool FAnimNotifyValidationReport::HasAnyIssues() const
{
    return !Issues.empty();
}

bool FAnimNotifyValidationReport::HasIssuesForField(EAnimNotifyValidationField Field) const
{
    return std::any_of(
        Issues.begin(),
        Issues.end(),
        [Field](const FAnimNotifyValidationIssue& Issue)
        {
            return Issue.Field == Field;
        });
}

bool FAnimNotifyValidationReport::HasIssuesOfSeverity(EAnimNotifyValidationSeverity Severity) const
{
    return std::any_of(
        Issues.begin(),
        Issues.end(),
        [Severity](const FAnimNotifyValidationIssue& Issue)
        {
            return Issue.Severity == Severity;
        });
}

FAnimNotifyValidationReport ValidateAnimNotifyEvent(
    const FAnimNotifyEvent& NotifyEvent,
    int32 TrackIndex,
    int32 EventIndex,
    const FAnimNotifyValidationContext& Context)
{
    FAnimNotifyValidationReport Report;
    const FString NotifyClassName = NotifyEvent.GetResolvedNotifyClassName();
    const UClass* NotifyClass = FindRegisteredClass(NotifyClassName);
    if (!NotifyClass)
    {
        Report.AddIssue(
            EAnimNotifyValidationSeverity::Error,
            EAnimNotifyValidationField::NotifyClass,
            "Notify Class is not registered: " + NotifyClassName,
            "Choose a registered notify class from the dropdown.",
            TrackIndex,
            EventIndex,
            NotifyEvent.StableId);
    }
    else if (!IsNotifyClassCompatible(NotifyClass, NotifyEvent.EventType))
    {
        Report.AddIssue(
            EAnimNotifyValidationSeverity::Error,
            EAnimNotifyValidationField::NotifyClass,
            "Selected class does not match the " + GetNotifyTypeLabel(NotifyEvent.EventType) + " type.",
            "Use a class derived from " + GetDefaultAnimNotifyClassName(NotifyEvent.EventType) + ".",
            TrackIndex,
            EventIndex,
            NotifyEvent.StableId);
    }

    if (NotifyEvent.EventType == EAnimNotifyEventType::NotifyState && NotifyEvent.Duration <= 0.0f)
    {
        Report.AddIssue(
            EAnimNotifyValidationSeverity::Warning,
            EAnimNotifyValidationField::Duration,
            "Notify State duration is zero or negative. Begin/End will fire at the same time.",
            "Set a positive duration for state-style behavior.",
            TrackIndex,
            EventIndex,
            NotifyEvent.StableId);
    }
    else if (NotifyEvent.EventType == EAnimNotifyEventType::Notify && NotifyEvent.Duration > 0.0f)
    {
        Report.AddIssue(
            EAnimNotifyValidationSeverity::Info,
            EAnimNotifyValidationField::Duration,
            "Duration is ignored for one-shot notifies.",
            "Use Notify State if you need a begin/end window.",
            TrackIndex,
            EventIndex,
            NotifyEvent.StableId);
    }

    if (!NotifyEvent.Name.IsValid() || NotifyEvent.Name.ToString().empty())
    {
        Report.AddIssue(
            EAnimNotifyValidationSeverity::Info,
            EAnimNotifyValidationField::Name,
            "Notify name is empty.",
            "Set a name to make timeline markers and runtime logs easier to read.",
            TrackIndex,
            EventIndex,
            NotifyEvent.StableId);
    }

    if (const FAnimNotifyPayloadSchema* Schema = FindAnimNotifyPayloadSchema(NotifyClassName))
    {
        const FAnimNotifyPayloadParser Payload(NotifyEvent.Payload);
        AddRequiredPayloadIssues(Report, *Schema, Payload, TrackIndex, EventIndex, NotifyEvent.StableId);
        AddMalformedPayloadIssues(Report, *Schema, Payload, TrackIndex, EventIndex, NotifyEvent.StableId);
        AddPreviewContextIssues(Report, *Schema, Payload, Context, TrackIndex, EventIndex, NotifyEvent.StableId);
    }

    return Report;
}

FAnimNotifyValidationReport ValidateAnimNotifyDocument(
    const UAnimSequence* Sequence,
    const FAnimNotifyValidationContext& Context)
{
    FAnimNotifyValidationReport Report;
    const TArray<FAnimNotifyTrack>* Tracks = AnimationSequenceViewer::GetSequenceNotifyTracks(Sequence);
    if (!Tracks)
    {
        return Report;
    }

    TMap<FString, int32> StableIdCounts;
    for (const FAnimNotifyTrack& Track : *Tracks)
    {
        for (const FAnimNotifyEvent& NotifyEvent : Track.Events)
        {
            if (NotifyEvent.StableId.IsValid())
            {
                StableIdCounts[NotifyEvent.StableId.ToString()]++;
            }
        }
    }

    for (int32 TrackIndex = 0; TrackIndex < static_cast<int32>(Tracks->size()); ++TrackIndex)
    {
        const FAnimNotifyTrack& Track = (*Tracks)[TrackIndex];
        for (int32 EventIndex = 0; EventIndex < static_cast<int32>(Track.Events.size()); ++EventIndex)
        {
            const FAnimNotifyEvent& NotifyEvent = Track.Events[EventIndex];
            Report.Append(ValidateAnimNotifyEvent(NotifyEvent, TrackIndex, EventIndex, Context));

            if (NotifyEvent.StableId.IsValid())
            {
                const auto CountIterator = StableIdCounts.find(NotifyEvent.StableId.ToString());
                if (CountIterator != StableIdCounts.end() && CountIterator->second > 1)
                {
                    Report.AddIssue(
                        EAnimNotifyValidationSeverity::Error,
                        EAnimNotifyValidationField::General,
                        "Duplicate StableId detected: " + NotifyEvent.StableId.ToString(),
                        "Each notify event should keep a unique StableId for reliable selection and save behavior.",
                        TrackIndex,
                        EventIndex,
                        NotifyEvent.StableId);
                }
            }
        }
    }

    return Report;
}

FString FormatAnimNotifyValidationSummary(const FAnimNotifyValidationReport& Report)
{
    if (Report.ErrorCount <= 0 && Report.WarningCount <= 0 && Report.InfoCount <= 0)
    {
        return "No notify validation issues.";
    }

    FString Summary;
    if (Report.ErrorCount > 0)
    {
        Summary += std::to_string(Report.ErrorCount) + " error";
        if (Report.ErrorCount != 1)
        {
            Summary += "s";
        }
    }

    if (Report.WarningCount > 0)
    {
        if (!Summary.empty())
        {
            Summary += ", ";
        }
        Summary += std::to_string(Report.WarningCount) + " warning";
        if (Report.WarningCount != 1)
        {
            Summary += "s";
        }
    }

    if (Report.InfoCount > 0)
    {
        if (!Summary.empty())
        {
            Summary += ", ";
        }
        Summary += std::to_string(Report.InfoCount) + " info";
        if (Report.InfoCount != 1)
        {
            Summary += "s";
        }
    }

    return Summary;
}

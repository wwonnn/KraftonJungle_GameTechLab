#include "Editor/UI/EditorAnimInstanceEditorWidget.h"

#include "Animation/AnimInstanceAsset.h"
#include "Core/ResourceManager.h"
#include "Editor/EditorEngine.h"
#include "ImGui/imgui.h"

#include <algorithm>

namespace
{
    FString MakeStateName(int32 Index)
    {
        return FString("State") + std::to_string(Index);
    }

    int32 FindStateIndex(const UAnimInstanceAsset* Asset, const FName& Name)
    {
        if (!Asset)
        {
            return -1;
        }

        for (int32 Index = 0; Index < static_cast<int32>(Asset->States.size()); ++Index)
        {
            if (Asset->States[Index].Name == Name)
            {
                return Index;
            }
        }
        return -1;
    }

    const char* ToConditionLabel(EAnimTransitionConditionType Type)
    {
        switch (Type)
        {
        case EAnimTransitionConditionType::BoolEquals:
            return "Bool Equals";
        case EAnimTransitionConditionType::FloatGreater:
            return "Float Greater";
        case EAnimTransitionConditionType::FloatLessEqual:
            return "Float Less Equal";
        case EAnimTransitionConditionType::IntEquals:
            return "Int Equals";
        case EAnimTransitionConditionType::Trigger:
            return "Trigger";
        case EAnimTransitionConditionType::Native:
        default:
            return "Native";
        }
    }

    void InputFString(const char* Label, FString& Value, bool& bChanged)
    {
        char Buffer[512] = {};
        strncpy_s(Buffer, sizeof(Buffer), Value.c_str(), _TRUNCATE);
        if (ImGui::InputText(Label, Buffer, sizeof(Buffer)))
        {
            Value = Buffer;
            bChanged = true;
        }
    }

    void InputFName(const char* Label, FName& Value, bool& bChanged)
    {
        FString Text = Value.ToString();
        InputFString(Label, Text, bChanged);
        if (bChanged)
        {
            Value = FName(Text);
        }
    }
}

void FEditorAnimInstanceEditorWidget::OpenAnimInstanceAsset(const FString& AnimInstancePath)
{
    CurrentPath = AnimInstancePath;
    CurrentAsset = FResourceManager::Get().LoadAnimInstanceAsset(CurrentPath);
    SelectedStateIndex = CurrentAsset && !CurrentAsset->States.empty() ? 0 : -1;
    SelectedTransitionIndex = -1;
    bDirty = false;
    bVisible = true;
}

void FEditorAnimInstanceEditorWidget::Render(float DeltaTime)
{
    (void)DeltaTime;

    if (!bVisible)
    {
        return;
    }

    FString Title = FString("Anim Instance Editor");
    if (!CurrentPath.empty())
    {
        Title += FString(" - ") + CurrentPath;
    }
    if (bDirty)
    {
        Title += " *";
    }

    bool bOpen = true;
    ImGui::SetNextWindowSize(ImVec2(920.0f, 620.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(Title.c_str(), &bOpen))
    {
        ImGui::End();
        bVisible = bOpen;
        return;
    }

    if (!CurrentAsset)
    {
        ImGui::TextDisabled("AnimInstance asset could not be loaded.");
        if (ImGui::Button("Reload"))
        {
            ReloadAsset();
        }
        ImGui::End();
        bVisible = bOpen;
        return;
    }

    DrawToolbar();
    ImGui::Separator();

    if (ImGui::BeginTable("##AnimInstanceEditorLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed, 300.0f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        DrawGraph();
        ImGui::TableSetColumnIndex(1);
        DrawDetails();
        ImGui::EndTable();
    }

    ImGui::End();
    bVisible = bOpen;
}

void FEditorAnimInstanceEditorWidget::DrawToolbar()
{
    if (ImGui::Button("Save"))
    {
        SaveAsset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        ReloadAsset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add State"))
    {
        AddState();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Transition"))
    {
        AddTransition();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("Entry: %s", CurrentAsset && CurrentAsset->EntryState.IsValid() ? CurrentAsset->EntryState.ToString().c_str() : "<None>");
}

void FEditorAnimInstanceEditorWidget::DrawGraph()
{
    const ImVec2 CanvasSize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
    ImGui::InvisibleButton("##AnimInstanceGraphCanvas", CanvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const ImVec2 CanvasMin = ImGui::GetItemRectMin();
    const ImVec2 CanvasMax = ImGui::GetItemRectMax();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    DrawList->AddRectFilled(CanvasMin, CanvasMax, ImGui::GetColorU32(ImVec4(0.12f, 0.13f, 0.15f, 1.0f)));
    DrawList->PushClipRect(CanvasMin, CanvasMax, true);

    const float GridStep = 32.0f;
    const ImU32 GridColor = ImGui::GetColorU32(ImVec4(0.23f, 0.24f, 0.27f, 0.45f));
    for (float X = CanvasMin.x; X < CanvasMax.x; X += GridStep)
    {
        DrawList->AddLine(ImVec2(X, CanvasMin.y), ImVec2(X, CanvasMax.y), GridColor);
    }
    for (float Y = CanvasMin.y; Y < CanvasMax.y; Y += GridStep)
    {
        DrawList->AddLine(ImVec2(CanvasMin.x, Y), ImVec2(CanvasMax.x, Y), GridColor);
    }

    const ImVec2 NodeSize(150.0f, 62.0f);
    for (const FAnimInstanceTransitionAssetData& Transition : CurrentAsset->Transitions)
    {
        const int32 FromIndex = FindStateIndex(CurrentAsset, Transition.FromState);
        const int32 ToIndex = FindStateIndex(CurrentAsset, Transition.ToState);
        if (FromIndex < 0 || ToIndex < 0)
        {
            continue;
        }

        const FVector2& FromPos = CurrentAsset->States[FromIndex].EditorNodePosition;
        const FVector2& ToPos = CurrentAsset->States[ToIndex].EditorNodePosition;
        ImVec2 Start(CanvasMin.x + FromPos.X + NodeSize.x, CanvasMin.y + FromPos.Y + NodeSize.y * 0.5f);
        ImVec2 End(CanvasMin.x + ToPos.X, CanvasMin.y + ToPos.Y + NodeSize.y * 0.5f);
        const ImVec2 ControlOffset(std::max(40.0f, std::abs(End.x - Start.x) * 0.45f), 0.0f);
        DrawList->AddBezierCubic(
            Start,
            ImVec2(Start.x + ControlOffset.x, Start.y),
            ImVec2(End.x - ControlOffset.x, End.y),
            End,
            ImGui::GetColorU32(ImVec4(0.83f, 0.72f, 0.45f, 1.0f)),
            2.0f);
    }

    for (int32 Index = 0; Index < static_cast<int32>(CurrentAsset->States.size()); ++Index)
    {
        FAnimInstanceStateAssetData& State = CurrentAsset->States[Index];
        ImVec2 NodeMin(CanvasMin.x + State.EditorNodePosition.X, CanvasMin.y + State.EditorNodePosition.Y);
        ImVec2 NodeMax(NodeMin.x + NodeSize.x, NodeMin.y + NodeSize.y);
        const bool bSelected = SelectedStateIndex == Index && SelectedTransitionIndex < 0;
        const bool bEntry = State.Name == CurrentAsset->EntryState;
        const ImU32 NodeColor = ImGui::GetColorU32(bSelected ? ImVec4(0.25f, 0.34f, 0.42f, 1.0f) : ImVec4(0.18f, 0.21f, 0.25f, 1.0f));
        const ImU32 BorderColor = ImGui::GetColorU32(bEntry ? ImVec4(0.85f, 0.74f, 0.44f, 1.0f) : ImVec4(0.36f, 0.40f, 0.46f, 1.0f));

        DrawList->AddRectFilled(NodeMin, NodeMax, NodeColor, 6.0f);
        DrawList->AddRect(NodeMin, NodeMax, BorderColor, 6.0f, 0, bSelected ? 2.5f : 1.5f);
        DrawList->AddText(ImVec2(NodeMin.x + 10.0f, NodeMin.y + 10.0f), ImGui::GetColorU32(ImGuiCol_Text), State.Name.ToString().c_str());
        DrawList->AddText(ImVec2(NodeMin.x + 10.0f, NodeMin.y + 34.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), bEntry ? "Entry" : "State");

        ImGui::SetCursorScreenPos(NodeMin);
        ImGui::PushID(Index);
        ImGui::InvisibleButton("##AnimStateNode", NodeSize);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            SelectedStateIndex = Index;
            SelectedTransitionIndex = -1;
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 Delta = ImGui::GetIO().MouseDelta;
            State.EditorNodePosition.X += Delta.x;
            State.EditorNodePosition.Y += Delta.y;
            MarkDirty();
        }
        ImGui::PopID();
    }

    DrawList->PopClipRect();
}

void FEditorAnimInstanceEditorWidget::DrawDetails()
{
    ImGui::TextUnformatted("States");
    for (int32 Index = 0; Index < static_cast<int32>(CurrentAsset->States.size()); ++Index)
    {
        const FString Label = CurrentAsset->States[Index].Name.ToString();
        if (ImGui::Selectable(Label.c_str(), SelectedStateIndex == Index && SelectedTransitionIndex < 0))
        {
            SelectedStateIndex = Index;
            SelectedTransitionIndex = -1;
        }
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Transitions");
    for (int32 Index = 0; Index < static_cast<int32>(CurrentAsset->Transitions.size()); ++Index)
    {
        const FAnimInstanceTransitionAssetData& Transition = CurrentAsset->Transitions[Index];
        FString Label = Transition.FromState.ToString() + FString(" -> ") + Transition.ToState.ToString();
        if (ImGui::Selectable(Label.c_str(), SelectedTransitionIndex == Index))
        {
            SelectedTransitionIndex = Index;
            SelectedStateIndex = -1;
        }
    }

    ImGui::Separator();
    if (SelectedStateIndex >= 0 && SelectedStateIndex < static_cast<int32>(CurrentAsset->States.size()))
    {
        FAnimInstanceStateAssetData& State = CurrentAsset->States[SelectedStateIndex];
        bool bChanged = false;
        InputFName("Name", State.Name, bChanged);
        InputFString("Anim Sequence", State.AnimSequencePath, bChanged);
        bChanged |= ImGui::Checkbox("Loop", &State.bLoop);
        bChanged |= ImGui::DragFloat("Play Rate", &State.PlayRate, 0.01f, 0.0f, 10.0f);

        const bool bIsEntry = State.Name == CurrentAsset->EntryState;
        if (!bIsEntry && ImGui::Button("Set Entry"))
        {
            CurrentAsset->EntryState = State.Name;
            bChanged = true;
        }
        if (bChanged)
        {
            MarkDirty();
        }
        return;
    }

    if (SelectedTransitionIndex >= 0 && SelectedTransitionIndex < static_cast<int32>(CurrentAsset->Transitions.size()))
    {
        FAnimInstanceTransitionAssetData& Transition = CurrentAsset->Transitions[SelectedTransitionIndex];
        bool bChanged = false;
        InputFName("From", Transition.FromState, bChanged);
        InputFName("To", Transition.ToState, bChanged);
        InputFName("Parameter", Transition.ParameterName, bChanged);

        const EAnimTransitionConditionType Types[] =
        {
            EAnimTransitionConditionType::BoolEquals,
            EAnimTransitionConditionType::FloatGreater,
            EAnimTransitionConditionType::FloatLessEqual,
            EAnimTransitionConditionType::IntEquals,
            EAnimTransitionConditionType::Trigger,
        };
        if (ImGui::BeginCombo("Condition", ToConditionLabel(Transition.ConditionType)))
        {
            for (EAnimTransitionConditionType Type : Types)
            {
                const bool bSelected = Transition.ConditionType == Type;
                if (ImGui::Selectable(ToConditionLabel(Type), bSelected))
                {
                    Transition.ConditionType = Type;
                    bChanged = true;
                }
                if (bSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (Transition.ConditionType == EAnimTransitionConditionType::BoolEquals)
        {
            bChanged |= ImGui::Checkbox("Expected Bool", &Transition.ExpectedBool);
        }
        else if (Transition.ConditionType == EAnimTransitionConditionType::FloatGreater
            || Transition.ConditionType == EAnimTransitionConditionType::FloatLessEqual)
        {
            bChanged |= ImGui::DragFloat("Compare Float", &Transition.CompareFloat, 0.01f);
        }
        else if (Transition.ConditionType == EAnimTransitionConditionType::IntEquals)
        {
            bChanged |= ImGui::DragInt("Expected Int", &Transition.ExpectedInt, 1);
        }

        bChanged |= ImGui::DragFloat("Blend Speed", &Transition.BlendSpeed, 0.01f, 0.0f, 100.0f);
        bChanged |= ImGui::DragInt("Priority", &Transition.Priority, 1);
        if (bChanged)
        {
            MarkDirty();
        }
        return;
    }

    ImGui::TextDisabled("Select a state or transition.");
}

void FEditorAnimInstanceEditorWidget::AddState()
{
    if (!CurrentAsset)
    {
        return;
    }

    FAnimInstanceStateAssetData State;
    State.Name = FName(MakeStateName(static_cast<int32>(CurrentAsset->States.size())));
    State.EditorNodePosition = FVector2(120.0f + CurrentAsset->States.size() * 40.0f, 120.0f + CurrentAsset->States.size() * 24.0f);
    CurrentAsset->States.push_back(State);

    if (!CurrentAsset->EntryState.IsValid())
    {
        CurrentAsset->EntryState = State.Name;
    }

    SelectedStateIndex = static_cast<int32>(CurrentAsset->States.size()) - 1;
    SelectedTransitionIndex = -1;
    MarkDirty();
}

void FEditorAnimInstanceEditorWidget::AddTransition()
{
    if (!CurrentAsset || CurrentAsset->States.size() < 2)
    {
        return;
    }

    FAnimInstanceTransitionAssetData Transition;
    Transition.FromState = CurrentAsset->States[0].Name;
    Transition.ToState = CurrentAsset->States[1].Name;
    Transition.ParameterName = FName("Direction");
    Transition.ConditionType = EAnimTransitionConditionType::IntEquals;
    Transition.ExpectedInt = 1;
    CurrentAsset->Transitions.push_back(Transition);
    SelectedTransitionIndex = static_cast<int32>(CurrentAsset->Transitions.size()) - 1;
    SelectedStateIndex = -1;
    MarkDirty();
}

bool FEditorAnimInstanceEditorWidget::SaveAsset()
{
    if (!CurrentAsset || CurrentPath.empty())
    {
        return false;
    }

    if (!FResourceManager::Get().SaveAnimInstanceAsset(CurrentPath, CurrentAsset))
    {
        return false;
    }

    bDirty = false;
    if (EditorEngine)
    {
        EditorEngine->GetAssetService().RefreshAssetDatabase();
    }
    return true;
}

bool FEditorAnimInstanceEditorWidget::ReloadAsset()
{
    if (CurrentPath.empty())
    {
        return false;
    }

    CurrentAsset = FResourceManager::Get().LoadAnimInstanceAsset(CurrentPath);
    SelectedStateIndex = CurrentAsset && !CurrentAsset->States.empty() ? 0 : -1;
    SelectedTransitionIndex = -1;
    bDirty = false;
    return CurrentAsset != nullptr;
}

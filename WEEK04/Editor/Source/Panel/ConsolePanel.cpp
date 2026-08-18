#include "ConsolePanel.h"

#include "Core/Logging/LogMacros.h"
#include "Core/Misc/Paths.h"
#include "Editor/Editor.h"
#include "Editor/EditorContext.h"
#include "Editor/EditorPaths.h"
#include "Editor/Logging/EditorLogBuffer.h"
#include "Engine/Component/Core/SceneComponent.h"
#include "Engine/EngineStatics.h"
#include "Engine/Game/Actor.h"
#include "Engine/Game/CubeActor.h"
#include "Engine/Game/SphereActor.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/World.h"
#include "CoreUObject/ObjectIterator.h"
#include "Engine/Asset/Asset.h"
#include "Content/EditorContentIndex.h"
#include "Renderer/Types/ViewMode.h"
#include "Viewport/EditorViewportClient.h"
#include "Viewport/Navigation/ViewportNavigationController.h"
#include "Viewport/RenderSetting/ViewportRenderSetting.h"
#include "Viewport/Selection/ViewportSelectionController.h"
#include "imgui.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace
{
    FString TrimCopy(const FString& Value)
    {
        const size_t Start = Value.find_first_not_of(" \t\r\n");
        if (Start == FString::npos)
        {
            return {};
        }

        const size_t End = Value.find_last_not_of(" \t\r\n");
        return Value.substr(Start, End - Start + 1);
    }

    bool StartsWith(const FString& Value, const char* Prefix)
    {
        const size_t PrefixLength = std::char_traits<char>::length(Prefix);
        return Value.size() >= PrefixLength && Value.compare(0, PrefixLength, Prefix) == 0;
    }

    FString ToLowerAsciiCopy(const FString& Value)
    {
        FString Lower = Value;
        std::transform(
            Lower.begin(), Lower.end(), Lower.begin(), [](char Character)
            { return static_cast<char>(std::tolower(static_cast<unsigned char>(Character))); });
        return Lower;
    }

    bool EqualsIgnoreCase(const FString& Left, const FString& Right)
    {
        return ToLowerAsciiCopy(Left) == ToLowerAsciiCopy(Right);
    }

    TArray<FString> TokenizeCommandLine(const FString& CommandLine)
    {
        TArray<FString> Tokens;
        FString         CurrentToken;
        bool            bInQuotes = false;

        for (char Character : CommandLine)
        {
            if (Character == '"')
            {
                bInQuotes = !bInQuotes;
                continue;
            }

            if (!bInQuotes &&
                (Character == ' ' || Character == '\t' || Character == '\r' || Character == '\n'))
            {
                if (!CurrentToken.empty())
                {
                    Tokens.push_back(CurrentToken);
                    CurrentToken.clear();
                }
                continue;
            }

            CurrentToken.push_back(Character);
        }

        if (!CurrentToken.empty())
        {
            Tokens.push_back(CurrentToken);
        }

        return Tokens;
    }

    bool TryParseInt32(const FString& Value, int32& OutValue)
    {
        char*      EndPtr = nullptr;
        const long ParsedValue = std::strtol(Value.c_str(), &EndPtr, 10);
        if (EndPtr == Value.c_str() || (EndPtr != nullptr && *EndPtr != '\0'))
        {
            return false;
        }

        OutValue = static_cast<int32>(ParsedValue);
        return true;
    }

    bool TryParseUInt32(const FString& Value, uint32& OutValue)
    {
        char*               EndPtr = nullptr;
        const unsigned long ParsedValue = std::strtoul(Value.c_str(), &EndPtr, 10);
        if (EndPtr == Value.c_str() || (EndPtr != nullptr && *EndPtr != '\0'))
        {
            return false;
        }

        OutValue = static_cast<uint32>(ParsedValue);
        return true;
    }

    bool TryParseFloat(const FString& Value, float& OutValue)
    {
        char* EndPtr = nullptr;
        OutValue = std::strtof(Value.c_str(), &EndPtr);
        if (EndPtr == Value.c_str() || (EndPtr != nullptr && *EndPtr != '\0'))
        {
            return false;
        }

        return true;
    }

    bool TryParseToggle(const FString& Value, bool& OutEnabled)
    {
        const FString LowerValue = ToLowerAsciiCopy(Value);
        if (LowerValue == "on" || LowerValue == "true" || LowerValue == "1")
        {
            OutEnabled = true;
            return true;
        }

        if (LowerValue == "off" || LowerValue == "false" || LowerValue == "0")
        {
            OutEnabled = false;
            return true;
        }

        return false;
    }

    FString PathToUtf8String(const std::filesystem::path& Path)
    {
        const std::u8string Utf8Path = Path.u8string();
        return FString(reinterpret_cast<const char*>(Utf8Path.data()), Utf8Path.size());
    }

    std::filesystem::path Utf8PathToFilesystemPath(const FString& Value)
    {
        std::u8string Utf8Value;
        Utf8Value.reserve(Value.size());
        for (char Character : Value)
        {
            Utf8Value.push_back(static_cast<char8_t>(Character));
        }

        return std::filesystem::path(Utf8Value);
    }

    std::filesystem::path NormalizeScenePath(const std::filesystem::path& InPath)
    {
        if (InPath.empty())
        {
            return {};
        }

        if (InPath.has_extension() && _wcsicmp(InPath.extension().c_str(), L".Scene") == 0)
        {
            return InPath;
        }

        std::filesystem::path NormalizedPath = InPath;
        NormalizedPath.replace_extension(L".Scene");
        return NormalizedPath;
    }

    std::filesystem::path ResolveVirtualPathToAbsolute(const FString& Input)
    {
        if (Input.rfind("/Content/", 0) != 0)
        {
            return {};
        }

        std::filesystem::path RelativePath = Utf8PathToFilesystemPath(Input.substr(6));
        return FPaths::Combine(FPaths::AppContentDir(), RelativePath);
    }

    std::filesystem::path ResolveSceneCommandPath(const FString& Input, const FEditor* Editor)
    {
        if (Input.empty())
        {
            return {};
        }

        std::filesystem::path Path = ResolveVirtualPathToAbsolute(Input);
        if (Path.empty())
        {
            Path = Utf8PathToFilesystemPath(Input);
            if (Path.is_relative())
            {
                const std::filesystem::path BaseDirectory =
                    Editor != nullptr ? Editor->GetDefaultSceneDirectory() : FPaths::AppRoot();
                Path = FPaths::Combine(BaseDirectory, Path);
            }
        }

        return NormalizeScenePath(Path);
    }

    FString GetObjectName(const UObject* Object)
    {
        if (Object == nullptr)
        {
            return "<null>";
        }

        FString Name = Object->Name.ToFString();
        if (Name.empty())
        {
            Name = Object->GetTypeName();
        }
        return Name;
    }

    const char* ViewModeToString(EViewModeIndex ViewMode)
    {
        switch (ViewMode)
        {
        case EViewModeIndex::VMI_Unlit:
            return "unlit";
        case EViewModeIndex::VMI_Wireframe:
            return "wireframe";
        case EViewModeIndex::VMI_Lit:
        default:
            return "lit";
        }
    }

    ImVec4 GetLogTextColor(ELogLevel Level)
    {
        switch (Level)
        {
        case ELogLevel::Verbose:
            return ImVec4(0.60f, 0.60f, 0.60f, 1.0f); // Gray
        case ELogLevel::Debug:
            return ImVec4(1.00f, 1.00f, 1.00f, 1.0f); // White
        case ELogLevel::Info:
            return ImVec4(0.00f, 1.00f, 1.00f, 1.0f); // Cyan
        case ELogLevel::Warning:
            return ImVec4(1.00f, 1.00f, 0.00f, 1.0f); // Yellow
        case ELogLevel::Error:
            return ImVec4(1.00f, 0.20f, 0.20f, 1.0f); // Red
        default:
            return ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
        }
    }



    std::string GetEditorImGuiDefaultIniPath()
    {
        return FEditorPaths::ImGuiDefaultIniFile().string();
    }

    std::string GetEditorImGuiUserIniPath()
    {
        return FEditorPaths::ImGuiUserIniFile().string();
    }

    bool ResetEditorImGuiLayoutToDefault()
    {
        namespace fs = std::filesystem;

        std::error_code Ec;
        const fs::path DefaultPath(GetEditorImGuiDefaultIniPath());
        const fs::path UserPath(GetEditorImGuiUserIniPath());

        if (!fs::exists(DefaultPath, Ec) || Ec)
        {
            return false;
        }

        fs::create_directories(UserPath.parent_path(), Ec);
        Ec.clear();
        fs::copy_file(DefaultPath, UserPath, fs::copy_options::overwrite_existing, Ec);
        if (Ec)
        {
            return false;
        }

        ImGui::LoadIniSettingsFromDisk(DefaultPath.string().c_str());
        return true;
    }

    bool SaveCurrentImGuiLayoutAs(bool& bOutCanceled)
    {
        namespace fs = std::filesystem;

        std::array<wchar_t, 1024> FileBuffer{};
        const fs::path DefaultPath = FEditorPaths::ImGuiUserIniFile();
        const std::wstring DefaultPathText = DefaultPath.wstring();
        wcsncpy_s(FileBuffer.data(), FileBuffer.size(), DefaultPathText.c_str(), _TRUNCATE);

        OPENFILENAMEW Dialog = {};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.lpstrFilter = L"ImGui ini (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
        Dialog.lpstrFile = FileBuffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(FileBuffer.size());
        Dialog.lpstrInitialDir = DefaultPath.parent_path().empty() ? nullptr : DefaultPath.parent_path().c_str();
        Dialog.lpstrDefExt = L"ini";
        Dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

        if (!GetSaveFileNameW(&Dialog))
        {
            const DWORD ExtendedError = CommDlgExtendedError();
            bOutCanceled = (ExtendedError == 0);
            return false;
        }

        bOutCanceled = false;

        const fs::path SelectedPath(FileBuffer.data());
        std::error_code Ec;
        fs::create_directories(SelectedPath.parent_path(), Ec);
        Ec.clear();

        ImGui::SaveIniSettingsToDisk(SelectedPath.string().c_str());
        return !Ec;
    }
    ImU32 GetLogBackgroundColorU32(ELogLevel Level)
    {
        switch (Level)
        {
        case ELogLevel::Error:
            return IM_COL32(48, 12, 12, 255);
        default:
            return IM_COL32(0, 0, 0, 0);
        }
    }

    struct FObjectTypeStatLine
    {
        FString TypeName;
        uint32  Count = 0;
        size_t  MemoryBytes = 0;
    };

    struct FObjectResourceStatLine
    {
        FString TypeName;
        FString ResourceKey;
        uint32  Count = 0;
        size_t  MemoryBytes = 0;
    };

    struct FObjectOverlayStats
    {
        uint32 TotalCount = 0;
        size_t TotalMemoryBytes = 0;
        TArray<FObjectTypeStatLine>     TypeStats;
        TArray<FObjectResourceStatLine> ResourceStats;
    };

    const char* GetLogLevelButtonLabel(ELogLevel Level)
    {
        switch (Level)
        {
        case ELogLevel::Verbose:
            return "Verbose";
        case ELogLevel::Debug:
            return "Debug";
        case ELogLevel::Info:
            return "Info";
        case ELogLevel::Warning:
            return "Warning";
        case ELogLevel::Error:
            return "Error";
        default:
            return "Unknown";
        }
    }

    FObjectOverlayStats BuildObjectOverlayStats()
    {
        FObjectOverlayStats Stats;
        TMap<FString, FObjectTypeStatLine> TypeMap;
        TMap<FString, FObjectResourceStatLine> ResourceMap;

        for (FObjectIterator It; It; ++It)
        {
            UObject* Object = *It;
            if (Object == nullptr)
            {
                continue;
            }

            const FString TypeName = Object->GetTypeName();
            const size_t  MemoryBytes = Object->GetStatMemoryBytes();

            FObjectTypeStatLine& TypeStat = TypeMap[TypeName];
            TypeStat.TypeName = TypeName;
            TypeStat.Count += 1;
            TypeStat.MemoryBytes += MemoryBytes;

            Stats.TotalCount += 1;
            Stats.TotalMemoryBytes += MemoryBytes;

            const FString ResourceKey = Object->GetStatResourceKey();
            if (!ResourceKey.empty())
            {
                const FString ResourceMapKey = TypeName + "||" + ResourceKey;
                FObjectResourceStatLine& ResourceStat = ResourceMap[ResourceMapKey];
                ResourceStat.TypeName = TypeName;
                ResourceStat.ResourceKey = ResourceKey;
                ResourceStat.Count += 1;
                ResourceStat.MemoryBytes += MemoryBytes;
            }
        }

        Stats.TypeStats.reserve(TypeMap.size());
        for (const auto& [Key, Value] : TypeMap)
        {
            Stats.TypeStats.push_back(Value);
        }

        Stats.ResourceStats.reserve(ResourceMap.size());
        for (const auto& [Key, Value] : ResourceMap)
        {
            Stats.ResourceStats.push_back(Value);
        }

        std::sort(Stats.TypeStats.begin(), Stats.TypeStats.end(),
                  [](const FObjectTypeStatLine& Left, const FObjectTypeStatLine& Right)
                  {
                      if (Left.MemoryBytes != Right.MemoryBytes)
                      {
                          return Left.MemoryBytes > Right.MemoryBytes;
                      }
                      return Left.Count > Right.Count;
                  });

        std::sort(Stats.ResourceStats.begin(), Stats.ResourceStats.end(),
                  [](const FObjectResourceStatLine& Left, const FObjectResourceStatLine& Right)
                  {
                      if (Left.Count != Right.Count)
                      {
                          return Left.Count > Right.Count;
                      }
                      return Left.MemoryBytes > Right.MemoryBytes;
                  });

        return Stats;
    }

    const char* ContentItemTypeToString(EContentBrowserItemType ItemType)
    {
        switch (ItemType)
        {
        case EContentBrowserItemType::Folder:
            return "Folder";
        case EContentBrowserItemType::Scene:
            return "Scene";
        case EContentBrowserItemType::Texture:
            return "Texture";
        case EContentBrowserItemType::Font:
            return "Font";
        case EContentBrowserItemType::UnknownFile:
        default:
            return "Unknown";
        }
    }

    bool ActorMatchesToken(const AActor* Actor, const FString& Token)
    {
        if (Actor == nullptr)
        {
            return false;
        }

        uint32 ParsedUUID = 0;
        if (TryParseUInt32(Token, ParsedUUID) && Actor->UUID == ParsedUUID)
        {
            return true;
        }

        return EqualsIgnoreCase(GetObjectName(Actor), Token) ||
               EqualsIgnoreCase(Actor->GetTypeName(), Token);
    }

    bool ComponentMatchesToken(const Engine::Component::USceneComponent* Component,
                               const FString&                            Token)
    {
        if (Component == nullptr)
        {
            return false;
        }

        uint32 ParsedUUID = 0;
        if (TryParseUInt32(Token, ParsedUUID) && Component->UUID == ParsedUUID)
        {
            return true;
        }

        return EqualsIgnoreCase(GetObjectName(Component), Token) ||
               EqualsIgnoreCase(Component->GetTypeName(), Token);
    }

    void LogActorSummary(const AActor* Actor)
    {
        if (Actor == nullptr)
        {
            return;
        }

        UE_LOG(Console, ELogLevel::Info, "[Actor] name=%s type=%s uuid=%u",
               GetObjectName(Actor).c_str(), Actor->GetTypeName(), Actor->UUID);
        UE_LOG(Console, ELogLevel::Info,
               "        location=(%.2f, %.2f, %.2f) rotation=(%.2f, %.2f, %.2f) scale=(%.2f, %.2f, "
               "%.2f)",
               Actor->GetLocation().X, Actor->GetLocation().Y, Actor->GetLocation().Z,
               Actor->GetRotation().Rotator().Euler().X, Actor->GetRotation().Rotator().Euler().Y,
               Actor->GetRotation().Rotator().Euler().Z, Actor->GetScale().X, Actor->GetScale().Y,
               Actor->GetScale().Z);
    }

    void LogComponentSummary(const Engine::Component::USceneComponent* Component,
                             const AActor*                             OwnerActor)
    {
        if (Component == nullptr)
        {
            return;
        }

        UE_LOG(Console, ELogLevel::Info, "[Component] name=%s type=%s uuid=%u owner=%s",
               GetObjectName(Component).c_str(), Component->GetTypeName(), Component->UUID,
               OwnerActor != nullptr ? GetObjectName(OwnerActor).c_str() : "<none>");
        UE_LOG(Console, ELogLevel::Info,
               "            location=(%.2f, %.2f, %.2f) rotation=(%.2f, %.2f, %.2f) scale=(%.2f, "
               "%.2f, %.2f)",
               Component->GetRelativeLocation().X, Component->GetRelativeLocation().Y,
               Component->GetRelativeLocation().Z, Component->GetRelativeRotation().Euler().X,
               Component->GetRelativeRotation().Euler().Y,
               Component->GetRelativeRotation().Euler().Z, Component->GetRelativeScale3D().X,
               Component->GetRelativeScale3D().Y, Component->GetRelativeScale3D().Z);
    }

    void CollectContentMatches(const FContentBrowserFolderNode& Folder, const FString& QueryLower,
                               TArray<FString>& OutMatches)
    {
        const FString FolderNameLower = ToLowerAsciiCopy(Folder.DisplayName);
        const FString FolderVirtualLower = ToLowerAsciiCopy(Folder.VirtualPath);
        if ((!Folder.DisplayName.empty() && FolderNameLower.find(QueryLower) != FString::npos) ||
            (!Folder.VirtualPath.empty() && FolderVirtualLower.find(QueryLower) != FString::npos))
        {
            OutMatches.push_back(FString("[Folder] ") + Folder.VirtualPath);
        }

        for (const FContentBrowserItem& File : Folder.Files)
        {
            const FString FileNameLower = ToLowerAsciiCopy(File.DisplayName);
            const FString FileVirtualLower = ToLowerAsciiCopy(File.VirtualPath);
            if ((!File.DisplayName.empty() && FileNameLower.find(QueryLower) != FString::npos) ||
                (!File.VirtualPath.empty() && FileVirtualLower.find(QueryLower) != FString::npos))
            {
                OutMatches.push_back(FString("[") + ContentItemTypeToString(File.ItemType) + "] " +
                                     File.VirtualPath);
            }
        }

        for (const FContentBrowserFolderNode& ChildFolder : Folder.ChildFolders)
        {
            CollectContentMatches(ChildFolder, QueryLower, OutMatches);
        }
    }
} // namespace

FConsolePanel::FConsolePanel(FEditorLogBuffer* InLogBuffer) : LogBuffer(InLogBuffer)
{
    InputBuffer.fill('\0');
    SearchBuffer.fill('\0');
}

const wchar_t* FConsolePanel::GetPanelID() const { return L"ConsolePanel"; }

const wchar_t* FConsolePanel::GetDisplayName() const { return L"Console Panel"; }

void FConsolePanel::Draw()
{
    if (!ImGui::Begin("Console Panel", nullptr))
    {
        ImGui::End();
        return;
    }

    DrawToolbar();
    ImGui::Separator();
    DrawLogOutput();
    ImGui::Separator();
    DrawInputRow();
    RenderCommandOverlays();

    ImGui::End();
}

void FConsolePanel::DrawToolbar()
{
    ImGui::TextUnformatted("Minimum Level");
    ImGui::SameLine();

    const ELogLevel CurrentLogLevel = GetGlobalLogLevel();
    ImGui::SetNextItemWidth(180.0f);

    int CurrentLogLevelIndex = static_cast<int>(CurrentLogLevel);
    if (ImGui::BeginCombo("##EngineLogLevelCombo", GetLogLevelButtonLabel(CurrentLogLevel)))
    {
        for (int32 Index = static_cast<int32>(ELogLevel::Verbose);
             Index <= static_cast<int32>(ELogLevel::Error); ++Index)
        {
            const ELogLevel OptionLevel = static_cast<ELogLevel>(Index);
            const bool bSelected = (CurrentLogLevelIndex == Index);
            if (ImGui::Selectable(GetLogLevelButtonLabel(OptionLevel), bSelected))
            {
                SetGlobalLogLevel(OptionLevel);
                UE_LOG(Console, ELogLevel::Info, "Log level changed to %s.",
                       GetLogLevelLabel(OptionLevel));
                bScrollToBottom = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        ExecuteCommand("clear");
    }

    ImGui::SameLine();
    if (ImGui::Button("Help"))
    {
        ExecuteCommand("help");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &bAutoScroll);

    ImGui::SameLine();
    if (ImGui::Button("Reset Layout"))
    {
        if (ResetEditorImGuiLayoutToDefault())
        {
            UE_LOG(Console, ELogLevel::Info, "ImGui layout restored to default.");
        }
        else
        {
            UE_LOG(Console, ELogLevel::Error, "Failed to restore ImGui layout to default.");
        }
        bScrollToBottom = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Save Layout As..."))
    {
        bool bCanceled = false;
        if (SaveCurrentImGuiLayoutAs(bCanceled))
        {
            UE_LOG(Console, ELogLevel::Info, "Current ImGui layout saved.");
            bScrollToBottom = true;
        }
        else if (!bCanceled)
        {
            UE_LOG(Console, ELogLevel::Error, "Failed to save current ImGui layout.");
            bScrollToBottom = true;
        }
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("Search");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputText("##ConsoleSearch", SearchBuffer.data(), SearchBuffer.size());
}

void FConsolePanel::DrawLogOutput()
{
    const float FooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (!ImGui::BeginChild("##ConsoleOutput", ImVec2(0.0f, -FooterHeight), ImGuiChildFlags_Borders))
    {
        ImGui::EndChild();
        return;
    }

    if (LogBuffer == nullptr)
    {
        ImGui::TextUnformatted("Console log buffer is unavailable.");
        ImGui::EndChild();
        return;
    }

    const TArray<FEditorLogEntry>& Entries = LogBuffer->GetLogBuffer();
    const ELogLevel               VisibleLogLevel = GetGlobalLogLevel();
    const FString                 SearchText = TrimCopy(SearchBuffer.data());
    const FString                 SearchTextLower = ToLowerAsciiCopy(SearchText);
    const bool                    bUseSearchFilter = !SearchTextLower.empty();
    int32                         VisibleEntryCount = 0;
    for (const FEditorLogEntry& Entry : Entries)
    {
        if (static_cast<uint8>(Entry.Level) < static_cast<uint8>(VisibleLogLevel))
        {
            continue;
        }

        if (bUseSearchFilter)
        {
            const FString EntryMessageLower = ToLowerAsciiCopy(Entry.Message);
            if (EntryMessageLower.find(SearchTextLower) == FString::npos)
            {
                continue;
            }
        }

        ++VisibleEntryCount;

        if (Entry.Level == ELogLevel::Error)
        {
            const float  AvailableWidth = ImGui::GetContentRegionAvail().x;
            const ImVec2 StartPos = ImGui::GetCursorScreenPos();
            const float  PaddingX = 6.0f;
            const float  PaddingY = 3.0f;

            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + AvailableWidth - PaddingX * 2.0f);
            const ImVec2 TextSize = ImGui::CalcTextSize(Entry.Message.c_str(), nullptr, false,
                                                        AvailableWidth - PaddingX * 2.0f);
            ImGui::PopTextWrapPos();

            ImGui::GetWindowDrawList()->AddRectFilled(
                StartPos,
                ImVec2(StartPos.x + AvailableWidth, StartPos.y + TextSize.y + PaddingY * 2.0f),
                GetLogBackgroundColorU32(Entry.Level));

            ImGui::Dummy(ImVec2(0.0f, PaddingY));
            ImGui::SetCursorScreenPos(ImVec2(StartPos.x + PaddingX, StartPos.y + PaddingY));
            ImGui::PushStyleColor(ImGuiCol_Text, GetLogTextColor(Entry.Level));
            ImGui::PushTextWrapPos(StartPos.x + AvailableWidth - PaddingX);
            ImGui::TextWrapped("%s", Entry.Message.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();

            const float ConsumedHeight = TextSize.y + PaddingY * 2.0f;
            const float CursorOffsetY = ConsumedHeight - (PaddingY + TextSize.y);
            if (CursorOffsetY > 0.0f)
            {
                ImGui::Dummy(ImVec2(0.0f, CursorOffsetY));
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GetLogTextColor(Entry.Level));
            ImGui::TextWrapped("%s", Entry.Message.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (VisibleEntryCount != LastVisibleLogCount)
    {
        if (bAutoScroll || bScrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
        }

        LastVisibleLogCount = VisibleEntryCount;
        bScrollToBottom = false;
    }

    ImGui::EndChild();
}

void FConsolePanel::DrawInputRow()
{
    if (bReclaimInputFocus)
    {
        ImGui::SetKeyboardFocusHere();
        bReclaimInputFocus = false;
    }

    ImGui::PushItemWidth(-1.0f);
    const bool bSubmitted =
        ImGui::InputText("##ConsoleInput", InputBuffer.data(), InputBuffer.size(),
                         ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();

    if (bSubmitted)
    {
        SubmitInput();
    }
}

void FConsolePanel::SubmitInput()
{
    const FString CommandLine = TrimCopy(InputBuffer.data());
    InputBuffer.fill('\0');
    bReclaimInputFocus = true;

    if (CommandLine.empty())
    {
        return;
    }

    ExecuteCommand(CommandLine);
}

void FConsolePanel::ExecuteCommand(const FString& CommandLine)
{
    const FString TrimmedCommand = TrimCopy(CommandLine);

    if (TrimmedCommand.empty())
    {
        return;
    }

    const TArray<FString> Tokens = TokenizeCommandLine(TrimmedCommand);
    if (Tokens.empty())
    {
        return;
    }

    FString CommandName = ToLowerAsciiCopy(Tokens[0]);

    if (TrimmedCommand == "clear")
    {
        if (LogBuffer != nullptr)
        {
            LogBuffer->Clear();
        }

        UE_LOG(Console, ELogLevel::Info, "Console cleared.");
        bScrollToBottom = true;
        return;
    }

    if (TrimmedCommand == "help")
    {
        UE_LOG(Console, ELogLevel::Info, "Commands:");
        UE_LOG(Console, ELogLevel::Info,
               "  help, clear, verbose <text>, debug <text>, info <text>, warn <text>, error <text>");
        UE_LOG(Console, ELogLevel::Info,
               "  scene.new, scene.open <path>, scene.save, scene.saveas <path>, scene.clear, "
               "scene.list, scene.summary");
        UE_LOG(Console, ELogLevel::Info,
               "  actor.spawn <cube|sphere> [count], actor.delete_selected, actor.list_selected, "
               "actor.inspect <name|uuid>");
        UE_LOG(Console, ELogLevel::Info,
               "  component.inspect <name|uuid>, select.clear, select.focus, selection.dump");
        UE_LOG(Console, ELogLevel::Info,
               "  camera.reset, camera.speed [value], camera.rot_speed [value], grid.spacing "
               "[value], viewmode <lit|unlit|wireframe>");
        UE_LOG(Console, ELogLevel::Info,
               "  show.bounds <on|off>, show.grid <on|off>, show.outline <on|off>");
        UE_LOG(Console, ELogLevel::Info,
               "  stats.fps, stats.memory, stats.gpu, stats.uobject, content.refresh, content.find <keyword>");
        bScrollToBottom = true;
        return;
    }

    if (StartsWith(TrimmedCommand, "verbose "))
    {
        UE_LOG(Console, ELogLevel::Verbose, "%s", TrimmedCommand.substr(8).c_str());
        bScrollToBottom = true;
        return;
    }

    if (StartsWith(TrimmedCommand, "debug "))
    {
        UE_LOG(Console, ELogLevel::Debug, "%s", TrimmedCommand.substr(6).c_str());
        bScrollToBottom = true;
        return;
    }

    if (StartsWith(TrimmedCommand, "info "))
    {
        UE_LOG(Console, ELogLevel::Info, "%s", TrimmedCommand.substr(5).c_str());
        bScrollToBottom = true;
        return;
    }

    if (StartsWith(TrimmedCommand, "warn "))
    {
        UE_LOG(Console, ELogLevel::Warning, "%s", TrimmedCommand.substr(5).c_str());
        bScrollToBottom = true;
        return;
    }

    if (StartsWith(TrimmedCommand, "error "))
    {
        UE_LOG(Console, ELogLevel::Error, "%s", TrimmedCommand.substr(6).c_str());
        bScrollToBottom = true;
        return;
    }


    if (StartsWith(TrimmedCommand, "stat "))
    {
        UE_LOG(Console, ELogLevel::Info, "[STAT] %s", TrimmedCommand.substr(5).c_str());
        if (Tokens.size() >= 2)
            CommandName = "stat " + ToLowerAsciiCopy(Tokens[1]);
        bScrollToBottom = true;
    }

    FEditorContext* Context = GetContext();
    FEditor*        Editor = Context != nullptr ? Context->Editor : nullptr;
    FScene*         Scene = (Context != nullptr && Context->World != nullptr)
                                ? Context->World->GetActiveScene()
                                : nullptr;

    auto RequireEditor = [&]() -> bool
    {
        if (Editor != nullptr)
        {
            return true;
        }

        UE_LOG(Console, ELogLevel::Warning, "Editor is unavailable.");
        return false;
    };

    auto RequireScene = [&]() -> bool
    {
        if (Scene != nullptr)
        {
            return true;
        }

        UE_LOG(Console, ELogLevel::Warning, "Scene is unavailable.");
        return false;
    };

    if (CommandName == "scene.new")
    {
        if (RequireEditor())
        {
            Editor->CreateNewScene();
            UE_LOG(Console, ELogLevel::Info, "Created a new scene.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "scene.open")
    {
        if (RequireEditor())
        {
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Warning, "Usage: scene.open <path>");
            }
            else
            {
                Editor->OpenSceneFromPath(ResolveSceneCommandPath(Tokens[1], Editor));
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "scene.save")
    {
        if (RequireEditor() && !Editor->SaveCurrentSceneToDisk())
        {
            UE_LOG(Console, ELogLevel::Warning, "Scene save failed.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "scene.saveas")
    {
        if (RequireEditor())
        {
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Warning, "Usage: scene.saveas <path>");
            }
            else if (!Editor->SaveSceneAsPath(ResolveSceneCommandPath(Tokens[1], Editor)))
            {
                UE_LOG(Console, ELogLevel::Warning, "Scene saveas failed.");
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "scene.clear")
    {
        if (RequireEditor())
        {
            Editor->ClearScene();
            UE_LOG(Console, ELogLevel::Info, "Cleared the current scene.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "scene.list")
    {
        if (RequireScene())
        {
            const TArray<AActor*>* Actors = Scene->GetActors();
            if (Actors == nullptr || Actors->empty())
            {
                UE_LOG(Console, ELogLevel::Info, "Scene is empty.");
            }
            else
            {
                UE_LOG(Console, ELogLevel::Info, "Scene actors: %u",
                       static_cast<uint32>(Actors->size()));
                for (AActor* Actor : *Actors)
                {
                    LogActorSummary(Actor);
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "scene.summary")
    {
        if (RequireScene())
        {
            uint32                 ActorCount = 0;
            uint32                 ComponentCount = 0;
            uint32                 RenderableCount = 0;
            const TArray<AActor*>* Actors = Scene->GetActors();
            if (Actors != nullptr)
            {
                ActorCount = static_cast<uint32>(Actors->size());
                for (AActor* Actor : *Actors)
                {
                    if (Actor == nullptr)
                    {
                        continue;
                    }

                    ComponentCount += static_cast<uint32>(Actor->GetOwnedComponents().size());
                    if (Actor->IsRenderable())
                    {
                        ++RenderableCount;
                    }
                }
            }

            const uint32 SelectedCount =
                Context != nullptr ? static_cast<uint32>(Context->SelectedActors.size()) : 0u;
            UE_LOG(Console, ELogLevel::Info,
                   "Scene summary: actors=%u, components=%u, renderable=%u, selected=%u",
                   ActorCount, ComponentCount, RenderableCount, SelectedCount);
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "actor.spawn")
    {
        if (RequireEditor())
        {
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Warning, "Usage: actor.spawn <cube|sphere> [count]");
            }
            else
            {
                int32 SpawnCount = 1;
                if (Tokens.size() >= 3 && !TryParseInt32(Tokens[2], SpawnCount))
                {
                    UE_LOG(Console, ELogLevel::Warning, "Invalid spawn count.");
                }
                else
                {
                    SpawnCount = FMath::Clamp(SpawnCount, 1, 256);
                    const FString ActorType = ToLowerAsciiCopy(Tokens[1]);
                    int32         CreatedCount = 0;
                    for (int32 Index = 0; Index < SpawnCount; ++Index)
                    {
                        AActor* NewActor = nullptr;
                        if (ActorType == "cube")
                        {
                            NewActor = new ACubeActor();
                        }
                        else if (ActorType == "sphere")
                        {
                            NewActor = new ASphereActor();
                        }
                        else
                        {
                            UE_LOG(Console, ELogLevel::Warning,
                                   "Unsupported actor type. Use cube or sphere.");
                            break;
                        }

                        Editor->AddActorToScene(NewActor, Index == SpawnCount - 1);
                        ++CreatedCount;
                    }

                    if (CreatedCount > 0)
                    {
                        UE_LOG(Console, ELogLevel::Info, "Spawned %d %s actor(s).", CreatedCount,
                               ActorType.c_str());
                    }
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "actor.delete_selected")
    {
        if (RequireEditor() && !Editor->DeleteSelectedActors())
        {
            UE_LOG(Console, ELogLevel::Warning, "No selected actors to delete.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "actor.list_selected")
    {
        if (Context == nullptr || Context->SelectedActors.empty())
        {
            UE_LOG(Console, ELogLevel::Info, "No selected actors.");
        }
        else
        {
            UE_LOG(Console, ELogLevel::Info, "Selected actors: %u",
                   static_cast<uint32>(Context->SelectedActors.size()));
            for (AActor* Actor : Context->SelectedActors)
            {
                LogActorSummary(Actor);
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "actor.inspect")
    {
        if (RequireScene())
        {
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Warning, "Usage: actor.inspect <name|uuid>");
            }
            else
            {
                bool bFoundActor = false;
                for (AActor* Actor : *Scene->GetActors())
                {
                    if (!ActorMatchesToken(Actor, Tokens[1]))
                    {
                        continue;
                    }

                    bFoundActor = true;
                    LogActorSummary(Actor);
                    UE_LOG(Console, ELogLevel::Info, "        components=%u",
                           static_cast<uint32>(Actor->GetOwnedComponents().size()));
                    for (Engine::Component::USceneComponent* Component :
                         Actor->GetOwnedComponents())
                    {
                        LogComponentSummary(Component, Actor);
                    }
                }

                if (!bFoundActor)
                {
                    UE_LOG(Console, ELogLevel::Warning, "Actor not found: %s", Tokens[1].c_str());
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "component.inspect")
    {
        if (RequireScene())
        {
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Warning, "Usage: component.inspect <name|uuid>");
            }
            else
            {
                bool bFoundComponent = false;
                for (AActor* Actor : *Scene->GetActors())
                {
                    if (Actor == nullptr)
                    {
                        continue;
                    }

                    for (Engine::Component::USceneComponent* Component :
                         Actor->GetOwnedComponents())
                    {
                        if (!ComponentMatchesToken(Component, Tokens[1]))
                        {
                            continue;
                        }

                        bFoundComponent = true;
                        LogComponentSummary(Component, Actor);
                    }
                }

                if (!bFoundComponent)
                {
                    UE_LOG(Console, ELogLevel::Warning, "Component not found: %s",
                           Tokens[1].c_str());
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "select.clear")
    {
        if (RequireEditor())
        {
            Editor->GetViewportTab()
                .GetViewport(0)
                ->GetViewportClient()
                ->GetSelectionController()
                .ClearSelection();
            UE_LOG(Console, ELogLevel::Info, "Selection cleared.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "select.focus")
    {
        if (RequireEditor())
        {
            Editor->GetViewportTab()
                .GetViewport(0)
                ->GetViewportClient()
                ->GetNavigationController()
                .FocusActors();
            UE_LOG(Console, ELogLevel::Info, "Focused camera on current selection.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "selection.dump")
    {
        if (Context == nullptr)
        {
            UE_LOG(Console, ELogLevel::Warning, "Editor context is unavailable.");
        }
        else
        {
            UObject* SelectedObject = Context->SelectedObject;
            UE_LOG(Console, ELogLevel::Info, "Selection dump:");
            UE_LOG(Console, ELogLevel::Info, "  selected_object=%s (%s)",
                   GetObjectName(SelectedObject).c_str(),
                   SelectedObject != nullptr ? SelectedObject->GetTypeName() : "null");
            UE_LOG(Console, ELogLevel::Info, "  selected_actor_count=%u",
                   static_cast<uint32>(Context->SelectedActors.size()));
            for (AActor* Actor : Context->SelectedActors)
            {
                LogActorSummary(Actor);
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "camera.reset")
    {
        if (RequireEditor())
        {
            FViewportCamera& Camera =
                Editor->GetViewportTab().GetViewport(0)->GetViewportClient()->GetCamera();
            Camera.SetProjectionType(EViewportProjectionType::Perspective);
            Camera.SetFOV(3.141592f * 0.5f);
            Camera.SetNearPlane(0.1f);
            Camera.SetFarPlane(2000.0f);
            Camera.SetLocation(FVector(-20.0f, 1.0f, 10.0f));
            Camera.SetRotation(FRotator(0.0f, 0.0f, 0.0f));
            UE_LOG(Console, ELogLevel::Info, "Camera reset to default editor transform.");
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "camera.speed")
    {
        if (RequireEditor())
        {
            FViewportNavigationController& NavigationController = Editor->GetViewportTab()
                                                                      .GetViewport(0)
                                                                      ->GetViewportClient()
                                                                      ->GetNavigationController();

            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Info, "Camera move speed = %.2f",
                       NavigationController.GetMoveSpeed());
            }
            else
            {
                float MoveSpeed = 0.0f;
                if (!TryParseFloat(Tokens[1], MoveSpeed))
                {
                    UE_LOG(Console, ELogLevel::Warning, "Usage: camera.speed [value]");
                }
                else
                {
                    NavigationController.SetMoveSpeed(MoveSpeed);
                    UE_LOG(Console, ELogLevel::Info, "Camera move speed = %.2f",
                           NavigationController.GetMoveSpeed());
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "camera.rot_speed")
    {
        if (RequireEditor())
        {
            FViewportNavigationController& NavigationController = Editor->GetViewportTab()
                                                                      .GetViewport(0)
                                                                      ->GetViewportClient()
                                                                      ->GetNavigationController();

            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Info, "Camera rotation speed = %.2f",
                       NavigationController.GetRotationSpeed());
            }
            else
            {
                float RotationSpeed = 0.0f;
                if (!TryParseFloat(Tokens[1], RotationSpeed))
                {
                    UE_LOG(Console, ELogLevel::Warning, "Usage: camera.rot_speed [value]");
                }
                else
                {
                    NavigationController.SetRotationSpeed(RotationSpeed);
                    UE_LOG(Console, ELogLevel::Info, "Camera rotation speed = %.2f",
                           NavigationController.GetRotationSpeed());
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "grid.spacing")
    {
        if (Tokens.size() < 2)
        {
            UE_LOG(Console, ELogLevel::Info, "Grid spacing = %.2f", UEngineStatics::GridSpacing);
        }
        else
        {
            float GridSpacing = 0.0f;
            if (!TryParseFloat(Tokens[1], GridSpacing))
            {
                UE_LOG(Console, ELogLevel::Warning, "Usage: grid.spacing [value]");
            }
            else
            {
                UEngineStatics::GridSpacing = FMath::Clamp(GridSpacing, 1.0f, 1000.0f);
                UE_LOG(Console, ELogLevel::Info, "Grid spacing = %.2f",
                       UEngineStatics::GridSpacing);
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "viewmode")
    {
        if (RequireEditor())
        {
            FViewportRenderSetting& RenderSetting =
                Editor->GetViewportTab().GetViewport(0)->GetViewportClient()->GetRenderSetting();
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Info, "View mode = %s",
                       ViewModeToString(RenderSetting.GetViewMode()));
            }
            else
            {
                const FString Mode = ToLowerAsciiCopy(Tokens[1]);
                if (Mode == "lit")
                {
                    RenderSetting.SetViewMode(EViewModeIndex::VMI_Lit);
                }
                else if (Mode == "unlit")
                {
                    RenderSetting.SetViewMode(EViewModeIndex::VMI_Unlit);
                }
                else if (Mode == "wireframe")
                {
                    RenderSetting.SetViewMode(EViewModeIndex::VMI_Wireframe);
                }
                else
                {
                    UE_LOG(Console, ELogLevel::Warning, "Usage: viewmode <lit|unlit|wireframe>");
                    bScrollToBottom = true;
                    return;
                }

                UE_LOG(Console, ELogLevel::Info, "View mode = %s",
                       ViewModeToString(RenderSetting.GetViewMode()));
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "show.bounds")
    {
        if (RequireScene())
        {
            if (Tokens.size() < 2)
            {
                uint32 VisibleBoundsCount = 0;
                for (AActor* Actor : *Scene->GetActors())
                {
                    if (Actor != nullptr && Actor->IsShowBounds())
                    {
                        ++VisibleBoundsCount;
                    }
                }

                UE_LOG(Console, ELogLevel::Info, "Bounds visible on %u actor(s).",
                       VisibleBoundsCount);
            }
            else
            {
                bool bShowBounds = false;
                if (!TryParseToggle(Tokens[1], bShowBounds))
                {
                    UE_LOG(Console, ELogLevel::Warning, "Usage: show.bounds <on|off>");
                }
                else
                {
                    uint32 UpdatedCount = 0;
                    for (AActor* Actor : *Scene->GetActors())
                    {
                        if (Actor == nullptr)
                        {
                            continue;
                        }

                        Actor->SetShowBounds(bShowBounds);
                        ++UpdatedCount;
                    }

                    UE_LOG(Console, ELogLevel::Info, "Bounds visibility set to %s for %u actor(s).",
                           bShowBounds ? "on" : "off", UpdatedCount);
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "show.grid")
    {
        if (RequireEditor())
        {
            FViewportRenderSetting& RenderSetting =
                Editor->GetViewportTab().GetViewport(0)->GetViewportClient()->GetRenderSetting();
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Info, "Grid visibility = %s",
                       RenderSetting.IsGridVisible() ? "on" : "off");
            }
            else
            {
                bool bShowGrid = false;
                if (!TryParseToggle(Tokens[1], bShowGrid))
                {
                    UE_LOG(Console, ELogLevel::Warning, "Usage: show.grid <on|off>");
                }
                else
                {
                    RenderSetting.SetGridVisible(bShowGrid);
                    UE_LOG(Console, ELogLevel::Info, "Grid visibility = %s",
                           bShowGrid ? "on" : "off");
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "show.outline")
    {
        if (RequireEditor())
        {
            FViewportRenderSetting& RenderSetting =
                Editor->GetViewportTab().GetViewport(0)->GetViewportClient()->GetRenderSetting();
            if (Tokens.size() < 2)
            {
                UE_LOG(Console, ELogLevel::Info, "Selection outline visibility = %s",
                       RenderSetting.IsSelectionOutlineVisible() ? "on" : "off");
            }
            else
            {
                bool bShowOutline = false;
                if (!TryParseToggle(Tokens[1], bShowOutline))
                {
                    UE_LOG(Console, ELogLevel::Warning, "Usage: show.outline <on|off>");
                }
                else
                {
                    RenderSetting.SetSelectionOutlineVisible(bShowOutline);
                    UE_LOG(Console, ELogLevel::Info, "Selection outline visibility = %s",
                           bShowOutline ? "on" : "off");
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "stat all")
    {
        ActiveStatOverlays = STAT_FPS | STAT_MEMORY | STAT_UOBJECT;
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "stat none")
    {
        ActiveStatOverlays = STAT_NONE;
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "stat fps")
    {
        if (Context == nullptr)
        {
            UE_LOG(Console, ELogLevel::Warning, "Editor context is unavailable.");
        }
        else
        {
            UE_LOG(Console, ELogLevel::Info, "FPS = %.1f (%.3f ms)", Context->CurrentFPS,
                   Context->DeltaTime * 1000.0f);

            ActiveStatOverlays ^= STAT_FPS;
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "stat memory")
    {
        UE_LOG(Console, ELogLevel::Info, "TotalAllocationCount = %u",
               UEngineStatics::TotalAllocationCount);
        UE_LOG(Console, ELogLevel::Info, "Heap Usage = %.2f KB",
               UEngineStatics::TotalAllocatedBytes / 1024.0f);
        ActiveStatOverlays ^= STAT_MEMORY;
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "stat gpu")
    {
        UE_LOG(Console, ELogLevel::Warning,
               "GPU memory stats are not available in the current build.");
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "stat uobject")
    {
        const FObjectOverlayStats ObjectStats = BuildObjectOverlayStats();
        UE_LOG(Console, ELogLevel::Info, "UObject count = %u", ObjectStats.TotalCount);
        UE_LOG(Console, ELogLevel::Info, "UObject wrapper memory = %.2f KB",
               static_cast<float>(ObjectStats.TotalMemoryBytes) / 1024.0f);
        ActiveStatOverlays ^= STAT_UOBJECT;
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "content.refresh")
    {
        if (Editor == nullptr || Context == nullptr || Context->ContentIndex == nullptr)
        {
            UE_LOG(Console, ELogLevel::Warning, "Content index is unavailable.");
        }
        else
        {
            Editor->RefreshContentIndex();
            const FContentIndexSnapshot& Snapshot = Context->ContentIndex->GetSnapshot();
            UE_LOG(Console, ELogLevel::Info, "Content refreshed: folders=%d files=%d root=%s",
                   Snapshot.FolderCount, Snapshot.FileCount,
                   PathToUtf8String(Snapshot.ContentRootPath).c_str());
        }
        bScrollToBottom = true;
        return;
    }

    if (CommandName == "content.find")
    {
        if (Context == nullptr || Context->ContentIndex == nullptr)
        {
            UE_LOG(Console, ELogLevel::Warning, "Content index is unavailable.");
        }
        else if (Tokens.size() < 2)
        {
            UE_LOG(Console, ELogLevel::Warning, "Usage: content.find <keyword>");
        }
        else
        {
            const FContentIndexSnapshot& Snapshot = Context->ContentIndex->GetSnapshot();
            TArray<FString>              Matches;
            CollectContentMatches(Snapshot.RootFolder, ToLowerAsciiCopy(Tokens[1]), Matches);

            if (Matches.empty())
            {
                UE_LOG(Console, ELogLevel::Info, "No content items matched '%s'.",
                       Tokens[1].c_str());
            }
            else
            {
                constexpr size_t MaxResultsToPrint = 64;
                UE_LOG(Console, ELogLevel::Info, "Content matches for '%s': %u", Tokens[1].c_str(),
                       static_cast<uint32>(Matches.size()));
                for (size_t Index = 0; Index < Matches.size() && Index < MaxResultsToPrint; ++Index)
                {
                    UE_LOG(Console, ELogLevel::Info, "  %s", Matches[Index].c_str());
                }

                if (Matches.size() > MaxResultsToPrint)
                {
                    UE_LOG(Console, ELogLevel::Info, "  ... %u more result(s)",
                           static_cast<uint32>(Matches.size() - MaxResultsToPrint));
                }
            }
        }
        bScrollToBottom = true;
        return;
    }

    UE_LOG(Console, ELogLevel::Warning, "Unknown command: %s", TrimmedCommand.c_str());
    bScrollToBottom = true;
}

void FConsolePanel::RenderCommandOverlays()
{
    if (ActiveStatOverlays == STAT_NONE)
    {
        return;
    }

    FEditorContext* Context = GetContext();
    ImDrawList*     DrawList = ImGui::GetForegroundDrawList();
    float           Y = 10.0f;
    const float     LineHeight = ImGui::GetTextLineHeight() + 4.0f;

    auto DrawStatText = [&](const char* Text, ImVec4 Color = {1, 1, 0, 1})
    {
        const ImVec2 Pos = ImVec2(10.0f, Y + 100.0f);
        DrawList->AddText(ImVec2(Pos.x + 1, Pos.y + 1), IM_COL32(0, 0, 0, 200), Text);
        DrawList->AddText(Pos, ImGui::ColorConvertFloat4ToU32(Color), Text);
        Y += LineHeight;
    };

    char Buf[256];

    if ((ActiveStatOverlays & STAT_FPS) != 0 && Context != nullptr)
    {
        snprintf(Buf, sizeof(Buf), "FPS: %.1f (%.3f ms)", Context->CurrentFPS,
                 Context->DeltaTime * 1000.0f);
        DrawStatText(Buf);
    }

    if ((ActiveStatOverlays & STAT_MEMORY) != 0)
    {
        snprintf(Buf, sizeof(Buf), "TotalAllocationCount = %u",
                 UEngineStatics::TotalAllocationCount);
        DrawStatText(Buf, {0.4f, 1.0f, 0.4f, 1.0f});

        snprintf(Buf, sizeof(Buf), "Heap Usage = %.2f KB",
                 UEngineStatics::TotalAllocatedBytes / 1024.0f);
        DrawStatText(Buf, {0.4f, 1.0f, 0.4f, 1.0f});
    }

    if ((ActiveStatOverlays & STAT_UOBJECT) != 0)
    {
        const FObjectOverlayStats ObjectStats = BuildObjectOverlayStats();
        snprintf(Buf, sizeof(Buf), "UObject: %u objects / %.2f KB", ObjectStats.TotalCount,
                 static_cast<float>(ObjectStats.TotalMemoryBytes) / 1024.0f);
        DrawStatText(Buf, {0.2f, 0.9f, 1.0f, 1.0f});

        const size_t MaxTypeLines = std::min<size_t>(ObjectStats.TypeStats.size(), 8);
        for (size_t Index = 0; Index < MaxTypeLines; ++Index)
        {
            const FObjectTypeStatLine& TypeStat = ObjectStats.TypeStats[Index];
            snprintf(Buf, sizeof(Buf), "  %s: %u / %.2f KB", TypeStat.TypeName.c_str(),
                     TypeStat.Count, static_cast<float>(TypeStat.MemoryBytes) / 1024.0f);
            DrawStatText(Buf, {0.85f, 0.95f, 1.0f, 1.0f});
        }

        const size_t MaxResourceLines = std::min<size_t>(ObjectStats.ResourceStats.size(), 6);
        for (size_t Index = 0; Index < MaxResourceLines; ++Index)
        {
            const FObjectResourceStatLine& ResourceStat = ObjectStats.ResourceStats[Index];
            snprintf(Buf, sizeof(Buf), "  %s x%u : %s", ResourceStat.TypeName.c_str(),
                     ResourceStat.Count, ResourceStat.ResourceKey.c_str());
            DrawStatText(Buf, {1.0f, 0.95f, 0.65f, 1.0f});
        }
    }
}

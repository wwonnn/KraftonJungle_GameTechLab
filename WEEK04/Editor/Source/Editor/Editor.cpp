#include "Editor.h"

#include "Editor/EditorPaths.h"
#include "Asset/Core/AssetNaming.h"
#include "Engine/Asset/AssetObjectManager.h"
#include "Viewport/EditorViewportClient.h"
#include "Engine/Scene/SceneAssetPath.h"

#include "Core/Misc/Paths.h"
#include "Engine/Component/Core/SceneComponent.h"
#include "Engine/EngineStatics.h"
#include "Engine/Scene/World.h"
#include "Engine/Game/Actor.h"
#include "Renderer/RenderAsset/TextureResource.h"
#include "Engine/Scene/Serialization/Core/SceneSerializer.h"
#include "Engine/Scene/Serialization/Core/SceneDeserializer.h"
#include "Engine/Scene/CameraInfo.h"
#include "Engine/Scene/SceneAssetBinder.h"

#include "Panel/ConsolePanel.h"
#include "Panel/ContentBrowserPanel.h"
#include "Panel/ControlPanel.h"
#include "Panel/OutlinerPanel.h"
#include "Panel/PanelManager.h"
#include "Panel/PropertiesPanel.h"
#include "Panel/ShortcutsPanel.h"
#include "Panel/StatePanel.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#pragma comment(lib, "Comdlg32.lib")

#include "Engine/Asset/StaticMesh.h"
#include "CoreUObject/ObjectIterator.h"
#include "Asset/Manager/AssetCacheManager.h"
#include "Renderer/RendererModule.h"

// TODO 삭제 예정
namespace
{
    UStaticMesh* FindPreloadedStaticMeshObject(const FString& AssetPath)
    {
        for (TObjectIterator<UStaticMesh> It; It; ++It)
        {
            UStaticMesh* StaticMesh = *It;
            if (StaticMesh == nullptr || !StaticMesh->IsValidLowLevel())
            {
                continue;
            }

            if (StaticMesh->GetAssetPath() == AssetPath)
            {
                return StaticMesh;
            }
        }
        return nullptr;
    }

    UStaticMesh* EnsurePreloadedStaticMeshObject(const FString&             AssetPath,
                                                 Asset::FAssetCacheManager* AssetCacheManager,
                                                 RHI::FDynamicRHI*          DynamicRHI)
    {
        FObjectFactory::RegisterObjectType(UStaticMesh::GetClass(),
                                           []() -> UObject* { return new UStaticMesh(); });

        UE_LOG(FEditor, ELogLevel::Info, "EnsurePreloadedStaticMeshObject START: %s",
               AssetPath.c_str());

        if (AssetCacheManager == nullptr || DynamicRHI == nullptr)
        {
            UE_LOG(FEditor, ELogLevel::Warning,
                   "EnsurePreloadedStaticMeshObject early fail: cache=%p rhi=%p", AssetCacheManager,
                   DynamicRHI);
            return nullptr;
        }

        if (UStaticMesh* Existing = FindPreloadedStaticMeshObject(AssetPath))
        {
            UE_LOG(FEditor, ELogLevel::Info,
                   "EnsurePreloadedStaticMeshObject found existing: %s ptr=%p", AssetPath.c_str(),
                   Existing);
            return Existing;
        }

        auto CookedData = AssetCacheManager->BuildStaticMesh(AssetPath);
        UE_LOG(FEditor, ELogLevel::Info,
               "EnsurePreloadedStaticMeshObject BuildStaticMesh: %s cooked=%p", AssetPath.c_str(),
               CookedData.get());

        if (CookedData == nullptr)
        {
            UE_LOG(FEditor, ELogLevel::Warning,
                   "EnsurePreloadedStaticMeshObject fail: cooked data null: %s", AssetPath.c_str());
            return nullptr;
        }

        if (UStaticMesh::GetClass() == nullptr)
        {
            UE_LOG(FEditor, ELogLevel::Error, "UStaticMesh::GetClass() returned null");
            return nullptr;
        }
        UE_LOG(FEditor, ELogLevel::Info, "UStaticMesh::GetClass() = %p", UStaticMesh::GetClass());

        UStaticMesh* NewObject =
            (UStaticMesh*)FObjectFactory::ConstructObject(UStaticMesh::GetClass());

        UE_LOG(FEditor, ELogLevel::Info,
               "EnsurePreloadedStaticMeshObject ConstructObject: %s obj=%p", AssetPath.c_str(),
               NewObject);

        if (NewObject == nullptr)
        {
            UE_LOG(FEditor, ELogLevel::Warning,
                   "EnsurePreloadedStaticMeshObject fail: construct object null: %s",
                   AssetPath.c_str());
            return nullptr;
        }

        const bool bLoadSucceeded = NewObject->LoadFromCooked(AssetPath, CookedData, *DynamicRHI);
        UE_LOG(FEditor, ELogLevel::Info,
               "EnsurePreloadedStaticMeshObject LoadFromCooked: %s success=%s obj=%p",
               AssetPath.c_str(), bLoadSucceeded ? "true" : "false", NewObject);

        if (!bLoadSucceeded)
        {
            UE_LOG(FEditor, ELogLevel::Warning,
                   "EnsurePreloadedStaticMeshObject fail: LoadFromCooked false: %s",
                   AssetPath.c_str());
            delete NewObject;
            return nullptr;
        }

        UE_LOG(FEditor, ELogLevel::Info, "EnsurePreloadedStaticMeshObject RETURN: %s obj=%p",
               AssetPath.c_str(), NewObject);

        return NewObject;
    }
} // namespace

namespace
{
#ifdef IMGUI_HAS_DOCK
    constexpr ImGuiDockNodeFlags RootDockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
#endif

    constexpr char    AboutPopupId[] = "About##EditorAbout";
    constexpr float   RestoredSideGutter = 3.0f;
    constexpr float   RestoredBottomGutter = 3.0f;
    constexpr ImU32   GutterColor = IM_COL32(46, 46, 48, 255);
    constexpr wchar_t SceneFileDialogFilter[] =
        L"Scene Files (*.Scene)\0*.Scene\0All Files (*.*)\0*.*\0";

    struct FEditorGutterMetrics
    {
        float Left = 0.0f;
        float Right = 0.0f;
        float Bottom = 0.0f;
    };

    FEditorGutterMetrics GetDockSpaceGutterMetrics(const IEditorChromeHost* Host)
    {
        const bool bIsMaximized = Host != nullptr && Host->IsWindowMaximized();
        if (bIsMaximized)
        {
            return {};
        }

        FEditorGutterMetrics Metrics;
        Metrics.Left = RestoredSideGutter;
        Metrics.Right = RestoredSideGutter;
        Metrics.Bottom = RestoredBottomGutter;
        return Metrics;
    }

    void DrawDockSpaceGutters(ImGuiViewport* Viewport, const FEditorGutterMetrics& Metrics)
    {
        if (Viewport == nullptr)
        {
            return;
        }

        const float Top = FEditorChrome::TitleBarHeight;
        const float ContentTop = Viewport->Pos.y + Top;
        const float ContentBottom = Viewport->Pos.y + Viewport->Size.y;
        if (ContentBottom <= ContentTop)
        {
            return;
        }

        ImDrawList* DrawList = ImGui::GetBackgroundDrawList(Viewport);
        if (DrawList == nullptr)
        {
            return;
        }

        if (Metrics.Left > 0.0f)
        {
            DrawList->AddRectFilled(ImVec2(Viewport->Pos.x, ContentTop),
                                    ImVec2(Viewport->Pos.x + Metrics.Left, ContentBottom),
                                    GutterColor);
        }

        if (Metrics.Right > 0.0f)
        {
            DrawList->AddRectFilled(
                ImVec2(Viewport->Pos.x + Viewport->Size.x - Metrics.Right, ContentTop),
                ImVec2(Viewport->Pos.x + Viewport->Size.x, ContentBottom), GutterColor);
        }

        if (Metrics.Bottom > 0.0f)
        {
            DrawList->AddRectFilled(
                ImVec2(Viewport->Pos.x, Viewport->Pos.y + Viewport->Size.y - Metrics.Bottom),
                ImVec2(Viewport->Pos.x + Viewport->Size.x, Viewport->Pos.y + Viewport->Size.y),
                GutterColor);
        }
    }

    FString BuildPanelCommandId(const FPanelDescriptor& Descriptor)
    {
        return "panel.toggle." + std::to_string(Descriptor.PanelType.hash_code());
    }

    HWND GetEditorOwnerWindow(const IEditorChromeHost* Host)
    {
        return static_cast<HWND>(Host != nullptr ? Host->GetNativeWindowHandle() : nullptr);
    }

    std::filesystem::path NormalizeSceneFilePath(const std::filesystem::path& FilePath)
    {
        if (FilePath.empty())
        {
            return {};
        }

        if (FilePath.has_extension() && _wcsicmp(FilePath.extension().c_str(), L".Scene") == 0)
        {
            return FilePath;
        }

        std::filesystem::path NormalizedPath = FilePath;
        NormalizedPath.replace_extension(L".Scene");
        return NormalizedPath;
    }

    FString PathToUtf8String(const std::filesystem::path& Path)
    {
        return FPaths::Utf8FromPath(Path);
    }

    FWString Utf8ToWide(const FString& InText)
    {
        if (InText.empty())
        {
            return {};
        }

        const int RequiredSize = MultiByteToWideChar(CP_UTF8, 0, InText.c_str(),
                                                     static_cast<int>(InText.size()), nullptr, 0);
        if (RequiredSize <= 0)
        {
            return {};
        }

        FWString OutText(static_cast<size_t>(RequiredSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, InText.c_str(), static_cast<int>(InText.size()),
                            OutText.data(), RequiredSize);
        return OutText;
    }

    FString WideToUtf8(const FWString& InText)
    {
        if (InText.empty())
        {
            return {};
        }

        const int RequiredSize =
            WideCharToMultiByte(CP_UTF8, 0, InText.c_str(), static_cast<int>(InText.size()),
                                nullptr, 0, nullptr, nullptr);
        if (RequiredSize <= 0)
        {
            return {};
        }

        FString OutText(static_cast<size_t>(RequiredSize), '\0');
        WideCharToMultiByte(CP_UTF8, 0, InText.c_str(), static_cast<int>(InText.size()),
                            OutText.data(), RequiredSize, nullptr, nullptr);
        return OutText;
    }

    bool ShowOpenSceneFileDialog(const IEditorChromeHost*     Host,
                                 const std::filesystem::path& InitialDirectory,
                                 std::filesystem::path&       OutSelectedPath)
    {
        std::array<wchar_t, 1024> FileBuffer{};
        const FWString            InitialDirectoryPath =
            InitialDirectory.empty() ? FWString() : InitialDirectory.wstring();

        OPENFILENAMEW Dialog = {};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = GetEditorOwnerWindow(Host);
        Dialog.lpstrFilter = SceneFileDialogFilter;
        Dialog.lpstrFile = FileBuffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(FileBuffer.size());
        Dialog.lpstrInitialDir =
            InitialDirectoryPath.empty() ? nullptr : InitialDirectoryPath.c_str();
        Dialog.lpstrDefExt = L"Scene";
        Dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameW(&Dialog))
        {
            const DWORD ExtendedError = CommDlgExtendedError();
            if (ExtendedError != 0)
            {
                UE_LOG(FEditor, ELogLevel::Error, "Open scene dialog failed with error code %lu.",
                       static_cast<unsigned long>(ExtendedError));
            }
            return false;
        }

        OutSelectedPath = std::filesystem::path(FileBuffer.data());
        return true;
    }

    bool ShowSaveSceneFileDialog(const IEditorChromeHost*     Host,
                                 const std::filesystem::path& InitialDirectory,
                                 const std::filesystem::path& CurrentScenePath,
                                 std::filesystem::path&       OutSelectedPath)
    {
        std::array<wchar_t, 1024> FileBuffer{};

        const std::filesystem::path DefaultPath =
            CurrentScenePath.empty() ? (InitialDirectory / L"Untitled.Scene") : CurrentScenePath;
        const FWString DefaultPathText = DefaultPath.wstring();
        wcsncpy_s(FileBuffer.data(), FileBuffer.size(), DefaultPathText.c_str(), _TRUNCATE);

        const FWString InitialDirectoryPath =
            InitialDirectory.empty() ? FWString() : InitialDirectory.wstring();

        OPENFILENAMEW Dialog = {};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = GetEditorOwnerWindow(Host);
        Dialog.lpstrFilter = SceneFileDialogFilter;
        Dialog.lpstrFile = FileBuffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(FileBuffer.size());
        Dialog.lpstrInitialDir =
            InitialDirectoryPath.empty() ? nullptr : InitialDirectoryPath.c_str();
        Dialog.lpstrDefExt = L"Scene";
        Dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

        if (!GetSaveFileNameW(&Dialog))
        {
            const DWORD ExtendedError = CommDlgExtendedError();
            if (ExtendedError != 0)
            {
                UE_LOG(FEditor, ELogLevel::Error, "Save scene dialog failed with error code %lu.",
                       static_cast<unsigned long>(ExtendedError));
            }
            return false;
        }

        OutSelectedPath = NormalizeSceneFilePath(std::filesystem::path(FileBuffer.data()));
        return true;
    }

    FString TrimIniValueCopy(const FString& InValue)
    {
        const size_t Start = InValue.find_first_not_of(" \t\r\n");
        if (Start == FString::npos)
        {
            return {};
        }

        const size_t End = InValue.find_last_not_of(" \t\r\n");
        return InValue.substr(Start, End - Start + 1);
    }

    std::filesystem::path GetEditorStartupIniPath()
    {
        return FPaths::Combine(FPaths::Combine(FPaths::AppRoot(), L"Config"), L"EditorStartup.ini");
    }

    bool LoadStartupAssetEntries(const std::filesystem::path& FilePath,
                                 TArray<FString>& OutAssetPaths, FString* OutErrorMessage)
    {
        std::error_code ErrorCode;
        if (!std::filesystem::exists(FilePath, ErrorCode))
        {
            return false;
        }

        std::ifstream File(FilePath);
        if (!File.is_open())
        {
            if (OutErrorMessage != nullptr)
            {
                *OutErrorMessage = "Failed to open startup settings file.";
            }
            return false;
        }

        bool    bInPreloadSection = false;
        FString Line;
        int32   LineNumber = 0;
        while (std::getline(File, Line))
        {
            ++LineNumber;
            const FString TrimmedLine = TrimIniValueCopy(Line);
            if (TrimmedLine.empty() || TrimmedLine[0] == ';' || TrimmedLine[0] == '#')
            {
                continue;
            }

            if (TrimmedLine.front() == '[')
            {
                bInPreloadSection = (TrimmedLine == "[PreloadAssets]");
                continue;
            }

            if (!bInPreloadSection)
            {
                continue;
            }

            size_t EqualsIndex = TrimmedLine.find('=');
            if (EqualsIndex == FString::npos)
            {
                continue;
            }

            FString Key = TrimIniValueCopy(TrimmedLine.substr(0, EqualsIndex));
            if (!Key.empty() && Key[0] == '+')
            {
                Key.erase(Key.begin());
            }

            if (Key != "Asset" && Key != "Assets" && Key != "Path" && Key != "Paths")
            {
                continue;
            }

            FString Value = TrimIniValueCopy(TrimmedLine.substr(EqualsIndex + 1));
            if (Value.size() >= 2 && Value.front() == '\"' && Value.back() == '\"')
            {
                Value = Value.substr(1, Value.size() - 2);
            }

            if (Value.size() >= 2 && Value.substr(Value.size() - 2) == "\\n")
            {
                Value.resize(Value.size() - 2);
            }

            Value = TrimIniValueCopy(Value);
            if (Value.empty())
            {
                continue;
            }

            OutAssetPaths.push_back(Value);
        }

        return true;
    }
} // namespace

// class FSamplePanel : public IPanel
//{
// public:
//     explicit FSamplePanel(const FEditorLogBuffer* InLogBuffer)
//         : LogBuffer(InLogBuffer)
//     {
//     }
//
//     const wchar_t* GetPanelID() const override { return L"SamplePanel"; }
//     const wchar_t* GetDisplayName() const override { return L"Sample Panel"; }
//     bool ShouldOpenByDefault() const override { return true; }
//
//     void Draw() override
//     {
//         if (ImGui::Begin("Sample Panel", nullptr))
//         {
//             ImGui::Text("PanelManager registration test panel");
//             ImGui::Separator();
//
//
//             if (LogBuffer != nullptr)
//             {
//                 for (const auto& Log : LogBuffer->GetLogBuffer())
//                 {
//                     ImGui::Spacing();
//                     ImGui::Text("%s", Log.Message.c_str());
//                 }
//             }
//         }
//         ImGui::End();
//     }
//
// private:
//     const FEditorLogBuffer* LogBuffer = nullptr;
// };

void FEditor::Create()
{
    //  LOG
    GLog = &LogBuffer;

    //  TODO : Viewport Client
    EditorContext.Editor = this;
    EditorContext.ContentIndex = &ContentIndex;
    ContentIndex.Refresh();
    UE_LOG(FEditor, ELogLevel::Info,
           "Content index refreshed during editor startup: folders=%d files=%d root=%s",
           ContentIndex.GetSnapshot().FolderCount, ContentIndex.GetSnapshot().FileCount,
           PathToUtf8String(ContentIndex.GetSnapshot().ContentRootPath).c_str());

    CurWorld = new FWorld();
    EditorContext.World = CurWorld;

    ViewportTab.Construct();
    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->SetEditorContext(&EditorContext);
            Viewport->GetViewportClient()->SetWorld(CurWorld);
        }
    }

    GlobalInputContext.SetNavigationController(
        &ViewportTab.GetViewport(0)->GetViewportClient()->GetNavigationController());
    GlobalInputController.SetEditorContext(&EditorContext);
    GlobalInputController.SetSelectionController(
        &ViewportTab.GetViewport(0)->GetViewportClient()->GetSelectionController());
    GlobalInputRouter.AddContext(&GlobalInputContext);

    LoadEditorSettings();
    UE_LOG(FEditor, ELogLevel::Info,
           "Editor settings loaded. GridSpacing=%.2f ContentBrowserLeftPaneWidth=%.2f",
           UEngineStatics::GridSpacing, EditorContext.ContentBrowserLeftPaneWidth);

    LoadStartupAssetPreloadList();

    // 메뉴 시스템은 command 등록과 배치 등록을 분리해서 초기화합니다.
    MenuRegistry.Clear();
    RegisterDefaultCommands();
    RegisterDefaultMenus();

    PanelManager = new FPanelManager();
    PanelManager->Initialize(&EditorContext);
    // 새 패널이 등록되면 Window 메뉴 항목도 함께 자동 등록합니다.
    PanelManager->SetPanelDescriptorRegisteredCallback([this](const FPanelDescriptor& Descriptor)
                                                       { RegisterWindowPanelCommand(Descriptor); });
    PanelManager->RegisterPanelInstance<FConsolePanel>(&LogBuffer);
    PanelManager->RegisterPanelInstance<FContentBrowserPanel>();
    // PanelManager->RegisterPanelInstance<FControlPanel>();
    PanelManager->RegisterPanelInstance<FOutlinerPanel>();
    PanelManager->RegisterPanelInstance<FPropertiesPanel>();
    PanelManager->RegisterPanelInstance<FShortcutsPanel>();
    // PanelManager->RegisterPanelInstance<FStatePanel>();

    // TODO: ViewportTab도 Panel로 만들기
    ViewportTab.InitializeControlPanels(&EditorContext);

    UE_LOG(FEditor, ELogLevel::Info, "Editor created successfully. Viewports=%zu PanelsReady=%s",
           ViewportTab.GetViewports().size(), PanelManager != nullptr ? "true" : "false");

    // PanelManager->RegisterPanelInstance<FSamplePanel>(&LogBuffer);

    //  TODO : Gizmo
}

void FEditor::Release()
{
    UE_LOG(FEditor, ELogLevel::Info, "Editor release started.");
    SaveEditorSettings();
    AboutImageResource = nullptr;
    bAttemptedAboutImageLoad = false;

    if (PanelManager != nullptr)
    {
        PanelManager->Shutdown();
        delete PanelManager;
        PanelManager = nullptr;
    }

    ViewportTab.Release();

    delete CurWorld;
    CurWorld = nullptr;
    GlobalInputContext.SetNavigationController(nullptr);
    GlobalInputController.SetSelectionController(nullptr);
    GlobalInputController.SetEditorContext(nullptr);
    EditorContext.World = nullptr;
    EditorContext.ContentIndex = nullptr;

    MenuRegistry.Clear();
    ChromeHost = nullptr;
    EditorChrome.SetHost(nullptr);

    if (GLog == &LogBuffer)
    {
        GLog = nullptr;
    }

    UE_LOG(FEditor, ELogLevel::Info, "Editor released successfully.");
}

void FEditor::Initialize()
{
    UE_LOG(FEditor, ELogLevel::Verbose, "Editor initialize called.");
    if (CurWorld == nullptr)
    {
        CurWorld = new FWorld();
        for (auto Viewport : ViewportTab.GetViewports())
        {
            if (Viewport->IsValid())
            {
                Viewport->GetViewportClient()->SetWorld(CurWorld);
            }
        }
        // GlobalInputController.Context->World = CurWorld;
        EditorContext.World = CurWorld;
    }
}

void FEditor::SetChromeHost(IEditorChromeHost* InChromeHost)
{
    ChromeHost = InChromeHost;
    EditorChrome.SetHost(InChromeHost);
    UE_LOG(FEditor, ELogLevel::Verbose, "Editor chrome host set: %p", InChromeHost);
}

void FEditor::SetRuntimeServices(FRendererModule*     InRenderer, RHI::FDynamicRHI* InDynamicRHI,
                                 FAssetObjectManager* InAssetObjectManager)
{
    if (InRenderer)
        EditorContext.RHI = &InRenderer->GetRHI();
    else
        EditorContext.RHI = nullptr;
    
    EditorContext.Renderer = InRenderer;
    EditorContext.DynamicRHI = InDynamicRHI;
    EditorContext.AssetObjectManager = InAssetObjectManager;
    AboutImageResource = nullptr;
    bAttemptedAboutImageLoad = false;
    EnsureAboutImageLoaded();
    ResolveSceneAssetReferences(CurWorld != nullptr ? CurWorld->GetActiveScene() : nullptr);
    PreloadStartupAssets();
    UE_LOG(FEditor, ELogLevel::Info,
           "Runtime services updated. RHI=%p DynamicRHI=%p AssetObjectManager=%p", 
           EditorContext.RHI,
           EditorContext.DynamicRHI, 
           EditorContext.AssetObjectManager
    );
}

void FEditor::LoadEditorSettings()
{
    FEditorSettingsData SettingsData;
    SettingsData.GridSpacing = UEngineStatics::GridSpacing;
    SettingsData.CameraMoveSpeed =
        ViewportTab.GetViewport(0)->GetViewportClient()->GetNavigationController().GetMoveSpeed();
    SettingsData.CameraRotationSpeed = ViewportTab.GetViewport(0)
                                           ->GetViewportClient()
                                           ->GetNavigationController()
                                           .GetRotationSpeed();
    SettingsData.ContentBrowserLeftPaneWidth = EditorContext.ContentBrowserLeftPaneWidth;
    SettingsData.ViewportLayoutType = (int32)ViewportTab.GetCurrentLayoutType();
    SettingsData.SplitterRatio1 = 0.5f;
    SettingsData.SplitterRatio2 = 0.5f;
    SettingsData.SplitterRatio3 = 0.5f;

    FString                         ErrorMessage;
    const EEditorSettingsLoadResult LoadResult =
        PersistentSettings.Load(SettingsData, &ErrorMessage);

    if (LoadResult == EEditorSettingsLoadResult::Missing)
    {
        UE_LOG(FEditor, ELogLevel::Info,
               "Editor.ini was not found. Using default editor settings.");
        return;
    }

    if (LoadResult == EEditorSettingsLoadResult::InvalidFormat)
    {
        UE_LOG(FEditor, ELogLevel::Error, "Invalid Editor.ini format: %s", ErrorMessage.c_str());
        return;
    }

    if (LoadResult == EEditorSettingsLoadResult::IOError)
    {
        UE_LOG(FEditor, ELogLevel::Error, "Failed to read Editor.ini: %s", ErrorMessage.c_str());
        return;
    }

    ViewportTab.SetLayout((EViewportLayoutType)SettingsData.ViewportLayoutType);

    float Ratios[3] = {SettingsData.SplitterRatio1, SettingsData.SplitterRatio2,
                       SettingsData.SplitterRatio3};

    ViewportTab.SetSplitterRatios(Ratios);
    OnViewportResized();

    UEngineStatics::GridSpacing = FMath::Clamp(SettingsData.GridSpacing, 1.0f, 1000.0f);

    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->GetNavigationController().SetMoveSpeed(
                SettingsData.CameraMoveSpeed);
            Viewport->GetViewportClient()->GetNavigationController().SetRotationSpeed(
                SettingsData.CameraRotationSpeed);
        }
    }
    EditorContext.ContentBrowserLeftPaneWidth =
        std::max(SettingsData.ContentBrowserLeftPaneWidth, 120.0f);

    UE_LOG(FEditor, ELogLevel::Info,
           "Loaded editor settings: GridSpacing=%.2f CameraMoveSpeed=%.2f CameraRotationSpeed=%.2f "
           "LeftPaneWidth=%.2f",
           UEngineStatics::GridSpacing, SettingsData.CameraMoveSpeed,
           SettingsData.CameraRotationSpeed, EditorContext.ContentBrowserLeftPaneWidth);
}

void FEditor::LoadStartupAssetPreloadList()
{
    FString         ErrorMessage;
    TArray<FString> StartupAssetPaths;
    if (!LoadStartupAssetEntries(GetEditorStartupIniPath(), StartupAssetPaths, &ErrorMessage))
    {
        if (!ErrorMessage.empty())
        {
            UE_LOG(FEditor, ELogLevel::Error, "Failed to read EditorStartup.ini: %s",
                   ErrorMessage.c_str());
        }
        else
        {
            UE_LOG(FEditor, ELogLevel::Info,
                   "EditorStartup.ini was not found. Startup asset preloading is disabled.");
        }
        return;
    }

    EditorContext.StartupPreloadAssetPaths = std::move(StartupAssetPaths);
    UE_LOG(FEditor, ELogLevel::Info, "Loaded EditorStartup.ini preload entries: Count=%zu",
           EditorContext.StartupPreloadAssetPaths.size());
}

void FEditor::PreloadStartupAssets()
{
    if (EditorContext.AssetObjectManager == nullptr || EditorContext.DynamicRHI == nullptr)
    {
        UE_LOG(FEditor, ELogLevel::Warning,
               "Skipped startup asset preloading because runtime asset services are incomplete.");
        return;
    }

    if (EditorContext.StartupPreloadAssetPaths.empty())
    {
        UE_LOG(FEditor, ELogLevel::Verbose, "No startup preload assets were configured.");
        return;
    }

    int32 RequestedCount = 0;
    int32 SuccessCount = 0;
    int32 FailedCount = 0;
    int32 SkippedCount = 0;

    UE_LOG(FEditor, ELogLevel::Info, "Startup asset preloading begin. Count=%zu",
           EditorContext.StartupPreloadAssetPaths.size());

    for (const FString& AssetPathValue : EditorContext.StartupPreloadAssetPaths)
    {
        std::filesystem::path ResolvedPath =
            Engine::Scene::ResolveSceneAssetPathToAbsolute(AssetPathValue);
        if (ResolvedPath.empty())
        {
            ResolvedPath = FPaths::PathFromUtf8(AssetPathValue);
        }

        const FString               AbsolutePath = PathToUtf8String(ResolvedPath);
        const Asset::EAssetFileKind AssetKind = Asset::ClassifyAssetPath(AbsolutePath);

        if (AssetKind == Asset::EAssetFileKind::Unknown)
        {
            ++SkippedCount;
            UE_LOG(
                FEditor, ELogLevel::Warning,
                "Skipped startup preload entry because the asset type could not be classified: %s",
                AssetPathValue.c_str());
            continue;
        }

        ++RequestedCount;

        bool bSucceeded = false;

        switch (AssetKind)
        {
        case Asset::EAssetFileKind::Texture:
        case Asset::EAssetFileKind::MaterialLibrary:
        case Asset::EAssetFileKind::StaticMesh:
        case Asset::EAssetFileKind::TextureAtlas:
        case Asset::EAssetFileKind::Font:
        {
            UObject* AssetObject = EditorContext.AssetObjectManager->LoadAssetObject(AbsolutePath);
            bSucceeded = (AssetObject != nullptr);
            break;
        }

        case Asset::EAssetFileKind::Scene:
        {
            ++SkippedCount;
            UE_LOG(FEditor, ELogLevel::Warning,
                   "Skipped startup preload scene asset because scene UObject creation is not "
                   "supported: %s",
                   AbsolutePath.c_str());
            continue;
        }

        default:
        {
            ++SkippedCount;
            UE_LOG(FEditor, ELogLevel::Warning,
                   "Skipped startup preload unsupported asset kind=%d path=%s",
                   static_cast<int32>(AssetKind), AbsolutePath.c_str());
            continue;
        }
        }

        if (bSucceeded)
        {
            ++SuccessCount;
            UE_LOG(FEditor, ELogLevel::Info, "Startup preload succeeded: Kind=%d Path=%s",
                   static_cast<int32>(AssetKind), AbsolutePath.c_str());
        }
        else
        {
            ++FailedCount;
            UE_LOG(FEditor, ELogLevel::Warning, "Startup preload failed: Kind=%d Path=%s",
                   static_cast<int32>(AssetKind), AbsolutePath.c_str());
        }
    }

    UE_LOG(FEditor, ELogLevel::Info,
           "Startup asset preloading finished. Requested=%d Succeeded=%d Failed=%d Skipped=%d",
           RequestedCount, SuccessCount, FailedCount, SkippedCount);
}

void FEditor::SaveEditorSettings() const
{
    FEditorSettingsData SettingsData;
    SettingsData.GridSpacing = FMath::Clamp(UEngineStatics::GridSpacing, 1.0f, 1000.0f);
    SettingsData.CameraMoveSpeed =
        ViewportTab.GetViewport(0)->GetViewportClient()->GetNavigationController().GetMoveSpeed();
    SettingsData.CameraRotationSpeed = ViewportTab.GetViewport(0)
                                           ->GetViewportClient()
                                           ->GetNavigationController()
                                           .GetRotationSpeed();
    SettingsData.ContentBrowserLeftPaneWidth =
        std::max(EditorContext.ContentBrowserLeftPaneWidth, 120.0f);

    SettingsData.ViewportLayoutType = (int32)ViewportTab.GetCurrentLayoutType();
    float Ratios[3];
    ViewportTab.GetSplitterRatios(Ratios);
    SettingsData.SplitterRatio1 = Ratios[0];
    SettingsData.SplitterRatio2 = Ratios[1];
    SettingsData.SplitterRatio3 = Ratios[2];

    if (PersistentSettings.Save(SettingsData))
    {
        UE_LOG(FEditor, ELogLevel::Verbose,
               "Saved editor settings: GridSpacing=%.2f CameraMoveSpeed=%.2f "
               "CameraRotationSpeed=%.2f LeftPaneWidth=%.2f",
               SettingsData.GridSpacing, SettingsData.CameraMoveSpeed,
               SettingsData.CameraRotationSpeed, SettingsData.ContentBrowserLeftPaneWidth);
    }
    else
    {
        UE_LOG(FEditor, ELogLevel::Warning, "Failed to save editor settings.");
    }
}

void FEditor::MarkSceneClean() { SceneDocument.bDirty = false; }

std::filesystem::path FEditor::GetSceneDirectory() const
{
    return FEditorPaths::DefaultSceneDirectory();
}

bool FEditor::SaveScene()
{
    if (!SceneDocument.CurrentScenePath.empty())
    {
        return SaveSceneToPath(SceneDocument.CurrentScenePath, true);
    }

    SaveSceneAs();
    return false;
}

void FEditor::SaveSceneAs()
{
    std::filesystem::path       SelectedPath;
    const std::filesystem::path InitialDirectory =
        !SceneDocument.CurrentScenePath.empty() ? SceneDocument.CurrentScenePath.parent_path()
                                                : GetSceneDirectory();

    if (!ShowSaveSceneFileDialog(ChromeHost, InitialDirectory, SceneDocument.CurrentScenePath,
                                 SelectedPath))
    {
        SceneDocument.DeferredAction.Reset();
        return;
    }

    if (!SaveSceneToPath(SelectedPath, true))
    {
        SceneDocument.DeferredAction.Reset();
        return;
    }

    const FDeferredSceneAction DeferredAction = SceneDocument.DeferredAction;
    SceneDocument.DeferredAction.Reset();
    ExecuteDeferredSceneAction(DeferredAction);
}

void FEditor::RequestOpenScene()
{
    std::filesystem::path       SelectedPath;
    const std::filesystem::path InitialDirectory =
        !SceneDocument.CurrentScenePath.empty() ? SceneDocument.CurrentScenePath.parent_path()
                                                : GetSceneDirectory();

    if (!ShowOpenSceneFileDialog(ChromeHost, InitialDirectory, SelectedPath))
    {
        UE_LOG(FEditor, ELogLevel::Verbose, "Open scene dialog was cancelled.");
        return;
    }

    UE_LOG(FEditor, ELogLevel::Info, "Scene open requested: %s",
           PathToUtf8String(SelectedPath).c_str());

    const FDeferredSceneAction Action{.Type = EDeferredSceneActionType::OpenScene,
                                      .ScenePath = SelectedPath};
    if (!ConfirmProceedWithDirtyScene(Action))
    {
        return;
    }

    LoadSceneFromPath(SelectedPath);
}

void FEditor::PerformNewScene()
{
    UE_LOG(FEditor, ELogLevel::Info, "Creating a new scene document.");
    ReplaceCurrentScene(std::make_unique<FScene>());
    SceneDocument.CurrentScenePath.clear();
    MarkSceneClean();
}

void FEditor::PerformClearScene()
{
    UE_LOG(FEditor, ELogLevel::Info, "Clearing the current scene.");
    FScene* ActiveScene = (CurWorld != nullptr) ? CurWorld->GetActiveScene() : nullptr;
    if (ActiveScene == nullptr)
    {
        ReplaceCurrentScene(std::make_unique<FScene>());
        return;
    }

    const TArray<AActor*>* SceneActors = ActiveScene->GetActors();
    const bool             bHadActors = SceneActors != nullptr && !SceneActors->empty();
    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->GetSelectionController().ClearSelection();
        }
    }
    ActiveScene->Clear();

    if (bHadActors || !SceneDocument.CurrentScenePath.empty())
    {
        MarkSceneDirty();
    }
}

bool FEditor::SaveSceneToPath(const std::filesystem::path& FilePath, bool bUpdateCurrentPath)
{
    FScene* ActiveScene = (CurWorld != nullptr) ? CurWorld->GetActiveScene() : nullptr;
    if (ActiveScene == nullptr)
    {
        UE_LOG(FEditor, ELogLevel::Error, "No scene is available to save.");
        return false;
    }

    if (ViewportTab.GetViewport(0)->IsValid() == false)
    {
        UE_LOG(FEditor, ELogLevel::Error, "No viewport is available to save scene from.");
        return false;
    }

    FCameraInfo      CameraInfo;
    FViewportCamera& Camera = ViewportTab.GetViewport(0)->GetViewportClient()->GetCamera();
    CameraInfo.Location = Camera.GetLocation();
    CameraInfo.Rotation = Camera.GetRotation().Rotator();
    CameraInfo.FOV = Camera.GetFOV();
    CameraInfo.NearClip = Camera.GetNearPlane();
    CameraInfo.FarClip = Camera.GetFarPlane();

    FString ErrorMessage;
    if (!FSceneSerializer::SaveToFile(*ActiveScene, CameraInfo, FilePath, &ErrorMessage))
    {
        UE_LOG(FEditor, ELogLevel::Error, "Failed to save scene: %s", ErrorMessage.c_str());
        return false;
    }

    if (bUpdateCurrentPath)
    {
        SceneDocument.CurrentScenePath = FilePath;
    }
    MarkSceneClean();

    UE_LOG(FEditor, ELogLevel::Info, "Saved scene: %s", PathToUtf8String(FilePath).c_str());
    return true;
}

bool FEditor::LoadSceneFromPath(const std::filesystem::path& FilePath)
{
    FString                 ErrorMessage;
    std::unique_ptr<FScene> LoadedScene = FSceneDeserializer::LoadFromFile(FilePath, &ErrorMessage);
    if (!LoadedScene)
    {
        UE_LOG(FEditor, ELogLevel::Error, "Failed to load scene: %s", ErrorMessage.c_str());
        return false;
    }

    ReplaceCurrentScene(std::move(LoadedScene));
    SceneDocument.CurrentScenePath = FilePath;
    MarkSceneClean();

    UE_LOG(FEditor, ELogLevel::Info, "Loaded scene: %s", PathToUtf8String(FilePath).c_str());
    return true;
}

void FEditor::ReplaceCurrentScene(std::unique_ptr<FScene> NewScene)
{
    UE_LOG(FEditor, ELogLevel::Info, "Replacing current scene.");
    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->GetSelectionController().ClearSelection();
        }
    }

    if (ViewportTab.GetViewport(0)->IsValid() && NewScene != nullptr)
    {
        FViewportCamera&   Camera = ViewportTab.GetViewport(0)->GetViewportClient()->GetCamera();
        const FCameraInfo& CameraInfo = NewScene->GetEditorCameraInfo();
        Camera.SetLocation(CameraInfo.Location);
        Camera.SetRotation(CameraInfo.Rotation);
        Camera.SetFOV(CameraInfo.FOV);
        Camera.SetNearPlane(CameraInfo.NearClip);
        Camera.SetFarPlane(CameraInfo.FarClip);

        FViewportNavigationController& NavController =
            ViewportTab.GetViewport(0)->GetViewportClient()->GetNavigationController();
        NavController.SetYaw(CameraInfo.Rotation.Yaw);
        NavController.SetPitch(CameraInfo.Rotation.Pitch);
        NavController.SetTargetLocation(CameraInfo.Location);
        NavController.SetHasTargetLocation(true);
    }

    if (CurWorld == nullptr)
    {
        CurWorld = new FWorld();
        EditorContext.World = CurWorld;
    }

    if (NewScene == nullptr)
    {
        NewScene = std::make_unique<FScene>();
    }

    CurWorld->ReplaceActiveScene(std::move(NewScene));

    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->SetWorld(CurWorld);
        }
    }
    EditorContext.SelectedObject = nullptr;
    EditorContext.SelectedActors.clear();
    ResolveSceneAssetReferences(CurWorld->GetActiveScene());
}

void FEditor::ResolveActorAssetReferences(AActor* Actor)
{
    if (Actor == nullptr || EditorContext.AssetObjectManager == nullptr ||
        EditorContext.DynamicRHI == nullptr)
    {
        return;
    }

    UE_LOG(FEditor, ELogLevel::Verbose, "Resolving asset references for actor: %s",
           Actor->GetTypeName());
    FSceneAssetBinder::BindActor(Actor, EditorContext.AssetObjectManager);
}

void FEditor::ResolveSceneAssetReferences(FScene* Scene)
{
    if (Scene == nullptr || EditorContext.AssetObjectManager == nullptr ||
        EditorContext.DynamicRHI == nullptr)
    {
        return;
    }

    TArray<AActor*>* SceneActors = (Scene->GetActors());
    UE_LOG(FEditor, ELogLevel::Info, "Resolving scene asset references for %zu actor(s).",
           SceneActors != nullptr ? SceneActors->size() : size_t(0));
    FSceneAssetBinder::BindScene(Scene, EditorContext.AssetObjectManager);
}

void FEditor::Tick(float DeltaTime, Engine::ApplicationCore::FInputSystem* InputSystem)
{
    EditorContext.DeltaTime = DeltaTime;
    Engine::ApplicationCore::FInputEvent Event;
    Engine::ApplicationCore::FInputState InputState = InputSystem->GetInputState();

    FViewport*    HoveredViewport = nullptr;
    FViewportRect HoveredRect{};
    ;
    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (!Viewport->IsValid())
            continue;

        FViewportRect Rect = Viewport->GetSceneView()->GetViewRect();
        if (InputState.MouseX >= Rect.X && InputState.MouseX < Rect.X + Rect.Width &&
            InputState.MouseY >= Rect.Y && InputState.MouseY < Rect.Y + Rect.Height)
        {
            HoveredViewport = Viewport;
            HoveredRect = Rect;
            break;
        }
    }

    FViewport* ActiveViewport = bIsMouseCaptured ? CapturedViewport : HoveredViewport;

    if (HoveredViewport != nullptr)
    {
        GlobalInputContext.SetNavigationController(
            &HoveredViewport->GetViewportClient()->GetNavigationController());
        GlobalInputController.SetSelectionController(
            &HoveredViewport->GetViewportClient()->GetSelectionController());
    }

    FViewportRect ActiveRect = {};
    if (ActiveViewport != nullptr)
        ActiveRect = ActiveViewport->GetSceneView()->GetViewRect();

    while (InputSystem->PollEvent(Event))
    {
        if (Event.Type == Engine::ApplicationCore::EInputEventType::MouseButtonDown)
        {
            if (HoveredViewport != nullptr && !bIsMouseCaptured)
            {
                CapturedViewport = HoveredViewport;
                bIsMouseCaptured = true;
                ActiveViewport = CapturedViewport;
                ActiveRect = HoveredRect;
            }
        }

        Event.MouseX -= ActiveRect.X;
        Event.MouseY -= ActiveRect.Y;
        InputState.ChangeToLocal(Event.MouseX, Event.MouseY);

        if (GlobalInputRouter.RouteEvent(Event, InputState))
        {
            if (Event.Type == Engine::ApplicationCore::EInputEventType::MouseButtonUp)
            {
                bIsMouseCaptured = false;
                CapturedViewport = nullptr;
            }
            continue;
        }

        if (ActiveViewport != nullptr)
            ActiveViewport->GetViewportClient()->HandleInputEvent(Event, InputState);

        if (Event.Type == Engine::ApplicationCore::EInputEventType::MouseButtonUp)
        {
            bIsMouseCaptured = false;
            CapturedViewport = nullptr;
        }
    }

    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->Tick(DeltaTime, InputState);
        }
    }

    if (PanelManager != nullptr)
    {
        PanelManager->Tick(DeltaTime);
    }

    if (CurWorld)
    {
        CurWorld->Tick(DeltaTime);
    }

    OnViewportResized();
    BuildRenderData();
}

void FEditor::OnWindowResized(float Width, float Height)
{
    if (Width <= 0 || Height <= 0)
    {
        return;
    }

    WindowHeight = Height;
    WindowWidth = Width;
    EditorContext.WindowWidth = Width;
    EditorContext.WindowHeight = Height;

    OnViewportResized();
}

void FEditor::OnViewportResized()
{
    if (RootDockSpaceId != 0)
    {
        if (ImGuiDockNode* node = ImGui::DockBuilderGetCentralNode(RootDockSpaceId))
        {
            if (node->Size.x > 0 && node->Size.y > 0)
                ViewportTab.OnResize({(int32)node->Pos.x, (int32)node->Pos.y, (int32)node->Size.x,
                                      (int32)node->Size.y});
        }
    }
    else
    {
        ViewportTab.OnResize({0, 0, (int32)WindowWidth, (int32)WindowHeight});
    }
}

void FEditor::CreateNewScene()
{
    const FDeferredSceneAction Action{.Type = EDeferredSceneActionType::NewScene};
    if (!ConfirmProceedWithDirtyScene(Action))
    {
        return;
    }

    PerformNewScene();
}

bool FEditor::SaveCurrentSceneToDisk() { return SaveScene(); }

void FEditor::RequestSaveSceneAs() { SaveSceneAs(); }

void FEditor::RequestOpenSceneDialog() { RequestOpenScene(); }

void FEditor::RequestAboutPopUp() { RequestAboutPopup(); }

bool FEditor::SaveSceneAsPath(const std::filesystem::path& FilePath)
{
    return SaveSceneToPath(FilePath, true);
}

bool FEditor::OpenSceneFromPath(const std::filesystem::path& FilePath)
{
    return LoadSceneFromPath(FilePath);
}

bool FEditor::CanDeleteSelectedActors() const
{
    return GlobalInputController.CanDeleteSelectedActors();
}

bool FEditor::DeleteSelectedActors() { return GlobalInputController.DeleteSelectedActors(); }

void FEditor::RefreshContentIndex() { ContentIndex.Refresh(); }

std::filesystem::path FEditor::GetDefaultSceneDirectory() const { return GetSceneDirectory(); }

void FEditor::ClearScene()
{
    const FDeferredSceneAction Action{.Type = EDeferredSceneActionType::ClearScene};
    if (!ConfirmProceedWithDirtyScene(Action))
    {
        return;
    }

    PerformClearScene();
}

bool FEditor::RequestCloseEditor()
{
    const FDeferredSceneAction Action{.Type = EDeferredSceneActionType::CloseEditor};
    return ConfirmProceedWithDirtyScene(Action);
}

void FEditor::SetSelectedObject(UObject* InSelectedObject)
{
    EditorContext.SelectedActors.clear();
    if (AActor* SelectedActor = Cast<AActor>(InSelectedObject))
    {
        EditorContext.SelectedActors.push_back(SelectedActor);
    }
    else if (auto* SelectedComponent = Cast<Engine::Component::USceneComponent>(InSelectedObject))
    {
        if (AActor* OwnerActor = SelectedComponent->GetOwnerActor())
        {
            EditorContext.SelectedActors.push_back(OwnerActor);
        }
    }

    EditorContext.SelectedObject = InSelectedObject;

    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->SyncSelectionFromContext();
        }
    }
}

void FEditor::AddActorToScene(AActor* InActor, bool bSelectActor)
{
    if (InActor == nullptr)
    {
        return;
    }

    if (CurWorld == nullptr || CurWorld->GetActiveScene() == nullptr)
    {
        delete InActor;
        return;
    }

    CurWorld->GetActiveScene()->AddActor(InActor);
    ResolveActorAssetReferences(InActor);
    MarkSceneDirty();

    if (bSelectActor)
    {
        for (auto Viewport : ViewportTab.GetViewports())
        {
            if (Viewport->IsValid())
            {
                Viewport->GetViewportClient()->GetSelectionController().SelectActor(
                    InActor, ESelectionMode::Replace);
            }
        }
    }
}

void FEditor::MarkSceneDirty() { SceneDocument.bDirty = true; }

void FEditor::EnsureAboutImageLoaded()
{
    if (AboutImageResource != nullptr || bAttemptedAboutImageLoad ||
        EditorContext.AssetObjectManager == nullptr)
    {
        return;
    }

    bAttemptedAboutImageLoad = true;

    const std::filesystem::path ImagePath = FEditorPaths::AboutLogo();
    // TODO
}

void FEditor::RequestAboutPopup()
{
    bRequestOpenAboutPopup = true;
    bAboutPopupOpen = true;
}

bool FEditor::ConfirmProceedWithDirtyScene(const FDeferredSceneAction& Action)
{
    if (!SceneDocument.bDirty)
    {
        return true;
    }

    FString ActionLabel = "continue";
    switch (Action.Type)
    {
    case EDeferredSceneActionType::NewScene:
        ActionLabel = "create a new scene";
        break;
    case EDeferredSceneActionType::ClearScene:
        ActionLabel = "clear the scene";
        break;
    case EDeferredSceneActionType::OpenScene:
        ActionLabel = "open another scene";
        break;
    case EDeferredSceneActionType::CloseEditor:
        ActionLabel = "close the editor";
        break;
    default:
        break;
    }

    const FWString Message =
        L"The current scene has unsaved changes.\n\nDo you want to save before you " +
        Utf8ToWide(ActionLabel) + L"?";
    const HWND OwnerWindow =
        static_cast<HWND>(ChromeHost != nullptr ? ChromeHost->GetNativeWindowHandle() : nullptr);
    const int DialogResult = MessageBoxW(OwnerWindow, Message.c_str(), L"Unsaved Scene",
                                         MB_ICONWARNING | MB_YESNOCANCEL | MB_SETFOREGROUND);

    if (DialogResult == IDCANCEL)
    {
        return false;
    }

    if (DialogResult == IDNO)
    {
        return true;
    }

    if (!SceneDocument.CurrentScenePath.empty())
    {
        return SaveSceneToPath(SceneDocument.CurrentScenePath, true);
    }

    SceneDocument.DeferredAction = Action;
    SaveSceneAs();
    return false;
}

void FEditor::ExecuteDeferredSceneAction(FDeferredSceneAction Action)
{
    if (Action.Type == EDeferredSceneActionType::None)
    {
        return;
    }

    switch (Action.Type)
    {
    case EDeferredSceneActionType::NewScene:
        PerformNewScene();
        break;

    case EDeferredSceneActionType::ClearScene:
        PerformClearScene();
        break;

    case EDeferredSceneActionType::OpenScene:
        if (!Action.ScenePath.empty())
        {
            LoadSceneFromPath(Action.ScenePath);
        }
        break;

    case EDeferredSceneActionType::CloseEditor:
        if (ChromeHost != nullptr)
        {
            ChromeHost->CloseWindow();
        }
        break;

    default:
        break;
    }
}

void FEditor::RegisterDefaultCommands()
{
    // 여기서는 "무엇을 실행할지"만 등록하고, 메뉴 어디에 둘지는 RegisterDefaultMenus에서 정합니다.
    MenuRegistry.RegisterCommand(
        FEditorCommandDefinition{.CommandId = "file.new_scene",
                                 .Label = L"New Scene",
                                 .ShortcutLabel = "Ctrl+N",
                                 .Execute = [this]() { CreateNewScene(); }});

    MenuRegistry.RegisterCommand(
        FEditorCommandDefinition{.CommandId = "file.open_scene",
                                 .Label = L"Open Scene",
                                 .ShortcutLabel = "Ctrl+O",
                                 .Execute = [this]() { RequestOpenScene(); }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "file.save_scene",
                                                          .Label = L"Save Scene",
                                                          .ShortcutLabel = "Ctrl+S",
                                                          .Execute = [this]() { SaveScene(); }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "file.save_scene_as",
                                                          .Label = L"Save Scene as..",
                                                          .ShortcutLabel = "Ctrl+Shift+S",
                                                          .Execute = [this]() { SaveSceneAs(); }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "file.clear_scene",
                                                          .Label = L"Clear Scene",
                                                          .Execute = [this]() { ClearScene(); }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "file.exit",
                                                          .Label = L"Exit",
                                                          .ShortcutLabel = "Alt+F4",
                                                          .Execute = [this]()
                                                          {
                                                              if (ChromeHost != nullptr)
                                                              {
                                                                  ChromeHost->CloseWindow();
                                                              }
                                                          }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "edit.undo",
                                                          .Label = L"Undo",
                                                          .ShortcutLabel = "Ctrl+Z",
                                                          .CanExecute = []() { return false; }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "edit.redo",
                                                          .Label = L"Redo",
                                                          .ShortcutLabel = "Ctrl+Y",
                                                          .CanExecute = []() { return false; }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{
        .CommandId = "edit.delete_selection",
        .Label = L"Delete Selection",
        .ShortcutLabel = "Delete",
        .Execute = [this]() { GlobalInputController.DeleteSelectedActors(); },
        .CanExecute = [this]() { return GlobalInputController.CanDeleteSelectedActors(); }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "edit.preferences",
                                                          .Label = L"Preferences (Not Ready)",
                                                          .CanExecute = []() { return false; }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{
        .CommandId = "tool.content_browser",
        .Label = L"Content Browser",
        .Execute =
            [this]()
        {
            if (PanelManager == nullptr)
            {
                return;
            }

            FPanelOpenRequest Request;
            Request.PanelType = std::type_index(typeid(FContentBrowserPanel));
            Request.OpenPolicy = EPanelOpenPolicy::FocusIfOpenElseCreate;
            PanelManager->OpenPanel(Request);
        },
        .CanExecute = [this]() { return PanelManager != nullptr; }});

    MenuRegistry.RegisterCommand(FEditorCommandDefinition{.CommandId = "tool.build",
                                                          .Label = L"Build (Not Ready)",
                                                          .CanExecute = []() { return false; }});

    MenuRegistry.RegisterCommand(
        FEditorCommandDefinition{.CommandId = "help.shortcuts",
                                 .Label = L"Shortcuts",
                                 .Execute =
                                     [this]()
                                 {
                                     if (PanelManager == nullptr)
                                     {
                                         return;
                                     }

                                     FPanelOpenRequest Request;
                                     Request.PanelType = std::type_index(typeid(FShortcutsPanel));
                                     Request.OpenPolicy = EPanelOpenPolicy::FocusIfOpenElseCreate;
                                     PanelManager->OpenPanel(Request);
                                 },
                                 .CanExecute = [this]() { return PanelManager != nullptr; }});

    MenuRegistry.RegisterCommand(
        FEditorCommandDefinition{.CommandId = "help.about",
                                 .Label = L"About",
                                 .ShortcutLabel = "F1",
                                 .Execute = [this]() { RequestAboutPopup(); }});
}

void FEditor::RegisterDefaultMenus()
{
    // 여기서는 이미 등록된 command를 어떤 메뉴/순서에 배치할지만 정의합니다.
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::File, .CommandId = "file.new_scene", .Order = 0});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::File, .CommandId = "file.open_scene", .Order = 10});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::File, .CommandId = "file.save_scene", .Order = 20});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::File, .CommandId = "file.save_scene_as", .Order = 30});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::File, .CommandId = "file.clear_scene", .Order = 40});
    MenuRegistry.RegisterMenuSeparator(EEditorMainMenu::File, {}, 50);
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::File, .CommandId = "file.exit", .Order = 60});

    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Edit, .CommandId = "edit.undo", .Order = 0});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Edit, .CommandId = "edit.redo", .Order = 10});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Edit, .CommandId = "edit.delete_selection", .Order = 20});
    MenuRegistry.RegisterMenuSeparator(EEditorMainMenu::Edit, {}, 30);
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Edit, .CommandId = "edit.preferences", .Order = 40});

    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Tool, .CommandId = "tool.content_browser", .Order = 0});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Tool, .CommandId = "tool.build", .Order = 10});

    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Help, .CommandId = "help.shortcuts", .Order = 0});
    MenuRegistry.RegisterMenuItem(FEditorMenuEntryDefinition{
        .MainMenu = EEditorMainMenu::Help, .CommandId = "help.about", .Order = 10});
}

void FEditor::RegisterWindowPanelCommand(const FPanelDescriptor& Descriptor)
{
    if (!Descriptor.bShowInWindowMenu)
    {
        return;
    }

    // PanelDescriptor를 Window 메뉴용 체크형 command로 변환합니다.
    const FString         CommandId = BuildPanelCommandId(Descriptor);
    const std::type_index PanelType = Descriptor.PanelType;

    MenuRegistry.RegisterCommand(
        FEditorCommandDefinition{.CommandId = CommandId,
                                 .Label = Descriptor.DisplayName,
                                 .Execute =
                                     [this, PanelType]()
                                 {
                                     if (PanelManager == nullptr)
                                     {
                                         return;
                                     }

                                     FPanelOpenRequest Request;
                                     Request.PanelType = PanelType;
                                     PanelManager->TogglePanel(Request);
                                 },
                                 .CanExecute = [this]() { return PanelManager != nullptr; },
                                 .IsChecked =
                                     [this, PanelType]()
                                 {
                                     if (PanelManager == nullptr)
                                     {
                                         return false;
                                     }

                                     FPanelOpenRequest Request;
                                     Request.PanelType = PanelType;
                                     if (IPanel* Panel = PanelManager->FindPanel(Request))
                                     {
                                         return Panel->IsOpen();
                                     }

                                     return false;
                                 },
                                 .bCheckable = true});

    MenuRegistry.RegisterMenuItem(
        FEditorMenuEntryDefinition{.MainMenu = EEditorMainMenu::Window,
                                   .SubmenuPath = Descriptor.WindowMenuPath,
                                   .CommandId = CommandId,
                                   .Order = Descriptor.WindowMenuOrder});
}

void FEditor::DrawAboutPopup()
{
    if (bRequestOpenAboutPopup)
    {
        ImGui::OpenPopup(AboutPopupId);
        bRequestOpenAboutPopup = false;
    }

    EnsureAboutImageLoaded();
    ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(AboutPopupId, &bAboutPopupOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static const std::array<FWString, 4> Contributors = {
            L"강명호",
            L"김연하",
            L"김형도",
            L"양현석",
        };

        static const std::array<FWString, 4> ContributorSummaries = {
            L"에디터 UI, 패널, 도구 기능, 에셋 연결",
            L"렌더러, 스프라이트·텍스트 렌더링, ShowFlags",
            L"기즈모 조작, 중심 하이라이트, 다중 회전",
            L"엔진 루프, 뷰포트 입력, 카메라, 스냅핑",
        };

        static const std::array<FWString, 4> ContributorDisplayNames = {
            L"\uAC15\uBA85\uD638",
            L"\uAE40\uC5F0\uD558",
            L"\uAE40\uD615\uB3C4",
            L"\uC591\uD604\uC11D",
        };

        static const std::array<FWString, 4> ContributorDisplaySummaries = {
            L"Editor UI, panels, tools, and asset integration",
            L"Renderer, sprite/text rendering, and show flags",
            L"Gizmo interaction, center highlight, and multi-rotation",
            L"Engine loop, viewport input, camera, and snapping",
        };

        ImGui::TextUnformatted("CO-PASS Engine");
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(185, 185, 190, 255));
        ImGui::TextWrapped("Level editor and rendering sandbox for the CO-PASS project.");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::TextUnformatted("Version: Internal Prototype");
#ifdef _DEBUG
        ImGui::Text("Build: Debug | %s | commit %s", __DATE__, "1ddf265");
#else
        ImGui::Text("Build: Release | %s | commit %s", __DATE__, "1ddf265");
#endif
        ImGui::TextUnformatted("Tech Stack: C++, Win32, Direct3D 11, ImGui");
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(185, 185, 190, 255));
        ImGui::TextWrapped("Includes third-party software such as Dear ImGui and nlohmann/json.");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::SeparatorText("First Contributors");
        ImGui::Spacing();

        for (size_t ContributorIndex = 0; ContributorIndex < ContributorDisplayNames.size();
             ++ContributorIndex)
        {
            const FString ContributorUtf8 = WideToUtf8(ContributorDisplayNames[ContributorIndex]);
            const FString SummaryUtf8 = WideToUtf8(ContributorDisplaySummaries[ContributorIndex]);
            ImGui::BulletText("%s", ContributorUtf8.c_str());
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(185, 185, 190, 255));
            ImGui::TextWrapped("%s", SummaryUtf8.c_str());
            ImGui::PopStyleColor();
            ImGui::Unindent();
            ImGui::Spacing();
        }

        ImGui::Spacing();

        if (AboutImageResource != nullptr && AboutImageResource->GetSRV() != nullptr &&
            AboutImageResource->Width > 0 && AboutImageResource->Height > 0)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            const float AvailableWidth = ImGui::GetContentRegionAvail().x;
            const float TargetWidth = std::min(AvailableWidth, 460.0f);
            const float AspectRatio = static_cast<float>(AboutImageResource->Height) /
                                      static_cast<float>(AboutImageResource->Width);
            const ImVec2 ImageSize(TargetWidth, TargetWidth * AspectRatio);
            if (AvailableWidth > ImageSize.x)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     (AvailableWidth - ImageSize.x) * 0.5f);
            }

            ImGui::Image(ImTextureRef(static_cast<ImTextureID>(
                             reinterpret_cast<uintptr_t>(AboutImageResource->GetSRV()))),
                         ImageSize);
        }

        ImGui::Spacing();
        {
            const char* CopyrightText = "Copyright (c) 2026 CO-PASS Engine Contributors";
            const float AvailableWidth = ImGui::GetContentRegionAvail().x;
            const float TextWidth = ImGui::CalcTextSize(CopyrightText).x;
            if (AvailableWidth > TextWidth)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (AvailableWidth - TextWidth) * 0.5f);
            }

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 155, 255));
            ImGui::TextUnformatted(CopyrightText);
            ImGui::PopStyleColor();
        }

        ImGui::EndPopup();
    }
}

void FEditor::BuildSceneView()
{
    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetSceneView()->SetViewMatrix(
                Viewport->GetViewportClient()->GetCamera().GetViewMatrix());
            Viewport->GetSceneView()->SetProjectionMatrix(
                Viewport->GetViewportClient()->GetCamera().GetProjectionMatrix());
            Viewport->GetSceneView()->SetViewLocation(
                Viewport->GetViewportClient()->GetCamera().GetLocation());
            Viewport->GetSceneView()->SetClipPlanes(
                Viewport->GetViewportClient()->GetCamera().GetNearPlane(),
                Viewport->GetViewportClient()->GetCamera().GetFarPlane());
        }
    }
}

void FEditor::DrawRootDockSpace()
{
#ifdef IMGUI_HAS_DOCK
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) == 0)
    {
        return;
    }

    ImGuiViewport* Viewport = ImGui::GetMainViewport();
    if (Viewport == nullptr)
    {
        return;
    }

    const FEditorGutterMetrics GutterMetrics = GetDockSpaceGutterMetrics(ChromeHost);
    DrawDockSpaceGutters(Viewport, GutterMetrics);

    const float DockSpaceWidth = (Viewport->Size.x > (GutterMetrics.Left + GutterMetrics.Right))
                                     ? (Viewport->Size.x - GutterMetrics.Left - GutterMetrics.Right)
                                     : 0.0f;
    const float DockSpaceHeight =
        (Viewport->Size.y > (FEditorChrome::TitleBarHeight + GutterMetrics.Bottom))
            ? (Viewport->Size.y - FEditorChrome::TitleBarHeight - GutterMetrics.Bottom)
            : 0.0f;

    if (DockSpaceWidth <= 0.0f || DockSpaceHeight <= 0.0f)
    {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(Viewport->Pos.x + GutterMetrics.Left,
                                   Viewport->Pos.y + FEditorChrome::TitleBarHeight));
    ImGui::SetNextWindowSize(ImVec2(DockSpaceWidth, DockSpaceHeight));
    ImGui::SetNextWindowViewport(Viewport->ID);

    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings;

    if ((RootDockSpaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) != 0)
    {
        WindowFlags |= ImGuiWindowFlags_NoBackground;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("##EditorRootDockSpace", nullptr, WindowFlags))
    {
        RootDockSpaceId = ImGui::GetID("EditorRootDockSpace");
        ImGui::DockSpace(RootDockSpaceId, ImVec2(0.0f, 0.0f), RootDockSpaceFlags);
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
#endif
}

void FEditor::DrawPanel()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();
    // 매 프레임 현재 enabled/checked 상태를 평가해서 chrome이 소비할 메뉴 스냅샷을 만듭니다.
    const TArray<FEditorChromeMenu> ChromeMenus = MenuRegistry.BuildChromeMenus();
    DrawRootDockSpace();

    if (PanelManager != nullptr)
    {
        PanelManager->DrawPanels();
    }

    ViewportTab.DrawControlPanels();
    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            Viewport->GetViewportClient()->DrawViewportOverlay();
        }
    }

    EditorChrome.Draw(ChromeMenus);
    DrawAboutPopup();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void FEditor::BuildRenderData()
{
    EditorRenderDatas.clear();
    SceneRenderDatas.clear();

    BuildSceneView();

    for (auto Viewport : ViewportTab.GetViewports())
    {
        if (Viewport->IsValid())
        {
            FEditorRenderData EditorRenderData = FEditorRenderData{};
            FSceneRenderData  SceneRenderData = FSceneRenderData{};

            SceneRenderData.SceneView = Viewport->GetSceneView();
            SceneRenderData.ViewMode =
                Viewport->GetViewportClient()->GetRenderSetting().GetViewMode();

            const EEditorShowFlags EditorShowFlags =
                Viewport->GetViewportClient()->GetRenderSetting().BuildEditorShowFlags(true);
            const ESceneShowFlags SceneShowFlags =
                Viewport->GetViewportClient()->GetRenderSetting().BuildSceneShowFlags();

            // World에서 렌더링되어야 할 것들로 RenderData 채우기
            if (CurWorld != nullptr)
            {
                CurWorld->BuildRenderData(SceneRenderData, SceneShowFlags);
            }
            // Viewport에서 렌더링되어야 할 것들로 RenderData 채우기 & 뷰포트 카메라 정보 설정 (기즈모 등)
            Viewport->GetViewportClient()->BuildRenderData(EditorRenderData, SceneRenderData, EditorShowFlags);

            EditorRenderDatas.push_back(EditorRenderData);
            SceneRenderDatas.push_back(SceneRenderData);
        }
    }
}

#pragma once

#include "Panel.h"
#include "imgui.h"

class FViewportCamera;

class FControlPanel : public IPanel
{
public:
    const wchar_t* GetPanelID() const override;
    const wchar_t* GetDisplayName() const override;
    bool ShouldOpenByDefault() const override { return true; }
    int32 GetWindowMenuOrder() const override { return 20; }

    void Draw() override;

    void SetViewportIndex(int32 Index) { ViewportIndex = Index; }

private:
    template <typename Func>
    void DrawSectionButton(const char* Label, const char* PopupId, Func DrawContent);

    FViewportCamera* ResolveViewportCamera() const;
    void DrawUnavailableState() const;
    void DrawTransformSection(FViewportCamera& Camera) const;
    void DrawProjectionSection(FViewportCamera& Camera) const;
    void DrawViewModeSection() const;
    void DrawLayoutSection() const;
    void DrawShowFlagsSection() const;
    void DrawNavigationSection() const;
    void DrawWorldSection() const;

private:
    int32 ViewportIndex = 0;
};

template <typename Func>
inline void FControlPanel::DrawSectionButton(const char* Label, const char* PopupId,
                                             Func DrawContent)
{
    const std::string ButtonId = std::string(Label) + "##btn_" + std::to_string(ViewportIndex);
    const std::string FullPopupId = std::string(PopupId) + "##" + std::to_string(ViewportIndex);

    if (ImGui::Button(ButtonId.c_str()))
    {
        ImGui::OpenPopup(FullPopupId.c_str());
    }

    // 팝업 위치를 버튼 바로 아래에 고정
    ImVec2 ButtonPos = ImGui::GetItemRectMin();
    ImVec2 ButtonSize = ImGui::GetItemRectSize();
    ImGui::SetNextWindowPos(ImVec2(ButtonPos.x, ButtonPos.y + ButtonSize.y), ImGuiCond_Always);

    if (ImGui::BeginPopup(FullPopupId.c_str()))
    {
        ImGui::SetNextWindowBgAlpha(0.9f);
        DrawContent();
        ImGui::EndPopup();
    }
}

#include "ImGuiManager.h"

void UImGuiManager::Create(HWND hWnd, URenderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer->GetDevice(), renderer->GetDeviceContext());
}

void UImGuiManager::Update() {
    beginFrame();
    ImGui::Text(buffer);
    endFrame();
}

void UImGuiManager::Release() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void UImGuiManager::beginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("timer");
}

void UImGuiManager::endFrame() {
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UImGuiManager::DrawMyText(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
}
#include "stdafx.h"
#include "GuiHandler.h"

void GuiHandler::Initialize(ComPtr<ID3D12Device14> pd3dDevice)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    {
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;
    }
    pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pFontSrvDescriptorHeap.GetAddressOf()));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(WinCore::sm_hWnd);
    ImGui_ImplDX12_Init(pd3dDevice.Get(), 2,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_pFontSrvDescriptorHeap.Get(),
        m_pFontSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        m_pFontSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void GuiHandler::Update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void GuiHandler::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
    // Rendering
    ImGui::Render();

    pd3dCommandList->SetDescriptorHeaps(1, m_pFontSrvDescriptorHeap.GetAddressOf());

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pd3dCommandList.Get());

}

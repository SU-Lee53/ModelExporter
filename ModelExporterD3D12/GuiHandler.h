#pragma once

class GuiHandler {
public:
	void Initialize(ComPtr<ID3D12Device14> pd3dDevice);
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

private:
	ComPtr<ID3D12DescriptorHeap> m_pFontSrvDescriptorHeap = nullptr;



};


#pragma once


class Texture {
public:
	Texture() = default;
	Texture(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::wstring_view wsvPath);
	
	//void UpdateShaderVariables(ComPtr<ID3D12Device14> pDevice, UINT DescriptorBase, UINT offset);
	void SetName(std::string_view svName) { m_strName = svName; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() { return m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
private:
	void LoadFromDDSFile(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::wstring_view wsvPath);
	void LoadFromWICFile(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::wstring_view wsvPath);

private:
	std::string m_strName = "";

	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap = nullptr;
	ComPtr<ID3D12Resource>		m_pTexture = nullptr;
	ComPtr<ID3D12Resource>		m_pTextureUploadBuffer = nullptr;

};



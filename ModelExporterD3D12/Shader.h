#pragma once

class Shader {
public:
	Shader();

	void Create(ComPtr<ID3D12Device> pd3dDevice);
	void Bind(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	
private:
	void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice);
	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice);
	void CompileShader();

private:
	ComPtr<ID3D12RootSignature> m_pd3dRootSignature = nullptr;
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;

	ComPtr<ID3DBlob> m_pVSBlob = nullptr;
	ComPtr<ID3DBlob> m_pPSBlob = nullptr;

};


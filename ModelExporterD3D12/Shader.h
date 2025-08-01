#pragma once

enum SHADER_TYPE : uint8_t {
	SHADER_TYPE_DIFFUSED = 0,
	SHADER_TYPE_ALBEDO,

	SHADER_TYPE_COUNT
};

class Shader {
public:
	Shader();

	void Create(ComPtr<ID3D12Device> pd3dDevice);
	void Bind(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	
protected:
	virtual void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice) = 0;
	virtual void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice) = 0;
	void CompileShader(std::string_view svVSEntryName, std::string_view svPSEntryName);

protected:
	ComPtr<ID3D12RootSignature> m_pd3dRootSignature = nullptr;
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;

	ComPtr<ID3DBlob> m_pVSBlob = nullptr;
	ComPtr<ID3DBlob> m_pPSBlob = nullptr;

};

class DiffusedShader : public Shader {
public:
	void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice) override;
	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice) override;

};

class AlbedoShader : public Shader {
public:
	void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice) override;
	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice) override;


};
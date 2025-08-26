#include "stdafx.h"
#include "Shader.h"

Shader::Shader()
{
}

void Shader::Create(ComPtr<ID3D12Device> pd3dDevice)
{
	CreatePipelineState(pd3dDevice);
}

void Shader::Bind(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dRootSignature.Get());
}

void Shader::CompileShader(std::string_view svVSEntryName, std::string_view svPSEntryName)
{
	UINT nCompileFlags = 0;
#ifdef _DEBUG
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> pErrorBlob = nullptr;

	// VS
	HRESULT hr = ::D3DCompileFromFile(
		L"Shader.hlsl",
		NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		svVSEntryName.data(),
		"vs_5_1",
		nCompileFlags,
		0,
		m_pVSBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf()
	);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
	}

	// PS
	hr = ::D3DCompileFromFile(
		L"Shader.hlsl",
		NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		svPSEntryName.data(),
		"ps_5_1",
		nCompileFlags,
		0,
		m_pPSBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf()
	);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
	}
}


// ==============
// DiffusedShader
// ==============
void DiffusedShader::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	std::array<CD3DX12_DESCRIPTOR_RANGE, 3> descRange{};
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);	// b1 transform 
	descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);	// b2 material
	descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0);	// t0, t1, t2, t3, t4, t5 : 각각 Albedo(diffuse), Specular, Metallic, Normal, Emission, Detail Albedo, Detail Normal
	
	std::array<CD3DX12_ROOT_PARAMETER, 6> rootParameters{};
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// b0 ViewProj
	rootParameters[1].InitAsDescriptorTable(1, &descRange[0]);	// transform, material
	rootParameters[2].InitAsDescriptorTable(1, &descRange[1]);	// transform, material
	rootParameters[3].InitAsDescriptorTable(1, &descRange[2]);	// texture 6개(최대)
	rootParameters[4].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// Animation Control
	rootParameters[5].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// Animation Control

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	{
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(rootParameters.size(), rootParameters.data(), 1, &samplerDesc, rootSignatureFlag);

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = ::D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	if (FAILED(hr)) {
		if (pd3dErrorBlob) {
			OutputDebugStringA((char*)pd3dErrorBlob->GetBufferPointer());
		}
	}

	hr = pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dRootSignature.GetAddressOf()));

	if (FAILED(hr)) __debugbreak();
}

void DiffusedShader::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice)
{
	CompileShader("VSMain", "PSDiffused");
	CreateRootSignature(pd3dDevice);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc;
	::ZeroMemory(&pipelineDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	{
		pipelineDesc.pRootSignature = m_pd3dRootSignature.Get();
		pipelineDesc.VS = { m_pVSBlob->GetBufferPointer(), m_pVSBlob->GetBufferSize() };
		pipelineDesc.PS = { m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize() };
		pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		//pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pipelineDesc.InputLayout = VertexType::GetInputLayout();
		pipelineDesc.SampleMask = UINT_MAX;
		pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		pipelineDesc.SampleDesc.Count = 1;
		pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));

	if (FAILED(hr)) {
		::SHOW_ERROR("Create Pipeline Failed");
		__debugbreak();
	}
}

void AlbedoShader::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	std::array<CD3DX12_DESCRIPTOR_RANGE, 3> descRange{};
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);	// b1 transform 
	descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);	// b2 material
	descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0);	// t0, t1, t2, t3, t4, t5 : 각각 Albedo(diffuse), Specular, Metallic, Normal, Emission, Detail Albedo, Detail Normal

	std::array<CD3DX12_ROOT_PARAMETER, 6> rootParameters{};
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// b0 ViewProj
	rootParameters[1].InitAsDescriptorTable(1, &descRange[0]);	// transform, material
	rootParameters[2].InitAsDescriptorTable(1, &descRange[1]);	// transform, material
	rootParameters[3].InitAsDescriptorTable(1, &descRange[2]);	// texture 6개(최대)
	rootParameters[4].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// Animation Control
	rootParameters[5].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// Animation Control

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	{
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(rootParameters.size(), rootParameters.data(), 1, &samplerDesc, rootSignatureFlag);

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = ::D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	if (FAILED(hr)) {
		if (pd3dErrorBlob) {
			OutputDebugStringA((char*)pd3dErrorBlob->GetBufferPointer());
		}
	}

	hr = pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dRootSignature.GetAddressOf()));

	if (FAILED(hr)) __debugbreak();
}

void AlbedoShader::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice)
{
	CompileShader("VSMain", "PSAlbedo");
	CreateRootSignature(pd3dDevice);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc;
	::ZeroMemory(&pipelineDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	{
		pipelineDesc.pRootSignature = m_pd3dRootSignature.Get();
		pipelineDesc.VS = { m_pVSBlob->GetBufferPointer(), m_pVSBlob->GetBufferSize() };
		pipelineDesc.PS = { m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize() };
		pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		//pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pipelineDesc.InputLayout = VertexType::GetInputLayout();
		pipelineDesc.SampleMask = UINT_MAX;
		pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		pipelineDesc.SampleDesc.Count = 1;
		pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));

	if (FAILED(hr)) {
		::SHOW_ERROR("Create Pipeline Failed");
		__debugbreak();
	}

}

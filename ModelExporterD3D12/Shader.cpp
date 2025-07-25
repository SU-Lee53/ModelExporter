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

void Shader::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	CD3DX12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// ViewProj
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);	// Transform

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(2, rootParameters, 0, NULL, rootSignatureFlag);


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

void Shader::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice)
{
	CompileShader();
	CreateRootSignature(pd3dDevice);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc;
	::ZeroMemory(&pipelineDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	{
		pipelineDesc.pRootSignature = m_pd3dRootSignature.Get();
		pipelineDesc.VS = { m_pVSBlob->GetBufferPointer(), m_pVSBlob->GetBufferSize() };
		pipelineDesc.PS = { m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize() };
		pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
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

void Shader::CompileShader()
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
		NULL,
		"VSMain",
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
		NULL,
		"PSMain",
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

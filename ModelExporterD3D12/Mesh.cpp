#include "stdafx.h"
#include "Mesh.h"

std::vector<D3D12_INPUT_ELEMENT_DESC> VertexType::inputElements = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 5, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 6, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 7, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 4, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 5, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 6, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 7, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

Mesh::Mesh()
{
}

void Mesh::CreateIndexBuffer(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::vector<UINT> Indices)
{
	HRESULT hr;

	UINT nIndices = Indices.size();
	UINT IndexBufferSize = sizeof(UINT) * nIndices;

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIndexUploadBuffer.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	m_pIndexUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	{
		memcpy(pVertexDataBegin, Indices.data(), IndexBufferSize);
	}
	m_pIndexUploadBuffer->Unmap(0, nullptr);


	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pIndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	{
		pd3dCommandList->CopyBufferRegion(m_pIndexBuffer.Get(), 0, m_pIndexUploadBuffer.Get(), 0, IndexBufferSize);
	}
	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	m_d3dIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = IndexBufferSize;
}

std::shared_ptr<Mesh> Mesh::LoadFromInfo(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const MESH_IMPORT_INFO& info)
{
	std::shared_ptr<Mesh> pMesh = std::make_shared<Mesh>();

	std::vector<VertexType> vertices(info.xmf3Positions.size());
	std::vector<UINT> indices;

	size_t idx{ 0 };
	std::generate(vertices.begin(), vertices.end(), [&info, &idx]()-> VertexType {
		VertexType v;
		v.xmf3Position	= info.xmf3Positions[idx];
		v.xmf3Normal	= info.xmf3Normals[idx];
		v.xmf3Tangent	= info.xmf3Tangents[idx];
		v.xmf3BiTangent = info.xmf3BiTangents[idx];

		for (int i = 0; i < info.xmf4Colors.size(); ++i) {
			//v.xmf4Color[i] = info.xmf4Colors[i][idx];
			if (info.xmf4Colors[i].empty()) {
				v.xmf4Color[i] = XMFLOAT4{ 0.f, 1.f, 0.f, 0.f };
			}
			else {
				v.xmf4Color[i] = info.xmf4Colors[i][idx];
			}
		}
		
		for (int i = 0; i < info.xmf2TexCoords.size(); ++i) {
			if (info.xmf2TexCoords[i].empty()) {
				v.xmf2TexCoords[i] = XMFLOAT2{ 0.f, 0.f };
			}
			else {
				v.xmf2TexCoords[i] = info.xmf2TexCoords[i][idx];
			}
		}

		++idx;
		return v;
	});

	indices.reserve(info.uiIndices.size());
	std::copy(info.uiIndices.begin(), info.uiIndices.end(), std::back_inserter(indices));

	pMesh->CreateVertexBuffer(pd3dDevice, pd3dCommandList, vertices);
	pMesh->CreateIndexBuffer(pd3dDevice, pd3dCommandList, indices);
	
	return pMesh;
}

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

	m_nIndices = Indices.size();
	UINT IndexBufferSize = sizeof(UINT) * m_nIndices;

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
	pMesh->m_strMeshName = info.strMeshName;

	size_t idx{ 0 };
	pMesh->m_Vertices.reserve(info.xmf3Positions.size());
	std::generate_n(std::back_inserter(pMesh->m_Vertices), info.xmf3Positions.size(), [&info, &idx]()-> VertexType {
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

	pMesh->m_Indices.reserve(info.uiIndices.size());
	std::copy(info.uiIndices.begin(), info.uiIndices.end(), std::back_inserter(pMesh->m_Indices));

	pMesh->CreateVertexBuffer(pd3dDevice, pd3dCommandList, pMesh->m_Vertices);
	pMesh->CreateIndexBuffer(pd3dDevice, pd3dCommandList, pMesh->m_Indices);
	
	return pMesh;
}

void Mesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList)
{
	pd3dRenderCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dRenderCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);

	pd3dRenderCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
	pd3dRenderCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

void Mesh::ShowMeshInfo()
{
	std::string strTreeKey1 = std::format("{} - nVertices: {:<5}", m_strMeshName, m_Vertices.size());
	if(ImGui::TreeNode(strTreeKey1.c_str())) {
		ImGui::TreePop();
	}

	std::string strTreeKey2 = std::format("{} - nIndices: {:<5}", m_strMeshName, m_Indices.size());
	if(ImGui::TreeNode(strTreeKey2.c_str())) {


		ImGui::TreePop();
	}

}

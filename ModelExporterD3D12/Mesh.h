#pragma once

struct VertexType {
	XMFLOAT3	xmf3Position;
	XMFLOAT3	xmf3Normal;
	XMFLOAT3	xmf3Tangent;
	XMFLOAT3	xmf3BiTangent;

	XMFLOAT4	xmf4Color[8];
	XMFLOAT2	xmf2TexCoords[8];

	XMINT4			blendIndices;
	XMFLOAT4		blendWeights;

	static D3D12_INPUT_LAYOUT_DESC GetInputLayout() {
		return {
			VertexType::inputElements.data(),
			(UINT)VertexType::inputElements.size()
		};
	}

private:
	static std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

};

struct MESH_IMPORT_INFO {
	std::string strMeshName;

	std::vector<XMFLOAT3>					xmf3Positions;
	std::vector<XMFLOAT3>					xmf3Normals;
	std::vector<XMFLOAT3>					xmf3Tangents;
	std::vector<XMFLOAT3>					xmf3BiTangents;
	std::array<std::vector<XMFLOAT4>, 8>	xmf4Colors;
	std::vector<UINT>						uiIndices;
	std::array<std::vector<XMFLOAT2>, 8>	xmf2TexCoords;


	std::vector<XMINT4>			blendIndices;
	std::vector<XMFLOAT4>		blendWeights;
};

class Mesh {
public:
	Mesh();

	template<typename T>
	void CreateVertexBuffer(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::vector<T> vertices);
	void CreateIndexBuffer(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::vector<UINT> Indices);

	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList);

	void ShowMeshInfo();

public:
	static std::shared_ptr<Mesh> LoadFromInfo(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const MESH_IMPORT_INFO& info);
	auto& GetVertices() { return m_Vertices; }

private:
	std::vector<VertexType> m_Vertices;
	std::vector<UINT> m_Indices;
	std::string m_strMeshName;

	ComPtr<ID3D12Resource>		m_pVertexBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pVertexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_d3dVertexBufferView = {};
	
	ComPtr<ID3D12Resource>		m_pIndexBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pIndexUploadBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW		m_d3dIndexBufferView = {};

private:
	D3D12_PRIMITIVE_TOPOLOGY		m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT							m_nSlot = 0;
	UINT							m_nVertices = 0;
	UINT							m_nIndices = 0;
	UINT							m_nOffset = 0;
};

template<typename T>
inline void Mesh::CreateVertexBuffer(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::vector<T> vertices)
{
	HRESULT hr;

	m_nVertices = vertices.size();
	UINT VertexBufferSize = sizeof(T) * m_nVertices;

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexUploadBuffer.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	m_pVertexUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	{
		memcpy(pVertexDataBegin, vertices.data(), VertexBufferSize);
	}
	m_pVertexUploadBuffer->Unmap(0, nullptr);


	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	{
		pd3dCommandList->CopyBufferRegion(m_pVertexBuffer.Get(), 0, m_pVertexUploadBuffer.Get(), 0, VertexBufferSize);
	}
	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_d3dVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = sizeof(T);
	m_d3dVertexBufferView.SizeInBytes = VertexBufferSize;

}

#include "stdafx.h"
#include "Texture.h"

namespace fs = std::filesystem;

Texture::Texture(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::wstring_view wsvPath)
{
	fs::path texPath = { wsvPath };
	
	if (texPath.extension().string() == ".dds") {
		LoadFromDDSFile(pd3dDevice, pd3dCommandList, wsvPath);
	}
	else {
		LoadFromWICFile(pd3dDevice, pd3dCommandList, wsvPath);
	}

	D3D12_RESOURCE_DESC textureDesc = m_pTexture->GetDesc();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	{
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NodeMask = 0;
	}
	pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	{
		srvDesc.Format = textureDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	}
	pd3dDevice->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

void Texture::LoadFromDDSFile(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::wstring_view wsvPath)
{
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subResourceData;

	HRESULT hr = LoadDDSTextureFromFile(pd3dDevice.Get(), wsvPath.data(), m_pTexture.GetAddressOf(), ddsData, subResourceData);
	if (FAILED(hr)) {
		__debugbreak();
	}

	UINT uiSubResourceSize = (UINT)subResourceData.size();
	UINT64 ui64UploadBufferSize = ::GetRequiredIntermediateSize(m_pTexture.Get(), 0, uiSubResourceSize);

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ui64UploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pTextureUploadBuffer.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	{
		// UpdateSubresources 는 D3DX에서 제공하는 함수로 아래 UpdateTextureForWrite 에서 하던 CopyTextureRegion 을 수행
		// 이번에는 subResource 가 존재하므로(밉맵) 이 함수를 이용하면 간편하게 Command List 에 복사하는 커맨드를 기록할 수 있음
		::UpdateSubresources(pd3dCommandList.Get(), m_pTexture.Get(), m_pTextureUploadBuffer.Get(), 0, 0, uiSubResourceSize, subResourceData.data());
	}
	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

}

void Texture::LoadFromWICFile(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::wstring_view wsvPath)
{
	D3D12_RESOURCE_DESC textureDesc{};
	std::unique_ptr<uint8_t[]> decodedData;
	D3D12_SUBRESOURCE_DATA subResourceData;

	HRESULT hr = LoadWICTextureFromFile(pd3dDevice.Get(), wsvPath.data(), m_pTexture.GetAddressOf(), decodedData, subResourceData);
	if (FAILED(hr)) {
		__debugbreak();
	}

	UINT uiSubResourceSize = 1;
	UINT64 ui64UploadBufferSize = ::GetRequiredIntermediateSize(m_pTexture.Get(), 0, 1);

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ui64UploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pTextureUploadBuffer.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	{
		UpdateSubresources(pd3dCommandList.Get(), m_pTexture.Get(), m_pTextureUploadBuffer.Get(), 0, 0, uiSubResourceSize, &subResourceData);
	}
	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

}

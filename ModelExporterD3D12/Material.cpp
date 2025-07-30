#include "stdafx.h"
#include "Material.h"
#include "GameObject.h"

Material::Material(std::shared_ptr<GameObject> pOwner)
	: m_wpOwner{ pOwner }
{
}

void Material::UpdateShaderVariables(ComPtr<ID3D12Device14> pDevice)
{
	CB_MATERIAL_DATA bindData{};

	bindData.xmf4AlbedoColor	= m_xmf4AlbedoColor;
	bindData.xmf4SpecularColor	= m_xmf4SpecularColor;
	bindData.xmf4AmbientColor	= m_xmf4AmbientColor;
	bindData.xmf4EmissiveColor	= m_xmf4EmissiveColor;

	bindData.fGlossiness		= m_fGlossiness;
	bindData.fSmoothness		= m_fSmoothness;
	bindData.fSpecularHighlight = m_fSpecularHighlight;
	bindData.fMetallic			= m_fMetallic;
	bindData.fGlossyReflection	= m_fGlossyReflection;

	::memcpy(m_pMappedPtr, &bindData, sizeof(CB_MATERIAL_DATA));
	
	auto pOwner = m_wpOwner.lock();

	//pDevice->CopyDescriptorsSimple(
	//	MATERIAL_DESCRIPTOR_COUNT_PER_DRAW,
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE(pOwner->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), 1, D3DCore::g_nCBVSRVDescriptorIncrementSize),
	//	m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
	//	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	//);

}

std::shared_ptr<Material> Material::LoadFromInfo(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const MATERIAL_IMPORT_INFO& info, std::shared_ptr<GameObject> pOwner)
{
	std::shared_ptr<Material> pMaterial = std::make_shared<Material>(pOwner);

	pMaterial->m_xmf4AlbedoColor = info.xmf4AlbedoColor;
	pMaterial->m_xmf4SpecularColor = info.xmf4SpecularColor;
	pMaterial->m_xmf4AmbientColor = info.xmf4AmbientColor;
	pMaterial->m_xmf4EmissiveColor = info.xmf4EmissiveColor;

	pMaterial->m_fGlossiness = info.fGlossiness;
	pMaterial->m_fSmoothness = info.fSmoothness;
	pMaterial->m_fSpecularHighlight = info.fSpecularHighlight;
	pMaterial->m_fMetallic = info.fMetallic;
	pMaterial->m_fGlossyReflection = info.fGlossyReflection;

	
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(::AlignConstantBuffersize(sizeof(CB_TRANSFORM_DATA))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pMaterial->m_pCBMaterialData.GetAddressOf())
	);

	pMaterial->m_pCBMaterialData->Map(0, NULL, reinterpret_cast<void**>(&pMaterial->m_pMappedPtr));

	// Material data Heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	{
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = MATERIAL_DESCRIPTOR_COUNT_PER_DRAW;	// CB 1개 + Texture(SRV) 7개
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NodeMask = 0;
	}
	pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(pMaterial->m_pd3dDescriptorHeap.GetAddressOf()));

	// Material Data CBuffer 뷰 생성
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	{
		cbvDesc.BufferLocation = pMaterial->m_pCBMaterialData->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = ::AlignConstantBuffersize(sizeof(CB_TRANSFORM_DATA));
	}

	pd3dDevice->CreateConstantBufferView(&cbvDesc, pMaterial->m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 생성
	pMaterial->LoadTextures(pd3dDevice, pd3dCommandList, info);

	// 힙에다 텍스쳐 복사
	for (int i = 0; i < MATERIAL_TEXTURE_TYPE_COUNT; ++i) {
		if (pMaterial->m_pTextures[i]) {
			pd3dDevice->CopyDescriptorsSimple(
				1,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(pMaterial->m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 1 + i, D3DCore::g_nCBVSRVDescriptorIncrementSize),	// 1번 자리에는 Materaial Data
				pMaterial->m_pTextures[i]->GetCPUHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}
	}

	return pMaterial;
}

void Material::LoadTextures(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const MATERIAL_IMPORT_INFO& info)
{
	if (!info.strAlbedoMapName.empty()) {
		std::filesystem::path p{ info.strAlbedoMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_ALBEDO] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_ALBEDO]->SetName(p.filename().string());
	}
	
	if (!info.strSpecularMapName.empty()) {
		std::filesystem::path p{ info.strSpecularMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_SPECULAR] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_SPECULAR]->SetName(p.filename().string());
	}
	
	if (!info.strMetallicMapName.empty()) {
		std::filesystem::path p{ info.strMetallicMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_METALLIC] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_METALLIC]->SetName(p.filename().string());
	}
	
	if (!info.strNormalMapName.empty()) {
		std::filesystem::path p{ info.strNormalMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_NORMAL] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_NORMAL]->SetName(p.filename().string());
	}
	
	if (!info.strEmissionMapName.empty()) {
		std::filesystem::path p{ info.strEmissionMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_EMISSION] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_EMISSION]->SetName(p.filename().string());
	}
	
	if (!info.strDetailAlbedoMapName.empty()) {
		std::filesystem::path p{ info.strDetailAlbedoMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_DETAIL_ALBEDO] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_DETAIL_ALBEDO]->SetName(p.filename().string());
	}
	
	if (!info.strDetailNormalMapName.empty()) {
		std::filesystem::path p{ info.strDetailNormalMapName };
		m_pTextures[MATERIAL_TEXTURE_TYPE_DETAIL_NORMAL] = std::make_shared<Texture>(pd3dDevice, pd3dCommandList, p.wstring());
		m_pTextures[MATERIAL_TEXTURE_TYPE_DETAIL_NORMAL]->SetName(p.filename().string());
	}

}

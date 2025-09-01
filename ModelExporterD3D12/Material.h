#pragma once
#include "Texture.h"

class GameObject;

struct MATERIAL_IMPORT_INFO {
	std::string		strMaterialName;

	XMFLOAT4		xmf4AlbedoColor;
	XMFLOAT4		xmf4SpecularColor;
	XMFLOAT4		xmf4AmbientColor;
	XMFLOAT4		xmf4EmissiveColor;
	
	float			fGlossiness = 0.0f;
	float			fSmoothness = 0.0f;
	float			fSpecularHighlight = 0.0f;
	float			fMetallic = 0.0f;
	float			fGlossyReflection = 0.0f;
	
	std::string		strAlbedoMapName;
	std::string		strSpecularMapName;
	std::string		strMetallicMapName;
	std::string		strNormalMapName;
	std::string		strEmissionMapName;
	std::string		strDetailAlbedoMapName;
	std::string		strDetailNormalMapName;

	void Export(std::ofstream& os) const;

};

enum MATERAIAL_TEXTURE_TYPE : uint8_t {
	MATERIAL_TEXTURE_TYPE_ALBEDO = 0,
	MATERIAL_TEXTURE_TYPE_SPECULAR,
	MATERIAL_TEXTURE_TYPE_METALLIC,
	MATERIAL_TEXTURE_TYPE_NORMAL,
	MATERIAL_TEXTURE_TYPE_EMISSION,
	MATERIAL_TEXTURE_TYPE_DETAIL_ALBEDO,
	MATERIAL_TEXTURE_TYPE_DETAIL_NORMAL,

	MATERIAL_TEXTURE_TYPE_COUNT
};

struct CB_MATERIAL_DATA {
	XMFLOAT4		xmf4AlbedoColor;		// c0
	XMFLOAT4		xmf4SpecularColor;		// c1
	XMFLOAT4		xmf4AmbientColor;		// c2
	XMFLOAT4		xmf4EmissiveColor;		// c3
			
	float			fGlossiness;			// c4.x
	float			fSmoothness;			// c4.y
	float			fSpecularHighlight;		// c4.z
	float			fMetallic;				// c4.w
	float			fGlossyReflection;		// c5.x

};

class Material {
public:
	Material(std::shared_ptr<GameObject> pOwner);

	void UpdateShaderVariables(ComPtr<ID3D12Device14> pDevice);
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return m_pd3dDescriptorHeap; }

public:
	static std::shared_ptr<Material> LoadFromInfo(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const MATERIAL_IMPORT_INFO& info, std::shared_ptr<GameObject> pOwner);

private:
	void LoadTextures(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const MATERIAL_IMPORT_INFO& info);

private:
	std::weak_ptr<GameObject> m_wpOwner;

private:
	XMFLOAT4	m_xmf4AlbedoColor = { 0.f, 0.f, 0.f, 1.f };
	XMFLOAT4	m_xmf4SpecularColor = { 0.f, 0.f, 0.f, 1.f };
	XMFLOAT4	m_xmf4AmbientColor = { 0.f, 0.f, 0.f, 1.f };
	XMFLOAT4	m_xmf4EmissiveColor = { 0.f, 0.f, 0.f, 1.f };
				
	float		m_fGlossiness = 0.0f;
	float		m_fSmoothness = 0.0f;
	float		m_fSpecularHighlight = 0.0f;
	float		m_fMetallic = 0.0f;
	float		m_fGlossyReflection = 0.0f;

	std::array<std::shared_ptr<Texture>, MATERAIAL_TEXTURE_TYPE::MATERIAL_TEXTURE_TYPE_COUNT> m_pTextures = {};

private:
	ComPtr<ID3D12DescriptorHeap>	m_pd3dDescriptorHeap = nullptr; 
	ComPtr<ID3D12Resource>			m_pCBMaterialData = nullptr;
	UINT8*							m_pMappedPtr = nullptr;

public:
	constexpr static UINT MATERIAL_DESCRIPTOR_COUNT_PER_DRAW = 8;

};


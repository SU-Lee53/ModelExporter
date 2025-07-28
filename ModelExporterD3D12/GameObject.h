#pragma once
#include "Mesh.h"
#include "Material.h"

struct OBJECT_IMPORT_INFO {
	std::string strNodeName;

	std::vector<MESH_IMPORT_INFO> meshInfos;
	std::vector<MATERIAL_IMPORT_INFO> materialInfos;
	XMFLOAT4X4 xmf4x4Bone;

	std::shared_ptr<OBJECT_IMPORT_INFO> m_pParent;
	std::vector<std::shared_ptr<OBJECT_IMPORT_INFO>> m_pChildren;

};

struct CB_TRANSFORM_DATA {
	XMFLOAT4X4 xmf4x4Local;
	XMFLOAT4X4 xmf4x4World;
};

class GameObject {
public:
	GameObject();

	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList);

	XMFLOAT4X4 ComputeLocalMatrix();

	void ShowObjInfo();

public:
	static std::shared_ptr<GameObject> 
		LoadFromImporter(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::shared_ptr<OBJECT_IMPORT_INFO> pInfo, std::shared_ptr<GameObject> m_pParent);

private:
	std::vector<std::shared_ptr<Mesh>> m_pMeshes;
	std::string m_strName = "";

	XMFLOAT4X4 m_xmf4x4Local;
	XMFLOAT4X4 m_xmf4x4Bone;
	XMFLOAT4X4 m_xmf4x4World;
	ComPtr<ID3D12Resource> m_pCBTransform = nullptr;
	UINT8* m_pTransformMappedPtr;

	std::weak_ptr<GameObject> m_pParent;
	std::vector<std::shared_ptr<GameObject>> m_pChildren;

	XMFLOAT4 m_xmf4Color = {1.f, 0.f, 0.f, 1.f};
	ComPtr<ID3D12Resource> m_pCBColor = nullptr;
	UINT8* m_pColorMappedPtr;


};


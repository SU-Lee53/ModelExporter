#pragma once
#include "Mesh.h"
#include "Material.h"

struct OBJECT_IMPORT_INFO {
	std::string strNodeName;

	std::vector<MESH_IMPORT_INFO> meshInfos;
	std::vector<MATERIAL_IMPORT_INFO> materialInfos;
	XMFLOAT4X4 xmf4x4Transform;

	std::shared_ptr<OBJECT_IMPORT_INFO> pParent;
	std::vector<std::shared_ptr<OBJECT_IMPORT_INFO>> pChildren;

};

class GameObject {
public:
	GameObject();

public:
	static std::shared_ptr<GameObject> 
		LoadFromImporter(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::shared_ptr<OBJECT_IMPORT_INFO> pInfo, std::shared_ptr<GameObject> pParent);

private:
	std::vector<std::shared_ptr<Mesh>> m_pMeshes;
	XMFLOAT4X4 m_xmf4x4Transform = {};
	std::string m_strName = "";

	std::shared_ptr<GameObject> pParent;
	std::vector<std::shared_ptr<GameObject>> pChildren;


};


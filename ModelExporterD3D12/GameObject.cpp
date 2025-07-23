#include "stdafx.h"
#include "GameObject.h"

GameObject::GameObject()
{
	XMStoreFloat4x4(&m_xmf4x4Transform, XMMatrixIdentity());
}

std::shared_ptr<GameObject> GameObject::LoadFromImporter(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
	std::shared_ptr<OBJECT_IMPORT_INFO> pInfo, std::shared_ptr<GameObject> pParent)
{
	std::shared_ptr<GameObject> pObj = std::make_shared<GameObject>();

	// TODO : Load Mesh & Material
	for (int i = 0; i < pInfo->meshInfos.size(); ++i) {
		pObj->m_pMeshes.push_back(Mesh::LoadFromInfo(pd3dDevice, pd3dCommandList, pInfo->meshInfos[i]));
	}

	pObj->m_strName = pInfo->strNodeName;
	pObj->m_xmf4x4Transform = pInfo->xmf4x4Transform;
	pObj->pParent = pParent;

	for (int i = 0; i < pInfo->pChildren.size(); ++i) {
		pObj->pChildren.push_back(LoadFromImporter(pd3dDevice, pd3dCommandList, pInfo->pChildren[i], pObj));
	}

	return pObj;
}

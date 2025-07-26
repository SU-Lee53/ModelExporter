#include "stdafx.h"
#include "GameObject.h"

GameObject::GameObject()
{
	XMStoreFloat4x4(&m_xmf4x4Local, XMMatrixIdentity());
	XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());
}

void GameObject::ShowObjInfo()
{
	std::string strTreeKey = std::format("{}", m_strName);
	if (ImGui::TreeNode(strTreeKey.c_str())) {
		ImGui::Text("Obj Name : %s", m_strName.c_str());
		ImGui::Text("Local Transform :\n%s", ::FormatMatrix(m_xmf4x4Local).c_str());
		ImGui::Text("World Transform :\n%s", ::FormatMatrix(m_xmf4x4World).c_str());
		
		for (auto pMesh : m_pMeshes) {
			std::string strTreeKey2 = std::format("{} MESH", strTreeKey);
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				pMesh->ShowMeshInfo();
				ImGui::TreePop();
			}
		}

		ImGui::SeparatorText("Children");

		for (auto pChild : m_pChildren) {
			pChild->ShowObjInfo();
		}

		ImGui::TreePop();
	}
}

std::shared_ptr<GameObject> GameObject::LoadFromImporter(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
	std::shared_ptr<OBJECT_IMPORT_INFO> pInfo, std::shared_ptr<GameObject> m_pParent)
{
	std::shared_ptr<GameObject> pObj = std::make_shared<GameObject>();

	// TODO : Load Mesh & Material
	for (int i = 0; i < pInfo->meshInfos.size(); ++i) {
		pObj->m_pMeshes.push_back(Mesh::LoadFromInfo(pd3dDevice, pd3dCommandList, pInfo->meshInfos[i]));
	}

	pObj->m_strName = pInfo->strNodeName;
	pObj->m_xmf4x4Local = pInfo->xmf4x4Transform;
	pObj->m_pParent = m_pParent;

	for (int i = 0; i < pInfo->m_pChildren.size(); ++i) {
		pObj->m_pChildren.push_back(LoadFromImporter(pd3dDevice, pd3dCommandList, pInfo->m_pChildren[i], pObj));
	}

	// Create CBV
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(::AlignConstantBuffersize(sizeof(CB_TRANSFORM_DATA))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pObj->m_pCBTransform.GetAddressOf())
	);

	pObj->m_pCBTransform->Map(0, NULL, reinterpret_cast<void**>(&pObj->m_pTransformMappedPtr));
	
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(::AlignConstantBuffersize(sizeof(XMFLOAT4))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pObj->m_pCBColor.GetAddressOf())
	);

	pObj->m_pCBColor->Map(0, NULL, reinterpret_cast<void**>(&pObj->m_pColorMappedPtr));

	return pObj;
}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList) 
{
	CB_TRANSFORM_DATA bindData;

	XMStoreFloat4x4(&bindData.xmf4x4Local, XMMatrixTranspose(XMLoadFloat4x4(&ComputeLocalMatrix())));
	//XMStoreFloat4x4(&bindData.xmf4x4Local, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	XMStoreFloat4x4(&bindData.xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	::memcpy(m_pTransformMappedPtr, &bindData, sizeof(CB_TRANSFORM_DATA));

	pd3dRenderCommandList->SetGraphicsRootConstantBufferView(1, m_pCBTransform->GetGPUVirtualAddress());

	for (auto& pMesh : m_pMeshes) {
		pMesh->Render(pd3dRenderCommandList);
	}

	for (auto& pChild : m_pChildren) {
		pChild->Render(pd3dRenderCommandList);
	}

}

// Computes Node Local Space to Model Local Space
XMFLOAT4X4 GameObject::ComputeLocalMatrix()
{
	//  ????
	XMMATRIX xmmtxResult = XMLoadFloat4x4(&m_xmf4x4Local);

	//assert(XMMatrixIsIdentity(xmmtxResult));

	std::shared_ptr<GameObject> pParent = m_pParent.lock();
	while (pParent != nullptr) {
		xmmtxResult = XMMatrixMultiply(XMLoadFloat4x4(&pParent->m_xmf4x4Local), xmmtxResult);
		pParent = pParent->m_pParent.lock();
	}

	XMFLOAT4X4 xmf4x4Ret;
	XMStoreFloat4x4(&xmf4x4Ret, xmmtxResult);

	//assert(XMMatrixIsIdentity(xmmtxResult));

	return xmf4x4Ret;
}

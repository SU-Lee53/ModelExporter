#include "stdafx.h"
#include "GameObject.h"

std::map<std::string, BONE_IMPORT_INFO> OBJECT_IMPORT_INFO::boneMap{};

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
		
		for (auto pMesh : m_pMeshMaterialPairs | std::views::transform([](auto p) { return p.first; })) {
			std::string strTreeKey2 = std::format("{} MESH", strTreeKey);
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				pMesh->ShowMeshInfo();
				ImGui::TreePop();
			}
		}

		ImGui::SeparatorText("Children");

		for (auto& pChild : m_pChildren) {
			pChild->ShowObjInfo();
		}

		ImGui::TreePop();
	}
}

std::shared_ptr<GameObject> GameObject::LoadFromImporter(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
	std::shared_ptr<OBJECT_IMPORT_INFO> pInfo, std::shared_ptr<GameObject> m_pParent)
{
	std::shared_ptr<GameObject> pObj = std::make_shared<GameObject>();

	pObj->m_pMeshMaterialPairs.reserve(pInfo->MeshMaterialInfoPairs.size());
	// TODO : Load Mesh & Material
	for (auto& [meshInfo, materialInfo] : pInfo->MeshMaterialInfoPairs) {
		pObj->m_pMeshMaterialPairs.emplace_back(Mesh::LoadFromInfo(pd3dDevice, pd3dCommandList, meshInfo), Material::LoadFromInfo(pd3dDevice, pd3dCommandList, materialInfo, pObj));
	}

	pObj->m_strName = pInfo->strNodeName;
	pObj->m_xmf4x4Transform = pInfo->xmf4x4Bone;
	pObj->m_pParent = m_pParent;

	// TODO : BONE
	//auto it = std::ranges::find(pInfo->boneInfos, pObj->m_strName, &BONE_IMPORT_INFO::strName);
	auto it = OBJECT_IMPORT_INFO::boneMap.find(pObj->m_strName);
	if (it != OBJECT_IMPORT_INFO::boneMap.end()) {
		pObj->m_pBone = Bone::LoadFromInfo(it->second);
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

	// Transform CBuffer data Heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	{
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NodeMask = 0;
	}
	pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(pObj->m_pd3dTransformDescriptorHeap.GetAddressOf()));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	{
		cbvDesc.BufferLocation = pObj->m_pCBTransform->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = ::AlignConstantBuffersize(sizeof(CB_TRANSFORM_DATA));
	}
	pd3dDevice->CreateConstantBufferView(&cbvDesc, pObj->m_pd3dTransformDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Descriptor Heap Per Obj -> SHADER_VISIBLE
	{
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NumDescriptors = 1 + (Material::MATERIAL_DESCRIPTOR_COUNT_PER_DRAW * pInfo->MeshMaterialInfoPairs.size()) + Animation::ANIMATION_DESCRIPTOR_COUNT_PER_DRAW;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NodeMask = 0;
	}
	pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(pObj->m_pd3dDescriptorHeap.GetAddressOf()));

	for (int i = 0; i < pInfo->pChildren.size(); ++i) {
		pObj->m_pChildren.push_back(LoadFromImporter(pd3dDevice, pd3dCommandList, pInfo->pChildren[i], pObj));
	}

	// TODO : ANIMATION (only when Root)
	// Bone 정보들이 생성되어야 하기 때문에 재귀가 끝나고 Root 에 도달하였을때 호출
	if (!pInfo->pParent) {
		if (pInfo->animationInfos.size() != 0) {
			pObj->m_pAnimation = Animation::LoadFromInfo(pd3dDevice, pd3dCommandList, pInfo->animationInfos, pObj);
		}
	}



	return pObj;
}

void GameObject::UpdateShaderVariables(ComPtr<ID3D12Device14> pDevice, ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList)
{
	CB_TRANSFORM_DATA bindData;

	XMStoreFloat4x4(&bindData.xmf4x4Local, XMMatrixTranspose(XMLoadFloat4x4(&ComputeLocalMatrix())));
	XMStoreFloat4x4(&bindData.xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	::memcpy(m_pTransformMappedPtr, &bindData, sizeof(CB_TRANSFORM_DATA));
	
	pDevice->CopyDescriptorsSimple(
		1,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, D3DCore::g_nCBVSRVDescriptorIncrementSize),
		m_pd3dTransformDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
	pd3dRenderCommandList->SetGraphicsRootDescriptorTable(1, m_pd3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// TODO : Update Animation data
	if (m_pParent.expired() && m_pAnimation) {
		m_pAnimation->UpdateShaderVariables(pd3dRenderCommandList);
	}

}

void GameObject::Render(ComPtr<ID3D12Device14> pDevice, ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList)
{
	pd3dRenderCommandList->SetDescriptorHeaps(1, m_pd3dDescriptorHeap.GetAddressOf());

	UpdateShaderVariables(pDevice, pd3dRenderCommandList);

	CD3DX12_CPU_DESCRIPTOR_HANDLE MaterialCPUDescriptorHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE MaterialGPUDescriptorHandle(m_pd3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	for (auto&& [idx, pPairs] : m_pMeshMaterialPairs | std::views::enumerate) {
		pPairs.second->UpdateShaderVariables(pDevice);

		pDevice->CopyDescriptorsSimple(
			Material::MATERIAL_DESCRIPTOR_COUNT_PER_DRAW,
			MaterialCPUDescriptorHandle,
			pPairs.second->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		pd3dRenderCommandList->SetGraphicsRootDescriptorTable(2, MaterialGPUDescriptorHandle);
		pd3dRenderCommandList->SetGraphicsRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(MaterialGPUDescriptorHandle, 1, D3DCore::g_nCBVSRVDescriptorIncrementSize));
		MaterialCPUDescriptorHandle.Offset(Material::MATERIAL_DESCRIPTOR_COUNT_PER_DRAW, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		MaterialGPUDescriptorHandle.Offset(Material::MATERIAL_DESCRIPTOR_COUNT_PER_DRAW, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		pPairs.first->Render(pd3dRenderCommandList);
	}


	for (auto& pChild : m_pChildren) {
		pChild->Render(pDevice, pd3dRenderCommandList);
	}

}

// Computes Node Local Space to Model Local Space
XMFLOAT4X4 GameObject::ComputeLocalMatrix()
{
	//  ????
	XMMATRIX xmmtxResult = XMLoadFloat4x4(&m_xmf4x4Transform);

	//assert(XMMatrixIsIdentity(xmmtxResult));

	std::shared_ptr<GameObject> pParent = m_pParent.lock();
	while (pParent != nullptr) {
		xmmtxResult = XMMatrixMultiply(XMLoadFloat4x4(&pParent->m_xmf4x4Transform), xmmtxResult);
		pParent = pParent->m_pParent.lock();
	}

	XMFLOAT4X4 xmf4x4Ret;
	XMStoreFloat4x4(&xmf4x4Ret, xmmtxResult);

	//assert(XMMatrixIsIdentity(xmmtxResult));

	return xmf4x4Ret;
}

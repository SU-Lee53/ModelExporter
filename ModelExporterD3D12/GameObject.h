#pragma once
#include "Mesh.h"
#include "Material.h"
#include "Bone.h"
#include "Animation.h"

// ============================================================================================
// �� �ִϸ��̼� �����ϰ�ʹ�
// 1. Bone �� Keyframe ��ĵ��� ���� �迭�� GPU�� ���ε� -> �Ƹ��� StructuredBuffer
//	-> �� Node ���� �ش��ϴ� Ű������ �迭�� Index�� ������ �ʿ䰡 ����. �̰͵� GPU ���ε� �ʿ�
// 2. Bone �� ���� ���� ��ȯ ��ĵ鵵 �迭�� GPU ���ε尡 �ʿ� -> �̰͵� StructuredBuffer
//	-> ��Ű���� ���� ��ȯ�� ����
// 3. �ռ� Timer ������ �ʿ�
//
// ���� �������� 1,2 �� ��� �����ϳİ� �߿��ҵ�
// ��? <- �ͽ����Ͱ� �ƴ� �����ӿ�ũ�� ������Ʈ ������ ���� ���� �����
//
// ============================================================================================



struct OBJECT_IMPORT_INFO {
	std::string strNodeName;

	std::vector<std::pair<MESH_IMPORT_INFO, MATERIAL_IMPORT_INFO>> MeshMaterialInfoPairs;
	std::vector<BONE_IMPORT_INFO> boneInfos;
	std::vector<std::variant<ANIMATION_CONTROLLER_IMPORT_INFO, ANIMATION_NODE_IMPORT_INFO>> animationInfos;

	XMFLOAT4X4 xmf4x4Bone;

	std::shared_ptr<OBJECT_IMPORT_INFO> m_pParent;
	std::vector<std::shared_ptr<OBJECT_IMPORT_INFO>> m_pChildren;

};

struct CB_TRANSFORM_DATA {
	XMFLOAT4X4 xmf4x4Local;
	XMFLOAT4X4 xmf4x4World;
};

class GameObject : public std::enable_shared_from_this<GameObject> {
public:
	GameObject();

	void UpdateShaderVariables(ComPtr<ID3D12Device14> pDevice);
	void Render(ComPtr<ID3D12Device14> pDevice, ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList);
	XMFLOAT4X4 ComputeLocalMatrix();
	void ShowObjInfo();

	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return m_pd3dDescriptorHeap; }

public:
	static std::shared_ptr<GameObject> 
		LoadFromImporter(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::shared_ptr<OBJECT_IMPORT_INFO> pInfo, std::shared_ptr<GameObject> m_pParent);

private:
	std::vector<std::pair<std::shared_ptr<Mesh>, std::shared_ptr<Material>>> m_pMeshMaterialPairs;
	std::shared_ptr<Bone> m_pBone;

	std::string m_strName;

	XMFLOAT4X4 m_xmf4x4Transform;
	XMFLOAT4X4 m_xmf4x4Local;
	XMFLOAT4X4 m_xmf4x4World;

	ComPtr<ID3D12Resource> m_pCBTransform = nullptr;
	UINT8* m_pTransformMappedPtr;
	ComPtr<ID3D12DescriptorHeap> m_pd3dTransformDescriptorHeap = nullptr;

private:
	std::shared_ptr<Animation> m_pAnimation = nullptr;

private:
	std::weak_ptr<GameObject> m_pParent;
	std::vector<std::shared_ptr<GameObject>> m_pChildren;

	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap = nullptr;

	constexpr static UINT DESCRIPTOR_COUNT_PER_DRAW = 9;
	// cBuffer 2��
	// texture 6��

};


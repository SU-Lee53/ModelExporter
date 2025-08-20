#pragma once
#include "Mesh.h"
#include "Material.h"
#include "Bone.h"
#include "Animation.h"

// ============================================================================================
// 아 애니매이션 구현하고싶다
// 1. Bone 별 Keyframe 행렬들을 구해 배열로 GPU에 업로드 -> 아마도 StructuredBuffer
//	-> 각 Node 들은 해당하는 키프레임 배열의 Index를 보관할 필요가 있음. 이것도 GPU 업로드 필요
// 2. Bone 별 최종 계층 변환 행렬들도 배열로 GPU 업로드가 필요 -> 이것도 StructuredBuffer
//	-> 스키닝을 위한 변환을 위해
// 3. 앞서 Timer 구현이 필요
//
// 현재 구조에서 1,2 를 어떻게 구현하냐가 중요할듯
// 왜? <- 익스포터가 아닌 프레임워크도 오브젝트 구조가 현재 거의 비슷함
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
	// cBuffer 2개
	// texture 6개

};


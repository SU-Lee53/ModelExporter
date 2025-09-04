#pragma once
#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"

class AssimpImporter
{
public:
	AssimpImporter(ComPtr<ID3D12Device14> pDevice);
	~AssimpImporter();

	void LoadFBXFilesFromPath(std::string_view svPath);
	bool LoadModel(std::string_view svPath);

	void Run();
	void RenderLoadedObject(ComPtr<ID3D12Device14> pDevice, ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList);

	std::shared_ptr<Shader> GetShader() { return m_pShaders[m_eShaderType]; }

private:
	void ShowSceneAttribute();
	void ShowNodeAll();

private:
	void PrintTabs();
	void PrintTabs(int nTabs);
	unsigned int m_nTabs = 0;

private:
	void ShowNode(const aiNode& node);
	void PrintMesh(const aiMesh& mesh);
	void PrintBone(const aiBone& bone);
	void PrintMaterial(const aiMaterial& material);
	void PrintAnimation(const aiAnimation& animation);
	void PrintAnimationNode(const aiNodeAnim& node);
	void PrintMeshAnimation(const aiMeshAnim& mesh);
	void PrintMeshMorphAnimation(const aiMeshMorphAnim& meshMorphAnim);

private:
	void PrintMatrix(const aiMatrix4x4& aimtx);
	std::string FormatMetaData(const aiMetadata& metaData, size_t idx);
	std::string QuaryAndFormatMaterialData(const aiMaterial& material, const aiMaterialProperty& matProperty);

private:
	void CreateCommandList();
	void CreateFence();
	void WaitForGPUComplete();
	void ExcuteCommandList();
	void ResetCommandList();

private:
	void CollectSkeletonFromScene(const aiScene* scene);
	void FillGlobalBoneMapToObjectStatic(); // OBJECT_IMPORT_INFO::boneMap 채우기

	std::shared_ptr<OBJECT_IMPORT_INFO> BuildObjectHierarchy(const aiNode* node, const aiScene* scene, std::shared_ptr<OBJECT_IMPORT_INFO> parent);
	MESH_IMPORT_INFO LoadMeshData(const aiMesh& mesh); // 정점/인덱스 + 스키닝 데이터 생성

	MATERIAL_IMPORT_INFO LoadMaterialData(const aiMaterial& node);
	BONE_IMPORT_INFO LoadBoneData(const aiBone& bone);
	std::vector<ANIMATION_IMPORT_INFO> LoadAnimationData(const aiScene* scene, float fTargetFPS);

	XMFLOAT3 InterpolateVectorKeyframe(float fTime, aiVectorKey* keys, UINT nKeys);
	XMFLOAT4 InterpolateQuaternionKeyframe(float fTime, aiQuatKey* keys, UINT nKeys);

	int FindNodeIndexByName(const aiNode* root, std::string_view svBoneName);

	std::string ExportTexture(const aiTexture& texture);
	HRESULT ExportDDSFile(std::wstring_view wsvSavePath, const aiTexture& texture);
	HRESULT ExportWICFile(std::wstring_view wsvSavePath, const aiTexture& texture);
	void GetContainerFormat(std::wstring_view wsvSaveName, GUID& formatGUID);

private:
	bool m_bLoaded = false;

private:
	std::string m_strPath;

	std::shared_ptr<Assimp::Importer> m_pImporter = nullptr;
	const aiScene* m_rpScene = nullptr;
	aiNode* m_rpRootNode = nullptr;
	UINT m_nNodes = 0;

private:
	// Scene 전역 스켈레톤
	std::vector<BONE_IMPORT_INFO> m_bones;                // boneIndex -> BoneInfoInternal
	std::unordered_map<std::string, int> m_boneNameToIndex;// 이름 -> boneIndex

	// Scene 전역 정점 가중치(정점ID는 각 Mesh의 Local index)
	// Mesh별로 ImportMesh() 안에서 자기 정점ID로 접근합니다.
	// (Assimp은 mesh별로 bone.weights 의 vertexId가 mesh-local 인덱스입니다)
	struct VtxWeight { int bone; float w; };
	// mesh 포인터 주소값을 key로 사용
	std::unordered_map<const aiMesh*, std::unordered_map<unsigned, std::vector<VtxWeight>>> m_meshVertexWeights;

private:
	std::string m_strCurrentPath;
	std::string m_strCurrentModelFilename;
	std::vector<std::string> m_strFBXFilesFromPath;
	UINT m_ItemSelected = 0;
	UINT m_ItemHighlighted = 0;

	std::string m_strError;
	std::string m_strExportName;

private:
	std::shared_ptr<GameObject> m_pLoadedObject = nullptr;
	std::shared_ptr<OBJECT_IMPORT_INFO> m_pRootInfo = nullptr;

private:
	ComPtr<ID3D12Device14>				m_pd3dDevice = nullptr;		// Reference to D3DCore::m_pd3dDevice
	ComPtr<ID3D12GraphicsCommandList>	m_pd3dCommandList = nullptr;
	ComPtr<ID3D12CommandAllocator>		m_pd3dCommandAllocator = nullptr;
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue = nullptr;

	ComPtr<ID3D12Fence>		m_pd3dFence = nullptr;
	HANDLE					m_hFenceEvent = nullptr;
	UINT64					m_nFenceValue = 0;

private:
	std::array<std::shared_ptr<Shader>, SHADER_TYPE_COUNT> m_pShaders = {};
	SHADER_TYPE m_eShaderType = SHADER_TYPE_DIFFUSED;

	std::shared_ptr<Camera> m_pCamera = nullptr;

public:
	std::shared_ptr<Camera> GetCamera() { return m_pCamera; }


};
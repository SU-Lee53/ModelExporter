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
	std::shared_ptr<OBJECT_IMPORT_INFO>
		LoadObject(const aiNode& node, std::shared_ptr<OBJECT_IMPORT_INFO> m_pParent);

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
	MESH_IMPORT_INFO LoadMeshData(const aiMesh& node);
	MATERIAL_IMPORT_INFO LoadMaterialData(const aiMaterial& node);
	BONE_IMPORT_INFO LoadBoneData(const aiBone& bone);
	ANIMATION_IMPORT_INFO LoadAnimationData(const aiAnimation& animation);

	XMFLOAT3 InterpolateVectorKeyframe(float fTime, aiVectorKey* keys, UINT nKeys);
	XMFLOAT4 InterpolateQuaternionKeyframe(float fTime, aiQuatKey* keys, UINT nKeys);

	int FindNodeIndexByName(std::string_view svBoneName);
	void BuildNodeList(aiNode* node, int counter);
	void BuildSkinData(const aiMesh& mesh);
	std::pair<std::array<int,4>, std::array<float,4>> LoadSkinData(size_t nVertexID);

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

	std::vector<BONE_IMPORT_INFO> m_boneInfos;
	std::map<std::string, int> m_NodeNameIndexMap; 
	std::map<std::string, int> m_BoneNameIndexMap; 
	std::map<UINT, std::vector<std::pair<UINT, float>>> m_VertexWeights;

private:
	std::string m_strCurrentPath;
	std::vector<std::string> m_strFBXFilesFromPath;
	UINT m_ItemSelected = 0;
	UINT m_ItemHighlighted = 0;

	std::string m_strError;

private:
	std::shared_ptr<GameObject> m_pLoadedObject = nullptr;

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

	std::shared_ptr<Camera> m_upCamera = nullptr;
};
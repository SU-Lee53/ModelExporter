#include "stdafx.h"
#include "AssimpImporter.h"
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

AssimpImporter::AssimpImporter(ComPtr<ID3D12Device14> pDevice)
	: m_pd3dDevice{ pDevice }
{
	CreateCommandList();
	CreateFence();

	m_pImporter = make_shared<Assimp::Importer>();

	m_pShaders[SHADER_TYPE_DIFFUSED] = std::make_shared<DiffusedShader>();
	m_pShaders[SHADER_TYPE_ALBEDO] = std::make_shared<AlbedoShader>();

	m_pShaders[SHADER_TYPE_DIFFUSED]->Create(m_pd3dDevice);
	m_pShaders[SHADER_TYPE_ALBEDO]->Create(m_pd3dDevice);

	m_upCamera = make_unique<Camera>(pDevice);
}

AssimpImporter::~AssimpImporter()
{
}

void AssimpImporter::LoadFBXFilesFromPath(std::string_view svPath)
{
	m_strFBXFilesFromPath.clear();

	m_strCurrentPath = std::string{ svPath };
	fs::path currentPath{ m_strCurrentPath };
	if (!fs::exists(currentPath)) {
		m_strError = "Path doesn't exist!";
		return;
	}

	m_strError.clear();

	std::string strExtentionList;
	m_pImporter->GetExtensionList(strExtentionList);
	// "*.AAA;*.BBB;*.CCC;*.DDD; .... " <- 이런식으로 저장되어 나옴

	std::erase(strExtentionList, '*');
	std::replace(strExtentionList.begin(), strExtentionList.end(), ';', ' ');

	std::istringstream ssExtentions{ strExtentionList };
	std::vector<std::string> strExtentionLists;
	std::copy(std::istream_iterator<std::string>{ssExtentions}, {}, std::back_inserter(strExtentionLists));

	for (const auto& entry : fs::directory_iterator(currentPath)) {
		for (std::string_view svExtention : strExtentionLists) {
			std::string strUpper{ svExtention };
			std::transform(strUpper.begin(), strUpper.end(), strUpper.begin(), [](char c) { return ::toupper(c); });

			if (entry.path().extension() == svExtention || entry.path().extension() == strUpper) {
				m_strFBXFilesFromPath.push_back(entry.path().filename().string());
			}
		}
	}

}

unsigned int CountNodes(const aiNode* node) {
	if (!node)
		return 0;

	unsigned int count = 1; 
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		count += CountNodes(node->mChildren[i]);
	}
	return count;
}

bool AssimpImporter::LoadModel(std::string_view svPath)
{
	fs::path modelPath = svPath;

	if (!fs::exists(modelPath)) {
		SHOW_ERROR("Invalid Path");
		return false;
	}

	m_rpScene = m_pImporter->ReadFile(
		modelPath.string(),
		aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |	// Convert to D3D
		//aiProcess_PreTransformVertices | 
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_PopulateArmatureData
	);

	if (!m_rpScene) {
		m_strError = m_pImporter->GetErrorString();
		SHOW_ERROR("File read failed");
		return false;
	}
	else {
		m_strError.clear();
	}

	m_bLoaded = true;
	m_strPath = svPath;
	m_rpRootNode = m_rpScene->mRootNode;

	m_nNodes = CountNodes(m_rpRootNode);

	m_Weights.clear();

	for (int i = 0; i < m_rpScene->mNumMeshes; ++i) {
		for (int j = 0; j < m_rpScene->mMeshes[i]->mNumBones; ++j) {
			for (int k = 0; k < m_rpScene->mMeshes[i]->mBones[j]->mNumWeights; ++k) {
				aiVertexWeight Weight = m_rpScene->mMeshes[i]->mBones[j]->mWeights[k];
				m_Weights[Weight.mVertexId].emplace_back(Weight.mVertexId, Weight.mWeight);
			}
		}
	}

	// Init Model
	auto p = LoadObject(*m_rpRootNode, nullptr);
	ResetCommandList();
	m_pLoadedObject = GameObject::LoadFromImporter(m_pd3dDevice, m_pd3dCommandList, p, nullptr);
	ExcuteCommandList();


	return true;
}

void AssimpImporter::Run()
{
	ImGui::Begin("AssimpImporter");

	static std::string s;
	ImGui::InputText("Model Path", &s);
	ImGui::Text("%s - Length : %d", s.c_str(), s.length());
	if (ImGui::Button("Load")) {
		LoadFBXFilesFromPath(s);
	}

	ImGui::Text("%s", m_bLoaded ? "Model Loaded" : m_strError.c_str());
	if (m_bLoaded) {
		ImGui::Text("Loaded Model Path : %s", m_strPath.c_str());
	}

	if (ImGui::Button("DIFFUSED")) {
		m_eShaderType = SHADER_TYPE_DIFFUSED;
	}

	if (ImGui::Button("ALBEDO")) {
		m_eShaderType = SHADER_TYPE_ALBEDO;
	}


	if (ImGui::BeginListBox(("LoadTargets"))) {
		for (int i = 0; i < m_strFBXFilesFromPath.size(); ++i) {
			const bool bSelected = (m_ItemSelected == i);
			if (ImGui::Selectable(m_strFBXFilesFromPath[i].c_str(), bSelected)) {
				m_ItemSelected = i;
			}

			if (m_ItemHighlighted && ImGui::IsItemHovered()) {
				m_ItemHighlighted = i;
			}

			if (bSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndListBox();
	}

	if (ImGui::Button("Load Model")) {
		std::string strModelPath = std::format("{}/{}", m_strCurrentPath, m_strFBXFilesFromPath[m_ItemSelected]);
		m_bLoaded = LoadModel(strModelPath);
	}

	ImGui::NewLine();

	if (ImGui::BeginTabBar("")){
		if (ImGui::BeginTabItem("Scene Info")) {
			if (m_rpScene) {
				ShowSceneAttribute();
			}
			else {
				ImGui::Text("Scene Not Loaded");
			}
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Node Info")) {
			if (m_rpRootNode) {
				ShowNodeAll();
			}
			else {
				ImGui::Text("Node Not Loaded");
			}
			ImGui::EndTabItem();
		}
		
		if (m_pLoadedObject) {
			if (ImGui::BeginTabItem("Loaded Model Hierarchy")) {
				if (m_pLoadedObject) {
					m_pLoadedObject->ShowObjInfo();
				}
				else {
					ImGui::Text("Model Not Loaded");
				}
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}
	
	ImGui::End();

	m_upCamera->Update();
}

void AssimpImporter::RenderLoadedObject(ComPtr<ID3D12Device14> pDevice, ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList)
{
	if (m_pLoadedObject) {
		m_pShaders[m_eShaderType]->Bind(pd3dRenderCommandList);

		m_upCamera->BindViewportAndScissorRects(pd3dRenderCommandList);
		m_upCamera->UpdateShaderVariables(pd3dRenderCommandList, 0);

		m_pLoadedObject->Render(pDevice, pd3dRenderCommandList);
	}
}

void AssimpImporter::ShowSceneAttribute()
{
	ImGui::Text("Scene Name : %s", m_rpScene->mName.C_Str());
	if (ImGui::TreeNode("Anim", "Has Animations ? : % s", m_rpScene->HasAnimations() ? "YES" : "NO")) {
		if (m_rpScene->HasAnimations()) {
			for (int i = 0; i < m_rpScene->mNumAnimations; ++i) {
				aiAnimation* anim = m_rpScene->mAnimations[i];
				std::string strTreeKey = std::format("{}", anim->mName.C_Str());
				if (ImGui::TreeNode(strTreeKey.c_str())) {
					PrintAnimation(*anim);
					ImGui::TreePop();
				}
				ImGui::Separator();
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Cam", "Has Cameras? : %s", m_rpScene->HasCameras() ? "YES" : "NO")) {
		if (m_rpScene->HasCameras()) {
			for (int i = 0; i < m_rpScene->mNumCameras; ++i) {
				ImGui::Text("Camera Name : %s", m_rpScene->mCameras[i]->mName.C_Str());
				ImGui::Separator();
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Light", "Has Lights? : %s", m_rpScene->HasLights() ? "YES" : "NO")) {
		if (m_rpScene->HasLights()) {
			for (int i = 0; i < m_rpScene->mNumLights; ++i) {
				ImGui::Text("Light Name : %s", m_rpScene->mLights[i]->mName.C_Str());
				ImGui::Separator();
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Mat", "Has Materials? : %s", m_rpScene->HasMaterials() ? "YES" : "NO")) {
		if (m_rpScene->HasMaterials()) {
			for (int i = 0; i < m_rpScene->mNumMaterials; ++i) {
				std::string strTreeKey1 = std::format("{} #{}", "Material", i);
				if (ImGui::TreeNode(strTreeKey1.c_str())) {
					aiMaterial* pMaterial = m_rpScene->mMaterials[i];
					PrintMaterial(*pMaterial);

					ImGui::TreePop();
				}
				ImGui::Separator();
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Mesh", "Has Meshes? : %s", m_rpScene->HasMeshes() ? "YES" : "NO")) {

		if (m_rpScene->HasMeshes()) {
			for (int i = 0; i < m_rpScene->mNumMeshes; ++i) {
				std::string strTreeKey1 = std::format("{} #{}", "Mesh", i);
				if (ImGui::TreeNode(strTreeKey1.c_str())) {
					PrintMesh(*m_rpScene->mMeshes[i]);
					ImGui::TreePop();
				}
				ImGui::Separator();
			}
		}

		ImGui::TreePop();
	}

	ImGui::Text("Has Skeletons? : %s", m_rpScene->hasSkeletons() ? "YES" : "NO");

	if (ImGui::TreeNode("Texture", "Has Textures? : %s", m_rpScene->HasTextures() ? "YES" : "NO")) {
		if (m_rpScene->HasTextures()) {
			ImGui::Text("NumTextures : %u", m_rpScene->mNumTextures);
			for (int i = 0; i < m_rpScene->mNumTextures; ++i) {
				aiTexture* pTexture = m_rpScene->mTextures[i];
				ImGui::Text("Texture Name : %s", pTexture->mFilename.C_Str());
				ImGui::Text("Texture Width : %u", pTexture->mWidth);
				ImGui::Text("Texture Height : %u", pTexture->mHeight);
			}

		}
		ImGui::TreePop();
	}

	ImGui::Text("%s", std::format("Has Skeletons : {}", m_rpScene->hasSkeletons()).c_str());

}

std::string AssimpImporter::QuaryAndFormatMaterialData(const aiMaterial& material, const aiMaterialProperty& matProperty)
{
	std::string matData;
	switch (matProperty.mType) {
	case aiPTI_Float :
	{
		ai_real f;
		material.Get(matProperty.mKey.C_Str(), matProperty.mSemantic, matProperty.mIndex, f);
		matData = std::format("{}", f);
		break;
	}
	case aiPTI_Double :
	{
		ai_real f;
		material.Get(matProperty.mKey.C_Str(), matProperty.mSemantic, matProperty.mIndex, f);
		matData = std::format("{}", f);
		break;
	}
	case aiPTI_String :
	{
		aiString str;
		material.Get(matProperty.mKey.C_Str(), matProperty.mSemantic, matProperty.mIndex, str);
		matData = std::format("{}", str.C_Str());
		break;
	}
	case aiPTI_Integer :
	{
		int n;
		material.Get(matProperty.mKey.C_Str(), matProperty.mSemantic, matProperty.mIndex, n);
		matData = std::format("{}", n);
		break;
	}
	case aiPTI_Buffer :
	{
		std::unreachable();
		break;
	}
	default:
		std::unreachable();
		break;
	}

	return matData;
}

void AssimpImporter::ShowNodeAll()
{
	ImGui::Text("Node Count : %u", m_nNodes);
	ShowNode(*m_rpRootNode);
}

void AssimpImporter::ShowNode(const aiNode& node)
{
	std::string strTreeKey1 = std::format("Node Name : {:<20} - NumChildren : {}", node.mName.C_Str(), node.mNumChildren);
	if (ImGui::TreeNode(strTreeKey1.c_str())) {
		ImGui::SeparatorText("Data");

		ImGui::Text("NumMeshes : %u", node.mNumMeshes);
		for (int i = 0; i < node.mNumMeshes; ++i) {
			aiMesh* mesh = m_rpScene->mMeshes[node.mMeshes[i]];
			std::string strTreeKey2 = std::format("Mesh #{:<5} : {:<20}", i, mesh->mName.C_Str());
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				PrintMesh(*mesh);
				ImGui::TreePop();
			}
		}
		ImGui::Separator();

		ImGui::Text("Transform : ");
		ImGui::Text("%s", ::FormatMatrix(node.mTransformation).c_str());

		ImGui::Separator();

		if (node.mMetaData) {
			aiMetadata* pMeta = node.mMetaData;
			std::string strTreeKey2 = std::format("Metadata NumProperties : {}", pMeta->mNumProperties);
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				for (int i = 0; i < pMeta->mNumProperties; ++i) {
					std::string strMetaProperty = std::format("MetaData #{:3} : {:>30} - {:<20}", i, pMeta->mKeys[i].C_Str(), FormatMetaData(*node.mMetaData, i));
					ImGui::Text("%s", strMetaProperty.c_str());
				}
				ImGui::TreePop();
			}
		}
		else {
			ImGui::Text("NO METADATA");
		}

		ImGui::NewLine();
		ImGui::SeparatorText("Children");
		for (int i = 0; i < node.mNumChildren; ++i) {
			ShowNode(*node.mChildren[i]);
		}
		
		ImGui::Separator();

		ImGui::TreePop();
	}

}

void AssimpImporter::PrintMatrix(const aiMatrix4x4& aimtx)
{
	XMFLOAT4X4 xmf4x4Matrix((float*)&aimtx.a1);

	PrintTabs(m_nTabs); std::println("\tR1 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._11, xmf4x4Matrix._12, xmf4x4Matrix._13, xmf4x4Matrix._14);
	PrintTabs(m_nTabs); std::println("\tR2 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._21, xmf4x4Matrix._22, xmf4x4Matrix._23, xmf4x4Matrix._24);
	PrintTabs(m_nTabs); std::println("\tR3 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._31, xmf4x4Matrix._32, xmf4x4Matrix._33, xmf4x4Matrix._34);
	PrintTabs(m_nTabs); std::println("\tR4 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._41, xmf4x4Matrix._42, xmf4x4Matrix._43, xmf4x4Matrix._44);
}

void AssimpImporter::PrintMesh(const aiMesh& mesh)
{
	ImGui::Text("Mesh Name : %s", mesh.mName.C_Str());
	ImGui::Text("NumAnimMeshes : %u", mesh.mNumAnimMeshes);

	if (ImGui::TreeNode("Bone", "HasBones : %s ", mesh.HasBones() ? "YES" : "NO")) {
		if (mesh.HasBones()) {
			ImGui::Text("Bone Count : %u", mesh.mNumBones);
			for (int j = 0; j < mesh.mNumBones; ++j) {
				aiBone* bone = mesh.mBones[j];
				if(ImGui::TreeNode(std::format("{}", bone->mName.C_Str()).c_str())) {
					PrintBone(*bone);

					ImGui::TreePop();
				}
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Pos", "HasPositions : %s ", mesh.HasPositions() ? "YES" : "NO")) {
		ImGui::Text("NumVertices : %u", mesh.mNumVertices);
		for (int j = 0; j < mesh.mNumVertices; ++j) {
			aiVector3D pos = mesh.mVertices[j];
			ImGui::Text("#%4d : %s", j, std::format("{{ {: 5.3f}, {: 5.3f}, {: 5.3f} }}", pos.x, pos.y, pos.z).c_str());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Face", "HasFaces : %s ", mesh.HasFaces() ? "YES" : "NO")) {
		ImGui::Text("NumFaces : %u", mesh.mNumFaces);
		for (int j = 0; j < mesh.mNumFaces; ++j) {
			aiFace pos = mesh.mFaces[j];
			//ImGui::Text("Primitive : %d", pos.mNumIndices);
			std::string strFace;
			switch (pos.mNumIndices) {
			case 1: strFace = std::format("{{ {:5} }}", pos.mIndices[0]); break;
			case 2: strFace = std::format("{{ {:5}, {:5} }}", pos.mIndices[0], pos.mIndices[1]); break;
			case 3: strFace = std::format("{{ {:5}, {:5}, {:5} }}", pos.mIndices[0], pos.mIndices[1], pos.mIndices[2]); break;
			case 4: strFace = std::format("{{ {:5}, {:5}, {:5}, {:5} }}", pos.mIndices[0], pos.mIndices[1], pos.mIndices[2], pos.mIndices[3]); break;
			default:
				std::unreachable();
			}

			ImGui::Text("#%4d : %s", j, strFace.c_str());
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Normal", "HasNormals : %s ", mesh.HasNormals() ? "YES" : "NO")) {
		ImGui::Text("NumVertices : %u", mesh.mNumVertices);
		for (int j = 0; j < mesh.mNumVertices; ++j) {
			aiVector3D normal = mesh.mNormals[j];
			ImGui::Text("#%4d : %s", j, std::format("{{ {: 5.3f}, {: 5.3f}, {: 5.3f} }}", normal.x, normal.y, normal.z).c_str());
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("TanBiTan", "HasTangentsAndBitangents : %s ", mesh.HasTangentsAndBitangents() ? "YES" : "NO")) {
		ImGui::Text("NumVertices : %u", mesh.mNumVertices);
		ImGui::Text("%s", std::format("		 {:<20}					   {:<20}", "Tangent", "Bitangent").c_str());
		for (int j = 0; j < mesh.mNumVertices; ++j) {
			aiVector3D tan = mesh.mTangents[j];
			aiVector3D biTan = mesh.mBitangents[j];
			ImGui::Text("#%5d : %s", j, std::format("{{ {: 5.3f}, {: 5.3f}, {: 5.3f} }}", tan.x, tan.y, tan.z).c_str());
			ImGui::SameLine();
			ImGui::Text("			%s", std::format("{{ {: 5.3f}, {: 5.3f}, {: 5.3f} }}", biTan.x, biTan.y, biTan.z).c_str());
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("UV", "HasUVChannels : %s ", mesh.GetNumUVChannels() ? "YES" : "NO")) {
		ImGui::Text("NumUVChannels : %u", mesh.GetNumUVChannels());
		ImGui::Text("NumUVComponents : [%u, %u, %u, %u, %u, %u, %u, %u]",
			mesh.mNumUVComponents[0], mesh.mNumUVComponents[1], mesh.mNumUVComponents[2], mesh.mNumUVComponents[3],
			mesh.mNumUVComponents[4], mesh.mNumUVComponents[5], mesh.mNumUVComponents[6], mesh.mNumUVComponents[7]);

		for (int j = 0; j < mesh.GetNumUVChannels(); ++j) {
			std::string strTreeKey1 = std::format("UVChannel #{}", j);
			if (ImGui::TreeNode(strTreeKey1.c_str())) {
				std::string strUVName = mesh.HasTextureCoordsName(j) ? mesh.GetTextureCoordsName(j)->C_Str() : "UNDEFINED"s;
				ImGui::Text("Texture Coord Name : %s", strUVName.c_str());
				aiVector3D* textureCoords = mesh.mTextureCoords[j];
				for (int k = 0; k < mesh.mNumVertices; ++k) {
					std::string strUV;
					switch (mesh.mNumUVComponents[j]) {
					case 1: strUV = std::format("{{ {:5} }}", mesh.mTextureCoords[j][k][0]); break;
					case 2: strUV = std::format("{{ {:5}, {:5} }}", mesh.mTextureCoords[j][k][0], mesh.mTextureCoords[j][k][1]); break;
					case 3: strUV = std::format("{{ {:5}, {:5}, {:5} }}",
						mesh.mTextureCoords[j][k][0], mesh.mTextureCoords[j][k][1], mesh.mTextureCoords[j][k][2]); break;
					case 4: strUV = std::format("{{ {:5}, {:5}, {:5}, {:5} }}",
						mesh.mTextureCoords[j][k][0], mesh.mTextureCoords[j][k][1], mesh.mTextureCoords[j][k][2], mesh.mTextureCoords[j][k][3]); break;
					default:
						std::unreachable();
					}
					ImGui::Text("#%4d : %s", k, strUV.c_str());
				}
				ImGui::TreePop();
			}

			ImGui::Separator();
		}


		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Color", "HasColorChannels : %s ", mesh.GetNumColorChannels() ? "YES" : "NO")) {
		ImGui::Text("NumColorChannels : %u", mesh.GetNumColorChannels());
		for (int j = 0; j < mesh.GetNumColorChannels(); ++j) {
			aiColor4D* colorChannel = mesh.mColors[j];
			std::string strTreeKey1 = std::format("ColorChannel #{}", j);
			for (int k = 0; k < mesh.mNumVertices; ++k) {
				ImGui::Text("#%4d : %s", j, std::format("{{ {: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} }}", colorChannel[k].r, colorChannel[k].g, colorChannel[k].b, colorChannel[k].a).c_str());
			}
		}
		ImGui::TreePop();
	}

	aiMaterial* pMaterial = m_rpScene->mMaterials[mesh.mMaterialIndex];
	if (ImGui::TreeNode("Material", "Material Index : %u", mesh.mMaterialIndex)) {
		PrintMaterial(*pMaterial);
		ImGui::TreePop();
	}

}

void AssimpImporter::PrintBone(const aiBone& bone)
{
	ImGui::Text("Bone Name : %s", bone.mName.C_Str());


	if (ImGui::TreeNode(std::format("{} - Node", bone.mName.C_Str()).c_str())) {
		if (bone.mNode) {
			ShowNode(*bone.mNode);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(std::format("NumWeights : {}", bone.mNumWeights).c_str())) {
		for (int i = 0; i < bone.mNumWeights; ++i) {
			ImGui::Text("Weight #%d : ", i);
			ImGui::SameLine();
			aiVertexWeight weight = bone.mWeights[i];
			ImGui::Text("VertexID : %u - Weight : %f", weight.mVertexId, weight.mWeight);
		}

		ImGui::TreePop();
	}

	ImGui::Text("OffsetMatrix : ");
	ImGui::Text("%s", ::FormatMatrix(bone.mOffsetMatrix).c_str());

}

void AssimpImporter::PrintMaterial(const aiMaterial& material)
{
	ImGui::Text("Material Name : %s", material.GetName().C_Str());

	std::string strTreeKey1 = std::format("{} - Material Datas", material.GetName().C_Str());
	if (ImGui::TreeNode(strTreeKey1.c_str())) {
		aiColor4D aicValue{};
		if (material.Get(AI_MATKEY_COLOR_DIFFUSE, aicValue) == AI_SUCCESS) {
			ImGui::Text("Diffuse(Albedo) Color : %s", std::format("[{:< 5.3f}, {:< 5.3f}, {:< 5.3f}, {:< 5.3f} ]", aicValue.r, aicValue.g, aicValue.b, aicValue.a).c_str());
		}

		if (material.Get(AI_MATKEY_COLOR_AMBIENT, aicValue) == AI_SUCCESS) {
			ImGui::Text("Ambient Color : %s", std::format("[{:< 5.3f}, {:< 5.3f}, {:< 5.3f}, {:< 5.3f} ]", aicValue.r, aicValue.g, aicValue.b, aicValue.a).c_str());
		}

		if (material.Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
			ImGui::Text("Specular Color : %s", std::format("[{:< 5.3f}, {:< 5.3f}, {:< 5.3f}, {:< 5.3f} ]", aicValue.r, aicValue.g, aicValue.b, aicValue.a).c_str());
		}

		if (material.Get(AI_MATKEY_COLOR_EMISSIVE, aicValue) == AI_SUCCESS) {
			ImGui::Text("Emissive Color : %s", std::format("[{:< 5.3f}, {:< 5.3f}, {:< 5.3f}, {:< 5.3f} ]", aicValue.r, aicValue.g, aicValue.b, aicValue.a).c_str());
		}

		if (material.Get(AI_MATKEY_COLOR_TRANSPARENT, aicValue) == AI_SUCCESS) {
			ImGui::Text("Transparent Color : %s", std::format("[{:< 5.3f}, {:< 5.3f}, {:< 5.3f}, {:< 5.3f} ]", aicValue.r, aicValue.g, aicValue.b, aicValue.a).c_str());
		}

		if (material.Get(AI_MATKEY_COLOR_REFLECTIVE, aicValue) == AI_SUCCESS) {
			ImGui::Text("Reflective Color : %s", std::format("[{:< 5.3f}, {:< 5.3f}, {:< 5.3f}, {:< 5.3f} ]", aicValue.r, aicValue.g, aicValue.b, aicValue.a).c_str());
		}

		float fValue{};
		if (material.Get(AI_MATKEY_SHININESS, fValue) == AI_SUCCESS) {
			ImGui::Text("Shininess(Glosiness) Factor : %f", fValue);
		}

		if (material.Get(AI_MATKEY_ROUGHNESS_FACTOR, fValue) == AI_SUCCESS) {
			ImGui::Text("Roughness Factor : %f", fValue);
			ImGui::Text("Smoothness Factor(1 - Roughness) : %f", 1 - fValue);
		}

		if (material.Get(AI_MATKEY_METALLIC_FACTOR, fValue) == AI_SUCCESS) {
			ImGui::Text("Metallic Factor : %f", fValue);
		}

		if (material.Get(AI_MATKEY_REFLECTIVITY, fValue) == AI_SUCCESS) {
			ImGui::Text("Glossy Reflection Factor : %f", fValue);
		}

		std::vector<aiTextureType> textureTypes = {
			aiTextureType_DIFFUSE,
			aiTextureType_SPECULAR,
			aiTextureType_METALNESS,
			aiTextureType_NORMALS,
		};

		aiString aistrTexturePath{};
		for (aiTextureType tType : textureTypes) {
			UINT nTextures = material.GetTextureCount(tType);
			std::string strTreeKey2 = std::format("{} Texture Count : {}", aiTextureTypeToString(tType), nTextures);
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				for (int i = 0; i < nTextures; ++i) {
					if (material.GetTexture(tType, i, &aistrTexturePath) == AI_SUCCESS) {
						ImGui::Text("%s", std::format("{} #{} - path : {}", aiTextureTypeToString(tType), i, aistrTexturePath.C_Str()).c_str());

						const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
						bool bisEmbedded = pEmbeddedTexture ? true : false;
						ImGui::Text("Embedded? : %s", std::format("{}", bisEmbedded).c_str());
					}
				}
				
				ImGui::TreePop();
			}
		}



		ImGui::TreePop();
	}

}

void AssimpImporter::PrintAnimation(const aiAnimation& animation)
{
	ImGui::Text("Anim Name : %s", animation.mName.C_Str());
	ImGui::Text("Anim Duration : %f", animation.mDuration);
	ImGui::Text("Anim TicksPerSecond : %f", animation.mTicksPerSecond);

	std::string strTreeKey1 = std::format("{} - Channels Count : {}", animation.mName.C_Str(), animation.mNumChannels);
	if (ImGui::TreeNode(strTreeKey1.c_str())) {
		for (int i = 0; i < animation.mNumChannels; ++i) {
			std::string strTreeKey2 = std::format("Channel #{:<3} - {}", i, animation.mChannels[i]->mNodeName.C_Str());
			if(ImGui::TreeNode(strTreeKey2.c_str())) {
				aiNodeAnim* node = animation.mChannels[i];
				PrintAnimationNode(*node);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	strTreeKey1 = std::format("{} - Mesh Channels Count : {}", animation.mName.C_Str(), animation.mNumMeshChannels);
	if (ImGui::TreeNode(strTreeKey1.c_str())) {
		for (int i = 0; i < animation.mNumMeshChannels; ++i) {
			std::string strTreeKey2 = std::format("Mesh Channel #{}", i);
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				aiMeshAnim* animMesh = animation.mMeshChannels[i];
				PrintMeshAnimation(*animMesh);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	strTreeKey1 = std::format("{} - Morph Mesh Channels Count : {}", animation.mName.C_Str(), animation.mNumMorphMeshChannels);
	if (ImGui::TreeNode(strTreeKey1.c_str())) {
		for (int i = 0; i < animation.mNumMorphMeshChannels; ++i) {
			std::string strTreeKey2 = std::format("Morph Mesh Channel #{}", i);
			if (ImGui::TreeNode(strTreeKey2.c_str())) {
				aiMeshMorphAnim* meshMorph = animation.mMorphMeshChannels[i];
				PrintMeshMorphAnimation(*meshMorph);

				ImGui::TreePop();
			}
		}
		animation.mMorphMeshChannels;

		ImGui::TreePop();
	}


}

void AssimpImporter::PrintAnimationNode(const aiNodeAnim& node)
{
	ImGui::Text("Name : %s", node.mNodeName.C_Str());

	if (ImGui::TreeNode(std::format("NumPositionKeys : {}", node.mNumPositionKeys).c_str())) {
		for (int i = 0; i < node.mNumPositionKeys; ++i) {
			if (ImGui::TreeNode(std::format("Position Key #{}", i).c_str())) {
				aiVectorKey posKey = node.mPositionKeys[i];
				ImGui::Text("Time : %lf", posKey.mTime);
				ImGui::Text("Value : %s", std::format("{{{: 5.3f}, {: 5.3f}, {: 5.3f} }}", posKey.mValue.x, posKey.mValue.y, posKey.mValue.z).c_str());

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(std::format("NumRotationKeys : {}", node.mNumRotationKeys).c_str())) {
		for (int i = 0; i < node.mNumRotationKeys; ++i) {
			if (ImGui::TreeNode(std::format("Position Key #{}", i).c_str())) {
				aiQuatKey quatKey = node.mRotationKeys[i];
				ImGui::Text("Time : %lf", quatKey.mTime);
				ImGui::Text("Value : %s", std::format("{{{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} }}", quatKey.mValue.x, quatKey.mValue.y, quatKey.mValue.z, quatKey.mValue.w).c_str());

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(std::format("NumScalingKeys : {}", node.mNumScalingKeys).c_str())) {
		for (int i = 0; i < node.mNumScalingKeys; ++i) {
			if (ImGui::TreeNode(std::format("Position Key #{}", i).c_str())) {
				aiVectorKey scaleKey = node.mScalingKeys[i];
				ImGui::Text("Time : %lf", scaleKey.mTime);
				ImGui::Text("Value : %s", std::format("{{{: 5.3f}, {: 5.3f}, {: 5.3f} }}", scaleKey.mValue.x, scaleKey.mValue.y, scaleKey.mValue.z).c_str());

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

}

void AssimpImporter::PrintMeshAnimation(const aiMeshAnim& mesh)
{
	ImGui::Text("Name : %s", mesh.mName.C_Str());

	for (int i = 0; i < mesh.mNumKeys; ++i) {
		aiMeshKey meshKey = mesh.mKeys[i];
		ImGui::Text("Time : %lf", meshKey.mTime);
		ImGui::Text("Value(index of aiMesh's AnimMesh) : %u", meshKey.mValue);
	}
}

void AssimpImporter::PrintMeshMorphAnimation(const aiMeshMorphAnim& meshMorphAnim)
{
	ImGui::Text("Name : %s", meshMorphAnim.mName.C_Str());
	for (int i = 0; i < meshMorphAnim.mNumKeys; ++i) {
		aiMeshMorphKey meshMorphKey = meshMorphAnim.mKeys[i];
		ImGui::Text("Time : %lf", meshMorphKey.mTime);
		for (int j = 0; j < meshMorphKey.mNumValuesAndWeights; ++j) {
			if (ImGui::TreeNode(std::format("Key #{}", j).c_str())) {
				ImGui::Text("Value : %u", meshMorphKey.mValues[j]);
				ImGui::Text("Value : %lf", meshMorphKey.mWeights[j]);

				ImGui::TreePop();
			}
		}
	}
}

std::string AssimpImporter::FormatMetaData(const aiMetadata& metaData, size_t idx)
{
	std::string strIndex{ "Metadata #{}", idx };

	switch (metaData.mValues[idx].mType)
	{
	case AI_BOOL:
	{
		bool data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(bool));
		return std::format("{:>10} - {}", "bool", data);
	}
	case AI_INT32:
	{
		int32_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(int32_t));
		return std::format("{:>10} - {}", "int32_t", data);
	}
	break;
	case AI_UINT64:
	{
		uint64_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(uint64_t));
		return std::format("{:>10} - {}", "uint64_t", data);
	}
	case AI_FLOAT:
	{
		float data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(float));
		return std::format("{:>10} - {}", "float", data);
	}
	case AI_DOUBLE:
	{
		double data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(double));
		return std::format("{:>10} - {}", "double", data);
	}
	case AI_AISTRING:
	{
		aiString data;
		metaData.Get<aiString>(static_cast<unsigned int>(idx), data);
		return std::format("{:>10} - {}", "string", data.C_Str());
	}
	case AI_AIVECTOR3D:
	{
		XMFLOAT3 data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(XMFLOAT3));
		return std::format("{:>10} - {{{: 5.3f}, {: 5.3f}, {: 5.3f} }}", "XMFLOAT3", data.x, data.y, data.z);
	}
	case AI_AIMETADATA:
	{
		aiMetadata v;
		metaData.Get<aiMetadata>(static_cast<unsigned int>(idx), v);
		return "aiMetaData..."s;
		break;
	}
	case AI_INT64:
	{
		int64_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(int64_t));
		return std::format("{:>10} - {}", "int64_t", data);
	}
	case AI_UINT32:
	{
		uint64_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(uint64_t));
		return std::format("{:>10} - {}", "uint64_t", data);
	}


	default:
		break;
	}
}

std::shared_ptr<OBJECT_IMPORT_INFO> AssimpImporter::LoadObject(const aiNode& node, std::shared_ptr<OBJECT_IMPORT_INFO> m_pParent)
{
	std::shared_ptr<OBJECT_IMPORT_INFO> pObjInfo = std::make_shared<OBJECT_IMPORT_INFO>();

	pObjInfo->strNodeName = node.mName.C_Str();

	XMMATRIX xmmtxTransform = XMLoadFloat4x4(&XMFLOAT4X4(&node.mTransformation.a1));
	xmmtxTransform = XMMatrixTranspose(xmmtxTransform);
	XMStoreFloat4x4(&pObjInfo->xmf4x4Bone, xmmtxTransform);
	pObjInfo->m_pParent = m_pParent;

	for (int i = 0; i < node.mNumMeshes; ++i) {
		aiMesh* pMesh = m_rpScene->mMeshes[node.mMeshes[i]];
		pObjInfo->MeshMaterialInfoPairs.emplace_back( LoadMeshData(*pMesh), LoadMaterialData(*m_rpScene->mMaterials[pMesh->mMaterialIndex]) );
		if (pMesh->HasBones()) {
			pObjInfo->boneInfos.reserve(pMesh->mNumBones);
			for (int j = 0; j < pMesh->mNumBones; ++j) {
				pObjInfo->boneInfos.push_back(LoadBoneData(*pMesh->mBones[j]));
			}
		}
	}

	pObjInfo->animationInfos.reserve(m_rpScene->mNumAnimations);
	for (int i = 0; i < m_rpScene->mNumAnimations; ++i) {
		if (!node.mParent) {
			// Load Controller
			pObjInfo->animationInfos.push_back(LoadAnimationController(*m_rpScene->mAnimations[i]));
		}
		else {
			// Load Node
			pObjInfo->animationInfos.push_back(LoadAnimationNode(*m_rpScene->mAnimations[i], node.mName.C_Str()));
		}
	}

	for (int i = 0; i < node.mNumChildren; ++i) {
		pObjInfo->m_pChildren.push_back(LoadObject(*node.mChildren[i], pObjInfo));
	}



	return pObjInfo;
}

MESH_IMPORT_INFO AssimpImporter::LoadMeshData(const aiMesh& mesh)
{
	MESH_IMPORT_INFO info;

	info.strMeshName = mesh.mName.C_Str();

	uint32_t nVertices = mesh.mNumVertices;
	info.xmf3Positions.reserve(nVertices);
	info.xmf3Normals.reserve(nVertices);
	info.xmf3Tangents.reserve(nVertices);
	info.xmf3BiTangents.reserve(nVertices);

	for (auto color : info.xmf4Colors) {
		color.reserve(nVertices);
	}

	for (auto texCoord : info.xmf2TexCoords) {
		texCoord.reserve(nVertices);
	}


	for (int i = 0; i < nVertices; ++i) {
		info.xmf3Positions.push_back(XMFLOAT3(mesh.mVertices[i].x, mesh.mVertices[i].y, mesh.mVertices[i].z));
		mesh.HasNormals() ? info.xmf3Normals.push_back(XMFLOAT3(mesh.mNormals[i].x, mesh.mNormals[i].y, mesh.mNormals[i].z)) : info.xmf3Normals.push_back(XMFLOAT3(0.f, 0.f, 0.f));
		if (mesh.HasTangentsAndBitangents()) {
			info.xmf3Tangents.push_back(XMFLOAT3(mesh.mTangents[i].x, mesh.mTangents[i].y, mesh.mTangents[i].z));
			info.xmf3BiTangents.push_back(XMFLOAT3(mesh.mBitangents[i].x, mesh.mBitangents[i].y, mesh.mBitangents[i].z));
		}
		else {
			info.xmf3Tangents.push_back(XMFLOAT3(0.f, 0.f, 0.f));
			info.xmf3BiTangents.push_back(XMFLOAT3(0.f, 0.f, 0.f));
		}

		std::array<int, 4> blendIndices{};
		std::array<float, 4> blendWeights{};

		for (int j = 0; j < m_Weights[i].size(); ++j) {
			if (j >= 4) break;
			blendIndices[j] = m_Weights[i][j].first;
			blendWeights[j] = m_Weights[i][j].second;
		}

		info.blendIndices.push_back(blendIndices);
		info.blendWeights.push_back(blendWeights);

		for (int j = 0; j < mesh.GetNumColorChannels(); ++j) {
			info.xmf4Colors[j].push_back(XMFLOAT4(mesh.mColors[j][i].r, mesh.mColors[j][i].g, mesh.mColors[j][i].b, mesh.mColors[j][i].a));
		}

		for (int j = 0; j < mesh.GetNumUVChannels(); ++j) {
			assert(mesh.mNumUVComponents[j] == 2);
			info.xmf2TexCoords[j].push_back(XMFLOAT2(mesh.mTextureCoords[j][i].x, mesh.mTextureCoords[j][i].y));
		}

	}

	info.uiIndices.reserve(mesh.mNumFaces);
	for (int i = 0; i < mesh.mNumFaces; ++i) {
		for (int j = 0; j < mesh.mFaces[i].mNumIndices; ++j) {
			info.uiIndices.push_back(mesh.mFaces[i].mIndices[j]);
		}
	}

	return info;
}

MATERIAL_IMPORT_INFO AssimpImporter::LoadMaterialData(const aiMaterial& material)
{
	MATERIAL_IMPORT_INFO info{};

	info.strMaterialName = material.GetName().C_Str();

	aiColor4D aicValue{};
	if (material.Get(AI_MATKEY_COLOR_DIFFUSE, aicValue) == AI_SUCCESS) {
		info.xmf4AlbedoColor = XMFLOAT4(&aicValue.r);
	}

	if (material.Get(AI_MATKEY_COLOR_AMBIENT, aicValue) == AI_SUCCESS) {
		info.xmf4AmbientColor = XMFLOAT4(&aicValue.r);
	}

	if (material.Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
		info.xmf4SpecularColor = XMFLOAT4(&aicValue.r);
	}

	if (material.Get(AI_MATKEY_COLOR_EMISSIVE, aicValue) == AI_SUCCESS) {
		info.xmf4EmissiveColor = XMFLOAT4(&aicValue.r);
	}

	float fValue{};
	if (material.Get(AI_MATKEY_SHININESS, fValue) == AI_SUCCESS) {
		info.fGlossiness = fValue;
	}

	if (material.Get(AI_MATKEY_ROUGHNESS_FACTOR, fValue) == AI_SUCCESS) {
		info.fSmoothness = 1 - fValue;
	}

	if (material.Get(AI_MATKEY_METALLIC_FACTOR, fValue) == AI_SUCCESS) {
		info.fMetallic = fValue;
	}

	if (material.Get(AI_MATKEY_REFLECTIVITY, fValue) == AI_SUCCESS) {
		info.fGlossyReflection = fValue;
	}

	// Texture?

	std::vector<aiTextureType> textureTypes = {
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_METALNESS,
		aiTextureType_NORMALS,
		aiTextureType_EMISSIVE,
	};

	aiString aistrTexturePath{};
	
	if (material.GetTexture(aiTextureType_DIFFUSE, 0, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strAlbedoMapName = ExportTexture(*pEmbeddedTexture);
		}
	}
	
	if (material.GetTexture(aiTextureType_SPECULAR, 0, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strSpecularMapName = ExportTexture(*pEmbeddedTexture);
		}
	}
	
	if (material.GetTexture(aiTextureType_METALNESS, 0, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strMetallicMapName = ExportTexture(*pEmbeddedTexture);
		}
	}
	
	if (material.GetTexture(aiTextureType_NORMALS, 0, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strNormalMapName = ExportTexture(*pEmbeddedTexture);
		}
	}
	
	if (material.GetTexture(aiTextureType_EMISSIVE, 0, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strEmissionMapName = ExportTexture(*pEmbeddedTexture);
		}
	}

	if (material.GetTexture(aiTextureType_DIFFUSE, 1, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strDetailAlbedoMapName = ExportTexture(*pEmbeddedTexture);
		}
	}

	if (material.GetTexture(aiTextureType_NORMALS, 1, &aistrTexturePath) == AI_SUCCESS) {
		const aiTexture* pEmbeddedTexture = m_rpScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
		if (pEmbeddedTexture) {
			info.strDetailNormalMapName = ExportTexture(*pEmbeddedTexture);
		}
	}

	return info;
}

BONE_IMPORT_INFO AssimpImporter::LoadBoneData(const aiBone& bone)
{
	BONE_IMPORT_INFO info;

	info.strName = bone.mName.C_Str();

	info.weights.reserve(bone.mNumWeights);
	for (int i = 0; i < bone.mNumWeights; ++i) {
		info.weights.emplace_back(bone.mWeights[i].mVertexId, bone.mWeights[i].mWeight);
	}

	info.xmf4x4Offset = XMFLOAT4X4(&bone.mOffsetMatrix.a1);

	return info;
}

ANIMATION_CONTROLLER_IMPORT_INFO AssimpImporter::LoadAnimationController(const aiAnimation& animation)
{
	ANIMATION_CONTROLLER_IMPORT_INFO controllerInfo;
	{
		controllerInfo.strName = animation.mName.C_Str();
		controllerInfo.fDuration = animation.mDuration;
		controllerInfo.fTicksPerSecond = animation.mTicksPerSecond;

		controllerInfo.channels.resize(animation.mNumChannels);
		for (int i = 0; i < animation.mNumChannels; ++i) {
			controllerInfo.channels[i].strName = animation.mChannels[i]->mNodeName.C_Str();

			// Position Keys
			controllerInfo.channels[i].keyframes.posKeys.reserve(animation.mChannels[i]->mNumPositionKeys);
			for (int j = 0; j < animation.mChannels[i]->mNumPositionKeys; ++j) {
				AnimKey posKey{};
				posKey.fTime = animation.mChannels[i]->mPositionKeys[j].mTime;
				posKey.xmf3Value = XMFLOAT3(&animation.mChannels[i]->mPositionKeys[j].mValue.x);

				controllerInfo.channels[i].keyframes.posKeys.push_back(posKey);
			}

			// Rotation Keys (Quaternion)
			controllerInfo.channels[i].keyframes.rotKeys.reserve(animation.mChannels[i]->mNumRotationKeys);
			for (int j = 0; j < animation.mChannels[i]->mNumRotationKeys; ++j) {
				AnimKey rotKey{};
				rotKey.fTime = animation.mChannels[i]->mRotationKeys[j].mTime;
				rotKey.xmf3Value = XMFLOAT3(&animation.mChannels[i]->mRotationKeys[j].mValue.x);

				controllerInfo.channels[i].keyframes.rotKeys.push_back(rotKey);
			}

			// Scailing Keys
			controllerInfo.channels[i].keyframes.scaleKeys.reserve(animation.mChannels[i]->mNumScalingKeys);
			for (int j = 0; j < animation.mChannels[i]->mNumScalingKeys; ++j) {
				AnimKey scaleKey{};
				scaleKey.fTime = animation.mChannels[i]->mScalingKeys[j].mTime;
				scaleKey.xmf3Value = XMFLOAT3(&animation.mChannels[i]->mScalingKeys[j].mValue.x);

				controllerInfo.channels[i].keyframes.scaleKeys.push_back(scaleKey);
			}
		}
	}

	return controllerInfo;

}

ANIMATION_NODE_IMPORT_INFO AssimpImporter::LoadAnimationNode(const aiAnimation& animation, std::string_view svNodeName)
{
	ANIMATION_NODE_IMPORT_INFO nodeInfo;

	for (int i = 0; i < animation.mNumChannels; ++i) {
		auto pAnimation = animation.mChannels[i];
		if (pAnimation->mNodeName.C_Str() == svNodeName) {
			nodeInfo.strName = pAnimation->mNodeName.C_Str();
			nodeInfo.nKeyframeIndex = i;
			return nodeInfo;
		}
	}

	// if not found
	nodeInfo.nKeyframeIndex = -1;
	nodeInfo.strName = "NOT FOUND";

	return nodeInfo;

}

std::string AssimpImporter::ExportTexture(const aiTexture& texture)
{
	fs::path curPath{ m_strPath };
	std::string strTextureExportPath = std::format("{}/{} Textures", m_strCurrentPath, curPath.stem().string());

	fs::path pathFromEmbeddedTexture{ texture.mFilename.C_Str() };
	fs::path exportDirectoryPath{ strTextureExportPath };
	fs::path savePath{ std::format("{}/{}", exportDirectoryPath.string(), pathFromEmbeddedTexture.filename().string()) };

	if (fs::exists(savePath)) {
		return savePath.string();
	}

	if (!fs::exists(exportDirectoryPath)) {
		try {
			fs::create_directories(exportDirectoryPath);
		}
		catch (const std::exception& e) {
			SHOW_ERROR(e.what());
		}
	}

	HRESULT hr;
	if (curPath.extension().string() == ".dds") {
		hr = ExportDDSFile(savePath.wstring(), texture);
	}
	else {
		hr = ExportWICFile(savePath.wstring(), texture);
	}

	if (FAILED(hr)) {
		__debugbreak();
	}


	return savePath.string();
}

HRESULT AssimpImporter::ExportDDSFile(std::wstring_view wsvSavePath, const aiTexture& texture)
{
	DirectX::ScratchImage scratchImg{};
	DirectX::TexMetadata  metaData{};

	// mHeight == 0 인 경우 압축된 데이터임
	if (texture.mHeight == 0) {
		HRESULT hr = ::LoadFromDDSMemory(
			reinterpret_cast<const uint8_t*>(texture.pcData),
			(size_t)texture.mWidth,
			DDS_FLAGS_NONE,
			&metaData,
			scratchImg
		);

		if (FAILED(hr)) {
			__debugbreak();
			return S_FALSE;
		}
	}
	else {
		DirectX::Image img{};
		img.width = texture.mWidth;
		img.height = texture.mHeight;
		img.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		img.rowPitch = size_t(texture.mWidth) * 4;
		img.slicePitch = img.rowPitch * texture.mHeight;
		img.pixels = const_cast<uint8_t*>(
			reinterpret_cast<const uint8_t*>(texture.pcData));

		scratchImg.Initialize2D(img.format, img.width, img.height, 1, 1);
		memcpy(scratchImg.GetPixels(), img.pixels, img.slicePitch);
		metaData = scratchImg.GetMetadata();
	}

	GUID containerFormat;
	GetContainerFormat(wsvSavePath, containerFormat);

	return ::SaveToDDSFile(
		scratchImg.GetImages(),
		scratchImg.GetImageCount(),
		scratchImg.GetMetadata(),
		DirectX::DDS_FLAGS_NONE,
		wsvSavePath.data()
	);
}

HRESULT AssimpImporter::ExportWICFile(std::wstring_view wsvSavePath, const aiTexture& texture)
{
	DirectX::ScratchImage scratchImg{};

	// mHeight == 0 인 경우 압축된 데이터임
	if (texture.mHeight == 0) {
		HRESULT hr = ::LoadFromWICMemory(
			reinterpret_cast<const uint8_t*>(texture.pcData),
			(size_t)texture.mWidth,
			::WIC_FLAGS_NONE,
			nullptr,
			scratchImg,
			nullptr
		);

		if (FAILED(hr)) {
			__debugbreak();
			return S_FALSE;
		}
	}
	else {
		DirectX::Image img = {};
		img.width = texture.mWidth;
		img.height = texture.mHeight;
		img.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		img.rowPitch = size_t(texture.mWidth * 4);
		img.slicePitch = img.rowPitch * texture.mHeight;
		img.pixels = reinterpret_cast<uint8_t*>(texture.pcData);

		scratchImg.Initialize2D(img.format, img.width, img.height, 1, 1);
		memcpy(scratchImg.GetPixels(), img.pixels, img.slicePitch);
	}

	GUID containerFormat;
	GetContainerFormat(wsvSavePath, containerFormat);

	return ::SaveToWICFile(
		scratchImg.GetImages(),
		scratchImg.GetImageCount(),
		DirectX::WIC_FLAGS_NONE,
		containerFormat,
		wsvSavePath.data()
	);

}

void AssimpImporter::GetContainerFormat(std::wstring_view wsvSaveName, GUID& formatGUID)
{
	fs::path savePath{ wsvSaveName };
	std::string format = savePath.extension().string();

	if (format == ".png") {
		formatGUID = GUID_ContainerFormatPng;
	}
	else if (format == ".jpg" || format == ".jpeg") {
		formatGUID = GUID_ContainerFormatJpeg;
	}
	else if (format == ".bmp") {
		formatGUID = GUID_ContainerFormatBmp;
	}
	else if (format == ".tif" || format == ".tiff") {
		formatGUID = GUID_ContainerFormatTiff;
	}
	else if (format == ".gif") {
		formatGUID = GUID_ContainerFormatGif;
	}
	else if (format == ".wmp") {
		formatGUID = GUID_ContainerFormatWmp;
	}
	else if (format == ".heif") {
		formatGUID = GUID_ContainerFormatHeif;
	}
}

void AssimpImporter::PrintTabs()
{
	std::string s;
	std::fill_n(back_inserter(s), m_nTabs, '\t');
	std::print("{}", s);
}

void AssimpImporter::PrintTabs(int nTabs)
{
	std::string s;
	std::fill_n(back_inserter(s), nTabs, '\t');
	std::print("{}", s);
}


void AssimpImporter::CreateCommandList()
{
	HRESULT hr{};

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}

	// Create Command Allocator
	hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pd3dCommandAllocator.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandAllocator");
	}

	// Create Command List
	hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator.Get(), NULL, IID_PPV_ARGS(m_pd3dCommandList.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandList");
	}

	// Close Command List(default is opened)
	hr = m_pd3dCommandList->Close();
}

void AssimpImporter::CreateFence()
{
	HRESULT hr{};

	hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create fence");
	}

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void AssimpImporter::WaitForGPUComplete()
{
	UINT64 nFenceValue = ++m_nFenceValue;
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void AssimpImporter::ExcuteCommandList()
{
	m_pd3dCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForGPUComplete();
}

void AssimpImporter::ResetCommandList()
{
	HRESULT hr;
	hr = m_pd3dCommandAllocator->Reset();
	hr = m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), NULL);
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandList");
		__debugbreak();
	}
}

#include "stdafx.h"
#include "AssimpImporter.h"

using namespace std;
namespace fs = std::filesystem;

AssimpImporter::AssimpImporter()
{
	m_pImporter = make_shared<Assimp::Importer>();
}

bool AssimpImporter::LoadModelFromPath(std::string_view svPath)
{
	fs::path modelPath = svPath;

	if (!fs::exists(modelPath)) {
		std::cout << "Invalid Path" << std::endl;
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
		aiProcess_CalcTangentSpace
	);

	if (!m_rpScene) {
		std::cout << "File read failed" << std::endl;
		return false;
	}

	m_rpRootNode = m_rpScene->mRootNode;

	return true;
}

void AssimpImporter::Run()
{
	ImGui::Begin("AssimpImporter");

	if (not m_bLoaded) {
		ImGui::InputText("Model Path", &m_strPath);
		ImGui::Text("%s - Length : %d", m_strPath.c_str(), m_strPath.length());
		if (ImGui::Button("Load")) {
			m_bLoaded = LoadModelFromPath(m_strPath);
		}
	}

	ImGui::Text("%s", m_bLoaded ? "Model Loaded" : "Model not yet Loaded");

	if(ImGui::BeginTabBar("")){
		if (ImGui::BeginTabItem("Scene Info")) {

			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Node Info")) {

			ImGui::EndTabItem();
		}



		ImGui::EndTabBar();
	}
	

	ImGui::End();
}

void AssimpImporter::ShowSceneAttribute()
{
	std::println("Scene Name : {}", m_rpScene->mName.C_Str());
	std::println("Has Animations? : {}", m_rpScene->HasAnimations());
	if (m_rpScene->HasAnimations()) {
		for (int i = 0; i < m_rpScene->mNumAnimations; ++i) {
			std::println("\tAnim Name : {}", m_rpScene->mAnimations[i]->mName.C_Str());
			std::println("\tAnim Duration : {}", m_rpScene->mAnimations[i]->mDuration);
			std::println("\tAnim TicksPerSecond : {}", m_rpScene->mAnimations[i]->mTicksPerSecond);
		}
	}

	std::println("Has Cameras? : {}", m_rpScene->HasCameras());
	if (m_rpScene->HasCameras()) {
		for (int i = 0; i < m_rpScene->mNumCameras; ++i) {
			std::println("\tCamera Name : {}", m_rpScene->mCameras[i]->mName.C_Str());
		}
	}

	std::println("Has Lights? : {}", m_rpScene->HasLights());
	if (m_rpScene->HasLights()) {
		for (int i = 0; i < m_rpScene->mNumLights; ++i) {
			std::println("\tLight Name : {}", m_rpScene->mLights[i]->mName.C_Str());
		}
	}

	std::println("Has Materials? : {}", m_rpScene->HasMaterials());
	if (m_rpScene->HasMaterials()) {
		for (int i = 0; i < m_rpScene->mNumMaterials; ++i) {
			aiMaterial* material = m_rpScene->mMaterials[i];
			std::println("\tMaterial Name : {}", material->GetName().C_Str());
			std::println("\tNumAllocated : {}", material->mNumAllocated);
			std::println("\tNumProperties : {}", material->mNumProperties);
			for (int j = 0; j < material->mNumProperties; ++j) {
				aiMaterialProperty* materialProperty = material->mProperties[j];
				std::println("\t\tProperty Key #{} : {}", j, materialProperty->mKey.C_Str());
			}
			std::println();
		}
	}

	std::println("Has Meshes? : {}", m_rpScene->HasMeshes());
	if (m_rpScene->HasMeshes()) {
		for (int i = 0; i < m_rpScene->mNumMeshes; ++i) {
			aiMesh* mesh = m_rpScene->mMeshes[i];

			std::println("\tMesh Name : {}", mesh->mName.C_Str());

			std::println("\t\tNumAnimMeshes : {}", mesh->mNumAnimMeshes);
			std::println("\t\tNumBones : {}", mesh->mNumBones);
			std::println("\t\tNumVertices : {}", mesh->mNumVertices);
			std::println("\t\tNumFaces : {}", mesh->mNumFaces);
			std::println("\t\tNumUVComponents : {}", mesh->mNumUVComponents);

			std::println("\t\tNumColorChannels : {}", mesh->GetNumColorChannels());
			std::println("\t\tNumUVChannels : {}", mesh->GetNumUVChannels());

			std::println("\t\tHasBones : {} ", mesh->HasBones());
			if (mesh->HasBones()) {
				std::println("\t\tNumBones : {}", mesh->mNumBones);
				for (int j = 0; j < mesh->mNumBones; ++j) {
					aiBone* pBone = mesh->mBones[j];
					std::println("\t\t\tBone Name : {}", pBone->mName.C_Str());
					std::println("\t\t\tNum Weights : {}", pBone->mNumWeights);
					//for (int k = 0; k < pBone->mNumWeights; ++k) {
					//	aiVertexWeight pWeight = pBone->mWeights[k];
					//	std::println("\t\t\t\tBone#{} : {{ VertexID : {:5}, Weight : {:5.2f} }}", k, pWeight.mVertexId, pWeight.mWeight);
					//}

				}

			}


			std::println("\t\tHasFaces : {} ", mesh->HasFaces());
			std::println("\t\tHasNormals : {} ", mesh->HasNormals());
			std::println("\t\tHasPositions : {} ", mesh->HasPositions());
			std::println("\t\tHasTangentsAndBitangents : {} ", mesh->HasTangentsAndBitangents());
			std::println("\t\tNumUVChannels : {} ", mesh->GetNumUVChannels());
			std::println("\t\tNumColorChannels : {} ", mesh->GetNumColorChannels());

			std::println();
		}
	}


	std::println("Has Skeletons? : {}", m_rpScene->hasSkeletons());
	std::println("Has Textures? : {}", m_rpScene->HasTextures());


}

void AssimpImporter::ShowNodeAll()
{
	ShowNode(*m_rpRootNode);
}

void AssimpImporter::ShowNode(const aiNode& node)
{
	auto nChildren = node.mNumChildren;
	auto nMeshes = node.mNumMeshes;
	
	std::cout << std::boolalpha;

	PrintTabs(m_nTabs); std::cout << "===============================================" << std::endl;
	PrintTabs(m_nTabs); std::cout << "Node Name : " << node.mName.C_Str() << std::endl;
	PrintTabs(m_nTabs); std::cout << "Children : " << nChildren << std::endl;
	PrintTabs(m_nTabs); std::cout << "Meshes : " << nMeshes << std::endl;
	for (int i = 0; i < nMeshes; ++i) {
		PrintTabs(m_nTabs); std::cout << "\tMesh #" << i<< std::endl;
		PrintMesh(*m_rpScene->mMeshes[node.mMeshes[i]]);
	}

	PrintTabs(m_nTabs); std::cout << "Transform : " << std::endl;
	PrintMatrix(node.mTransformation);

	PrintTabs(m_nTabs); std::cout << "MetaData? : " << (bool)node.mMetaData << std::endl;
	if (node.mMetaData) {
		aiMetadata* pMeta = node.mMetaData;
		for (int i = 0; i < pMeta->mNumProperties; ++i) {
			PrintTabs(m_nTabs); std::cout << '\t' << pMeta->mKeys[i].C_Str() << " : ";
			PrintMetaData(*node.mMetaData, i); std::cout << std::endl;
		}

	}

	PrintTabs(m_nTabs); std::cout << "===============================================" << std::endl;

	for (int i = 0; i < node.mNumChildren; ++i) {
		m_nTabs++;
		ShowNode(*node.mChildren[i]);
	}
	m_nTabs--;
}

void AssimpImporter::PrintMatrix(const aiMatrix4x4& aimtx)
{
	XMFLOAT4X4 xmf4x4Matrix((float*)&aimtx.a1);

	PrintTabs(m_nTabs); std::println("\tR1 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._11, xmf4x4Matrix._12, xmf4x4Matrix._13, xmf4x4Matrix._14);
	PrintTabs(m_nTabs); std::println("\tR2 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._21, xmf4x4Matrix._22, xmf4x4Matrix._23, xmf4x4Matrix._24);
	PrintTabs(m_nTabs); std::println("\tR3 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._31, xmf4x4Matrix._32, xmf4x4Matrix._33, xmf4x4Matrix._34);
	PrintTabs(m_nTabs); std::println("\tR4 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f}]", xmf4x4Matrix._41, xmf4x4Matrix._42, xmf4x4Matrix._43, xmf4x4Matrix._44);
}

void AssimpImporter::PrintMesh(const aiMesh& aimesh)
{
	PrintTabs(m_nTabs); std::println("\t{{ HasBones : {} }}",					aimesh.HasBones());
	PrintTabs(m_nTabs); std::println("\t{{ HasFaces : {} }}",					aimesh.HasFaces());
	PrintTabs(m_nTabs); std::println("\t{{ HasNormals : {} }}",				aimesh.HasNormals());
	PrintTabs(m_nTabs); std::println("\t{{ HasPositions : {} }}",				aimesh.HasPositions());
	PrintTabs(m_nTabs); std::println("\t{{ HasTangentsAndBitangents : {} }}",	aimesh.HasTangentsAndBitangents());

	PrintTabs(m_nTabs); std::println("\t{{ NumUVChannels : {} }}",		aimesh.GetNumUVChannels());
	PrintTabs(m_nTabs); std::println("\t{{ NumColorChannels : {} }}",	aimesh.GetNumColorChannels());


}

void AssimpImporter::PrintMetaData(const aiMetadata& metaData, size_t idx)
{
	switch (metaData.mValues[idx].mType)
	{
	case AI_BOOL:
	{
		bool data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(bool));
		std::cout << data;
		break;
	}
	case AI_INT32:
	{
		int32_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(int32_t));
		std::cout << data;
		break;
	}
	break;
	case AI_UINT64:
	{
		uint64_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(uint64_t));
		std::cout << data;
		break;
	}
	case AI_FLOAT:
	{
		float data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(float));
		std::cout << data;
		break;
	}
	case AI_DOUBLE:
	{
		double data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(double));
		std::cout << data;
		break;
	}
	case AI_AISTRING:
	{
		aiString v;
		metaData.Get<aiString>(static_cast<unsigned int>(idx), v);
		std::cout << v.C_Str();
		break;
	}
	case AI_AIVECTOR3D:
	{
		XMFLOAT3 data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(XMFLOAT3));
		std::print("XMFLOAT3 : {{ {}, {}, {} }}", data.x, data.y, data.z);
		break;
	}
	case AI_AIMETADATA:
	{
		aiMetadata v;
		metaData.Get<aiMetadata>(static_cast<unsigned int>(idx), v);
		std::cout << "aiMetaData..." << std::endl;
		break;
	}
	case AI_INT64:
	{
		int64_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(int64_t));
		std::cout << data;
		break;
	}
	case AI_UINT32:
	{
		uint64_t data;
		::memcpy(&data, metaData.mValues[idx].mData, sizeof(uint64_t));
		std::cout << data;
		break;
	}


	default:
		break;
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


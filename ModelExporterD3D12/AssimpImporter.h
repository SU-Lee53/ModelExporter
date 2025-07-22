#pragma once


class AssimpImporter
{
public:
	AssimpImporter();

	bool LoadModelFromPath(std::string_view svPath);

	void Run();

	void ShowSceneAttribute();
	void ShowNodeAll();

private:
	void PrintTabs();
	void PrintTabs(int nTabs);
	unsigned int m_nTabs = 0;

private:
	void ShowNode(const aiNode& node);
	void PrintMatrix(const aiMatrix4x4& aimtx);
	void PrintMesh(const aiMesh& aimesh);
	void PrintMetaData(const aiMetadata& metaData, size_t idx);


private:
	bool m_bLoaded = false;

private:
	std::string m_strPath = "";

	std::shared_ptr<Assimp::Importer> m_pImporter = nullptr;
	const aiScene* m_rpScene = nullptr;
	aiNode* m_rpRootNode = nullptr;

};
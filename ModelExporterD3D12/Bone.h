#pragma once

struct BoneWeight {
	int nVertexID;
	float fWeight;	// Blending Factor
};

struct BONE_IMPORT_INFO {
	std::string strName;
	XMFLOAT4X4 xmf4x4Offset;
	std::vector<std::pair<int, float>> weights;
};

class Bone {
public:
	
	static std::shared_ptr<Bone> LoadFromInfo(const BONE_IMPORT_INFO& info);


private:
	std::string m_strName;
	XMFLOAT4X4 m_xmf4x4Offset;
	std::vector<std::pair<int, float>> m_Weights;
};


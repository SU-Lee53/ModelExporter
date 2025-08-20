#pragma once

struct BoneWeight {
	int nVertexID;
	float fWeight;	// Blending Factor
};

struct BONE_IMPORT_INFO {
	std::string strName;
	XMFLOAT4X4 xmf4x4Offset;
	std::vector<std::pair<UINT, float>> weights;
};

class Bone {
public:
	
	static std::shared_ptr<Bone> LoadFromInfo(const BONE_IMPORT_INFO& info);


private:
	std::string m_strName;
	XMFLOAT4X4 m_xmf4x4Offset;	// model -> bone : 그릴때는 bone 에서 model 로 전환이 필요하므로 Inverse 필요함
	std::vector<std::pair<int, float>> m_Weights;
};


#pragma once

/*
	- 08.23 여기도 엎는다
	
	- Bone의 역할이 Skinning 을 위한 Weight 보관이었는데 이제 딱히 그럴 이유가 없음
		- Mesh의 VertexBuffer 에서 Weight를 보관할 수 있음
	- 여기는 이제 애니메이션 노드에 대한 정보를 보관하도록 수정할 것
	+ Offset Matrix는 일단 유지 -> Skinning 시에 필요할 수 있음
		- 얘는 부모 누적이 필요없다고 함

*/


struct BONE_IMPORT_INFO {
	std::string strName;
	XMFLOAT4X4 xmf4x4Offset;
};


class Bone {
public:
	std::string GetName() const { return m_strName; }
	XMFLOAT4X4& GetOffsetMatrix() { return m_xmf4x4Offset; }


private:
	std::string m_strName;
	XMFLOAT4X4 m_xmf4x4Offset;	// model -> bone local

	int m_nAnimationBoneIndex = -1;

public:
	static std::shared_ptr<Bone> LoadFromInfo(const BONE_IMPORT_INFO& boneInfo);

};


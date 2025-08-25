#pragma once

/*
	- 08.23 ���⵵ ���´�
	
	- Bone�� ������ Skinning �� ���� Weight �����̾��µ� ���� ���� �׷� ������ ����
		- Mesh�� VertexBuffer ���� Weight�� ������ �� ����
	- ����� ���� �ִϸ��̼� ��忡 ���� ������ �����ϵ��� ������ ��
	+ Offset Matrix�� �ϴ� ���� -> Skinning �ÿ� �ʿ��� �� ����
		- ��� �θ� ������ �ʿ���ٰ� ��

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


#pragma once

constexpr UINT MAX_BONE_COUNT = 250;

/*
	08.23 ���ƾ��´�
		- AnimationNode �� Bone ���� ��ü
		- IMPORT �ÿ��� Bone ������ keyframe ��ȯ ������ std::map���� ��� �޾ƿ�
		- ��� �ִϸ��̼� ������ ���� ��� + �̸� GPU�� ���� ID3D12Resource �� Root �� Animation ��ü���� ����
			- �ڽ� GameObject ���� Animation �� nullptr
		- ������ �ڽ� ������ Bone ������ GPU�� ���ε��Ͽ� �ִϸ��̼� ���

		- ���� �帥 �ð��� �°� keyframe ���� �����Ͽ� Bone ���� ��� array �� ����� GPU�� ���� -> StructuredBuffer?
		- �ᱹ GPU�� ���� ����:
			[Bone #0 Key Matrix][Bone #1 Key Matrix][Bone #2 Key Matrix] ... [Bone #250 Key Matrix] -> �ִ� 250���� Bone �� ����
		- ���߿� �ڽĵ��� �������ҽ� Bone ���� �˸´� �ε����� ã�ư����� Bone ������ GPU�� �����־�� ��
		- �ִϸ��̼� ��ĵ��� Bone Local ��ȯ�̹Ƿ� GameObject::xmf4x4Transforms ó�� �θ� ������ �ʿ���
			- �� ������ ��� �Ұ��ΰ��� ���� �ִ� �������ΰ� ����
		- �Ҳ� ����

	08.24
		- AnimationNode �� �ʿ��ϱ� �ҵ� -> ����ó�� �� �ڽ��� �ƴ� Root �� Animation ��ü�� �������ֵ��� �ϰ� GameObject ó�� ���� ������ ������ �Ѵ�
*/


struct AnimKeyframe {
	float fTime;
	XMFLOAT3 xmf3PositionKey;
	XMFLOAT4 xmf4RotationKey;
	XMFLOAT3 xmf3ScaleKey;
};

struct AnimChannel {
	std::string strName;	// Bone Name
	std::vector<AnimKeyframe> keyframes;
};

struct ANIMATION_IMPORT_INFO {
	std::string strAnimationName;
	float fDuration = 0.f;
	float fTicksPerSecond = 0.f;
	float fFrameRate = 0.f;

	std::vector<AnimChannel> animationDatas;
};

struct CB_ANIMATION_CONTROL_DATA {
	int		nCurrentAnimationIndex;
	float	fDuration;
	float	fTicksPerSecond;
	float	fTimeElapsed;
};

struct CB_ANIMATION_TRANSFORM_DATA {
	XMFLOAT4X4 xmf4x4Transforms[MAX_BONE_COUNT];
};

class Animation {
public:
	void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList);

public:
	struct Data {
		float	fDuration = 0.f;
		float	fTicksPerSecond = 0.f;
		int		nFrameCount = 0;

		std::vector<AnimChannel> channels;
	};

	int		m_nCurrentAnimationIndex = 0;
	float	m_fTimeElapsed = 0.f;

	std::vector<Data> m_AnimationDatas;

	ComPtr<ID3D12Resource> m_pControllerCBuffer;
	UINT8* m_pControllerDataMappedPtr;

	ComPtr<ID3D12Resource> m_pBoneTransformCBuffer;
	UINT8* m_pBoneTransformMappedPtr;

	std::weak_ptr<GameObject> m_wpOwner;
	std::vector<Bone> m_bones;

	bool m_bPlay = false;

public:
	static std::shared_ptr<Animation> LoadFromInfo(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<ANIMATION_IMPORT_INFO>& infos, std::shared_ptr<GameObject> pOwner);


public:
	constexpr static UINT ANIMATION_DESCRIPTOR_COUNT_PER_DRAW = 2;

};

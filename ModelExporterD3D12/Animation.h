#pragma once

constexpr UINT MAX_BONE_COUNT = 250;

/*
	08.23 갈아엎는다
		- AnimationNode 는 Bone 으로 대체
		- IMPORT 시에는 Bone 정보와 keyframe 변환 정보를 std::map으로 묶어서 받아옴
		- 모든 애니메이션 정보와 최종 행렬 + 이를 GPU로 보낼 ID3D12Resource 는 Root 의 Animation 객체에만 보관
			- 자식 GameObject 들의 Animation 은 nullptr
		- 나머지 자식 노드들은 Bone 정보를 GPU에 업로드하여 애니메이션 재생

		- 현재 흐른 시간에 맞게 keyframe 들을 보간하여 Bone 들의 행렬 array 를 만들어 GPU로 전송 -> StructuredBuffer?
		- 결국 GPU가 갖는 값은:
			[Bone #0 Key Matrix][Bone #1 Key Matrix][Bone #2 Key Matrix] ... [Bone #250 Key Matrix] -> 최대 250개의 Bone 을 지원
		- 나중에 자식들을 렌더링할시 Bone 별로 알맞는 인덱스를 찾아가도록 Bone 정보를 GPU로 보내주어야 함
		- 애니메이션 행렬들은 Bone Local 변환이므로 GameObject::xmf4x4Transforms 처럼 부모 누적이 필요함
			- 이 누적을 어떻게 할것인가가 현재 최대 문제점인거 같음
		- 할꺼 많노

	08.24
		- AnimationNode 가 필요하긴 할듯 -> 이전처럼 각 자식이 아닌 Root 의 Animation 객체가 가지고있도록 하고 GameObject 처럼 계층 구조를 갖도록 한다
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

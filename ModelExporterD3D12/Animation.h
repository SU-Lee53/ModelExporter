#pragma once

constexpr UINT MAX_ANIMATION_KEYFRAMES = 250;
constexpr UINT MAX_ANIMATION_COUNTS = 8;

struct AnimKey {
	XMFLOAT3 xmf3Value{ 0,0,0 };
	float fTime;
};

struct AnimKeyframe {
	std::vector<AnimKey> posKeys;
	std::vector<AnimKey> rotKeys;
	std::vector<AnimKey> scaleKeys;
	std::vector<XMFLOAT4X4> xmf4x4FinalTransform;
};

struct AnimChannel {
	std::string strName;
	AnimKeyframe keyframes;
};

struct CB_ANIMATION_CONTROL_DATA {
	int		nCurrentAnimationIndex;
	float	fDuration;
	float	fTicksPerSecond;
	float	fTimeElapsed;
};

struct SB_ANIMATION_TRANSFORM_DATA {
	XMFLOAT4X4 xmf4x4Transforms[MAX_ANIMATION_COUNTS][MAX_ANIMATION_KEYFRAMES];
};

struct ANIMATION_CONTROLLER_IMPORT_INFO {
	std::string strName;
	float fDuration = 0.f;
	float fTicksPerSecond = 0.f;
	std::vector<AnimChannel> channels;
};

struct ANIMATION_NODE_IMPORT_INFO {
	std::string strName;
	int nKeyframeIndex;
};

enum ANIMATION_COMPONENT_MODE : uint8_t {
	ANIMATION_COMPONENT_MODE_CONTROLLER = 0,
	ANIMATION_COMPONENT_MODE_NODE
};

struct AnimationNode {
	std::string strNodeName;
	std::vector<int> animMatrixIndices;	// vector index : animation index
										// value : bone index in animation transform structured buffer
	
public:
	void Initialize();

};

struct AnimationController {
	struct Data {
		float m_fDuration = 0.f;
		float m_fTicksPerSecond = 0.f;

		std::vector<AnimChannel> channels;
	};

	std::vector<Data> AnimationDatas;
	int nAnimIndex = -1;
	float m_fTimeElapsed = 0.f;

	ComPtr<ID3D12Resource> pControllerCBuffer;
	UINT8* pControllerDataMappedPtr;

	ComPtr<ID3D12Resource> pAnimationSBuffer;
	UINT8* pAnimationDataMappedPtr;


public:
	void Initialize();

};

template<typename T>
concept AnimationDataInfoType =	std::same_as<T, ANIMATION_CONTROLLER_IMPORT_INFO>
								|| std::same_as<T, ANIMATION_NODE_IMPORT_INFO>;

template<ANIMATION_COMPONENT_MODE mode>
struct _GET_COMPONENT_TYPE {
	using type = std::nullptr_t;
};

template<>
struct _GET_COMPONENT_TYPE<ANIMATION_COMPONENT_MODE_CONTROLLER> {
	using type = std::shared_ptr<AnimationController>;
};

template<>
struct _GET_COMPONENT_TYPE<ANIMATION_COMPONENT_MODE_NODE> {
	using type = std::shared_ptr<AnimationNode>;
};


class Animation {
public:
	Animation() = default;
	Animation(ANIMATION_COMPONENT_MODE initMode) : m_eMode{ initMode } {
		if (initMode == ANIMATION_COMPONENT_MODE_CONTROLLER) {
			m_pAnimComponent = std::make_shared<AnimationController>();
		}
		else {
			m_pAnimComponent = std::make_shared<AnimationNode>();
		}

	}

	template<ANIMATION_COMPONENT_MODE mode>
	typename _GET_COMPONENT_TYPE<mode>::type  Get() {
		assert(mode == m_eMode);
		return std::get<mode>(m_pAnimComponent);
	}

	template<AnimationDataInfoType T>
	void Set(const T& data) {
		if constexpr (std::is_same_v<T, ANIMATION_CONTROLLER_IMPORT_INFO>) {
			assert(std::holds_alternative<std::shared_ptr<AnimationController>>(m_pAnimComponent));

			auto pController = std::get<std::shared_ptr<AnimationController>>(m_pAnimComponent);

			AnimationController::Data animData{};
			animData.m_fDuration = data.fDuration;
			animData.m_fTicksPerSecond = data.fTicksPerSecond;
			animData.channels = data.channels;

			pController->AnimationDatas.push_back(animData);
		}
		else if constexpr (std::is_same_v<T, ANIMATION_NODE_IMPORT_INFO>) {
			assert(std::holds_alternative<std::shared_ptr<AnimationNode>>(m_pAnimComponent));
			auto pNode = std::get<std::shared_ptr<AnimationNode>>(m_pAnimComponent);

			pNode->strNodeName = data.strName;
			pNode->animMatrixIndices.push_back(data.nKeyframeIndex);
		}
		else {
			std::unreachable();
		}
	}

private:
	std::variant<std::shared_ptr<AnimationController>, std::shared_ptr<AnimationNode>> m_pAnimComponent;
	const ANIMATION_COMPONENT_MODE m_eMode;

};

#pragma once

struct ANIMATION_LOAD_INFO {
	std::string strName;
	std::string strChannelName;
	float fDuration = 0.f;
	float fTicksPerSecond = 0.f;

	std::vector<Keyframe> Keyframes;
};

struct Keyframe {
	Keyframe() = default;
	Keyframe(XMFLOAT3 pos, XMFLOAT4 quat, XMFLOAT3 scale) 
		: xmf3Position{ pos }, xmf3RotationQuat{ quat }, xmf3Scale{ scale } {
		xmf3Position = pos;
		xmf3RotationQuat = quat;
		xmf3Scale = scale;
		XMStoreFloat4x4(&xmf4x4Transform,
			XMMatrixAffineTransformation(XMLoadFloat3(&xmf3Scale), XMVectorSet(0, 0, 0, 0), XMLoadFloat4(&xmf3RotationQuat), XMLoadFloat3(&xmf3Position)));
	}

	XMFLOAT3 xmf3Position;
	XMFLOAT4 xmf3RotationQuat;
	XMFLOAT3 xmf3Scale;
	XMFLOAT4X4 xmf4x4Transform;
};

struct Animation {
	std::vector<Keyframe> Keyframes;
	ComPtr<ID3D12Resource> pAnimationSBuffer;
};

struct AnimationNode {
	std::string strNodeName;
	int animMatrixIndex;
};

class AnimationController {

private:
	float m_fDuration = 0.f;
	float m_fSumTime = 0.f;
	float m_fTicksPerSecond = 0.f;

	std::vector<Keyframe> Keyframes;

};

enum ANIMATION_COMPONENT_MODE : uint8_t{
	ANIMATION_COMPONENT_MODE_CONTROLLER = 0,
	ANIMATION_COMPONENT_MODE_NODE
};

template<ANIMATION_COMPONENT_MODE mode>
struct _GET_COMPONENT_TYPE {
	using type = std::void_t;
};

template<>
struct _GET_COMPONENT_TYPE<ANIMATION_COMPONENT_MODE_CONTROLLER> {
	using type = std::shared_ptr<AnimationController>;
};

template<>
struct _GET_COMPONENT_TYPE<ANIMATION_COMPONENT_MODE_NODE> {
	using type = std::shared_ptr<AnimationNode>;
};


class AnimationComponent {
	constexpr AnimationComponent(ANIMATION_COMPONENT_MODE initMode) : m_eMode{ initMode } {}

	template<ANIMATION_COMPONENT_MODE mode>
	typename _GET_COMPONENT_TYPE<mode>::type  Get() {
		assert(mode == m_eMode, "Template Argument must match with m_eMode");
		return std::get<mode>(m_pAnimComponent);
	}

private:
	std::variant<std::shared_ptr<AnimationController>, std::shared_ptr<AnimationNode>> m_pAnimComponent;
	const ANIMATION_COMPONENT_MODE m_eMode;

};


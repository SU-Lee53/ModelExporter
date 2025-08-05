#pragma once

struct Keyframe {
	Keyframe() = default;
	Keyframe(XMFLOAT3 pos, XMFLOAT4 quat, XMFLOAT3 scale) {
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

struct ANIMATION_LOAD_INFO 
{
	std::string strName;
	std::string strChannelName;
	float fDuration = 0.f;
	float fTicksPerSecond = 0.f;

	std::vector<Keyframe> Keyframes;
};

class Animation
{

private:
	std::string m_strName;
	std::string m_strChannelName;
	float m_fDuration = 0.f;
	float m_fTicksPerSecond = 0.f;

	std::vector<Keyframe> Keyframes;

};


#include "stdafx.h"
#include "Animation.h"



void Animation::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dRenderCommandList)
{
	auto curAnimation = m_AnimationDatas[m_nCurrentAnimationIndex];
	m_fTimeElapsed += DELTA_TIME;
	float durationInSec = curAnimation.fDuration / curAnimation.fTicksPerSecond;
	if (m_fTimeElapsed > durationInSec) {
		m_fTimeElapsed = fmod(m_fTimeElapsed, durationInSec);
	}

	// 현재 프레임의 Index
	int nFrameIndex = (int)((m_fTimeElapsed * curAnimation.nFrameCount) / durationInSec);
	if (nFrameIndex >= curAnimation.nFrameCount) {
		nFrameIndex = curAnimation.nFrameCount - 1;
	}

	// Channel 별 Local
	std::unordered_map<std::string, XMMATRIX> local;
	for (auto& channel : curAnimation.channels)
	{
		const AnimKeyframe& k = channel.keyframes[nFrameIndex];
		XMVECTOR xmvScale = XMLoadFloat3(&k.xmf3ScaleKey);
		XMVECTOR xmvRotation = XMLoadFloat4(&k.xmf4RotationKey);
		XMVECTOR xmvTranslate = XMLoadFloat3(&k.xmf3PositionKey);
	
		local[channel.strName] = XMMatrixAffineTransformation(xmvScale, XMVectorZero(), xmvRotation, xmvTranslate);
	}

	// 부모 누적
	auto pRoot = m_wpOwner.lock();
	std::unordered_map<std::string, XMMATRIX> global;

	std::vector<std::shared_ptr<Bone>> bones;
	UINT nBones = 0;

	std::function<void(std::shared_ptr<GameObject>, XMMATRIX)> ComputeGlobal = [&](std::shared_ptr<GameObject> pCur, XMMATRIX xmmtxParentGlobal) {
		
		XMMATRIX xmmtxLocal = local.contains(pCur->GetName()) ? local[pCur->GetName()] : XMLoadFloat4x4(&pCur->GetLocalTransform());
		XMMATRIX xmmtxGlobal = XMMatrixMultiply(xmmtxLocal, xmmtxParentGlobal);
		global[pCur->GetName()] = xmmtxGlobal;

		if (auto bone = pCur->GetBone()) {
			bones.push_back(pCur->GetBone());
			nBones++;
		}

		for (auto& pChild : pCur->GetChildren()) {
			ComputeGlobal(pChild, xmmtxGlobal);
		}
	};

	ComputeGlobal(pRoot, XMMatrixIdentity());

	// ConstantBuffer (Bone Transform) 에다가 행렬들 정리
	
	std::vector<XMFLOAT4X4> xmf4x4FinalMatrices;
	xmf4x4FinalMatrices.reserve(bones.size());

	for (const auto& bone : bones) {
		XMMATRIX xmmtxGlobal = global[bone->GetName()];
		XMMATRIX xmmtxOffset = XMLoadFloat4x4(&bone->GetOffsetMatrix());
		XMMATRIX xmmtxFinal = XMMatrixTranspose(XMMatrixMultiply(xmmtxGlobal, xmmtxOffset));
		
		XMFLOAT4X4 xmf4x4Final;
		XMStoreFloat4x4(&xmf4x4Final, xmmtxFinal);
		xmf4x4FinalMatrices.push_back(xmf4x4Final);
	}

	::memcpy(m_pBoneTransformMappedPtr, xmf4x4FinalMatrices.data(), sizeof(XMFLOAT4X4) * bones.size());

	// Control Data
	CB_ANIMATION_CONTROL_DATA controlData{};
	controlData.nCurrentAnimationIndex = m_nCurrentAnimationIndex;
	controlData.fDuration = m_AnimationDatas[m_nCurrentAnimationIndex].fDuration;
	controlData.fTicksPerSecond = m_AnimationDatas[m_nCurrentAnimationIndex].fTicksPerSecond;
	controlData.fTimeElapsed = m_fTimeElapsed;

	::memcpy(m_pControllerDataMappedPtr, &controlData, sizeof(CB_ANIMATION_CONTROL_DATA));

	pd3dRenderCommandList->SetGraphicsRootConstantBufferView(4, m_pControllerCBuffer->GetGPUVirtualAddress());
	pd3dRenderCommandList->SetGraphicsRootConstantBufferView(5, m_pBoneTransformCBuffer->GetGPUVirtualAddress());

	ImGui::Begin("Animation");
	{
		ImGui::Text("m_fTimeElapsed : %f", m_fTimeElapsed);
	}
	ImGui::End();
}

std::shared_ptr<Animation> Animation::LoadFromInfo(ComPtr<ID3D12Device14> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<ANIMATION_IMPORT_INFO>& infos, std::shared_ptr<GameObject> pOwner)
{
	// GPU에 보낼 데이터
	//	1. CB_ANIMATION_CONTROL_DATA -> 애니메이션의 기본적인 정보들이 보관됨
	//	2. SB_ANIMATION_TRANSFORM_DATA -> 각 Bone 들이 애니매이션을 위해 최종적으로 변환되어야 할 행렬들이 보관됨

	std::shared_ptr<Animation> pAnimation = std::make_shared<Animation>();

	for (const auto& info : infos) {
		Data data;
		data.fDuration = info.fDuration;
		data.fTicksPerSecond = info.fTicksPerSecond;
		data.channels = info.animationDatas;
		data.nFrameCount = info.animationDatas[0].keyframes.size();
		
		pAnimation->m_AnimationDatas.push_back(data);
	}

	// Controller data Constant Buffer 생성
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(::AlignConstantBuffersize(sizeof(CB_ANIMATION_CONTROL_DATA))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pAnimation->m_pControllerCBuffer.GetAddressOf())
	);

	pAnimation->m_pControllerCBuffer->Map(0, NULL, reinterpret_cast<void**>(&pAnimation->m_pControllerDataMappedPtr));

	// Transform data Constant Buffer 생성
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(::AlignConstantBuffersize(sizeof(CB_ANIMATION_TRANSFORM_DATA))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pAnimation->m_pBoneTransformCBuffer.GetAddressOf())
	);

	pAnimation->m_pBoneTransformCBuffer->Map(0, NULL, reinterpret_cast<void**>(&pAnimation->m_pBoneTransformMappedPtr));

	pAnimation->m_wpOwner = pOwner;

	return pAnimation;
}

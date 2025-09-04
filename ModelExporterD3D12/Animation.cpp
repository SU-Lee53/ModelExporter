#include "stdafx.h"
#include "Animation.h"

ComPtr<ID3D12Resource>  Animation::s_pIdentityBonesCB = nullptr;
UINT8*                  Animation::s_pIdentityBonesMapped = nullptr;
bool                    Animation::s_identityReady = false;

std::shared_ptr<Animation>
Animation::LoadFromInfo(ComPtr<ID3D12Device> device,
    ComPtr<ID3D12GraphicsCommandList> /*cmd*/,
    const std::vector<ANIMATION_IMPORT_INFO>& infos,
    std::shared_ptr<GameObject> pRoot)
{
    auto p = std::shared_ptr<Animation>(new Animation());
    p->m_wpOwner = pRoot;

    // 1) 클립 복사
    p->m_Clips.reserve(infos.size());
    for (auto& in : infos) {
        Clip c{};
        c.name = in.strAnimationName;
        c.durationTicks = in.fDuration;
        c.ticksPerSecond = (in.fTicksPerSecond != 0.f) ? in.fTicksPerSecond : 25.f;
        c.frameRate = (in.fFrameRate != 0.f) ? in.fFrameRate : 30.f;

        float durationSec = (c.ticksPerSecond != 0.f) ? c.durationTicks / c.ticksPerSecond : 0.f;
        c.frameCount = std::max(1u, (unsigned)std::ceil(durationSec * c.frameRate));

        for (auto& ch : in.animationDatas) {
            c.channels[ch.strName] = ch.keyframes; // [0..frameCount-1]
        }
        p->m_Clips.push_back(std::move(c));
    }

    // 2) 본 순서 (계층 DFS → boneMap에 존재하는 노드만)
    p->buildBoneOrderFromImporterOrder();

    // 3) CB 할당
    p->allocateCBuffers(device);

    // 4) 초기 클립 0
    p->SetClip(0);

    return p;
}

void Animation::SetClip(int idx)
{
    if (idx < 0 || idx >= (int)m_Clips.size()) return;
    m_nCurrentAnimationIndex = idx;
    m_fTimeElapsed = 0.f;
}

void Animation::buildBoneOrderFromImporterOrder()
{
    m_bones.clear();
    m_bones.reserve(OBJECT_IMPORT_INFO::boneList.size());
    for (const auto& b : OBJECT_IMPORT_INFO::boneList) {
        Bone bone{};
        bone.strName = b.strName;
        bone.xmf4x4Offset = b.xmf4x4Offset; // row-major
        bone.nParentIndex = b.nParentIndex; // row-major
        m_bones.push_back(bone);         // <-- 인덱스 0..N-1 == Importer 인덱스
    }
}

void Animation::allocateCBuffers(ComPtr<ID3D12Device> device)
{
    // Controller
    {
        CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(AlignConstantBuffersize(sizeof(CB_ANIMATION_CONTROL_DATA)));
        device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &buf,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pControllerCBuffer.GetAddressOf()));
        m_pControllerCBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pControllerDataMappedPtr));
    }

    // Bone transforms
    {
        CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(AlignConstantBuffersize(sizeof(CB_ANIMATION_TRANSFORM_DATA)));
        device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &buf,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pBoneTransformCBuffer.GetAddressOf()));
        m_pBoneTransformCBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pBoneTransformMappedPtr));
    }
}

void Animation::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> cmd)
{
    // 본 데이터가 없으면 그냥 리턴
    if (m_bones.size() == 0) return;

    if (m_Clips.empty()) return;

    if (m_bPlay) m_fTimeElapsed += DELTA_TIME;

    Clip& clip = m_Clips[m_nCurrentAnimationIndex];
    float fDurationSec = clip.durationTicks / clip.ticksPerSecond;
    m_fTimeElapsed = m_fTimeElapsed > fDurationSec ? fmodf(m_fTimeElapsed, fDurationSec) : m_fTimeElapsed;

    size_t nBones = m_bones.size();

    std::vector<XMFLOAT4X4> toParentTransforms(nBones);
    std::vector<XMFLOAT4X4> toRootTransforms(nBones);
    std::vector<XMFLOAT4X4> finalTransforms(MAX_BONE_COUNT);

    for (int i = 0; i < nBones; ++i) {
        toParentTransforms[i] = clip.GetSRT(m_bones[i].strName, m_fTimeElapsed, m_bLocalSRT);
    }

    toRootTransforms[0] = toParentTransforms[0];

    for (int i = 0; i < nBones; ++i) {
        XMMATRIX xmmtxToParent = XMLoadFloat4x4(&toParentTransforms[i]);
        int nParent = m_bones[i].nParentIndex;
        XMMATRIX xmmtxParentToRoot = nParent < 0 ? XMMatrixIdentity() : XMLoadFloat4x4(&toRootTransforms[nParent]);
        XMMATRIX xmmtxToRoot = XMMatrixMultiply(xmmtxToParent, xmmtxParentToRoot);
        XMStoreFloat4x4(&toRootTransforms[i], xmmtxToRoot);
    }

    for (int i = 0; i < nBones; ++i) {
        XMMATRIX xmmtxBoneOffset = XMLoadFloat4x4(&m_bones[i].xmf4x4Offset);
        XMMATRIX xmmtxToRootTransform = XMLoadFloat4x4(&toRootTransforms[i]);
        XMMATRIX xmTransform = XMMatrixMultiply(xmmtxBoneOffset, xmmtxToRootTransform);
        XMStoreFloat4x4(&finalTransforms[i], m_bFinalTranspose ? XMMatrixTranspose(xmTransform) : xmTransform);
    }

    for (int i = nBones; i < MAX_BONE_COUNT; ++i) {
        XMStoreFloat4x4(&finalTransforms[i], XMMatrixIdentity());
    }


    auto* pOut = reinterpret_cast<CB_ANIMATION_TRANSFORM_DATA*>(m_pBoneTransformMappedPtr);
    UINT boneCount = (UINT)std::min<size_t>(MAX_BONE_COUNT, m_bones.size());
    std::memcpy(pOut, finalTransforms.data(), sizeof(XMFLOAT4X4) * MAX_BONE_COUNT);

    auto* pCtrl = reinterpret_cast<CB_ANIMATION_CONTROL_DATA*>(m_pControllerDataMappedPtr);
    pCtrl->nCurrentAnimationIndex = m_nCurrentAnimationIndex;
    pCtrl->fDuration = clip.durationTicks;
    pCtrl->fTicksPerSecond = clip.ticksPerSecond;
    pCtrl->fTimeElapsed = m_fTimeElapsed;

    // 5) 루트시그에 바인딩 (root param index는 프로젝트 고정값 사용)
    // 예: b4 = controller, b5 = bones
    cmd->SetGraphicsRootConstantBufferView(4, m_pControllerCBuffer->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(5, m_pBoneTransformCBuffer->GetGPUVirtualAddress());

    ImGui::Begin("Anim");
    {
        ImGui::Text("m_fTimeElapsed : %f", m_fTimeElapsed);

        m_bPlay = ImGui::Button("Play") ? !m_bPlay : m_bPlay;
        
        if (!m_bPlay) {
            ImGui::SliderFloat("fTimeElapsed", &m_fTimeElapsed, 0.f, fDurationSec);
        }

        if (ImGui::Button(m_bLocalSRT ? "Local SRT" : "Local TRS")) {
            m_bLocalSRT = !m_bLocalSRT;
        }

        if (ImGui::Button(m_bGlobalMulOrder ? "Global local * parent" : "Global parent * local")) {
            m_bGlobalMulOrder = !m_bGlobalMulOrder;
        }

        if (ImGui::Button(m_bFinalTranspose ? "Final Transpose : Y" : "Final Transpose : N")) {
            m_bFinalTranspose = !m_bFinalTranspose;
        }

    }
    ImGui::End();
}

void Animation::BindCBVs(ComPtr<ID3D12GraphicsCommandList> cmd,
    UINT rootParamForCtrl, UINT rootParamForBones)
{
    if (m_pControllerCBuffer)
        cmd->SetGraphicsRootConstantBufferView(rootParamForCtrl, m_pControllerCBuffer->GetGPUVirtualAddress());
    if (m_pBoneTransformCBuffer)
        cmd->SetGraphicsRootConstantBufferView(rootParamForBones, m_pBoneTransformCBuffer->GetGPUVirtualAddress());
}

// 애니메이션이 없을 때도 VS(b5)를 만족시키기 위한 아이덴티티 본 배열 CB
void Animation::BindIdentityBones(ComPtr<ID3D12Device> device,
    ComPtr<ID3D12GraphicsCommandList> cmd,
    UINT rootParamIndexForBones)
{
    if (!s_identityReady) {
        // 1회 생성
        CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(AlignConstantBuffersize(sizeof(CB_ANIMATION_TRANSFORM_DATA)));
        device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &buf,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(s_pIdentityBonesCB.GetAddressOf()));
        s_pIdentityBonesCB->Map(0, nullptr, reinterpret_cast<void**>(&s_pIdentityBonesMapped));

        auto* pOut = reinterpret_cast<CB_ANIMATION_TRANSFORM_DATA*>(s_pIdentityBonesMapped);
        XMMATRIX I = XMMatrixTranspose(XMMatrixIdentity());
        for (UINT i = 0; i < MAX_BONE_COUNT; ++i) {
            XMStoreFloat4x4(&pOut->xmf4x4Transforms[i], I);
        }
        s_identityReady = true;
    }
    cmd->SetGraphicsRootConstantBufferView(rootParamIndexForBones, s_pIdentityBonesCB->GetGPUVirtualAddress());
}
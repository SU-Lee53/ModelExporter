#include "stdafx.h"
#include "SkeletonLines.h"
#include "GameObject.h"
#include "Animation.h"

struct CBCamera { XMFLOAT4X4 VP; };

void SkeletonLineRenderer::Create(ComPtr<ID3D12Device> device,
    const void* vs, size_t vsSize,
    const void* ps, size_t psSize,
    DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
    // RootSig: b0 (CB), no tables
    CD3DX12_ROOT_PARAMETER1 rp{};
    rp.InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rdesc{};
    rdesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rdesc.Desc_1_1.NumParameters = 1;
    rdesc.Desc_1_1.pParameters = &rp;
    rdesc.Desc_1_1.NumStaticSamplers = 0;
    rdesc.Desc_1_1.pStaticSamplers = nullptr;
    rdesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sig, err;
    D3D12SerializeVersionedRootSignature(&rdesc, sig.GetAddressOf(), err.GetAddressOf());
    device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
        IID_PPV_ARGS(m_rs.GetAddressOf()));

    // PSO
    D3D12_INPUT_ELEMENT_DESC il[] = {
        { "POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(LineVertex,pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "COLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)offsetof(LineVertex,col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = m_rs.Get();
    pso.VS = { vs, vsSize };
    pso.PS = { ps, psSize };
    pso.InputLayout = { il, _countof(il) };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    pso.RTVFormats[0] = rtvFormat; pso.NumRenderTargets = 1;
    pso.DSVFormat = dsvFormat;
    pso.SampleDesc.Count = 1;
    pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(m_pso.GetAddressOf()));

    // CB (upload)
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(256), // 충분
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_cb.GetAddressOf()));
    m_cb->Map(0, nullptr, (void**)&m_cbPtr);
}

void SkeletonLineRenderer::rebuildVB(ComPtr<ID3D12Device> device, size_t vertexCount)
{
    if (vertexCount <= m_capacity) return;
    m_capacity = std::max(vertexCount, m_capacity * 2 + 1024);

    size_t bytes = m_capacity * sizeof(LineVertex);
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bytes),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_vb.GetAddressOf()));
    m_vbv.BufferLocation = m_vb->GetGPUVirtualAddress();
    m_vbv.StrideInBytes = m_vbStride;
    m_vbv.SizeInBytes = (UINT)bytes;
}

void SkeletonLineRenderer::buildLinesDFS(const std::shared_ptr<GameObject>& node,
    const std::unordered_map<std::string, XMMATRIX>& globals,
    const XMMATRIX& parentGlobal,
    std::vector<LineVertex>& out,
    float axisLen,
    const XMFLOAT4& boneColor,
    const XMFLOAT4& axisColor)
{
    // Global = Local * Parent (애니가 제공한 글로벌이 있으면 우선 사용)
    XMMATRIX L = XMLoadFloat4x4(&node->GetLocalTransform());
    XMMATRIX G = XMMatrixMultiply(L, parentGlobal);
    auto it = globals.find(node->GetName());
    if (it != globals.end()) G = it->second;

    // 부모-자식 라인
    XMVECTOR p = XMVector3TransformCoord(XMVectorZero(), G);
    for (auto& ch : node->GetChildren()) {
        // 자식 글로벌 미리 계산
        XMMATRIX CL = XMLoadFloat4x4(&ch->GetLocalTransform());
        XMMATRIX CG = XMMatrixMultiply(CL, G);
        auto itc = globals.find(ch->GetName());
        if (itc != globals.end()) CG = itc->second;

        XMVECTOR pc = XMVector3TransformCoord(XMVectorZero(), CG);

        XMFLOAT3 a, b; XMStoreFloat3(&a, p); XMStoreFloat3(&b, pc);
        out.push_back({ a, boneColor });
        out.push_back({ b, boneColor });

        // 재귀
        buildLinesDFS(ch, globals, G, out, axisLen, boneColor, axisColor);
    }

    // 잎본 축(+Y) 그리기
    if (axisLen > 0.0f && node->GetChildren().empty()) {
        XMVECTOR tip = XMVector3TransformCoord(XMVectorSet(0, axisLen, 0, 1), G);
        XMFLOAT3 a, c; XMStoreFloat3(&a, p); XMStoreFloat3(&c, tip);
        out.push_back({ a, axisColor });
        out.push_back({ c, axisColor });
    }
}

void SkeletonLineRenderer::Draw(ComPtr<ID3D12GraphicsCommandList> cmd,
    const XMFLOAT4X4& viewProj,
    const std::shared_ptr<GameObject>& root,
    Animation* anim,
    float axisLen)
{
    if (!root) return;

    // 1) 현재 프레임 글로벌 맵
    std::unordered_map<std::string, XMMATRIX> globals;
    if (anim) anim->BuildGlobalForDebug(globals);

    // 2) 라인 버퍼 생성
    std::vector<LineVertex> verts;
    const XMFLOAT4 boneCol = XMFLOAT4(1, 1, 1, 1);
    const XMFLOAT4 axisCol = XMFLOAT4(0.2f, 1.0f, 0.2f, 1);

    buildLinesDFS(root, globals, XMMatrixIdentity(), verts, axisLen, boneCol, axisCol);

    // 3) 업로드
    //    (정점 수 0이면 리턴)
    if (verts.empty()) return;

    // VB 용량 보장
    ComPtr<ID3D12Device> device;
    cmd->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
    rebuildVB(device, verts.size());

    // Map & copy
    void* p; m_vb->Map(0, nullptr, &p);
    memcpy(p, verts.data(), verts.size() * sizeof(LineVertex));
    m_vb->Unmap(0, nullptr);

    // CB 뷰프로젝션
    CBCamera cb{}; cb.VP = viewProj; 
    memcpy(m_cbPtr, &cb, sizeof(cb));

    // 4) 드로우
    cmd->SetGraphicsRootSignature(m_rs.Get());
    cmd->SetPipelineState(m_pso.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    cmd->IASetVertexBuffers(0, 1, &m_vbv);
    cmd->SetGraphicsRootConstantBufferView(0, m_cb->GetGPUVirtualAddress());
    cmd->DrawInstanced((UINT)verts.size(), 1, 0, 0);
}

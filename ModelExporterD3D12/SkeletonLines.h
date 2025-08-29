#pragma once

class GameObject;
class Animation;

struct LineVertex {
    XMFLOAT3 pos;
    XMFLOAT4 col;
};

class SkeletonLineRenderer {
public:
    void Create(ComPtr<ID3D12Device> device, const void* vsBytecode, size_t vsSize,
        const void* psBytecode, size_t psSize, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat);

    // viewProj: 카메라 (row-major, mul(v, VP) 경로)
    // axisLen: 잎본(+Y) 축 길이, 0이면 축 미표시
    void Draw(ComPtr<ID3D12GraphicsCommandList> cmd,
        const XMFLOAT4X4& viewProj,
        const std::shared_ptr<GameObject>& root,
        Animation* anim,
        float axisLen = 0.05f);

private:
    void rebuildVB(ComPtr<ID3D12Device> device, size_t vertexCount);
    void buildLinesDFS(const std::shared_ptr<GameObject>& node,
        const std::unordered_map<std::string, XMMATRIX>& globals,
        const XMMATRIX& parentGlobal,
        std::vector<LineVertex>& out,
        float axisLen,
        const XMFLOAT4& boneColor,
        const XMFLOAT4& axisColor);

private:
    ComPtr<ID3D12RootSignature> m_rs;
    ComPtr<ID3D12PipelineState> m_pso;
    ComPtr<ID3D12Resource>      m_cb;   UINT8* m_cbPtr = nullptr;
    ComPtr<ID3D12Resource>      m_vb;   UINT      m_vbStride = sizeof(LineVertex);
    D3D12_VERTEX_BUFFER_VIEW    m_vbv{};
    size_t                      m_capacity = 0;
};

#pragma once

class Camera {
public:
	Camera(ComPtr<ID3D12Device14> m_pd3dDevice);
	
	void Update();
	void ProcessInput();

public:
	void GenerateViewMatrix();
	XMFLOAT4X4 GetViewProjTransposed() {
		return m_xmf4x4ViewProjectionTransposed;
	}

	void BindViewportAndScissorRects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const {
		pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
		pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
	}

	void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT rootParameterIndex) {
		::memcpy(m_pMappedPtr, &m_xmf4x4ViewProjectionTransposed, sizeof(XMFLOAT4X4));
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, m_pCBViewProj->GetGPUVirtualAddress());
	}

private:
	void Rotate(XMFLOAT3 xmf3Rotate);

private:
	XMFLOAT3 m_xmf3CamPosition = {0.f, 0.f, -15.f};
	XMFLOAT3 m_xmf3CamRotation = {0.f, 0.f, 0.f};	// Euler
	
	XMFLOAT3 m_xmf3Right = {1.f, 0.f, 0.f};
	XMFLOAT3 m_xmf3Up = {0.f, 1.f, 0.f};
	XMFLOAT3 m_xmf3Look = {0.f, 0.f, 1.f};

	XMFLOAT4X4 m_xmf4x4View;
	XMFLOAT4X4 m_xmf4x4Projection;
	XMFLOAT4X4 m_xmf4x4ViewProjectionTransposed;

	ComPtr<ID3D12Resource> m_pCBViewProj = nullptr;
	UINT8* m_pMappedPtr;
	
	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;

private:
	float m_fMovingSpeed = 0.05f;

};


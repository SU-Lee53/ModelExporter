#pragma once

class D3DCore {
public:
	D3DCore(BOOL bEnableDebugLayer, BOOL bEnableGBV);

public:
	void RenderBegin();
	void Render();
	void RenderEnd();
	void Present();
	void MoveToNextFrame();


private:
	void CreateD3DDevice(bool bEnableDebugLayer);
	void CreateSwapChain();
	void CreateRTVAndDSVDescriptorHeaps();
	void CreateCommandQueueAndList();

private:
	void CreateRenderTargetViews();
	void CreateDepthStencilView();

private:
	// GPU-CPU Sync
	void WaitForGPUComplete();
	void ChangeSwapChainState();

public:
	ComPtr<ID3D12Device14> GetDevice() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;


private:
	ComPtr<IDXGIFactory4>	m_pdxgiFactory = nullptr;
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain = nullptr;
	ComPtr<ID3D12Device14>	m_pd3dDevice = nullptr;

	bool m_bEnableDebugLayer = false;
	bool m_bEnableGBV = false;

	bool m_bMsaa4xEnable = false;
	bool m_nMsaa4xQualityLevels = 0;

	const static UINT m_nSwapChainBuffers = 2;

	UINT m_nSwapChainBufferIndex = 0;

	// Framework resources
	// RTV
	std::array<ComPtr<ID3D12Resource>, m_nSwapChainBuffers> m_pd3dRenderTargetBuffers = {};
	ComPtr<ID3D12DescriptorHeap>	m_pd3dRTVDescriptorHeap = nullptr;
	UINT							m_nRTVDescriptorIncrementSize = 0;

	// DSV
	ComPtr<ID3D12Resource>			m_pd3dDepthStencilBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap>	m_pd3dDSVDescriptorHeap = nullptr;
	UINT							m_nDSVDescriptorIncrementSize = 0;

	// Command Queue, Command Allocator, Command List
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator>		m_pd3dCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList>	m_pd3dCommandList = nullptr;

	// Pipeline State Object
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;

	// Fence & Fence value
	ComPtr<ID3D12Fence> m_pd3dFence = nullptr;
	HANDLE				m_hFenceEvent = nullptr;
	std::array<UINT64, m_nSwapChainBuffers> m_nFenceValues = {};

public:
	static UINT g_nCBVSRVDescriptorIncrementSize;
};
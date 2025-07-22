#include "stdafx.h"
#include "D3DCore.h"

UINT D3DCore::g_nCBVSRVDescriptorIncrementSize = 0;

D3DCore::D3DCore(BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	CreateD3DDevice(bEnableDebugLayer);
	CreateCommandQueueAndList();
	CreateRTVAndDSVDescriptorHeaps();
	CreateSwapChain();
	CreateDepthStencilView();

	g_nCBVSRVDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DCore::CreateD3DDevice(bool bEnableDebugLayer)
{
	HRESULT hr;

	UINT nDXGIFactoryFlags = 0;
	if (bEnableDebugLayer) {
		ComPtr<ID3D12Debug> pd3dDebugController = nullptr;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(pd3dDebugController.GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("D3D12GetDebugInterface failed");
		}
		nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		if (pd3dDebugController) {
			pd3dDebugController->EnableDebugLayer();
		}

		m_bEnableDebugLayer = bEnableDebugLayer;
	}

	// DXGI Factory
	hr = CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(m_pdxgiFactory.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("CreateDXGIFactory2 failed");
	}

	// Create DXGIAdapter
	ComPtr<IDXGIAdapter> pdxgiAdapter = nullptr;
	DXGI_ADAPTER_DESC1 adapterDesc{};
	size_t bestMemory = 0;
	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		ComPtr<IDXGIAdapter1> pCurAdapter = nullptr;
		if (m_pdxgiFactory->EnumAdapters1(adapterIndex, pCurAdapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) break;

		DXGI_ADAPTER_DESC1 curAdapterDesc{};
		if (FAILED(pCurAdapter->GetDesc1(&curAdapterDesc))) __debugbreak();

		size_t curMemory = curAdapterDesc.DedicatedVideoMemory / (1024 * 1024);

		if (curMemory > bestMemory)
		{
			bestMemory = curMemory;
			pdxgiAdapter = pCurAdapter;
			adapterDesc = curAdapterDesc;
		}
	}

	std::array<D3D_FEATURE_LEVEL, 3> featureLevels =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	// Device
	for (int i = 0; i < featureLevels.size(); i++) {
		hr = D3D12CreateDevice(pdxgiAdapter.Get(), featureLevels[i], IID_PPV_ARGS(m_pd3dDevice.GetAddressOf()));
		if (SUCCEEDED(hr))
			break;
	}

	if (!m_pd3dDevice) {
		m_pdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(pdxgiAdapter.GetAddressOf()));
		hr = D3D12CreateDevice(pdxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pd3dDevice.GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("D3D12CreateDevice failed");
		}
	}

	// MSAA
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	{
		d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dMsaaQualityLevels.SampleCount = 4;
		d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		d3dMsaaQualityLevels.NumQualityLevels = 0;
	}

	hr = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	if (SUCCEEDED(hr)) {
		m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
		m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;
	}
	else {
		::OutputDebugString(L"Device doesn't support MSAA4x");
	}

	// Fence
	hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create fence");
	}

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

}

void D3DCore::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	{
		dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
		dxgiSwapChainDesc.BufferDesc.Width = WinCore::sm_dwClientWidth;
		dxgiSwapChainDesc.BufferDesc.Height = WinCore::sm_dwClientHeight;
		dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		dxgiSwapChainDesc.OutputWindow = WinCore::sm_hWnd;
		dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
		dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
		dxgiSwapChainDesc.Windowed = TRUE;

		// Set backbuffer resolution as fullscreen resolution.
		dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}

	ComPtr<IDXGISwapChain> pSwapChain;
	HRESULT hr = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue.Get(), &dxgiSwapChainDesc, pSwapChain.GetAddressOf());
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create SwapChain");
	}

	pSwapChain->QueryInterface(IID_PPV_ARGS(m_pdxgiSwapChain.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create SwapChain QueryInterface");
	}

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	m_pdxgiFactory->MakeWindowAssociation(WinCore::sm_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void D3DCore::CreateRTVAndDSVDescriptorHeaps()
{
	HRESULT hr{};

	// Create DescriptorHeap for RTV
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc{};
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	{
		d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
		d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		d3dDescriptorHeapDesc.NodeMask = 0;
	}
	hr = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(m_pd3dRTVDescriptorHeap.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create RTV DescriptorHeap");
	}


	// Set descriptor increment size
	m_nRTVDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create DescriptorHeap for DSV
	{
		d3dDescriptorHeapDesc.NumDescriptors = 1;
		d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	}
	hr = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(m_pd3dDSVDescriptorHeap.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create DSV DescriptorHeap");
	}

	// Set descriptor increment size
	m_nDSVDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}

void D3DCore::CreateCommandQueueAndList()
{
	HRESULT hr{};

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}

	// Create Command Allocator
	hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pd3dCommandAllocator.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandAllocator");
	}

	// Create Command List
	hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator.Get(), NULL, IID_PPV_ARGS(m_pd3dCommandList.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandList");
	}

	// Close Command List(default is opened)
	hr = m_pd3dCommandList->Close();
}

void D3DCore::CreateRenderTargetViews()
{
	HRESULT hr{};

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = m_pd3dRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; ++i)
	{
		hr = m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pd3dRenderTargetBuffers[i].GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to get buffer from SwapChain");
		}

		m_pd3dDevice->CreateRenderTargetView(m_pd3dRenderTargetBuffers[i].Get(), NULL, d3dRTVCPUDescriptorHandle);
		d3dRTVCPUDescriptorHandle.ptr += m_nRTVDescriptorIncrementSize;
	}
}

void D3DCore::CreateDepthStencilView()
{
	HRESULT hr{};

	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dResourceDesc.Alignment = 0;
		d3dResourceDesc.Width = WinCore::sm_dwClientWidth;
		d3dResourceDesc.Height = WinCore::sm_dwClientHeight;
		d3dResourceDesc.DepthOrArraySize = 1;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
		d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	{
		d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 1;
		d3dHeapProperties.VisibleNodeMask = 1;
	}

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.f;
	d3dClearValue.DepthStencil.Stencil = 0;

	// Create Depth Stencil Buffer
	hr = m_pd3dDevice->CreateCommittedResource(
		&d3dHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&d3dClearValue,
		IID_PPV_ARGS(m_pd3dDepthStencilBuffer.GetAddressOf())
	);
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create depth-stencil buffer");
	}

	// Create Depth Stencil Buffer View
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDSVCPUDescriptorHandle = m_pd3dDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer.Get(), NULL, d3dDSVCPUDescriptorHandle);
}

void D3DCore::ChangeSwapChainState()
{
	WaitForGPUComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	{
		dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dxgiTargetParameters.Width = WinCore::sm_dwClientWidth;
		dxgiTargetParameters.Height = WinCore::sm_dwClientHeight;
		dxgiTargetParameters.RefreshRate.Numerator = 60;
		dxgiTargetParameters.RefreshRate.Denominator = 1;
		dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	}
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		if (m_pd3dRenderTargetBuffers[i])
			m_pd3dRenderTargetBuffers[i].Reset();
	}

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight,
		dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

ComPtr<ID3D12Device14> D3DCore::GetDevice() const
{
	return m_pd3dDevice;
}

ComPtr<ID3D12GraphicsCommandList> D3DCore::GetCommandList() const
{
	return m_pd3dCommandList;
}

void D3DCore::RenderBegin()
{
	HRESULT hr;

	hr = m_pd3dCommandAllocator->Reset();
	hr = m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), NULL);
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandList");
	}

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	{
		d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier.Transition.pResource = m_pd3dRenderTargetBuffers[m_nSwapChainBufferIndex].Get();
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = m_pd3dRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRTVCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRTVDescriptorIncrementSize);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = m_pd3dDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVDescriptorHandle);

	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRTVCPUDescriptorHandle, pfClearColor, 0, NULL);

	m_pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, NULL);
}

void D3DCore::Render()
{
}

void D3DCore::RenderEnd()
{
	HRESULT hr;

	// Change rendered render target's resource state from D3D12_RESOURCE_STATE_RENDER_TARGET to D3D12_RESOURCE_STATE_PRESENT
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	{
		d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier.Transition.pResource = m_pd3dRenderTargetBuffers[m_nSwapChainBufferIndex].Get();
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	m_pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGPUComplete();
}

void D3DCore::Present()
{
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;

	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
}

void D3DCore::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue) {
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3DCore::WaitForGPUComplete()
{
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

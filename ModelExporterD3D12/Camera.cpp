#include "stdafx.h"
#include "Camera.h"

Camera::Camera(ComPtr<ID3D12Device14> m_pd3dDevice)
{
	m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(::AlignConstantBuffersize(sizeof(XMFLOAT4X4))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pCBViewProj.GetAddressOf())
	);

	m_pCBViewProj->Map(0, NULL, reinterpret_cast<void**>(&m_pMappedData));

	GenerateViewMatrix();

	XMMATRIX xmmtxProjection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(90.f),
		WinCore::g_dwClientWidth / WinCore::g_dwClientHeight,
		1.01f,
		500.f
	);

	XMStoreFloat4x4(&m_xmf4x4Projection, xmmtxProjection);

	m_d3dViewport = { 0.f, 0.f, (float)WinCore::g_dwClientWidth, (float)WinCore::g_dwClientHeight, 0.f, 1.f };
	m_d3dScissorRect = { 0, 0, (LONG)WinCore::g_dwClientWidth, (LONG)WinCore::g_dwClientHeight };
}

void Camera::Update()
{
	ProcessInput();
	ImGui::Begin("Cam Debug");

	ImGui::Text("Cam Position { %.3f, %.3f, %.3f }", m_xmf3CamPosition.x, m_xmf3CamPosition.y, m_xmf3CamPosition.z);
	ImGui::Text("Cam Rotation { %.3f, %.3f, %.3f }", m_xmf3CamRotation.x, m_xmf3CamRotation.y, m_xmf3CamRotation.z);

	ImGui::Text("Cam Right { %.3f, %.3f, %.3f }", m_xmf3Right.x, m_xmf3Right.y, m_xmf3Right.z);
	ImGui::Text("Cam Up { %.3f, %.3f, %.3f }", m_xmf3Up.x, m_xmf3Up.y, m_xmf3Up.z);
	ImGui::Text("Cam Look { %.3f, %.3f, %.3f }", m_xmf3Look.x, m_xmf3Look.y, m_xmf3Look.z);


	ImGui::End();

}

void Camera::ProcessInput()
{
	// Key
	float fMovingSpeed = 0.05f;

	if (INPUT->GetButtonPressed('W')) {
		XMVECTOR newPos = XMVectorMultiplyAdd(XMLoadFloat3(&m_xmf3Look), XMVectorReplicate(fMovingSpeed), XMLoadFloat3(&m_xmf3CamPosition));
		XMStoreFloat3(&m_xmf3CamPosition, newPos);
	}
	if (INPUT->GetButtonPressed('S')) {
		XMVECTOR newPos = XMVectorMultiplyAdd(XMLoadFloat3(&m_xmf3Look), XMVectorReplicate(-fMovingSpeed), XMLoadFloat3(&m_xmf3CamPosition));
		XMStoreFloat3(&m_xmf3CamPosition, newPos);
	}

	if (INPUT->GetButtonPressed('A')) {
		XMVECTOR newPos = XMVectorMultiplyAdd(XMLoadFloat3(&m_xmf3Right), XMVectorReplicate(-fMovingSpeed), XMLoadFloat3(&m_xmf3CamPosition));
		XMStoreFloat3(&m_xmf3CamPosition, newPos);
	}
	if (INPUT->GetButtonPressed('D')) {
		XMVECTOR newPos = XMVectorMultiplyAdd(XMLoadFloat3(&m_xmf3Right), XMVectorReplicate(fMovingSpeed), XMLoadFloat3(&m_xmf3CamPosition));
		XMStoreFloat3(&m_xmf3CamPosition, newPos);
	}

	if (INPUT->GetButtonPressed('E')) {
		XMVECTOR newPos = XMVectorMultiplyAdd(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMVectorReplicate(fMovingSpeed), XMLoadFloat3(&m_xmf3CamPosition));
		XMStoreFloat3(&m_xmf3CamPosition, newPos);
	}
	if (INPUT->GetButtonPressed('Q')) {
		XMVECTOR newPos = XMVectorMultiplyAdd(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMVectorReplicate(-fMovingSpeed), XMLoadFloat3(&m_xmf3CamPosition));
		XMStoreFloat3(&m_xmf3CamPosition, newPos);
	}

	// Mouse

	if (INPUT->GetButtonPressed(VK_RBUTTON)) {
		HWND hWnd = ::GetActiveWindow();

		::SetCursor(NULL);

		RECT rtClientRect;
		::GetClientRect(hWnd, &rtClientRect);
		::ClientToScreen(hWnd, (LPPOINT)&rtClientRect.left);
		::ClientToScreen(hWnd, (LPPOINT)&rtClientRect.right);

		int nScreenCenterX = 0, nScreenCenterY = 0;
		nScreenCenterX = rtClientRect.left + WinCore::g_dwClientWidth / 2;
		nScreenCenterY = rtClientRect.top + WinCore::g_dwClientHeight / 2;

		POINT ptCursorPos;
		::GetCursorPos(&ptCursorPos);

		POINT ptDelta{ (ptCursorPos.x - nScreenCenterX), (ptCursorPos.y - nScreenCenterY) };

		XMFLOAT3 xmf3AddRotation{ ptDelta.y * 0.10f, ptDelta.x * 0.10f, 0.f };
		XMStoreFloat3(&m_xmf3CamRotation, XMVectorAdd(XMLoadFloat3(&m_xmf3CamRotation), XMLoadFloat3(&xmf3AddRotation)));

		::SetCursorPos(nScreenCenterX, nScreenCenterY);
	}

	GenerateViewMatrix();
}

void Camera::GenerateViewMatrix()
{
	// Update View
	float fPitch, fYaw, fRoll;
	fPitch = XMConvertToRadians(m_xmf3CamRotation.x);
	fYaw = XMConvertToRadians(m_xmf3CamRotation.y);
	fRoll = XMConvertToRadians(m_xmf3CamRotation.z);
	XMMATRIX xmmtxWorld = XMMatrixRotationRollPitchYaw(fPitch, fYaw, fRoll);
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, xmmtxWorld);

	m_xmf3Right = XMFLOAT3{ xmf4x4World._11, xmf4x4World._12, xmf4x4World._13 };
	m_xmf3Up = XMFLOAT3{ xmf4x4World._21, xmf4x4World._22, xmf4x4World._23 };
	m_xmf3Look = XMFLOAT3{ xmf4x4World._31, xmf4x4World._32, xmf4x4World._33 };

	m_xmf4x4View._11 = m_xmf3Right.x; m_xmf4x4View._12 = m_xmf3Up.x; m_xmf4x4View._13 = m_xmf3Look.x;
	m_xmf4x4View._21 = m_xmf3Right.y; m_xmf4x4View._22 = m_xmf3Up.y; m_xmf4x4View._23 = m_xmf3Look.y;
	m_xmf4x4View._31 = m_xmf3Right.z; m_xmf4x4View._32 = m_xmf3Up.z; m_xmf4x4View._33 = m_xmf3Look.z;
	m_xmf4x4View._41 = -XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_xmf3CamPosition), XMLoadFloat3(&m_xmf3Right)));
	m_xmf4x4View._42 = -XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_xmf3CamPosition), XMLoadFloat3(&m_xmf3Up)));
	m_xmf4x4View._43 = -XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_xmf3CamPosition), XMLoadFloat3(&m_xmf3Look)));

	// No need to update Projection

	// View * Proj
	XMStoreFloat4x4(&m_xmf4x4ViewProjectionTransposed, XMMatrixMultiplyTranspose(XMLoadFloat4x4(&m_xmf4x4View), XMLoadFloat4x4(&m_xmf4x4Projection)));
}

// D3D12Framework.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include "stdafx.h"
#include "GameFramework.h"

std::unique_ptr<GuiHandler> GameFramework::g_pGuiHandler = nullptr;
std::unique_ptr<InputManager> GameFramework::g_pInputManager = nullptr;

GameFramework::GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV)
	: m_pD3DCore{ std::make_shared<D3DCore>(bEnableDebugLayer, bEnableGBV) }
{
	// Init managers
	g_pGuiHandler = std::make_unique<GuiHandler>();
	g_pGuiHandler->Initialize(m_pD3DCore->GetDevice());

	g_pInputManager = std::make_unique<InputManager>();
	g_pInputManager->Initialize(WinCore::g_hWnd);

	m_pImporter = std::make_unique<AssimpImporter>(m_pD3DCore->GetDevice());
	m_pImporter->LoadFBXFilesFromPath("../../Models");
	//m_pImporter->LoadModel("../../Models/Sporty Granny.fbx");
}

void GameFramework::Update()
{
	INPUT->Update();
	g_pGuiHandler->Update();
	m_pImporter->Run();
}

void GameFramework::Render()
{
	m_pD3DCore->RenderBegin();

	// TODO : Render Logic Here
	m_pImporter->RenderLoadedObject(m_pD3DCore->GetCommandList());
	g_pGuiHandler->Render(m_pD3DCore->GetCommandList());

	m_pD3DCore->RenderEnd();

	m_pD3DCore->Present();
	m_pD3DCore->MoveToNextFrame();
}

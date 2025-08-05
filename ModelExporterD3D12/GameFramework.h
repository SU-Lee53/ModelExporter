#pragma once
#include "D3DCore.h"
#include "GuiHandler.h"
#include "InputManager.h"
#include "AssimpImporter.h"

class GameFramework {
public:
	GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV);

	void Update();
	void Render();

private:
	std::wstring wstrGameName = L"MODEL_IMPORTER";

private:
	std::shared_ptr<D3DCore> m_pD3DCore = nullptr;

public:
	static std::unique_ptr<AssimpImporter> g_pImporter;
	static std::unique_ptr<GuiHandler> g_pGuiHandler;
	static std::unique_ptr<InputManager> g_pInputManager;
	static std::unique_ptr<GameTimer> g_GameTimer;

};

#define GUI			GameFramework::g_pGuiHandler
#define INPUT		GameFramework::g_pInputManager
#define IMPORTER	GameFramework::g_pImporter
#define TIMER		GameFramework::g_GameTimer


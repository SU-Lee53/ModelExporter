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
	std::shared_ptr<D3DCore> m_pD3DCore = nullptr;
	std::unique_ptr<AssimpImporter> m_pImporter;

public:
	static std::unique_ptr<GuiHandler> g_pGuiHandler;
	static std::unique_ptr<InputManager> g_pInputManager;

};

#define GUI			GameFramework::g_pGuiHandler
#define INPUT		GameFramework::g_pInputManager


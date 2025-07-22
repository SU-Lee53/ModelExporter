#pragma once
#include "D3DCore.h"
#include "GuiHandler.h"
#include "AssimpImporter.h"

class GameFramework {
public:
	GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV);

	void Update();
	void Render();

private:
	std::shared_ptr<D3DCore> m_pD3DCore = nullptr;

	static std::unique_ptr<GuiHandler> g_pGuiHandler;
	std::unique_ptr<AssimpImporter> m_pImporter;


};

#define GUI			GameFramework::g_pGuiHandler


// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef min
#undef max

// STL Essentials
#include <iostream>
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <span>
#include <algorithm>
#include <type_traits>
#include <ranges>
#include <concepts>
#include <utility>
#include <print>
#include <filesystem>


// Direct3D related headers
#include <wrl.h>
#include <shellapi.h>	

// D3D12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#include "../d3dx/d3dx12.h"


// D3DCompiler
#include <d3dcompiler.h>

// DirectXMath
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

// DirectXTex
#include "DDSTextureLoader12.h"
#include "WICTextureLoader12.h"
#include "../library/include/DirectXTex/DirectXTex.h"
#include "../library/include/DirectXTex/DirectXTex.inl"
#include <wincodec.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

// Import libraries
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// ImGUI
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx12.h"
#include "../ImGui/imgui_stdlib.h"

// Assimp
#include "../library/include/assimp/Importer.hpp"
#include "../library/include/assimp/scene.h"
#include "../library/include/assimp/postprocess.h"
#include "../library/include/assimp/DefaultLogger.hpp"
#include "../library/include/assimp/importerdesc.h"
#include "../library/include/assimp/pbrmaterial.h"

// lib link
#if defined(_DEBUG)
#pragma comment(lib, "../library/lib/DirectXTex/Debug/DirectXTex.lib")
#pragma comment(lib, "../library/lib/assimp/x64/assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "../library/lib/DirectXTex/Release/DirectXTex.lib")
#pragma comment(lib, "../library/lib/assimp/x64/assimp-vc143-mt.lib")
#endif

// Additional Helper Headers
#include "Defines.h"
#include "Concepts.h"
#include "Utility.h"

// Managers

// Game Headers
#include "WinCore.h"
#include "D3DCore.h"
#include "GameFramework.h"

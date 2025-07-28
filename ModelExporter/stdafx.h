#pragma once

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// STL
#include <iostream>
#include <fstream>
#include <print>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <filesystem>
#include <variant>
using namespace std::literals;

// Math
#include <DirectXMath.h>
using namespace DirectX;

#if defined(_DEBUG)
#pragma comment(lib,  "../library/lib/assimp/x64/assimp-vc143-mtd.lib")
#else
#pragma comment(lib,  "../library/lib/assimp/x64/assimp-vc143-mt.lib")
#endif

#include "../library/include/assimp/Importer.hpp"
#include "../library/include/assimp/scene.h"
#include "../library/include/assimp/postprocess.h"
#include "../library/include/assimp/DefaultLogger.hpp"
#include "../library/include/assimp/importerdesc.h"

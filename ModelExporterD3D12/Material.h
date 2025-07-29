#pragma once

struct MATERIAL_IMPORT_INFO {
	XMFLOAT4		xmf4AlbedoColor;
	XMFLOAT4		xmf4SpecularColor;
	XMFLOAT4		xmf4AmbientColor;
	XMFLOAT4		xmf4EmissiveColor;
	
	float			fGlossiness = 0.0f;
	float			fSmoothness = 0.0f;
	float			fSpecularHighlight = 0.0f;
	float			fMetallic = 0.0f;
	float			fGlossyReflection = 0.0f;
	
	std::string		strAlbedoMapName;
	std::string		strSpecularMapName;
	std::string		strMetallicMapName;
	std::string		strNormalMapName;
	std::string		strEmissionMapName;
	std::string		strDetailAlbedoMapName;
	std::string		strDetailNormalMapName;
};

class Material
{
};


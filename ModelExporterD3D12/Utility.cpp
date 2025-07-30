#include "stdafx.h"
#include "Utility.h"

void ShowErrorMessage(std::string_view file, int line, std::string_view message)
{
	std::string strFileMsg{ file };
	strFileMsg += '\n';

	std::string strLineMsg = "Line : " + std::to_string(line);
	strLineMsg += '\n';

	std::string strDebugMsg{ message };
	strDebugMsg += '\n';

	::OutputDebugStringA("*************** ERROR!! ***************\n");
	::OutputDebugStringA(strFileMsg.data());
	::OutputDebugStringA(strLineMsg.data());
	::OutputDebugStringA(strDebugMsg.data());
	::OutputDebugStringA("***************************************\n");

}

std::string FormatMatrix(const aiMatrix4x4& aimtx)
{
	XMFLOAT4X4 xmf4x4Matrix((float*)&aimtx.a1);

	return std::format("R1 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\nR2 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\nR3 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\nR4 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\n",
		xmf4x4Matrix._11, xmf4x4Matrix._12, xmf4x4Matrix._13, xmf4x4Matrix._14,
		xmf4x4Matrix._21, xmf4x4Matrix._22, xmf4x4Matrix._23, xmf4x4Matrix._24,
		xmf4x4Matrix._31, xmf4x4Matrix._32, xmf4x4Matrix._33, xmf4x4Matrix._34,
		xmf4x4Matrix._41, xmf4x4Matrix._42, xmf4x4Matrix._43, xmf4x4Matrix._44
	);

}

std::string FormatMatrix(const XMFLOAT4X4& xmf4x4Matrix)
{
	return std::format("R1 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\nR2 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\nR3 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\nR4 : [{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]\n",
		xmf4x4Matrix._11, xmf4x4Matrix._12, xmf4x4Matrix._13, xmf4x4Matrix._14,
		xmf4x4Matrix._21, xmf4x4Matrix._22, xmf4x4Matrix._23, xmf4x4Matrix._24,
		xmf4x4Matrix._31, xmf4x4Matrix._32, xmf4x4Matrix._33, xmf4x4Matrix._34,
		xmf4x4Matrix._41, xmf4x4Matrix._42, xmf4x4Matrix._43, xmf4x4Matrix._44
	);
}

std::string FormatVector2(const XMFLOAT2& xmf2Vector)
{
	return std::format("[{: 5.3f}, {: 5.3f} ]",
		xmf2Vector.x, xmf2Vector.y);
}

std::string FormatVector3(const XMFLOAT3& xmf3Vector)
{
	return std::format("[{: 5.3f}, {: 5.3f}, {: 5.3f} ]",
		xmf3Vector.x, xmf3Vector.y, xmf3Vector.z);
}

std::string FormatVector4(const XMFLOAT4& xmf4Vector)
{
	return std::format("[{: 5.3f}, {: 5.3f}, {: 5.3f}, {: 5.3f} ]",
		xmf4Vector.x, xmf4Vector.y, xmf4Vector.z, xmf4Vector.w);
}

inline size_t AlignConstantBuffersize(size_t size)
{
	size_t alligned_size = (size + 255) & (~255);
	return alligned_size;
}

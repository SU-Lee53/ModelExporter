#pragma once

extern void ShowErrorMessage(std::string_view file, int line, std::string_view message);
#define SHOW_ERROR(strMsg)		ShowErrorMessage(__FILE__, __LINE__, strMsg);

std::string FormatMatrix(const aiMatrix4x4& aimtx);
std::string FormatMatrix(const XMFLOAT4X4& xmf4x4Matrix);

std::string FormatVector2(const XMFLOAT2& xmf2Vector);
std::string FormatVector3(const XMFLOAT3& xmf3Vector);
std::string FormatVector4(const XMFLOAT4& xmf4Vector);


extern inline size_t AlignConstantBuffersize(size_t size);

template<typename T>
struct ConstantBufferSize {
	constexpr static size_t value = (sizeof(T) + 255) & (~255);
};
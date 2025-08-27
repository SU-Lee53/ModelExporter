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

inline XMFLOAT4X4 aiMatrixToXMFLOAT4X4(aiMatrix4x4 m) {
    return XMFLOAT4X4(
        (float)m.a1, (float)m.b1, (float)m.c1, (float)m.d1,
        (float)m.a2, (float)m.b2, (float)m.c2, (float)m.d2,
        (float)m.a3, (float)m.b3, (float)m.c3, (float)m.d3,
        (float)m.a4, (float)m.b4, (float)m.c4, (float)m.d4
    );
}


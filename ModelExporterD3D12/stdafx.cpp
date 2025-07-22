#include "stdafx.h"

inline size_t AlignConstantBuffersize(size_t size)
{
	size_t alligned_size = (size + 255) & (~255);
	return alligned_size;
}

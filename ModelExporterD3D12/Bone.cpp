#include "stdafx.h"
#include "Bone.h"

std::shared_ptr<Bone> Bone::LoadFromInfo(const BONE_IMPORT_INFO& boneInfo)
{
	std::shared_ptr<Bone> pBone = std::make_shared<Bone>();

	pBone->strName = boneInfo.strName;
	pBone->xmf4x4Offset = boneInfo.xmf4x4Offset;

	return pBone;
}

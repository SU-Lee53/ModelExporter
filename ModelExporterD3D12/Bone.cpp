#include "stdafx.h"
#include "Bone.h"

std::shared_ptr<Bone> Bone::LoadFromInfo(const BONE_IMPORT_INFO& info)
{
	std::shared_ptr<Bone> pBone = std::make_shared<Bone>();

	//pBone->m_strName = info.strName;
	//pBone->m_xmf4x4Offset = info.xmf4x4Offset;
	//pBone->m_Weights = info.weights;

	return pBone;
}

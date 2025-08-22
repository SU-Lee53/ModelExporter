#include "stdafx.h"
#include "Animation.h"

void AnimationNode::Initialize()
{

}

void AnimationController::Initialize()
{
	// 1. make initial transform data array
	SB_ANIMATION_TRANSFORM_DATA sbData{};

	for (auto&& [idx, animData] : AnimationDatas | std::views::enumerate) {
		for (const auto& channel : animData.channels) {
			size_t nKeyframes = std::max(std::max(channel.keyframes.posKeys.size(), channel.keyframes.rotKeys.size()), channel.keyframes.scaleKeys.size());

			std::generate_n(std::begin(sbData.xmf4x4Transforms[idx]), nKeyframes, [&channel]() {
				return channel.keyframes.xmf4x4FinalTransform;
			});


			channel.keyframes.xmf4x4FinalTransform;

		}



	}



}

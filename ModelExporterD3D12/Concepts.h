#pragma once

template <typename C>
concept Bindable = requires {
	&C::Bind;
};
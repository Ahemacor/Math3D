#pragma once
#include <DirectXMath.h>

struct CB_VS_vertexshader
{
	DirectX::XMMATRIX wvp;
	std::uint32_t enableSpherical = 0;
};
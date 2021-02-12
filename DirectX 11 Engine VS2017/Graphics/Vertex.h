#pragma once
#include <DirectXMath.h>

struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z, float u, float v)
		: pos(x, y, z), texCoord(u, v) {}

	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 texCoord;
};

struct Vertex_COLOR
{
	Vertex_COLOR() {}
	Vertex_COLOR(float x, float y, float z, float r, float g, float b)
		: pos(x, y, z), color(r, g, b) {}

	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 color;
};

struct VertexCommon
{
	VertexCommon() = default;
	VertexCommon(float x, float y = 0, float z = 0, 
		         float r = 1, float g = 1, float b = 1, float a = 1,
		         float u = 0, float v = 0)
	: pos(x, y, z)
	, color(r, g, b, a)
	, texCoord(u, v) {}

	DirectX::XMFLOAT3 pos = { 0, 0, 0 };
	DirectX::XMFLOAT4 color = { 0, 0, 0, 0 };
	DirectX::XMFLOAT2 texCoord = { 0, 0 };
};
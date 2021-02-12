#pragma once
#include "AdapterReader.h"
#include "Shaders.h"
#include "Vertex.h"
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <WICTextureLoader.h>
#include "ConstantBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <vector>

class Graphics
{
public:
	bool Initialize(HWND hwnd, int width, int height);
	void RenderFrame();
	Camera camera;

private:
	void RenderFunctionsImGui();
	bool InitializeDirectX(HWND hwnd, int width, int height);
	bool InitializeShaders();
	bool InitializeScene();

	void InitArhimedeslModel();
	void UpdateArhimedesModel(float a, float t_min, float t_max, const XMFLOAT4& color);

	void InitFermatModel();
	void UpdateFermatModel(float a, float t_min, float t_max, const XMFLOAT4& color);

	void InitLemniscateOfBernoulliModel();
	void UpdateLemniscateOfBernoulliModel(float a, float t_min, float t_max, float phi_scale, const XMFLOAT4& color);

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;

	VertexShader vs_3d_colors;
	PixelShader ps_3d_colors;

	VertexShader commonVS;
	PixelShader coloredPS;
	PixelShader texturedPS;

	VertexBuffer<Vertex_COLOR> vb_grid;

	ConstantBuffer<CB_VS_vertexshader> cb_vs_vertexshader;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;

	int windowWidth = 0;
	int windowHeight = 0;

	bool renderXYaxis = true;
	bool renderXZaxis = true;

	enum FuntionType { NONE, ARHIMEDES, FERMAT, BERNOULLI };
	FuntionType funcType = ARHIMEDES;

	struct Model
	{
		VertexShader vs;
		PixelShader ps;
		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		DirectX::XMMATRIX transformatin = XMMatrixIdentity();
		VertexBuffer<VertexCommon> vertices;
		IndexBuffer indices;
		ConstantBuffer<CB_VS_vertexshader> cb;

		void draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext, Camera& camera)
		{
			const UINT offset = 0;
			cb.data.wvp = transformatin * camera.GetViewMatrix() * camera.GetProjectionMatrix();
			cb.data.wvp = XMMatrixTranspose(cb.data.wvp);
			cb.ApplyChanges();

			deviceContext->IASetPrimitiveTopology(topology);
			deviceContext->IASetInputLayout(vs.GetInputLayout());
			deviceContext->VSSetShader(vs.GetShader(), NULL, 0);
			deviceContext->PSSetShader(ps.GetShader(), NULL, 0);
			deviceContext->IASetVertexBuffers(0, 1, vertices.GetAddressOf(), vertices.Stride(), &offset);
			deviceContext->IASetIndexBuffer(indices.Get(), DXGI_FORMAT_R32_UINT, 0);
			deviceContext->VSSetConstantBuffers(0, 1, cb.GetAddressOf());
			deviceContext->DrawIndexed(this->indices.BufferSize(), 0, 0);
		}
	};

	const float t_num = 100000;
	Model arhimedesModel;
	Model fermatModel;
	Model lemniscateOfBernoulliModel;
};
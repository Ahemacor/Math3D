#include "Graphics.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>

auto lerp = [](float a, float b, float f) -> float
{
	return a + f * (b - a);
};

bool Graphics::Initialize(HWND hwnd, int width, int height)
{
	this->windowWidth = width;
	this->windowHeight = height;
	if (!InitializeDirectX(hwnd, width, height))
		return false;

	if (!InitializeShaders())
		return false;

	if (!InitializeScene())
		return false;

	return true;
}

void Graphics::RenderFunctionsImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	static bool my_tool_active = true;
	ImGui::Begin("Options window", &my_tool_active, ImGuiWindowFlags_MenuBar);

	ImGui::Checkbox("Render X-Y Axis", &renderXYaxis);
	ImGui::Checkbox("Render X-Z Axis", &renderXZaxis);
	ImGui::NewLine();

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Functions"))
		{
			if (ImGui::MenuItem("Empty", "")) { funcType = NONE; }
			if (ImGui::MenuItem("Archimedes spiral", "r = a*phi")) { funcType = ARHIMEDES; }
			if (ImGui::MenuItem("Fermat's spiral", "r = a*sqrt(phi)")) { funcType = FERMAT; }
			if (ImGui::MenuItem("Lemniscate of Bernoulli", "r^2 = a^2 * cos(2*phi)")) { funcType = BERNOULLI; }
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	switch(funcType)
	{
	case ARHIMEDES:
	{
		enum { MIN, MAX, A };
		static float param[] = { 0, 3.14f*10.0, 0.33f };

		ImGui::Text("Archimedes spiral:");
		ImGui::Text("r = %f * phi;    phi[%f, %f]", param[A], param[MIN], param[MAX]);

		ImGui::SliderFloat("Min", &param[MIN], -3.14f*100, 3.14f*100);
		ImGui::SliderFloat("Max", &param[MAX], -3.14f*100, 3.14f*100);
		ImGui::SliderFloat("a", &param[A], -3.14f*3.0f, 3.14f*3.0f);

		static XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		ImGui::ColorEdit4("Color", (float*)&color);
		ImGui::Text("Z coordinate: %f", zCoord);
		ImGui::SliderFloat("Z", &zCoord, -15.0f, +15.0f);
		static bool enableSpherical = 0;
		ImGui::Checkbox("Enable spherical coordinates", &enableSpherical);
		arhimedesModel.cb.data.enableSpherical = enableSpherical;
		if (ImGui::Button("Apply changes")) UpdateArhimedesModel(param[A], param[MIN], param[MAX], color);
	}
	break;

	case FERMAT:
	{
		enum { MIN, MAX, A };
		static float param[] = { 0.0f, 3.14f*10.0, 2.5f };

		ImGui::Text("Fermat's spiral:");
		ImGui::Text("r = %f*sqrt(phi);    phi[%f, %f]", param[A], param[MIN], param[MAX]);

		ImGui::SliderFloat("Max", &param[MAX], 0.0f, 250.0f);
		ImGui::SliderFloat("a", &param[A], -3.14f*3.0f, 3.14f*3.0f);

		static XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		ImGui::ColorEdit4("Color", (float*)&color);
		ImGui::Text("Z coordinate: %f", zCoord);
		ImGui::SliderFloat("Z", &zCoord, -15.0f, +15.0f);
		static bool enableSpherical = 0;
		ImGui::Checkbox("Enable spherical coordinates", &enableSpherical);
		fermatModel.cb.data.enableSpherical = enableSpherical;
		if (ImGui::Button("Apply changes")) UpdateFermatModel(param[A], param[MIN], param[MAX], color);
	}
	break;

	case BERNOULLI:
	{
		enum { MIN, MAX, A };
		static float param[] = { -3.14159f, 3.14159f, 5 };
		static int scale = 2;
		ImGui::Text("Lemniscate of Bernoulli:");
		ImGui::Text(" r^2 = %f^2 * cos(%d * phi);    phi[%f, %f]", param[A], scale, param[MIN], param[MAX]);
		ImGui::SliderFloat("Min", &param[MIN], -3.14159f, 0.0f);
		ImGui::SliderFloat("Max", &param[MAX], 0.0f, 3.14159f);
		ImGui::SliderFloat("a", &param[A], 0.0f, 20.0f);
		ImGui::SliderInt("scale", &scale, 0, 100);

		static XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		ImGui::ColorEdit4("Color", (float*)&color);
		ImGui::Text("Z coordinate: %f", zCoord);
		ImGui::SliderFloat("Z", &zCoord, -15.0f, +15.0f);
		static bool enableSpherical = 0;
		ImGui::Checkbox("Enable spherical coordinates", &enableSpherical);
		lemniscateOfBernoulliModel.cb.data.enableSpherical = enableSpherical;
		if (ImGui::Button("Apply changes")) UpdateLemniscateOfBernoulliModel(param[A], param[MIN], param[MAX], scale, color);
	}
	break;

	case NONE:
		break;
	default:
		break;
	}
	ImGui::NewLine();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::NewLine();
	ImGui::Text("Camera control: [WASD] [Space] [Z] [Hold right mouse button]");
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Graphics::InitArhimedeslModel()
{
	Model& model = arhimedesModel;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	model.transformatin = XMMatrixIdentity();

	const float a = 0.33f;
	float t_min = 0;
	float t_max = 3.14f * 10.0;

	std::vector<VertexCommon> vertices;
	for (int i = 0; i < t_num; ++i)
	{
		const float t = lerp(t_min, t_max, i / t_num);

		const float phi = t;
		const float r = a * phi;
		float x = r * cos(phi);
		float y = r * sin(phi);
		float z = zCoord;

		vertices.emplace_back(x, y, z);
	}

	HRESULT hr = model.vertices.Initialize(this->device.Get(), vertices.data(), vertices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create vertex buffer for ArhimedeslModel.");

	std::vector<DWORD> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	hr = model.indices.Initialize(this->device.Get(), indices.data(), indices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create indices buffer for ArhimedeslModel.");

	hr = model.cb.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create constant buffer for ArhimedeslModel.");
}

void Graphics::UpdateArhimedesModel(float a, float t_min, float t_max, const XMFLOAT4& color)
{
	Model& model = arhimedesModel;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP;
	model.transformatin = XMMatrixIdentity();

	std::vector<VertexCommon> vertices;
	for (int i = 0; i < t_num; ++i)
	{
		const float t = lerp(t_min, t_max, i / t_num);
		const float phi = t;
		const float r = a * phi;
		float x = r * cos(phi);
		float y = r * sin(phi);
		float z = zCoord;

		vertices.emplace_back(x, y, z, color.x, color.y, color.z, color.w);
	}

	model.vertices.Update(deviceContext.Get(), vertices.data(), vertices.size());
}

void Graphics::InitFermatModel()
{
	Model& model = fermatModel;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP;
	model.transformatin = XMMatrixIdentity();

	float a = 2.5f;
	float t_min = 0;
	float t_max = 3.14f * 10.0;

	std::vector<VertexCommon> vertices;
	// - branch
	for (int i = (t_num / 2) - 1; i >= 0; --i)
	{
		const float t = lerp(t_min, t_max, i / (t_num/2));
		const float phi = t;
		const float r = a * -sqrt(phi);
		const float x = r * cos(phi);
		const float y = r * sin(phi);
		const float z = zCoord;
		vertices.emplace_back(x, y, z);
	}
	// + branch
	for (int i = 0; i < t_num/2; ++i)
	{
		const float t = lerp(t_min, t_max, i / (t_num/2));
		const float phi = t;
		const float r = a * +sqrt(phi);
		const float x = r * cos(phi);
		const float y = r * sin(phi);
		const float z = zCoord;
		vertices.emplace_back(x, y, z);
	}

	HRESULT hr = model.vertices.Initialize(this->device.Get(), vertices.data(), vertices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create vertex buffer for FermatModel.");

	std::vector<DWORD> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	hr = model.indices.Initialize(this->device.Get(), indices.data(), indices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create indices buffer for FermatModel.");

	hr = model.cb.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create constant buffer for FermatModel.");
}

void Graphics::UpdateFermatModel(float a, float t_min, float t_max, const XMFLOAT4& color)
{
	Model& model = fermatModel;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP;
	model.transformatin = XMMatrixIdentity();

	std::vector<VertexCommon> vertices;
	// - branch
	for (int i = (t_num / 2) - 1; i >= 0; --i)
	{
		const float t = lerp(t_min, t_max, i / (t_num/2));
		const float phi = t;
		const float r = a * -sqrt(phi);
		const float x = r * cos(phi);
		const float y = r * sin(phi);
		const float z = zCoord;
		vertices.emplace_back(x, y, z, color.x, color.y, color.z, color.w);
	}
	// + branch
	for (int i = 0; i < t_num / 2; ++i)
	{
		const float t = lerp(t_min, t_max, i / (t_num/2));
		const float phi = t;
		const float r = a * +sqrt(phi);
		const float x = r * cos(phi);
		const float y = r * sin(phi);
		const float z = zCoord;
		vertices.emplace_back(x, y, z, color.x, color.y, color.z, color.w);
	}

	model.vertices.Update(deviceContext.Get(), vertices.data(), vertices.size());
}

void Graphics::InitLemniscateOfBernoulliModel()
{
	Model& model = lemniscateOfBernoulliModel;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP;
	model.transformatin = XMMatrixIdentity();

	const float a = 5;
	const float t_min = 0;
	const float t_max = 1000;
	const float phi_scale = 2;
	const float a_pow = 2;

	// r^2 = a^2 * cos(2*phi)
	std::vector<VertexCommon> vertices;
	for (int i = 0; i < t_num; ++i)
	{
		const float t = lerp(t_min, t_max, i / t_num);
		const float phi = t;
		const float r = sqrt(pow(a, a_pow) * cos(phi*phi_scale));
		const float x = r * cos(phi);
		const float y = r * sin(phi);
		const float z = zCoord;
		vertices.emplace_back(x, y, z);
	}

	HRESULT hr = model.vertices.Initialize(this->device.Get(), vertices.data(), vertices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create vertex buffer for LemniscateOfBernoulliModel.");

	std::vector<DWORD> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	hr = model.indices.Initialize(this->device.Get(), indices.data(), indices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create indices buffer for LemniscateOfBernoulliModel.");

	hr = model.cb.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create constant buffer for LemniscateOfBernoulliModel.");
}

void Graphics::UpdateLemniscateOfBernoulliModel(float a, float t_min, float t_max, float phi_scale, const XMFLOAT4& color)
{
	Model& model = lemniscateOfBernoulliModel;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP;
	model.transformatin = XMMatrixIdentity();

	// r^2 = a^2 * cos(2*phi)
	std::vector<VertexCommon> vertices;
	for (int i = 0; i < t_num; ++i)
	{
		const float t = lerp(t_min, t_max, i / t_num);
		const float phi = t;
		const float r = sqrt(pow(a, 2) * cos(phi * phi_scale));
		const float x = r * cos(phi);
		const float y = r * sin(phi);
		const float z = zCoord;
		vertices.emplace_back(x, y, z, color.x, color.y, color.z, color.w);
	}

	model.vertices.Update(deviceContext.Get(), vertices.data(), vertices.size());
}

void Graphics::InitGridModels()
{
	const XMFLOAT4 gridColor = { 0.3f, 0.3f, 0.3f, 1.0f };

	Model& model = gridXY;
	model.vs = commonVS;
	model.ps = coloredPS;
	model.topology = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
	model.transformatin = XMMatrixIdentity();

	const int numOfLines = (gridMax - gridMin) / gridStep;
	const int pointsPerLine = 100;

	std::vector<VertexCommon> vertices;
	vertices.emplace_back(gridMin, 0, 0, gridColor.x*2, gridColor.y*2, gridColor.z*2, gridColor.w);
	vertices.emplace_back(gridMax, 0, 0, gridColor.x*2, gridColor.y*2, gridColor.z*2, gridColor.w);
	vertices.emplace_back(0, gridMin, 0, gridColor.x*2, gridColor.y*2, gridColor.z*2, gridColor.w);
	vertices.emplace_back(0, gridMax, 0, gridColor.x*2, gridColor.y*2, gridColor.z*2, gridColor.w);

	for (int i = 1; i <= numOfLines/2; ++i)
	{
		vertices.emplace_back(gridMin, gridStep*i, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);
		vertices.emplace_back(gridMax, gridStep*i, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);

		vertices.emplace_back(gridMin, -gridStep*i, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);
		vertices.emplace_back(gridMax, -gridStep*i, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);

		vertices.emplace_back(gridStep*i, gridMin, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);
		vertices.emplace_back(gridStep*i, gridMax, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);

		vertices.emplace_back(-gridStep*i, gridMin, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);
		vertices.emplace_back(-gridStep*i, gridMax, 0, gridColor.x, gridColor.y, gridColor.z, gridColor.w);
	}

	HRESULT hr = model.vertices.Initialize(this->device.Get(), vertices.data(), vertices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create vertex buffer for grid.");

	std::vector<DWORD> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	hr = model.indices.Initialize(this->device.Get(), indices.data(), indices.size());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create indices buffer for grid.");

	hr = model.cb.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr)) ErrorLogger::Log(hr, "Failed to create constant buffer for grid.");
}

void Graphics::RenderFrame()
{
	// Setup common rendering.
	float bgcolor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	this->deviceContext->ClearRenderTargetView(this->renderTargetView.Get(), bgcolor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	this->deviceContext->RSSetState(this->rasterizerState.Get());
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0);

	// Render grid lines:
	cb_vs_vertexshader.data.wvp = camera.GetViewMatrix() * camera.GetProjectionMatrix();
	cb_vs_vertexshader.data.wvp = XMMatrixTranspose(cb_vs_vertexshader.data.wvp);
	cb_vs_vertexshader.ApplyChanges();

	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
	this->deviceContext->IASetInputLayout(this->vs_3d_colors.GetInputLayout());
	this->deviceContext->VSSetShader(vs_3d_colors.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(ps_3d_colors.GetShader(), NULL, 0);
	this->deviceContext->VSSetConstantBuffers(0, 1, this->cb_vs_vertexshader.GetAddressOf());

	UINT offset2 = 0;
	this->deviceContext->IASetVertexBuffers(0, 1, vb_grid.GetAddressOf(), vb_grid.Stride(), &offset2);
	
	if (renderXZaxis) this->deviceContext->Draw(this->vb_grid.BufferSize()/2, 0);

	if (renderXYaxis) gridXY.draw(deviceContext, camera);//this->deviceContext->Draw(this->vb_grid.BufferSize() / 2, this->vb_grid.BufferSize() / 2);

	// Render UI tool.
	RenderFunctionsImGui();

	// Render Functions
	switch (funcType)
	{
	case ARHIMEDES:
		arhimedesModel.draw(deviceContext, camera);
		break;
	case FERMAT:
		fermatModel.draw(deviceContext, camera);
		break;
	case BERNOULLI:
		lemniscateOfBernoulliModel.draw(deviceContext, camera);
		break;
	default:
		break;
	}

	this->swapchain->Present(1, NULL);
}

bool Graphics::InitializeDirectX(HWND hwnd, int width, int height)
{
	std::vector<AdapterData> adapters = AdapterReader::GetAdapters();

	if (adapters.size() < 1)
	{
		ErrorLogger::Log("No IDXGI Adapters found.");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferDesc.Width = width;
	scd.BufferDesc.Height = height;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 1;
	scd.OutputWindow = hwnd;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(	adapters[0].pAdapter, //IDXGI Adapter
										D3D_DRIVER_TYPE_UNKNOWN,
										NULL, //FOR SOFTWARE DRIVER TYPE
										NULL, //FLAGS FOR RUNTIME LAYERS
										NULL, //FEATURE LEVELS ARRAY
										0, //# OF FEATURE LEVELS IN ARRAY
										D3D11_SDK_VERSION,
										&scd, //Swapchain description
										this->swapchain.GetAddressOf(), //Swapchain Address
										this->device.GetAddressOf(), //Device Address
										NULL, //Supported feature level
										this->deviceContext.GetAddressOf()); //Device Context Address

	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create device and swapchain.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = this->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	if (FAILED(hr)) //If error occurred
	{
		ErrorLogger::Log(hr, "GetBuffer Failed.");
		return false;
	}

	hr = this->device->CreateRenderTargetView(backBuffer.Get(), NULL, this->renderTargetView.GetAddressOf());
	if (FAILED(hr)) //If error occurred
	{
		ErrorLogger::Log(hr, "Failed to create render target view.");
		return false;
	}

	//Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = this->device->CreateTexture2D(&depthStencilDesc, NULL, this->depthStencilBuffer.GetAddressOf());
	if (FAILED(hr)) //If error occurred
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil buffer.");
		return false;
	}

	hr = this->device->CreateDepthStencilView(this->depthStencilBuffer.Get(), NULL, this->depthStencilView.GetAddressOf());
	if (FAILED(hr)) //If error occurred
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil view.");
		return false;
	}

	this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());

	//Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthstencildesc;
	ZeroMemory(&depthstencildesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthstencildesc.DepthEnable = true;
	depthstencildesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
	depthstencildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

	hr = this->device->CreateDepthStencilState(&depthstencildesc, this->depthStencilState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil state.");
		return false;
	}

	//Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//Set the Viewport
	this->deviceContext->RSSetViewports(1, &viewport);

	//Create Rasterizer State
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
	hr = this->device->CreateRasterizerState(&rasterizerDesc, this->rasterizerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create rasterizer state.");
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(this->device.Get(), this->deviceContext.Get());
	ImGui::StyleColorsDark();

	return true;
}

bool Graphics::InitializeShaders()
{

	std::wstring shaderfolder = L"";
#pragma region DetermineShaderPath
	if (IsDebuggerPresent() == TRUE)
	{
#ifdef _DEBUG //Debug Mode
	#ifdef _WIN64 //x64
			shaderfolder = L"..\\x64\\Debug\\";
	#else  //x86 (Win32)
			shaderfolder = L"..\\Debug\\";
	#endif
	#else //Release Mode
	#ifdef _WIN64 //x64
			shaderfolder = L"..\\x64\\Release\\";
	#else  //x86 (Win32)
			shaderfolder = L"..\\Release\\";
	#endif
#endif
	}

	D3D11_INPUT_ELEMENT_DESC inputLayout_3d_colors[] =
	{
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
		{"COLOR", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
	};
	UINT numElements = ARRAYSIZE(inputLayout_3d_colors);


	if (!vs_3d_colors.Initialize(this->device, shaderfolder + L"vs_3d_colors.cso", inputLayout_3d_colors, numElements))
		return false;

	if (!ps_3d_colors.Initialize(this->device, shaderfolder + L"ps_3d_colors.cso"))
		return false;

	D3D11_INPUT_ELEMENT_DESC inputLayout_common[] =
	{
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
		{"COLOR", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
		{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
	};
	numElements = ARRAYSIZE(inputLayout_common);


	if (!commonVS.Initialize(this->device, shaderfolder + L"CommonVS.cso", inputLayout_common, numElements))
		return false;

	if (!coloredPS.Initialize(this->device, shaderfolder + L"ColoredPS.cso"))
		return false;

	if (!texturedPS.Initialize(this->device, shaderfolder + L"TexturedPS.cso"))
		return false;

	return true;
}

bool Graphics::InitializeScene()
{
	const XMFLOAT4 gridColor = {1.0f, 1.0f, 1.0f, 1.0f};

	//Set up constant buffer for vertex shader
	HRESULT hr = cb_vs_vertexshader.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create constant buffer.");
		return false;
	}

	//draw x grid
	std::vector<Vertex_COLOR> v2;
	float gridDimension = 25.0f;
	int gridSections = 25;
	//Draw -Z -> +Z (Red->White) //[X-Z Axis]
	for (int i = 0; i < gridSections; i++)
	{
		v2.push_back(Vertex_COLOR(0.0f - gridDimension / 2 + i * gridDimension / gridSections, 0.0f, -gridDimension / 2.0f, 1.0f, 0.0f, 0.0f));
		v2.push_back(Vertex_COLOR(0.0f - gridDimension / 2 + i * gridDimension / gridSections, 0.0f, +(gridDimension) / 2.0f - gridDimension / gridSections, 1.0f, 1.0f, 1.0f));
	}
	//Draw -X -> +X (Blue->White)  //[X-Z Axis]
	for (int i = 0; i < gridSections; i++)
	{
		v2.push_back(Vertex_COLOR(-gridDimension/2.0f, 0.0f, 0.0f - gridDimension / 2 + i * gridDimension / gridSections, 0.0f, 0.0f, 1.0f));
		v2.push_back(Vertex_COLOR(+gridDimension/2.0f - gridDimension / gridSections, 0.0f, 0.0f - gridDimension / 2 + i * gridDimension / gridSections, 1.0f, 1.0f, 1.0f));
	}
	//Draw -X -> +X (Blue->White) //[X-Y Axis]
	for (int i = 0; i < gridSections; i++)
	{
		v2.push_back(Vertex_COLOR(-gridDimension / 2.0f, 0.0f - gridDimension / 2 + i * gridDimension / gridSections, 0.0f, 0.0f, 0.0f, 1.0f));
		v2.push_back(Vertex_COLOR(+gridDimension / 2.0f - gridDimension / gridSections, 0.0f - gridDimension / 2 + i * gridDimension / gridSections, 0.0f, 1.0f, 1.0f, 1.0f));
	}
	//Draw -Y -> +Y (Green->White) //[X-Y Axis]
	for (int i = 0; i < gridSections; i++)
	{
		v2.push_back(Vertex_COLOR(0.0f - gridDimension / 2 + i * gridDimension / gridSections, -gridDimension/2.0f, 0.0f, 0.0f, 1.0f, 0.0f));
		v2.push_back(Vertex_COLOR(0.0f - gridDimension / 2 + i * gridDimension / gridSections, +gridDimension / 2.0f - gridDimension / gridSections, 0.0f, 1.0f, 1.0f, 1.0f));
	}

	hr = vb_grid.Initialize(this->device.Get(), v2.data(), v2.size());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create vertex buffer.");
		return false;
	}

	camera.SetProjectionValues(60, windowWidth, windowHeight, 1.0f, 1000.0f);
	camera.SetPosition(0.0f, 0.0f, -20.0f);
	ImGui::SetNextWindowSize(ImVec2(1000, 400));

	InitGridModels();
	InitArhimedeslModel();
	InitFermatModel();
	InitLemniscateOfBernoulliModel();

	return true;
}

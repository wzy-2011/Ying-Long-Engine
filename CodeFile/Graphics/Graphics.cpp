#include "Graphics.h"

namespace YingLong
{
	Graphics::Graphics(HWND hWnd)
	{
		//Initialize the foundation of rendering.
		//1. SwapChain - Swap the buffers.
		//2. Device - Create hWnd rendering stuff.
		//3. DeviceContext - Set rendering stuff.
		HRESULT hr;
		DXGI_SWAP_CHAIN_DESC sd = { 0 };
		sd.BufferDesc.Width = 0u;
		sd.BufferDesc.Height = 0u;
		sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 0u;
		sd.BufferDesc.RefreshRate.Denominator = 0u;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;		
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.SampleDesc.Count = 1u;
		sd.SampleDesc.Quality = 0u;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1u;
		sd.OutputWindow = hWnd;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		GRAPHICS_THROW_EXCEPTION(D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&sd,
			pSwapChain.GetAddressOf(),
			pDevice.GetAddressOf(),
			nullptr,
			pDeviceContext.GetAddressOf()
		));

		// 获取窗口实际客户区尺寸，避免硬编码 800x600 导致 ImGui 布局偏移
		// Get actual client area size from window to avoid hardcoded 800x600 causing ImGui layout offset
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		this->Resolution.x = (float)clientWidth;
		this->Resolution.y = (float)clientHeight;

		this->SceneDepthStencil = std::make_unique<DepthStencil>();
		this->SceneDepthStencil->InitializeDepthStencil(*this, clientWidth, clientHeight);
		this->SceneRenderTarget = std::make_unique<RenderTarget>();
		this->SceneRenderTarget->InitializeRenderTarget(
			RenderTargetType::DE_RTTYPE_TEXTUREOUTPUT,
			*this, clientWidth, clientHeight);
		this->GUIRenderTarget = std::make_unique<RenderTarget>();
		this->GUIRenderTarget->InitializeRenderTarget(
			RenderTargetType::DE_RTTYPE_WINOUTPUT, *this);

		GUIViewport.Width = (float)clientWidth;
		GUIViewport.Height = (float)clientHeight;
		GUIViewport.MinDepth = 0;
		GUIViewport.MaxDepth = 1;
		GUIViewport.TopLeftX = 0;
		GUIViewport.TopLeftY = 0;

		SceneViewport.Width = (float)clientWidth;
		SceneViewport.Height = (float)clientHeight;
		SceneViewport.MinDepth = 0;
		SceneViewport.MaxDepth = 1;
		SceneViewport.TopLeftX = 0;
		SceneViewport.TopLeftY = 0;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX11_Init(this->pDevice.Get(), this->pDeviceContext.Get());
	}

	void Graphics::UpdateGUIGraphicsResolution(int NewWidth, int NewHeight)
	{
		this->Resolution.x = (float)NewWidth;
		this->Resolution.y = (float)NewHeight;

		if (this->pDeviceContext && this->GUIRenderTarget->GetRenderTargetView() && this->pSwapChain)
		{
			pDeviceContext->OMSetRenderTargets(0u, NULL, NULL);

			this->GUIRenderTarget->GetRenderTargetView()->Release();

			pDeviceContext->Flush();

			pSwapChain->ResizeBuffers(0u, NewWidth, NewHeight, DXGI_FORMAT_UNKNOWN, 0u);

			this->GUIRenderTarget->InitializeRenderTarget(
				RenderTargetType::DE_RTTYPE_WINOUTPUT, *this);
			this->GUIViewport.Width = (float)NewWidth;
			this->GUIViewport.Height = (float)NewHeight;
		}
	}

	void Graphics::UpdateSceneGraphicsResolution(int NewWidth, int NewHeight)
	{
		if (this->SceneRenderTarget->GetRenderTargetView())
		{
			this->CameraObject.SetResolution({ (float)NewWidth, (float)NewHeight });

			this->SceneRenderTarget->GetRenderTargetView()->Release();
			this->SceneRenderTarget->GetRenderTargetResource()->Release();

			this->SceneRenderTarget->InitializeRenderTarget(
				RenderTargetType::DE_RTTYPE_TEXTUREOUTPUT, *this, NewWidth, NewHeight);
			this->SceneViewport.Width = (float)NewWidth;
			this->SceneViewport.Height = (float)NewHeight;

			this->SceneDepthStencil->GetDepthStencilView()->Release();
			this->SceneDepthStencil->GetDepthStencilState()->Release();

			this->SceneDepthStencil->InitializeDepthStencil(*this, NewWidth, NewHeight);
		}
	}

	void Graphics::EndFrame()
	{
		ImGuiIO& io = ImGui::GetIO();
		(void)io;

		ImGui::Render();
		this->GUIRenderTarget->BindRenderTarget(*this, WRL::ComPtr<ID3D11DepthStencilView>());
		pDeviceContext->RSSetViewports(1u, &GUIViewport);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		HRESULT hr;
		if (FAILED(hr = (pSwapChain->Present(1u, 0u))))
		{
			if (hr == DXGI_ERROR_REMOTE_OUTOFMEMORY)
			{
				throw GRAPHICS_DEVICE_REMOVED_EXCEPT(pDevice->GetDeviceRemovedReason());
			}
			else
			{
				GRAPHICS_THROW_EXCEPTION(hr);
			}
		}
	}

	void Graphics::ColorEditor() noexcept
	{
		ImGui::Begin("Color Editor");
		ImGui::ColorEdit4("Background Color", this->color, 1.0f);
		if (ImGui::Button("Default"))
		{
			this->color[0] = 0.2f;
			this->color[1] = 0.2f;
			this->color[2] = 0.2f;
		}
		ImGui::End();
	}

	void Graphics::ClearBuffer(float red, float green, float blue) noexcept
	{
		this->ClearBufferRed = red;
		this->ClearBufferGreen = green;
		this->ClearBufferBlue = blue;

		this->SceneRenderTarget->BindRenderTarget(*this, this->SceneDepthStencil->GetDepthStencilView());
		this->SceneDepthStencil->BindDepthStencil(*this);

		pDeviceContext->RSSetViewports(1u, &SceneViewport);

		pDeviceContext->RSSetState(nullptr);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(Viewport->WorkPos);
		ImGui::SetNextWindowSize(Viewport->WorkSize);
		ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		WindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		WindowFlags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Ying-Long Engine Editor", NULL, WindowFlags);

		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		ImGuiID dockspace_id = ImGui::GetID("Dracovis Editor Dockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

		ImGui::End();
	}

	void Graphics::DrawIndexed(UINT count) noexcept
	{
		pDeviceContext->DrawIndexed(count, 0u, 0u);
	}

	void Graphics::ReInitImGui()
	{
		// No-op. ImGuiDX12::Initialize no longer calls ImGui_ImplDX11_Shutdown();
		// it saves and nulls the DX11 multi-viewport callbacks instead. When
		// switching back to DX11, ImGuiDX12::Shutdown restores them. The DX11
		// backend data (device/context refs, font texture, VB/IB) was never
		// released, so no re-initialization is needed.
	}

	const XMFLOAT2& Graphics::GetGraphicsResolution() const noexcept
	{
		return this->Resolution;
	}

	void Graphics::SetCamera(Camera camera) noexcept
	{
		this->CameraObject = camera;
	}

	Camera Graphics::GetCamera() const noexcept
	{
		return CameraObject;
	}

	void Graphics::SaveBackgroundColor(const std::string& filePath) const
	{
		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('S'))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "ColorR" << YAML::Value << this->color[0];
			out << YAML::Key << "ColorG" << YAML::Value << this->color[1];
			out << YAML::Key << "ColorB" << YAML::Value << this->color[2];
			out << YAML::Key << "ColorA" << YAML::Value << this->color[3];

			out << YAML::EndMap;

			std::ofstream FileOutput(filePath);
			if (!FileOutput.is_open())
			{
				throw std::runtime_error("Couldn't output the file!(Camera)");
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Graphics::ImportBackgroundColor(const std::string& filePath)
	{
		std::ifstream FileInput(filePath);
		std::stringstream FileStringStream;
		FileStringStream << FileInput.rdbuf();
		auto ColorData = YAML::Load(FileStringStream);

		this->color[0] = ColorData["ColorR"].as<float>();
		this->color[1] = ColorData["ColorG"].as<float>();
		this->color[2] = ColorData["ColorB"].as<float>();
		this->color[3] = ColorData["ColorA"].as<float>();
	}

	Graphics::HRESULTException::HRESULTException(int line, const char* file,
		HRESULT hr) noexcept
		: Exception(line, file), hrGraphicsException(hr)
	{

	}
	const char* Graphics::HRESULTException::what() const noexcept
	{
		std::ostringstream HRESULTException;
		HRESULTException << GetType() << std::endl
			<< "������룺0x" << std::hex << std::uppercase
			<< GetErrorCode() << std::dec
			<< "(" << (unsigned long)GetErrorCode() << ")"
			<< std::endl << "������" << GetErrorDesciption()
			<< GetOriginString() << std::endl;
		what_buffer = HRESULTException.str();
		return what_buffer.c_str();
	}
	const char* Graphics::HRESULTException::GetType() const noexcept
	{
		return "Graphics Exception";
	}
	HRESULT Graphics::HRESULTException::GetErrorCode() const noexcept
	{
		return hrGraphicsException;
	}
	std::string Graphics::HRESULTException::GetErrorString() const noexcept
	{
		return std::string();
	}
	std::string Graphics::HRESULTException::GetErrorDesciption() const noexcept
	{
		char* MsgBuffer = NULL;
		DWORD MessageLength = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, this->hrGraphicsException, 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&MsgBuffer), 0, NULL);
		if (MessageLength == 0)
		{
			return "Unknown error!";
		}

		std::string ErrorString = MsgBuffer;
		LocalFree(MsgBuffer);

		return ErrorString;
	}
	const char* Graphics::DeviceRemovedException::GetType() const noexcept
	{
		return "Graphics Error: Device Removed Exception (Use GRAPHICS_DEVICE_REMOVED_EXCEPT macro)";
	}
}

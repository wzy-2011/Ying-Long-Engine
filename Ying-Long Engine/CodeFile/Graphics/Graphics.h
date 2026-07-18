/**
 * @file Graphics.h
 * @brief DX11 图形设备类（已弃用） / DX11 Graphics device class (deprecated)
 *
 * 旧版 DirectX 11 图形设备封装。负责 D3D11 设备、交换链、
 * 渲染目标、深度模板缓冲区的创建与管理，以及 ImGui 集成。
 *
 * Legacy DirectX 11 graphics device wrapper. Responsible for creating
 * and managing D3D11 device, swap chain, render targets, depth stencil
 * buffer, and ImGui integration.
 *
 * @deprecated 已弃用。新代码应使用 Graphics/DX12/ 目录下的 DX12 组件。
 *             Deprecated. New code should use DX12 components in the
 *             Graphics/DX12/ directory.
 */
#pragma once

// =============================================================================
// DEPRECATED: This file contains the old DX11 Graphics implementation.
// New code should use DX12 components in the Graphics/DX12/ directory.
// =============================================================================

#pragma message("WARNING: Graphics.h is deprecated. Use DX12Renderer instead.")

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <memory>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <string>
#include "../../yaml-cpp/include/yaml-cpp/yaml.h"
#include <filesystem>
#include <filesystem>
#include <vector>
#include <fstream>
#include "../../ImGui/CodeFile/ImGui/imgui_impl_win32.h"
#include "../../ImGui/CodeFile/ImGui/imgui_impl_dx11.h"
#include "../../ImGui/CodeFile/ImGui/imgui.h"

/**
 * @brief DX11 异常抛出宏 / DX11 exception throw macro
 *
 * 检查 HRESULT，如果失败则抛出 Graphics::HRESULTException。
 * Checks HRESULT and throws Graphics::HRESULTException if failed.
 */
#define GRAPHICS_THROW_EXCEPTION(hrcall) if (FAILED(hr = (hrcall))) throw Graphics::HRESULTException(__LINE__, __FILE__, hr);

/**
 * @brief 设备移除异常宏 / Device removed exception macro
 *
 * 构造 Graphics::DeviceRemovedException。
 * Constructs Graphics::DeviceRemovedException.
 */
#define GRAPHICS_DEVICE_REMOVED_EXCEPT(hr) Graphics::DeviceRemovedException(__LINE__, __FILE__, (hr))

#include "./DepthStencil/DepthStencil.h"
#include "../Exception/Exception.h"
#include "Camera/Camera.h"

using namespace DirectX;
using namespace Microsoft;

namespace YingLong
{
	class RenderTarget;
	class DepthStencil;

	/**
	 * @brief DX11 图形设备类 / DX11 graphics device class
	 *
	 * 封装 D3D11 设备、交换链、渲染目标视图、深度模板缓冲区等核心图形资源。
	 * 提供清屏、绘制、交换缓冲区等基础渲染操作。
	 *
	 * Wraps core graphics resources like D3D11 device, swap chain, render target
	 * views, and depth stencil buffer. Provides basic render operations like
	 * clearing, drawing, and presenting.
	 *
	 * @deprecated 使用 DX12Renderer 替代 / Use DX12Renderer instead
	 */
	class Graphics [[deprecated("Use DX12Renderer instead")]]
	{
	public:
		/**
		 * @brief HRESULT 异常类 / HRESULT exception class
		 *
		 * 封装 DirectX HRESULT 错误码，提供格式化的错误信息。
		 * Wraps DirectX HRESULT error codes and provides formatted error messages.
		 */
		class HRESULTException : public Exception
		{
			using Exception::Exception;
		public:
			/**
			 * @brief 构造函数 / Constructor
			 * @param line 行号 / Line number
			 * @param file 文件名 / File name
			 * @param hr HRESULT 错误码 / HRESULT error code
			 */
			HRESULTException(int line, const char* file, HRESULT hr) noexcept;
			const char* what() const noexcept override;
			const char* GetType() const noexcept override;
			HRESULT GetErrorCode() const noexcept;
			std::string GetErrorString() const noexcept;
			std::string GetErrorDesciption() const noexcept;

		private:
			HRESULT hrGraphicsException;  ///< HRESULT 错误码 / HRESULT error code
		};

		/**
		 * @brief 设备移除异常类 / Device removed exception class
		 *
		 * 当 GPU 设备被移除（如驱动崩溃、热插拔）时抛出。
		 * Thrown when the GPU device is removed (e.g. driver crash, hot-swap).
		 */
		class DeviceRemovedException : public HRESULTException
		{
			using HRESULTException::HRESULTException;
		public:
			const char* GetType() const noexcept override;
		};

		Graphics() = default;

		/**
		 * @brief 构造函数 / Constructor
		 *
		 * 从 HWND 创建 D3D11 设备、交换链和渲染目标。
		 * Creates D3D11 device, swap chain, and render targets from HWND.
		 *
		 * @param hWnd 窗口句柄 / Window handle
		 */
		Graphics(HWND hWnd);
		Graphics(const Graphics&) = delete;
		Graphics& operator=(const Graphics&) = delete;
		~Graphics() = default;

		/**
		 * @brief 更新 GUI 渲染目标分辨率 / Update GUI render target resolution
		 * @param NewWidth 新宽度 / New width
		 * @param NewHeight 新高度 / New height
		 */
		void UpdateGUIGraphicsResolution(int NewWidth, int NewHeight);

		/**
		 * @brief 更新场景渲染目标分辨率 / Update scene render target resolution
		 * @param NewWidth 新宽度 / New width
		 * @param NewHeight 新高度 / New height
		 */
		void UpdateSceneGraphicsResolution(int NewWidth, int NewHeight);

		/**
		 * @brief 结束帧（呈现交换链） / End frame (present swap chain)
		 *
		 * 调用 swap chain 的 Present 方法将后缓冲区提交到屏幕。
		 * Calls swap chain Present method to submit back buffer to screen.
		 */
		void EndFrame();

		/**
		 * @brief 颜色编辑器 ImGui 面板 / Color editor ImGui panel
		 *
		 * 提供背景色调整的 ImGui 窗口。
		 * Provides an ImGui window for background color adjustment.
		 */
		void ColorEditor() noexcept;

		/**
		 * @brief 清除缓冲区 / Clear buffer
		 *
		 * 使用指定颜色清除渲染目标和深度模板缓冲区，
		 * 同时启动新一帧的 ImGui 并设置 dockspace。
		 *
		 * Clears render target and depth stencil buffer with specified color,
		 * also starts a new ImGui frame and sets up dockspace.
		 *
		 * @param red 红色分量 / Red component
		 * @param green 绿色分量 / Green component
		 * @param blue 蓝色分量 / Blue component
		 */
		void ClearBuffer(float red, float green, float blue) noexcept;

		/**
		 * @brief 绘制索引几何体 / Draw indexed geometry
		 * @param count 索引数量 / Index count
		 */
		void DrawIndexed(UINT count) noexcept;

		/**
		 * @brief 重新初始化 ImGui / Re-initialize ImGui
		 *
		 * 当前为 no-op：切换回 DX11 时无需重建字体纹理等资源。
		 * Currently no-op: no need to rebuild font texture etc. when
		 * switching back to DX11.
		 */
		void ReInitImGui();

		/**
		 * @brief 获取图形分辨率 / Get graphics resolution
		 * @return const XMFLOAT2& 分辨率（宽，高） / Resolution (width, height)
		 */
		const XMFLOAT2& GetGraphicsResolution() const noexcept;

		/**
		 * @brief 设置相机 / Set camera
		 * @param camera 相机对象 / Camera object
		 */
		void SetCamera(Camera camera) noexcept;

		/**
		 * @brief 获取相机 / Get camera
		 * @return Camera 相机对象 / Camera object
		 */
		Camera GetCamera() const noexcept;

		/**
		 * @brief 保存背景色到文件 / Save background color to file
		 *
		 * 按住 Ctrl+S 时将背景色序列化为 YAML 文件。
		 * Serializes background color to YAML file when Ctrl+S is held.
		 *
		 * @param filePath 文件路径 / File path
		 */
		void SaveBackgroundColor(const std::string& filePath) const;

		/**
		 * @brief 从文件导入背景色 / Import background color from file
		 * @param filePath 文件路径 / File path
		 */
		void ImportBackgroundColor(const std::string& filePath);

	private:
		WRL::ComPtr<ID3D11Device> pDevice;                  ///< D3D11 设备 / D3D11 device
		WRL::ComPtr<IDXGISwapChain> pSwapChain;             ///< 交换链 / Swap chain
		WRL::ComPtr<ID3D11DeviceContext> pDeviceContext;    ///< 设备上下文 / Device context
		std::unique_ptr<RenderTarget> SceneRenderTarget;    ///< 场景渲染目标 / Scene render target
		std::unique_ptr<RenderTarget> GUIRenderTarget;      ///< GUI 渲染目标 / GUI render target
		std::unique_ptr<DepthStencil> SceneDepthStencil;    ///< 场景深度模板 / Scene depth stencil
		WRL::ComPtr<ID3D11Buffer> pVerTextBuffer;           ///< 顶点纹理缓冲区（未使用） / Vertex texture buffer (unused)
		WRL::ComPtr<ID3D11VertexShader> pVertexShader;      ///< 顶点着色器（未使用） / Vertex shader (unused)
		WRL::ComPtr<ID3D11PixelShader> pPixelShader;        ///< 像素着色器（未使用） / Pixel shader (unused)
		WRL::ComPtr<ID3D11InputLayout> pInputLayout;        ///< 输入布局（未使用） / Input layout (unused)
		WRL::ComPtr<ID3DBlob> pBlob;                        ///< 着色器编译 Blob（未使用） / Shader compile blob (unused)
		D3D11_VIEWPORT SceneViewport;                       ///< 场景视口 / Scene viewport
		D3D11_VIEWPORT GUIViewport;                         ///< GUI 视口 / GUI viewport

		XMFLOAT2 Resolution;                                ///< 分辨率 / Resolution
		Camera CameraObject;                                ///< 相机对象 / Camera object

		float ClearBufferRed = 0.2f;                        ///< 清屏红色 / Clear red
		float ClearBufferGreen = 0.2f;                      ///< 清屏绿色 / Clear green
		float ClearBufferBlue = 0.2f;                       ///< 清屏蓝色 / Clear blue
		float color[4] = { 0.2f, 0.2f, 0.2f, 1.0f };       ///< 背景色 RGBA / Background color RGBA

		friend class Bindable;          ///< 友元：Bindable 可以访问 pDevice/pDeviceContext / Friend: Bindable can access pDevice/pDeviceContext
		friend class RenderTarget;      ///< 友元：RenderTarget / Friend: RenderTarget
		friend class DepthStencil;      ///< 友元：DepthStencil / Friend: DepthStencil
		friend class Application;       ///< 友元：Application / Friend: Application
	};
}

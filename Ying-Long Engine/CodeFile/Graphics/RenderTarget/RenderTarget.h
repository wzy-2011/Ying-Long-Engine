/**
 * @file RenderTarget.h
 * @brief 渲染目标类（已弃用）/ Render target class (deprecated)
 *
 * 提供DirectX 11渲染目标的创建和管理功能。
 * 新代码应使用 Graphics/DX12/ 目录下的 RenderTargetDX12。
 * Provides DirectX 11 render target creation and management functionality.
 * New code should use RenderTargetDX12 in the Graphics/DX12/ directory.
 *
 * @deprecated 已弃用，请使用 RenderTargetDX12 替代
 *             / Deprecated, use RenderTargetDX12 instead
 */

#pragma once

// =============================================================================
// DEPRECATED: This file contains the old DX11 RenderTarget implementation.
// New code should use RenderTargetDX12 in the Graphics/DX12/ directory.
// =============================================================================

#pragma message("WARNING: RenderTarget.h is deprecated. Use RenderTargetDX12 instead.")

#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgiformat.h>
#include <fstream>

using namespace Microsoft;

namespace YingLong
{
	class Graphics;
	
	/**
	 * @brief 渲染目标类型枚举 / Render target type enumeration
	 *
	 * 指定渲染目标的类型：窗口输出或纹理输出。
	 * Specifies the type of render target: window output or texture output.
	 */
	enum class RenderTargetType
	{
		DE_RTTYPE_WINOUTPUT,       ///< 窗口输出渲染目标 / Window output render target
		DE_RTTYPE_TEXTUREOUTPUT,   ///< 纹理输出渲染目标 / Texture output render target
	};

	/**
	 * @brief 渲染目标类（DX11旧版）
	 *        Render target class (old DX11 version)
	 *
	 * 封装了DirectX 11的渲染目标视图和着色器资源视图。
	 * 支持窗口输出（交换链后台缓冲区）和纹理输出两种模式。
	 * Encapsulates DirectX 11 render target view and shader resource view.
	 * Supports window output (swap chain back buffer) and texture output modes.
	 *
	 * @deprecated 已弃用，请使用 RenderTargetDX12 替代
	 *             / Deprecated, use RenderTargetDX12 instead
	 */
	class RenderTarget [[deprecated("Use RenderTargetDX12 instead")]]
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		RenderTarget();

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 */
		RenderTarget(const RenderTarget&) = default;

		/**
		 * @brief 初始化渲染目标 / Initialize render target
		 *
		 * 根据类型创建渲染目标：窗口输出或纹理输出。
		 * Creates render target based on type: window output or texture output.
		 *
		 * @param type 渲染目标类型 / Render target type
		 * @param graphics 图形设备对象 / Graphics device object
		 * @param width 纹理宽度（仅纹理输出模式需要）
		 *        / Texture width (only needed for texture output mode)
		 * @param height 纹理高度（仅纹理输出模式需要）
		 *        / Texture height (only needed for texture output mode)
		 */
		void InitializeRenderTarget(
			RenderTargetType type, Graphics& graphics,
			int width = 0, int height = 0);

		/**
		 * @brief 绑定渲染目标到渲染管线
		 *        Bind render target to render pipeline
		 *
		 * 设置渲染目标视图并清除渲染目标缓冲区。
		 * Sets render target view and clears the render target buffer.
		 *
		 * @param graphics 图形设备对象 / Graphics device object
		 * @param dsv 深度模板视图 / Depth stencil view
		 */
		void BindRenderTarget(Graphics& graphics, 
			WRL::ComPtr<ID3D11DepthStencilView> dsv);

		/**
		 * @brief 获取渲染目标视图 / Get render target view
		 *
		 * @return WRL::ComPtr<ID3D11RenderTargetView> 渲染目标视图指针
		 *         / Render target view pointer
		 */
		WRL::ComPtr<ID3D11RenderTargetView> GetRenderTargetView();

		/**
		 * @brief 获取渲染目标着色器资源
		 *        Get render target shader resource
		 *
		 * @return WRL::ComPtr<ID3D11ShaderResourceView> 着色器资源视图指针
		 *         / Shader resource view pointer
		 */
		WRL::ComPtr<ID3D11ShaderResourceView> GetRenderTargetResource();

	private:
		WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView;      ///< 渲染目标视图 / Render target view
		WRL::ComPtr<ID3D11ShaderResourceView> pRenderTargetResource; ///< 着色器资源视图 / Shader resource view
	};
}

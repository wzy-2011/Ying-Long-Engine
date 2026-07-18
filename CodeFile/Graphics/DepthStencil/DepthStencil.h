/**
 * @file DepthStencil.h
 * @brief 深度模板缓冲区类（已弃用）/ Depth stencil buffer class (deprecated)
 *
 * 提供DirectX 11深度模板缓冲区的创建和管理功能。
 * 新代码应使用 Graphics/DX12/ 目录下的 DepthStencilDX12。
 * Provides DirectX 11 depth stencil buffer creation and management functionality.
 * New code should use DepthStencilDX12 in the Graphics/DX12/ directory.
 *
 * @deprecated 已弃用，请使用 DepthStencilDX12 替代
 *             / Deprecated, use DepthStencilDX12 instead
 */

#pragma once

// =============================================================================
// DEPRECATED: This file contains the old DX11 DepthStencil implementation.
// New code should use DepthStencilDX12 in the Graphics/DX12/ directory.
// =============================================================================

#pragma message("WARNING: DepthStencil.h is deprecated. Use DepthStencilDX12 instead.")

#include "../RenderTarget/RenderTarget.h"
#include "../../Exception/Exception.h"

namespace YingLong
{
	/**
	 * @brief 深度模板缓冲区类（DX11旧版）
	 *        Depth stencil buffer class (old DX11 version)
	 *
	 * 封装了DirectX 11的深度模板缓冲区、视图和状态对象。
	 * Encapsulates DirectX 11 depth stencil buffer, view, and state objects.
	 *
	 * @deprecated 已弃用，请使用 DepthStencilDX12 替代
	 *             / Deprecated, use DepthStencilDX12 instead
	 */
	class DepthStencil [[deprecated("Use DepthStencilDX12 instead")]]
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		DepthStencil();

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 */
		DepthStencil(const DepthStencil&) = default;

		/**
		 * @brief 初始化深度模板缓冲区 / Initialize depth stencil buffer
		 *
		 * 创建深度模板纹理、视图和状态对象。
		 * Creates depth stencil texture, view, and state objects.
		 *
		 * @param graphics 图形设备对象 / Graphics device object
		 * @param width 缓冲区宽度 / Buffer width
		 * @param height 缓冲区高度 / Buffer height
		 */
		void InitializeDepthStencil(Graphics& graphics, int width, int height);

		/**
		 * @brief 绑定深度模板缓冲区到渲染管线
		 *        Bind depth stencil buffer to render pipeline
		 *
		 * 设置深度模板状态并清除深度模板缓冲区。
		 * Sets depth stencil state and clears the depth stencil buffer.
		 *
		 * @param graphics 图形设备对象 / Graphics device object
		 */
		void BindDepthStencil(Graphics& graphics);

		/**
		 * @brief 获取深度模板视图 / Get depth stencil view
		 *
		 * @return WRL::ComPtr<ID3D11DepthStencilView> 深度模板视图指针
		 *         / Depth stencil view pointer
		 */
		WRL::ComPtr<ID3D11DepthStencilView> GetDepthStencilView();

		/**
		 * @brief 获取深度模板状态 / Get depth stencil state
		 *
		 * @return WRL::ComPtr<ID3D11DepthStencilState> 深度模板状态指针
		 *         / Depth stencil state pointer
		 */
		WRL::ComPtr<ID3D11DepthStencilState> GetDepthStencilState();

	private:
		WRL::ComPtr<ID3D11Texture2D> DepthStencilBuffer;          ///< 深度模板纹理缓冲区 / Depth stencil texture buffer
		WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView;     ///< 深度模板视图 / Depth stencil view
		WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;   ///< 深度模板状态 / Depth stencil state
	};
}

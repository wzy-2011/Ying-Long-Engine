/**
 * @file Rasterizer.h
 * @brief DX11 光栅化状态头文件 / DX11 rasterizer state header
 *
 * 封装 D3D11 光栅化状态绑定，支持实体/线框填充模式、
 * 面剔除和深度裁剪设置。
 * Encapsulates D3D11 rasterizer state binding, supporting solid/wireframe
 * fill modes, face culling, and depth clip settings.
 */

#pragma once
#include "Bindable.h"

namespace YingLong
{
	/**
	 * @brief DX11 光栅化状态类 / DX11 rasterizer state class
	 *
	 * 管理 D3D11 光栅化状态，支持线框模式切换等。
	 * 默认为实体填充、背面剔除、深度裁剪启用。
	 * Manages D3D11 rasterizer state, supports wireframe mode switching etc.
	 * Defaults to solid fill, back-face culling, depth clip enabled.
	 */
	class Rasterizer : public Bindable
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param fillMode 填充模式（默认实体）/ Fill mode (default solid)
		 *
		 * 创建指定填充模式的光栅化状态对象。
		 * Creates a rasterizer state object with the specified fill mode.
		 */
		Rasterizer(Graphics& graphics,
			D3D11_FILL_MODE fillMode = D3D11_FILL_SOLID,
			D3D11_CULL_MODE cullMode = D3D11_CULL_BACK);

		/**
		 * @brief 绑定光栅化状态 / Bind rasterizer state
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将光栅化状态设置到光栅化阶段。
		 * Sets the rasterizer state to the rasterizer stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

	protected:
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizerState;  ///< D3D11 光栅化状态 / D3D11 rasterizer state
	};
}

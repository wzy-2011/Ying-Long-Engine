/**
 * @file SolidColorConstantBuffer.h
 * @brief DX11 纯色常量缓冲区头文件 / DX11 solid color constant buffer header file
 *
 * 提供纯色常量缓冲区，将每实例颜色绑定到纯色像素着色器的常量缓冲区。
 * Provides a solid color constant buffer that binds per-instance color
 * to the solid pixel shader's constant buffer.
 */

#pragma once
#include <DirectXMath.h>
#include "ConstantBuffers.h"

namespace YingLong
{
	/**
	 * @brief 纯色常量缓冲区数据结构 / Solid color constant buffer data structure
	 *
	 * 存储纯色像素着色器所需的颜色数据。
	 * Stores color data required by the solid color pixel shader.
	 */
	struct SolidColorConstantBufferData
	{
		DirectX::XMFLOAT3 Color;   ///< RGB 颜色值 / RGB color value
	};

	/**
	 * @brief 纯色常量缓冲区类 / Solid color constant buffer class
	 *
	 * 将每实例颜色绑定到 SolidPixelShader 的常量缓冲区。通过存储指向
	 * XMFLOAT3 的指针，该类与任何特定的可绘制类型解耦。
	 * 调用者需要保证该指针的生命周期长于此可绑定对象的生命周期
	 * （通常指向拥有者可绘制对象的成员，该成员通过 DrawableBase 拥有此可绑定对象）。
	 *
	 * Binds a per-instance color to the SolidPixelShader's cbuffer. Decoupled
	 * from any specific drawable type by storing a pointer to an XMFLOAT3 the
	 * caller promises will outlive this bindable (typically a member of the
	 * owning drawable, which also owns the bindable via DrawableBase).
	 */
	class SolidColorConstantBuffer : public Bindable
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param colorPtr 颜色数据指针 / Color data pointer
		 *
		 * 创建纯色常量缓冲区，并绑定到外部颜色数据指针。
		 * Creates a solid color constant buffer and binds it to an external color data pointer.
		 */
		SolidColorConstantBuffer(Graphics& graphics, const DirectX::XMFLOAT3* colorPtr);

		/**
		 * @brief 绑定纯色常量缓冲区 / Bind solid color constant buffer
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 更新颜色数据并将常量缓冲区绑定到像素着色器阶段。
		 * Updates color data and binds the constant buffer to the pixel shader stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

	private:
		std::shared_ptr<PixelConstantBuffer<SolidColorConstantBufferData>> pConstantBuffer;   ///< 像素着色器常量缓冲区共享指针 / Pixel shader constant buffer shared pointer
		const DirectX::XMFLOAT3* pColor;   ///< 外部颜色数据指针 / External color data pointer
	};
}

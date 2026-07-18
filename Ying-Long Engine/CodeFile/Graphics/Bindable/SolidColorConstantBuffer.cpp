/**
 * @file SolidColorConstantBuffer.cpp
 * @brief DX11 纯色常量缓冲区实现文件 / DX11 solid color constant buffer implementation file
 *
 * 实现 SolidColorConstantBuffer 类的构造和绑定功能。
 * Implements SolidColorConstantBuffer class construction and binding functionality.
 */

#include "SolidColorConstantBuffer.h"

namespace YingLong
{
	SolidColorConstantBuffer::SolidColorConstantBuffer(Graphics& graphics, const DirectX::XMFLOAT3* colorPtr)
		: pColor(colorPtr)
	{
		// 创建像素着色器常量缓冲区
		// Create pixel shader constant buffer
		pConstantBuffer = std::make_shared<PixelConstantBuffer<SolidColorConstantBufferData>>(graphics);
	}

	void SolidColorConstantBuffer::Bind(Graphics& graphics) noexcept
	{
		// 先更新常量缓冲区中的颜色数据，然后绑定到像素着色器阶段
		// First update color data in the constant buffer, then bind to pixel shader stage
		pConstantBuffer->Update(graphics, { *pColor });
		pConstantBuffer->Bind(graphics);
	}
}

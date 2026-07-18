/**
 * @file InputLayout.cpp
 * @brief DX11 输入布局实现文件 / DX11 input layout implementation file
 *
 * 实现 InputLayout 类的构造和绑定功能。
 * Implements InputLayout class construction and binding functionality.
 */

#include "InputLayout.h"

namespace YingLong
{
	InputLayout::InputLayout(Graphics& graphics,
		const std::vector<D3D11_INPUT_ELEMENT_DESC> layout,
		ID3DBlob* pVertexShaderBytecode)
	{
		// 根据输入元素描述数组和顶点着色器字节码创建输入布局
		// Create input layout from input element descriptor array and vertex shader bytecode
		GetDevice(&graphics)->CreateInputLayout(layout.data(), (UINT)layout.size(),
			pVertexShaderBytecode->GetBufferPointer(),
			pVertexShaderBytecode->GetBufferSize(),
			this->pInputLayout.GetAddressOf());
	}

	void InputLayout::Bind(Graphics& graphics) noexcept
	{
		// 将输入布局绑定到输入装配阶段
		// Bind input layout to input assembler stage
		GetDevicContext(&graphics)->IASetInputLayout(this->pInputLayout.Get());
	}
}

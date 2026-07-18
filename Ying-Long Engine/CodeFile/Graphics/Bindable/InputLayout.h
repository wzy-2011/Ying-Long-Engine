/**
 * @file InputLayout.h
 * @brief DX11 输入布局头文件 / DX11 input layout header file
 *
 * 封装 D3D11 输入布局，定义顶点缓冲区数据如何映射到顶点着色器输入。
 * Encapsulates D3D11 input layout, defining how vertex buffer data
 * maps to vertex shader inputs.
 */

#pragma once
#include "Bindable.h"

namespace YingLong
{
	/**
	 * @brief DX11 输入布局类 / DX11 input layout class
	 *
	 * 管理 D3D11 输入布局对象，根据顶点元素描述数组和顶点着色器字节码
	 * 创建输入布局，并绑定到输入装配阶段。
	 * Manages D3D11 input layout objects, creating input layouts from
	 * vertex element descriptor arrays and vertex shader bytecode,
	 * and binding them to the input assembler stage.
	 */
	class InputLayout : public Bindable
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param layout 输入元素描述数组 / Input element descriptor array
		 * @param pVertexShaderBytecode 顶点着色器字节码指针 / Vertex shader bytecode pointer
		 *
		 * 根据输入元素布局和顶点着色器字节码创建 D3D11 输入布局。
		 * Creates a D3D11 input layout from input element layout and vertex shader bytecode.
		 */
		InputLayout(Graphics& graphics, const std::vector<D3D11_INPUT_ELEMENT_DESC> layout,
			ID3DBlob* pVertexShaderBytecode);

		/**
		 * @brief 绑定输入布局 / Bind input layout
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将输入布局绑定到输入装配阶段。
		 * Binds the input layout to the input assembler stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

	protected:
		WRL::ComPtr<ID3D11InputLayout> pInputLayout;   ///< D3D11 输入布局对象 / D3D11 input layout object
	};
}

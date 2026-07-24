/**
 * @file TransformConstantBuffer.cpp
 * @brief DX11 变换常量缓冲区实现文件 / DX11 transform constant buffer implementation file
 *
 * 实现 TransformConstantBuffer 类的构造和绑定功能。
 * Implements TransformConstantBuffer class construction and binding functionality.
 */

#include "TransformConstantBuffer.h"


namespace YingLong
{
	TransformConstantBuffer::TransformConstantBuffer(Graphics& graphics, 
		const Drawable& parent) : parent(parent)
	{
		// 延迟初始化静态常量缓冲区：如果尚未创建则创建
		// Lazy initialization of static constant buffer: create if not yet created
		if (!pVertexConstantBuffer)
		{
			pVertexConstantBuffer = std::make_unique<VertexConstantBuffer<Transforms>>(graphics, 2);
		}
	}

	void TransformConstantBuffer::Bind(Graphics& graphics) noexcept
	{
		// 从父 Drawable 对象获取模型变换矩阵
		// Get model transform matrix from parent Drawable object
		const auto model = parent.GetTransformXM();
		// 构造变换数据结构体
		// Construct transform data structure
		const Transforms TransformsObject =
		{
			// 转置模型矩阵（HLSL 默认列主序，XMMATRIX 为行主序）
			// Transpose model matrix (HLSL defaults to column-major, XMMATRIX is row-major)
			DirectX::XMMatrixTranspose(model),
			// 计算并转置 MVP 矩阵：模型 * 视图 * 投影
			// Calculate and transpose MVP matrix: model * view * projection
			DirectX::XMMatrixTranspose(model * graphics.GetCamera().GetMatrix() * graphics.GetCamera().GetProjection())
		};
		// 更新常量缓冲区数据并绑定到顶点着色器阶段
		// Update constant buffer data and bind to vertex shader stage
		pVertexConstantBuffer->Update(graphics, TransformsObject);
		pVertexConstantBuffer->Bind(graphics);
	}

	// 静态成员变量初始化
	// Static member variable initialization
	std::unique_ptr<VertexConstantBuffer<TransformConstantBuffer::Transforms>> TransformConstantBuffer::pVertexConstantBuffer;
}

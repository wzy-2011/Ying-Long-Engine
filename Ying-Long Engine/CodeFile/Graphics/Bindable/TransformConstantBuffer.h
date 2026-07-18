/**
 * @file TransformConstantBuffer.h
 * @brief DX11 变换常量缓冲区头文件 / DX11 transform constant buffer header file
 *
 * 提供变换常量缓冲区，将模型矩阵和模型-视图-投影矩阵
 * 上传到顶点着色器的常量缓冲区。
 * Provides a transform constant buffer that uploads the model matrix
 * and model-view-projection matrix to the vertex shader's constant buffer.
 */

#pragma once
#include <DirectXMath.h>
#include "ConstantBuffers.h"
#include "../Drawable/Drawable.h"

namespace YingLong
{
	/**
	 * @brief 变换常量缓冲区类 / Transform constant buffer class
	 *
	 * 将每实例的变换信息（模型矩阵和模型-视图-投影矩阵）绑定到
	 * 顶点着色器的常量缓冲区。使用静态共享的常量缓冲区对象
	 * 以减少资源开销，从父 Drawable 对象获取变换信息。
	 *
	 * Binds per-instance transform information (model matrix and
	 * model-view-projection matrix) to the vertex shader's constant buffer.
	 * Uses a static shared constant buffer object to reduce resource overhead,
	 * obtaining transform information from the parent Drawable object.
	 */
	class TransformConstantBuffer : public Bindable
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param parent 父 Drawable 对象引用 / Parent Drawable object reference
		 *
		 * 创建变换常量缓冲区，并关联到父 Drawable 对象以获取变换信息。
		 * Creates a transform constant buffer and associates it with the parent
		 * Drawable object to obtain transform information.
		 */
		TransformConstantBuffer(Graphics& graphics, const Drawable& parent);

		/**
		 * @brief 绑定变换常量缓冲区 / Bind transform constant buffer
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 从父 Drawable 获取当前变换，计算模型矩阵和 MVP 矩阵，
		 * 更新常量缓冲区并绑定到顶点着色器阶段。
		 * Obtains the current transform from the parent Drawable, calculates
		 * the model matrix and MVP matrix, updates the constant buffer,
		 * and binds it to the vertex shader stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

	private:
		/**
		 * @brief 变换数据结构 / Transform data structure
		 *
		 * 存储上传到顶点着色器的变换矩阵数据。
		 * Stores transform matrix data uploaded to the vertex shader.
		 */
		struct Transforms
		{
			DirectX::XMMATRIX Model;              ///< 模型矩阵 / Model matrix
			DirectX::XMMATRIX ModelViewProject;   ///< 模型-视图-投影矩阵 / Model-View-Projection matrix
		};

		static std::unique_ptr<VertexConstantBuffer<Transforms>> pVertexConstantBuffer;   ///< 静态共享顶点着色器常量缓冲区 / Static shared vertex shader constant buffer
		const Drawable& parent;   ///< 父 Drawable 对象引用 / Parent Drawable object reference
	};
}

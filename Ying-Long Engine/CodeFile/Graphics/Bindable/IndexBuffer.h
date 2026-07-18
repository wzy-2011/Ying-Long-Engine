/**
 * @file IndexBuffer.h
 * @brief DX11 索引缓冲区头文件 / DX11 index buffer header file
 *
 * 封装 D3D11 索引缓冲区，用于存储几何体的索引数据以实现索引化绘制。
 * Encapsulates a D3D11 index buffer for storing geometry index data
 * to enable indexed drawing.
 */

#pragma once
#include "Bindable.h"

namespace YingLong
{
	/**
	 * @brief DX11 索引缓冲区类 / DX11 index buffer class
	 *
	 * 管理 D3D11 索引缓冲区资源，支持将 16 位无符号短整型索引数据
	 * 上传到 GPU 并绑定到输入装配阶段。
	 * Manages D3D11 index buffer resources, supporting uploading 16-bit
	 * unsigned short index data to GPU and binding to the input assembler stage.
	 */
	class IndexBuffer : public Bindable
	{
	public:
		/**
		 * @brief 构造函数（16位索引）/ Constructor (16-bit indices)
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param indices 索引数据向量（16位）/ Index data vector (16-bit)
		 *
		 * 根据传入的16位索引数据创建 D3D11 索引缓冲区。
		 * Creates a D3D11 index buffer from the provided 16-bit index data.
		 */
		IndexBuffer(Graphics& graphics, const std::vector<unsigned short>& indices);

		/**
		 * @brief 构造函数（32位索引）/ Constructor (32-bit indices)
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param indices 索引数据向量（32位）/ Index data vector (32-bit)
		 *
		 * 根据传入的32位索引数据创建 D3D11 索引缓冲区，支持大型模型。
		 * Creates a D3D11 index buffer from the provided 32-bit index data, supports large models.
		 */
		IndexBuffer(Graphics& graphics, const std::vector<unsigned int>& indices);

		/**
		 * @brief 绑定索引缓冲区 / Bind index buffer
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将索引缓冲区绑定到输入装配阶段。
		 * Binds the index buffer to the input assembler stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

		/**
		 * @brief 获取索引数量 / Get index count
		 * @return 索引总数 / Total index count
		 */
		UINT GetCount() const noexcept;

	protected:
		UINT count;                                    ///< 索引数量 / Number of indices
		DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT; ///< 索引格式（16位或32位）/ Index format (16-bit or 32-bit)
		WRL::ComPtr<ID3D11Buffer> pIndexBuffer;        ///< D3D11 索引缓冲区对象 / D3D11 index buffer object
	};
}

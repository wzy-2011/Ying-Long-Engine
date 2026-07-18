/**
 * @file IndexBuffer.cpp
 * @brief DX11 索引缓冲区实现文件 / DX11 index buffer implementation file
 *
 * 实现 IndexBuffer 类的构造、绑定和索引数量查询功能。
 * Implements IndexBuffer class construction, binding, and index count query functionality.
 */

#include "IndexBuffer.h"
#include "../Graphics.h"

namespace YingLong
{
	IndexBuffer::IndexBuffer(Graphics& graphics, const std::vector<unsigned short>& indices)
		: count((UINT)indices.size()), indexFormat(DXGI_FORMAT_R16_UINT)
	{
		HRESULT hr;
		D3D11_BUFFER_DESC IndexBufferDesc = { 0 };
		// 设置为索引缓冲区绑定类型
		// Set as index buffer bind type
		IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		// 默认使用方式，GPU 读写，CPU 不可访问
		// Default usage, GPU read-write, CPU not accessible
		IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		IndexBufferDesc.CPUAccessFlags = 0u;
		IndexBufferDesc.MiscFlags = 0u;
		IndexBufferDesc.ByteWidth = UINT(count * sizeof(unsigned short));
		IndexBufferDesc.StructureByteStride = sizeof(unsigned short);
		D3D11_SUBRESOURCE_DATA IndexSubresourceData = { 0 };
		IndexSubresourceData.pSysMem = indices.data();
		// 创建 D3D11 索引缓冲区资源
		// Create D3D11 index buffer resource
		GRAPHICS_THROW_EXCEPTION(GetDevice(&graphics)->CreateBuffer(&IndexBufferDesc, 
			&IndexSubresourceData, &pIndexBuffer));
	}

	IndexBuffer::IndexBuffer(Graphics& graphics, const std::vector<unsigned int>& indices)
		: count((UINT)indices.size()), indexFormat(DXGI_FORMAT_R32_UINT)
	{
		HRESULT hr;
		D3D11_BUFFER_DESC IndexBufferDesc = { 0 };
		// 设置为索引缓冲区绑定类型
		// Set as index buffer bind type
		IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		// 默认使用方式，GPU 读写，CPU 不可访问
		// Default usage, GPU read-write, CPU not accessible
		IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		IndexBufferDesc.CPUAccessFlags = 0u;
		IndexBufferDesc.MiscFlags = 0u;
		IndexBufferDesc.ByteWidth = UINT(count * sizeof(unsigned int));
		IndexBufferDesc.StructureByteStride = sizeof(unsigned int);
		D3D11_SUBRESOURCE_DATA IndexSubresourceData = { 0 };
		IndexSubresourceData.pSysMem = indices.data();
		// 创建 D3D11 索引缓冲区资源
		// Create D3D11 index buffer resource
		GRAPHICS_THROW_EXCEPTION(GetDevice(&graphics)->CreateBuffer(&IndexBufferDesc,
			&IndexSubresourceData, &pIndexBuffer));
	}

	void IndexBuffer::Bind(Graphics& graphics) noexcept
	{
		// 将索引缓冲区绑定到输入装配阶段，使用存储的索引格式（16位或32位），偏移量为 0
		// Bind index buffer to input assembler stage, using stored index format (16-bit or 32-bit), offset 0
		GetDevicContext(&graphics)->IASetIndexBuffer(pIndexBuffer.Get(), this->indexFormat, 0u);
	}

	UINT IndexBuffer::GetCount() const noexcept
	{
		return this->count;
	}
}

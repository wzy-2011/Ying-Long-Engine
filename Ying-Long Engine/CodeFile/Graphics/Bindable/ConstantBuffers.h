/**
 * @file ConstantBuffers.h
 * @brief DX11 常量缓冲区模板类头文件 / DX11 constant buffer template class header file
 *
 * 提供通用的常量缓冲区模板类，以及顶点着色器和像素着色器专用的常量缓冲区子类。
 * Provides a generic constant buffer template class, along with specialized
 * subclasses for vertex shader and pixel shader constant buffers.
 */

#pragma once
#include "Bindable.h"
#include "../Graphics.h"

namespace YingLong
{
	/**
	 * @brief 通用常量缓冲区模板类 / Generic constant buffer template class
	 *
	 * 封装 D3D11 常量缓冲区，支持动态更新 CPU 端数据并映射到 GPU。
	 * Encapsulates a D3D11 constant buffer, supporting dynamic CPU-side data
	 * updates and mapping to GPU.
	 *
	 * @tparam C 常量缓冲区数据结构类型 / Constant buffer data structure type
	 */
	template<typename C>
	class  ConstantBuffer : public Bindable
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		ConstantBuffer() = default;

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 */
		ConstantBuffer(const ConstantBuffer&) = default;

		/**
		 * @brief 更新常量缓冲区数据 / Update constant buffer data
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param consts 新的常量数据 / New constant data
		 *
		 * 使用 D3D11_MAP_WRITE_DISCARD 映射方式更新 GPU 缓冲区内容，
		 * 避免与 GPU 读写冲突。
		 * Updates GPU buffer content using D3D11_MAP_WRITE_DISCARD mapping
		 * to avoid read-write conflicts with GPU.
		 */
		void Update(Graphics& graphics, const C& consts)
		{
			// 更新本地缓存的常量数据
			// Update locally cached constant data
			this->CBufferData = consts;

			HRESULT hr;
			D3D11_MAPPED_SUBRESOURCE MappedSubResource;
			// 映射缓冲区到 CPU 可访问内存，使用 WRITE_DISCARD 丢弃旧数据
			// Map buffer to CPU-accessible memory, use WRITE_DISCARD to discard old data
			GRAPHICS_THROW_EXCEPTION(GetDevicContext(&graphics)->Map(
				pConstantBuffer.Get(),
				0u,
				D3D11_MAP_WRITE_DISCARD,
				0u,
				&MappedSubResource));
			// 将新数据拷贝到映射的内存区域
			// Copy new data to the mapped memory region
			memcpy(MappedSubResource.pData, &this->CBufferData, sizeof(this->CBufferData));
			// 取消映射，让 GPU 可以访问更新后的数据
			// Unmap so GPU can access the updated data
			GetDevicContext(&graphics)->Unmap(pConstantBuffer.Get(), 0u);
		}

		/**
		 * @brief 构造函数（带初始数据）/ Constructor with initial data
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param consts 初始常量数据 / Initial constant data
		 * @param index 常量缓冲区槽位索引 / Constant buffer slot index (default: 0)
		 *
		 * 创建一个带有初始数据的动态常量缓冲区。
		 * Creates a dynamic constant buffer with initial data.
		 */
		ConstantBuffer(Graphics& graphics, const C& consts, UINT index = 0)
		{
			this->CBufferData = consts;
			this->CBufferIndex = index;

			HRESULT hr;
			D3D11_BUFFER_DESC ConstantBufferDesc = { 0 };
			// 设置为常量缓冲区绑定类型
			// Set as constant buffer bind type
			ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			// 动态使用方式，CPU 可写，GPU 可读
			// Dynamic usage, CPU writable, GPU readable
			ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			// 启用 CPU 写入访问
			// Enable CPU write access
			ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			ConstantBufferDesc.MiscFlags = 0u;
			ConstantBufferDesc.ByteWidth = sizeof(consts);
			ConstantBufferDesc.StructureByteStride = 0u;
			D3D11_SUBRESOURCE_DATA SubresourceData = { 0 };
			SubresourceData.pSysMem = &consts;
			GRAPHICS_THROW_EXCEPTION(GetDevice(&graphics)->CreateBuffer(&ConstantBufferDesc,
				&SubresourceData, &pConstantBuffer));
		}

		/**
		 * @brief 构造函数（无初始数据）/ Constructor without initial data
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param index 常量缓冲区槽位索引 / Constant buffer slot index (default: 0)
		 *
		 * 创建一个空的动态常量缓冲区，字节宽度自动对齐到 16 字节边界。
		 * Creates an empty dynamic constant buffer, with byte width
		 * automatically aligned to 16-byte boundaries.
		 */
		ConstantBuffer(Graphics& graphics, UINT index = 0)
		{
			this->CBufferIndex = index;

			HRESULT hr;
			D3D11_BUFFER_DESC ConstantBufferDesc = { 0 };
			ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			ConstantBufferDesc.MiscFlags = 0u;
			// 计算 16 字节对齐的缓冲区大小（HLSL 常量缓冲区要求 16 字节对齐）
			// Calculate 16-byte aligned buffer size (HLSL constant buffers require 16-byte alignment)
			ConstantBufferDesc.ByteWidth = sizeof(C) + (16 - (sizeof(C) % 16));
			ConstantBufferDesc.StructureByteStride = 0u;
			GRAPHICS_THROW_EXCEPTION(GetDevice(&graphics)->CreateBuffer(&ConstantBufferDesc,
				nullptr, &pConstantBuffer));
		}

	protected:
		WRL::ComPtr<ID3D11Buffer> pConstantBuffer; ///< D3D11 常量缓冲区对象 / D3D11 constant buffer object
		C CBufferData;                             ///< 本地缓存的常量数据 / Locally cached constant data
		UINT CBufferIndex;                         ///< 常量缓冲区绑定槽位索引 / Constant buffer bind slot index
	};

	/**
	 * @brief 顶点着色器常量缓冲区 / Vertex shader constant buffer
	 *
	 * 将常量缓冲区绑定到顶点着色器阶段的专用子类。
	 * Specialized subclass that binds the constant buffer to the vertex shader stage.
	 *
	 * @tparam C 常量缓冲区数据结构类型 / Constant buffer data structure type
	 */
	template<typename C>
	class VertexConstantBuffer : public ConstantBuffer<C>
	{
		using ConstantBuffer<C>::pConstantBuffer;
		using Bindable::GetDevicContext;

	public:
		using ConstantBuffer<C>::ConstantBuffer;

		/**
		 * @brief 绑定到顶点着色器阶段 / Bind to vertex shader stage
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将常量缓冲区绑定到顶点着色器的指定槽位。
		 * Binds the constant buffer to the specified slot of the vertex shader.
		 */
		void Bind(Graphics& graphics) noexcept override
		{
			Bindable::GetDevicContext(&graphics)->VSSetConstantBuffers(this->CBufferIndex, 1u,
				pConstantBuffer.GetAddressOf());
		}
	};

	/**
	 * @brief 像素着色器常量缓冲区 / Pixel shader constant buffer
	 *
	 * 将常量缓冲区绑定到像素着色器阶段的专用子类。
	 * Specialized subclass that binds the constant buffer to the pixel shader stage.
	 *
	 * @tparam C 常量缓冲区数据结构类型 / Constant buffer data structure type
	 */
	template<typename C>
	class PixelConstantBuffer : public ConstantBuffer<C>
	{
		using ConstantBuffer<C>::pConstantBuffer;
		using Bindable::GetDevicContext;
	public:
		using ConstantBuffer<C>::ConstantBuffer;

		/**
		 * @brief 绑定到像素着色器阶段 / Bind to pixel shader stage
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将常量缓冲区绑定到像素着色器的指定槽位。
		 * Binds the constant buffer to the specified slot of the pixel shader.
		 */
		void Bind(Graphics& graphics) noexcept override
		{
			Bindable::GetDevicContext(&graphics)->PSSetConstantBuffers(this->CBufferIndex, 1u,
				pConstantBuffer.GetAddressOf());
		}

		friend class PointLight;
		friend class LightManager;
	};
}

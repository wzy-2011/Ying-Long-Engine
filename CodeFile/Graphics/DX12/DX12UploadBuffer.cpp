/**
 * @file DX12UploadBuffer.cpp
 * @brief DX12 上传缓冲区实现文件 / DX12 Upload Buffer Implementation
 *
 * 本文件实现了 DX12UploadBuffer 类和 DX12ConstantBuffer 模板类，
 * 包括上传缓冲区的创建、内存映射、数据分配和复制等功能。
 *
 * This file implements the DX12UploadBuffer class and DX12ConstantBuffer template class,
 * including upload buffer creation, memory mapping, data allocation and copying, etc.
 */

#include "DX12UploadBuffer.h"
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 创建指定大小的上传缓冲区，使用 UPLOAD 堆类型，
     * 并将缓冲区映射到 CPU 内存地址空间。
     *
     * Creates an upload buffer of the specified size using UPLOAD heap type,
     * and maps the buffer to CPU memory address space.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @param bufferSize 缓冲区大小（字节）/ Buffer size in bytes
     * @throws std::runtime_error 如果设备为空、创建失败或映射失败
     *                             If device is null, creation fails, or mapping fails
     */
    DX12UploadBuffer::DX12UploadBuffer(ID3D12Device* device, UINT64 bufferSize)
        : BufferSize(bufferSize)       ///< 缓冲区大小 / Buffer size
        , CurrentOffset(0)             ///< 当前偏移初始化为0 / Current offset initialized to 0
        , MappedData(nullptr)          ///< 映射指针初始化为空 / Mapped pointer initialized to null
    {
        // 验证设备指针
        // Validate device pointer
        if (!device)
        {
            throw std::runtime_error("Null device passed to DX12UploadBuffer constructor");
        }

        // 配置缓冲区描述
        // Configure buffer description
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;         ///< 缓冲区维度 / Buffer dimension
        bufferDesc.Alignment = 0;                                        ///< 对齐（0表示默认）/ Alignment (0 means default)
        bufferDesc.Width = bufferSize;                                   ///< 缓冲区宽度（大小）/ Buffer width (size)
        bufferDesc.Height = 1;                                           ///< 高度（缓冲区为1）/ Height (1 for buffer)
        bufferDesc.DepthOrArraySize = 1;                                 ///< 深度/数组大小（缓冲区为1）/ Depth or array size (1 for buffer)
        bufferDesc.MipLevels = 1;                                        ///< Mip 等级（缓冲区为1）/ Mip levels (1 for buffer)
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;                         ///< 格式（缓冲区为未知）/ Format (unknown for buffer)
        bufferDesc.SampleDesc.Count = 1;                                 ///< 采样数 / Sample count
        bufferDesc.SampleDesc.Quality = 0;                               ///< 采样质量 / Sample quality
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;              ///< 行主序布局 / Row major layout
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;                     ///< 无标志 / No flags

        // 配置堆属性（UPLOAD 堆）
        // Configure heap properties (UPLOAD heap)
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;                          ///< 上传堆类型 / Upload heap type
        heapProps.CreationNodeMask = 1;                                   ///< 创建节点掩码 / Creation node mask
        heapProps.VisibleNodeMask = 1;                                    ///< 可见节点掩码 / Visible node mask

        // 创建提交资源
        // Create committed resource
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,   ///< 初始状态：通用读取 / Initial state: generic read
            nullptr,
            IID_PPV_ARGS(&pBuffer)
        );
        if (FAILED(hr) || !pBuffer)
        {
            throw std::runtime_error("Failed to create upload buffer");
        }

        // 映射缓冲区到 CPU 内存
        // Map the buffer to CPU memory
        hr = pBuffer->Map(0, nullptr, &MappedData);
        if (FAILED(hr) || !MappedData)
        {
            throw std::runtime_error("Failed to map upload buffer");
        }
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 解除缓冲区的 CPU 内存映射，并释放 D3D12 缓冲区资源。
     * Unmaps the buffer from CPU memory and releases the D3D12 buffer resource.
     */
    DX12UploadBuffer::~DX12UploadBuffer()
    {
        // 如果缓冲区和映射数据都存在，先解除映射
        // If buffer and mapped data both exist, unmap first
        if (pBuffer && MappedData)
        {
            pBuffer->Unmap(0, nullptr);
            MappedData = nullptr;
        }
        // 释放缓冲区资源
        // Release buffer resource
        pBuffer.Reset();
    }

    /**
     * @brief 在缓冲区中分配空间 / Allocate space in the buffer
     *
     * 从当前偏移位置分配指定大小的空间，并按指定对齐方式对齐。
     * 对齐通过向上取整到最近的对齐边界实现。
     *
     * Allocates space of the specified size from the current offset,
     * aligned by the specified alignment. Alignment is achieved by
     * rounding up to the nearest alignment boundary.
     *
     * @param size 要分配的大小（字节）/ Size to allocate in bytes
     * @param alignment 对齐方式（字节，默认256）/ Alignment in bytes (default 256)
     * @return 分配的偏移量 / Allocated offset
     * @throws std::runtime_error 如果缓冲区空间不足 / If buffer space is insufficient
     */
    UINT64 DX12UploadBuffer::Allocate(UINT64 size, UINT64 alignment)
    {
        // 将当前偏移向上对齐到指定的对齐边界
        // Align the current offset up to the specified alignment boundary
        UINT64 alignedOffset = (CurrentOffset + alignment - 1) & ~(alignment - 1);

        // 检查是否有足够的空间
        // Check if there is enough space
        if (alignedOffset + size > BufferSize)
        {
            throw std::runtime_error("DX12UploadBuffer::Allocate() - Buffer is full");
        }

        // 更新当前偏移位置
        // Update current offset position
        CurrentOffset = alignedOffset + size;
        return alignedOffset;
    }

    /**
     * @brief 复制数据到缓冲区 / Copy data to buffer
     *
     * 将数据从源地址复制到缓冲区的指定偏移位置。
     * Copies data from the source address to the specified offset in the buffer.
     *
     * @param offset 目标偏移量 / Destination offset
     * @param data 源数据指针 / Source data pointer
     * @param size 数据大小（字节）/ Data size in bytes
     */
    void DX12UploadBuffer::CopyData(UINT64 offset, const void* data, UINT64 size)
    {
        // 边界检查
        // Bounds check
        if (offset + size > BufferSize)
        {
            return;
        }

        // 复制数据到映射的内存
        // Copy data to mapped memory
        memcpy(static_cast<char*>(MappedData) + offset, data, size);
    }

    // ============================================================================
    // DX12ConstantBuffer template implementation
    // DX12ConstantBuffer 模板实现
    // ============================================================================

    /**
     * @brief 构造函数实现（模板）/ Constructor implementation (template)
     *
     * 创建指定数量的类型化常量缓冲区。每个元素自动对齐到256字节，
     * 这是 D3D12 常量缓冲区的硬件要求。
     *
     * Creates the specified number of typed constant buffers. Each element is
     * automatically aligned to 256 bytes, which is a hardware requirement
     * for D3D12 constant buffers.
     *
     * @tparam T 常量缓冲区的数据类型 / Data type of the constant buffer
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @param count 常量缓冲区数量 / Number of constant buffers
     */
    template<typename T>
    DX12ConstantBuffer<T>::DX12ConstantBuffer(ID3D12Device* device, UINT count)
        : Count(count)  ///< 元素数量 / Number of elements
    {
        // 常量缓冲区必须 256 字节对齐
        // Constant buffers must be 256-byte aligned
        ElementSize = (sizeof(T) + 255) & ~255;  ///< 向上对齐到256字节 / Align up to 256 bytes
        BufferSize = ElementSize * count;        ///< 总缓冲区大小 / Total buffer size

        // 配置缓冲区描述
        // Configure buffer description
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = BufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // 配置堆属性（UPLOAD 堆）
        // Configure heap properties (UPLOAD heap)
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 创建提交资源
        // Create committed resource
        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pBuffer)
        );

        // 映射缓冲区到 CPU 内存
        // Map the buffer to CPU memory
        pBuffer->Map(0, nullptr, &MappedData);
    }

    /**
     * @brief 析构函数实现（模板）/ Destructor implementation (template)
     *
     * 解除缓冲区的 CPU 内存映射，并释放 D3D12 缓冲区资源。
     * Unmaps the buffer from CPU memory and releases the D3D12 buffer resource.
     *
     * @tparam T 常量缓冲区的数据类型 / Data type of the constant buffer
     */
    template<typename T>
    DX12ConstantBuffer<T>::~DX12ConstantBuffer()
    {
        // 如果缓冲区和映射数据都存在，先解除映射
        // If buffer and mapped data both exist, unmap first
        if (pBuffer && MappedData)
        {
            pBuffer->Unmap(0, nullptr);
            MappedData = nullptr;
        }
        // 释放缓冲区资源
        // Release buffer resource
        pBuffer.Reset();
    }

    /**
     * @brief 复制数据到指定元素（模板）/ Copy data to specific element (template)
     *
     * 将数据复制到指定索引的常量缓冲区元素中。
     * Copies data to the constant buffer element at the specified index.
     *
     * @tparam T 常量缓冲区的数据类型 / Data type of the constant buffer
     * @param index 元素索引 / Element index
     * @param data 源数据引用 / Source data reference
     */
    template<typename T>
    void DX12ConstantBuffer<T>::CopyData(UINT index, const T& data)
    {
        // 边界检查
        // Bounds check
        if (index >= Count)
        {
            return;
        }

        // 复制数据到映射的内存中的指定元素位置
        // Copy data to the specified element position in mapped memory
        memcpy(static_cast<char*>(MappedData) + index * ElementSize, &data, sizeof(T));
    }

    /**
     * @brief 获取指定元素的 GPU 虚拟地址（模板）/ Get GPU virtual address for specific element (template)
     *
     * 返回指定索引的常量缓冲区元素的 GPU 虚拟地址，
     * 用于在根签名中设置常量缓冲区视图。
     *
     * Returns the GPU virtual address of the constant buffer element at the
     * specified index, used for setting constant buffer views in the root signature.
     *
     * @tparam T 常量缓冲区的数据类型 / Data type of the constant buffer
     * @param index 元素索引 / Element index
     * @return GPU 虚拟地址 / GPU virtual address
     */
    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS DX12ConstantBuffer<T>::GetGPUAddress(UINT index) const noexcept
    {
        return pBuffer->GetGPUVirtualAddress() + index * ElementSize;
    }
}

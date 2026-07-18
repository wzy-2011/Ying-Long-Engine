/**
 * @file DX12UploadBuffer.h
 * @brief DX12 上传缓冲区头文件 / DX12 Upload Buffer Header
 *
 * 本文件定义了 DX12UploadBuffer 类和 DX12ConstantBuffer 模板类，
 * 用于创建和管理 GPU 上传缓冲区，用于将 CPU 数据上传到 GPU 资源。
 *
 * This file defines the DX12UploadBuffer class and DX12ConstantBuffer template class,
 * used for creating and managing GPU upload buffers for uploading CPU data to GPU resources.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>

namespace YingLong
{
    /**
     * @brief DX12 上传缓冲区类 / DX12 Upload Buffer Class
     *
     * DX12UploadBuffer 类封装了 D3D12 上传缓冲区，提供：
     * - 上传缓冲区的创建和管理
     * - CPU 端内存映射
     * - 内存分配和对齐
     * - 数据复制功能
     *
     * 上传缓冲区位于 UPLOAD 堆上，CPU 可以写入，GPU 可以读取，
     * 常用于将常量数据、纹理数据等从 CPU 上传到 GPU。
     *
     * The DX12UploadBuffer class encapsulates the D3D12 upload buffer, providing:
     * - Upload buffer creation and management
     * - CPU-side memory mapping
     * - Memory allocation and alignment
     * - Data copy functionality
     *
     * Upload buffers reside on the UPLOAD heap, writable by CPU and readable by GPU,
     * commonly used for uploading constant data, texture data, etc. from CPU to GPU.
     */
    class DX12UploadBuffer
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 创建指定大小的上传缓冲区并映射到 CPU 内存。
         * Creates an upload buffer of the specified size and maps it to CPU memory.
         *
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @param bufferSize 缓冲区大小（字节）/ Buffer size in bytes
         * @throws std::runtime_error 如果创建或映射失败 / If creation or mapping fails
         */
        DX12UploadBuffer(ID3D12Device* device, UINT64 bufferSize);

        /**
         * @brief 析构函数 / Destructor
         *
         * 解除内存映射并释放缓冲区资源。
         * Unmaps memory and releases buffer resources.
         */
        ~DX12UploadBuffer();

        /**
         * @brief 获取缓冲区资源 / Get the buffer resource
         * @return ID3D12Resource 指针 / ID3D12Resource pointer
         */
        ID3D12Resource* GetResource() const noexcept { return pBuffer.Get(); }

        /**
         * @brief 获取映射的 CPU 内存指针 / Get mapped CPU memory pointer
         * @return 映射的内存指针 / Mapped memory pointer
         */
        void* GetMappedData() const noexcept { return MappedData; }

        /**
         * @brief 在缓冲区中分配空间 / Allocate space in the buffer
         *
         * 从当前偏移位置分配指定大小的空间，并按指定对齐方式对齐。
         * Allocates space of the specified size from the current offset,
         * aligned by the specified alignment.
         *
         * @param size 要分配的大小（字节）/ Size to allocate in bytes
         * @param alignment 对齐方式（字节，默认256）/ Alignment in bytes (default 256)
         * @return 分配的偏移量 / Allocated offset
         * @throws std::runtime_error 如果缓冲区空间不足 / If buffer space is insufficient
         */
        UINT64 Allocate(UINT64 size, UINT64 alignment = 256);

        /**
         * @brief 重置分配 / Reset allocation
         *
         * 将当前偏移重置为0，可以重新使用整个缓冲区。
         * Resets the current offset to 0, allowing reuse of the entire buffer.
         */
        void Reset() { CurrentOffset = 0; }

        /**
         * @brief 复制数据到缓冲区 / Copy data to buffer
         * @param offset 目标偏移量 / Destination offset
         * @param data 源数据指针 / Source data pointer
         * @param size 数据大小（字节）/ Data size in bytes
         */
        void CopyData(UINT64 offset, const void* data, UINT64 size);

        /**
         * @brief 获取缓冲区总大小 / Get total buffer size
         * @return 缓冲区大小（字节）/ Buffer size in bytes
         */
        UINT64 GetBufferSize() const noexcept { return BufferSize; }

        /**
         * @brief 获取剩余空间 / Get remaining space
         * @return 剩余空间大小（字节）/ Remaining space in bytes
         */
        UINT64 GetRemainingSpace() const noexcept { return BufferSize - CurrentOffset; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;  ///< D3D12 缓冲区资源 / D3D12 buffer resource
        void* MappedData;                                 ///< 映射的 CPU 内存指针 / Mapped CPU memory pointer
        UINT64 BufferSize;                                ///< 缓冲区总大小 / Total buffer size
        UINT64 CurrentOffset;                             ///< 当前分配偏移 / Current allocation offset
    };

    /**
     * @brief DX12 类型化常量缓冲区模板类 / DX12 Typed Constant Buffer Template Class
     *
     * DX12ConstantBuffer 是一个类型化的常量缓冲区模板类，提供：
     * - 指定类型 T 的常量缓冲区数组
     * - 自动 256 字节对齐（常量缓冲区要求）
     * - 按索引访问和更新单个常量缓冲区
     * - 获取 GPU 虚拟地址
     *
     * 每个元素的大小会自动向上对齐到 256 字节的倍数，
     * 这是 D3D12 常量缓冲区的硬件要求。
     *
     * DX12ConstantBuffer is a typed constant buffer template class, providing:
     * - Array of constant buffers of specified type T
     * - Automatic 256-byte alignment (required for constant buffers)
     * - Per-index access and update of individual constant buffers
     * - GPU virtual address retrieval
     *
     * Each element's size is automatically aligned up to a multiple of 256 bytes,
     * which is a hardware requirement for D3D12 constant buffers.
     *
     * @tparam T 常量缓冲区的数据类型 / Data type of the constant buffer
     */
    template<typename T>
    class DX12ConstantBuffer
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 创建指定数量的类型化常量缓冲区。
         * Creates the specified number of typed constant buffers.
         *
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @param count 常量缓冲区数量 / Number of constant buffers
         */
        DX12ConstantBuffer(ID3D12Device* device, UINT count = 1);

        /**
         * @brief 析构函数 / Destructor
         *
         * 解除内存映射并释放缓冲区资源。
         * Unmaps memory and releases buffer resources.
         */
        ~DX12ConstantBuffer();

        /**
         * @brief 获取缓冲区资源 / Get the buffer resource
         * @return ID3D12Resource 指针 / ID3D12Resource pointer
         */
        ID3D12Resource* GetResource() const noexcept { return pBuffer.Get(); }

        /**
         * @brief 获取映射的 CPU 内存指针 / Get mapped CPU memory pointer
         * @return 类型化的映射内存指针 / Typed mapped memory pointer
         */
        T* GetMappedData() const noexcept { return reinterpret_cast<T*>(MappedData); }

        /**
         * @brief 复制数据到指定元素 / Copy data to specific element
         * @param index 元素索引 / Element index
         * @param data 源数据引用 / Source data reference
         */
        void CopyData(UINT index, const T& data);

        /**
         * @brief 获取指定元素的 GPU 虚拟地址 / Get GPU virtual address for specific element
         * @param index 元素索引 / Element index
         * @return GPU 虚拟地址 / GPU virtual address
         */
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(UINT index) const noexcept;

        /**
         * @brief 获取缓冲区总大小 / Get total buffer size
         * @return 缓冲区大小（字节）/ Buffer size in bytes
         */
        UINT64 GetBufferSize() const noexcept { return BufferSize; }

        /**
         * @brief 获取元素数量 / Get element count
         * @return 元素数量 / Number of elements
         */
        UINT GetCount() const noexcept { return Count; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;  ///< D3D12 缓冲区资源 / D3D12 buffer resource
        void* MappedData;                                 ///< 映射的 CPU 内存指针 / Mapped CPU memory pointer
        UINT64 BufferSize;                                ///< 缓冲区总大小 / Total buffer size
        UINT Count;                                       ///< 元素数量 / Number of elements
        UINT64 ElementSize;                               ///< 单个元素大小（含256字节对齐）/ Single element size (with 256-byte alignment)
    };
}

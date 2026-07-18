/**
 * @file DX12DescriptorHeap.h
 * @brief DX12 描述符堆头文件 / DX12 Descriptor Heap Header
 *
 * 本文件定义了 DX12DescriptorHeap 类，封装了 D3D12 描述符堆的创建、
 * 管理和描述符分配功能。描述符堆是 D3D12 中用于存储描述符（如
 * CBV、SRV、UAV、RTV、DSV、Sampler 等）的内存区域。
 *
 * This file defines the DX12DescriptorHeap class, which encapsulates the creation,
 * management, and descriptor allocation of D3D12 descriptor heaps. A descriptor heap
 * is a memory region in D3D12 used to store descriptors (such as CBV, SRV, UAV,
 * RTV, DSV, Sampler, etc.).
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace YingLong
{
    /**
     * @brief DX12 描述符堆类 / DX12 Descriptor Heap Class
     *
     * DX12DescriptorHeap 类封装了 D3D12 描述符堆的管理，提供：
     * - 描述符堆的创建和销毁
     * - 描述符的分配和回收
     * - CPU/GPU 描述符句柄的获取
     * - 堆状态查询（剩余空间、是否着色器可见等）
     *
     * The DX12DescriptorHeap class encapsulates D3D12 descriptor heap management, providing:
     * - Descriptor heap creation and destruction
     * - Descriptor allocation and deallocation
     * - CPU/GPU descriptor handle retrieval
     * - Heap state queries (free space, shader visibility, etc.)
     */
    class DX12DescriptorHeap
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 创建指定类型和大小的描述符堆。
         * Creates a descriptor heap of the specified type and size.
         *
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @param type 描述符堆类型 / Descriptor heap type
         * @param numDescriptors 描述符数量 / Number of descriptors
         * @param flags 描述符堆标志（默认为着色器可见）
         *              Descriptor heap flags (shader visible by default)
         */
        DX12DescriptorHeap(
            ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            UINT numDescriptors,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        );

        /**
         * @brief 析构函数 / Destructor
         *
         * 释放描述符堆资源。
         * Releases descriptor heap resources.
         */
        ~DX12DescriptorHeap();

        /**
         * @brief 获取描述符堆对象 / Get the heap itself
         * @return ID3D12DescriptorHeap 指针 / ID3D12DescriptorHeap pointer
         */
        ID3D12DescriptorHeap* GetHeap() const noexcept { return pHeap.Get(); }

        /**
         * @brief 获取指定索引的 CPU 描述符句柄 / Get CPU descriptor handle for specified index
         * @param index 描述符索引 / Descriptor index
         * @return CPU 描述符句柄 / CPU descriptor handle
         */
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const noexcept;

        /**
         * @brief 获取指定索引的 GPU 描述符句柄 / Get GPU descriptor handle for specified index
         * @param index 描述符索引 / Descriptor index
         * @return GPU 描述符句柄 / GPU descriptor handle
         */
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const noexcept;

        /**
         * @brief 获取单个描述符的大小 / Get descriptor size
         * @return 描述符大小（字节）/ Descriptor size in bytes
         */
        UINT GetDescriptorSize() const noexcept { return DescriptorSize; }

        /**
         * @brief 获取描述符总数 / Get number of descriptors
         * @return 描述符总数 / Total number of descriptors
         */
        UINT GetNumDescriptors() const noexcept { return NumDescriptors; }

        /**
         * @brief 分配一个描述符槽 / Allocate a descriptor slot
         * @return 已分配的描述符索引 / Allocated descriptor index
         * @throws std::runtime_error 如果堆已满 / If heap is full
         */
        UINT Allocate();

        /**
         * @brief 检查是否还有空间分配更多描述符 / Check if there is space for more descriptors
         * @return 是否有空间 / Whether there is space
         */
        bool HasSpace() const noexcept;

        /**
         * @brief 获取空闲描述符数量 / Get number of free descriptors
         * @return 空闲描述符数量 / Number of free descriptors
         */
        UINT GetFreeCount() const noexcept;

        /**
         * @brief 重置分配计数器 / Reset allocation counter
         *
         * 将当前分配索引重置为0，相当于释放所有已分配的描述符。
         * Resets the current allocation index to 0, effectively deallocating all allocated descriptors.
         */
        void ResetAllocation() { CurrentIndex = 0; }

        /**
         * @brief 检查堆是否着色器可见 / Check if heap is shader visible
         * @return 是否着色器可见 / Whether shader visible
         */
        bool IsShaderVisible() const noexcept { return Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; }

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;  ///< D3D12 描述符堆对象 / D3D12 descriptor heap object
        D3D12_DESCRIPTOR_HEAP_TYPE Type;                      ///< 描述符堆类型 / Descriptor heap type
        D3D12_DESCRIPTOR_HEAP_FLAGS Flags;                    ///< 描述符堆标志 / Descriptor heap flags
        UINT NumDescriptors;                                  ///< 描述符总数 / Total number of descriptors
        UINT DescriptorSize;                                  ///< 单个描述符大小（字节）/ Single descriptor size in bytes
        UINT CurrentIndex;                                    ///< 当前分配索引 / Current allocation index
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapStart;             ///< CPU 堆起始句柄 / CPU heap start handle
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHeapStart;             ///< GPU 堆起始句柄 / GPU heap start handle
    };
}

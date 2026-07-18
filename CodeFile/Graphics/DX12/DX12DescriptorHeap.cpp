/**
 * @file DX12DescriptorHeap.cpp
 * @brief DX12 描述符堆实现文件 / DX12 Descriptor Heap Implementation
 *
 * 本文件实现了 DX12DescriptorHeap 类的所有方法，包括描述符堆的创建、
 * 描述符分配以及 CPU/GPU 句柄的计算。
 *
 * This file implements all methods of the DX12DescriptorHeap class, including
 * descriptor heap creation, descriptor allocation, and CPU/GPU handle calculation.
 */

#include "DX12DescriptorHeap.h"
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 创建 D3D12 描述符堆并初始化相关状态。
     * Creates the D3D12 descriptor heap and initializes related state.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @param type 描述符堆类型 / Descriptor heap type
     * @param numDescriptors 描述符数量 / Number of descriptors
     * @param flags 描述符堆标志 / Descriptor heap flags
     * @throws std::runtime_error 如果设备为空或创建失败
     *                             If device is null or creation fails
     */
    DX12DescriptorHeap::DX12DescriptorHeap(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        UINT numDescriptors,
        D3D12_DESCRIPTOR_HEAP_FLAGS flags)
        : Type(type)                    ///< 描述符堆类型 / Descriptor heap type
        , Flags(flags)                  ///< 描述符堆标志 / Descriptor heap flags
        , NumDescriptors(numDescriptors)  ///< 描述符总数 / Total number of descriptors
        , CurrentIndex(0)               ///< 当前分配索引初始化为0 / Current allocation index initialized to 0
        , DescriptorSize(0)             ///< 描述符大小初始化为0 / Descriptor size initialized to 0
    {
        // 验证设备指针
        // Validate device pointer
        if (!device)
        {
            throw std::runtime_error("Null device passed to DX12DescriptorHeap constructor");
        }

        // 配置描述符堆描述
        // Configure descriptor heap description
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = type;                    ///< 描述符堆类型 / Descriptor heap type
        heapDesc.NumDescriptors = numDescriptors;  ///< 描述符数量 / Number of descriptors
        heapDesc.Flags = flags;                  ///< 堆标志 / Heap flags
        heapDesc.NodeMask = 0;                   ///< 节点掩码（单GPU）/ Node mask (single GPU)

        // 创建描述符堆
        // Create descriptor heap
        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pHeap));
        if (FAILED(hr) || !pHeap)
        {
            throw std::runtime_error("Failed to create descriptor heap");
        }

        // 获取该类型描述符的增量大小
        // Get the increment size for this type of descriptor
        DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

        // 获取 CPU 堆起始句柄
        // Get CPU heap start handle
        CPUHeapStart = pHeap->GetCPUDescriptorHandleForHeapStart();
        
        // 如果堆是着色器可见的，获取 GPU 堆起始句柄
        // If heap is shader visible, get GPU heap start handle
        if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            GPUHeapStart = pHeap->GetGPUDescriptorHandleForHeapStart();
        }
        else
        {
            GPUHeapStart.ptr = 0;    ///< 非着色器可见堆没有 GPU 句柄 / Non-shader-visible heap has no GPU handle
        }
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 释放描述符堆的 ComPtr 引用。
     * Releases the ComPtr reference to the descriptor heap.
     */
    DX12DescriptorHeap::~DX12DescriptorHeap()
    {
        pHeap.Reset();
    }

    /**
     * @brief 获取指定索引的 CPU 描述符句柄 / Get CPU descriptor handle for specified index
     *
     * 通过起始句柄加上索引乘以描述符大小计算得到。
     * Calculated by adding index multiplied by descriptor size to the start handle.
     *
     * @param index 描述符索引 / Descriptor index
     * @return CPU 描述符句柄 / CPU descriptor handle
     */
    D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetCPUHandle(UINT index) const noexcept
    {
        // 计算指定索引的 CPU 句柄
        // Calculate CPU handle at specified index
        D3D12_CPU_DESCRIPTOR_HANDLE handle = CPUHeapStart;
        handle.ptr += static_cast<UINT64>(index) * DescriptorSize;
        return handle;
    }

    /**
     * @brief 获取指定索引的 GPU 描述符句柄 / Get GPU descriptor handle for specified index
     *
     * 通过起始句柄加上索引乘以描述符大小计算得到。
     * Calculated by adding index multiplied by descriptor size to the start handle.
     *
     * @param index 描述符索引 / Descriptor index
     * @return GPU 描述符句柄 / GPU descriptor handle
     */
    D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetGPUHandle(UINT index) const noexcept
    {
        // 计算指定索引的 GPU 句柄
        // Calculate GPU handle at specified index
        D3D12_GPU_DESCRIPTOR_HANDLE handle = GPUHeapStart;
        handle.ptr += static_cast<UINT64>(index) * DescriptorSize;
        return handle;
    }

    /**
     * @brief 分配一个描述符槽 / Allocate a descriptor slot
     *
     * 分配当前索引位置的描述符槽，并将索引前移。
     * 采用简单的线性分配策略，通过 ResetAllocation() 重置。
     *
     * Allocates the descriptor slot at the current index and advances the index.
     * Uses a simple linear allocation strategy, reset via ResetAllocation().
     *
     * @return 已分配的描述符索引 / Allocated descriptor index
     * @throws std::runtime_error 如果堆已满 / If heap is full
     */
    UINT DX12DescriptorHeap::Allocate()
    {
        // 检查堆是否已满
        // Check if heap is full
        if (CurrentIndex >= NumDescriptors)
        {
            throw std::runtime_error("DX12DescriptorHeap::Allocate() - Heap is full");
        }

        // 保存当前索引作为分配结果
        // Save current index as allocation result
        UINT allocatedIndex = CurrentIndex;

        // 前移当前索引
        // Advance current index
        CurrentIndex++;

        return allocatedIndex;
    }

    /**
     * @brief 检查是否还有空间分配更多描述符 / Check if there is space for more descriptors
     * @return 是否有空间 / Whether there is space
     */
    bool DX12DescriptorHeap::HasSpace() const noexcept
    {
        return CurrentIndex < NumDescriptors;
    }

    /**
     * @brief 获取空闲描述符数量 / Get number of free descriptors
     * @return 空闲描述符数量 / Number of free descriptors
     */
    UINT DX12DescriptorHeap::GetFreeCount() const noexcept
    {
        return NumDescriptors - CurrentIndex;
    }
}

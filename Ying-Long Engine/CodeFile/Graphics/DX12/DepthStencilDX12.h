/**
 * @file DepthStencilDX12.h
 * @brief DX12 深度模板缓冲区类定义 / DX12 depth stencil buffer class definition
 *
 * 本文件定义了 DepthStencilDX12 类，用于创建和管理
 * DX12 深度模板缓冲区资源，支持 DSV 创建、SRV 创建（用于阴影贴图等）、
 * 状态转换、清除等功能。
 *
 * This file defines the DepthStencilDX12 class for creating and managing
 * DX12 depth stencil buffer resources. Supports DSV creation, SRV creation
 * (for shadow mapping, etc.), state transitions, clearing, and more.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgiformat.h>
#include <wrl/client.h>
#include <cstdint>

namespace YingLong
{
    class DX12Core;

    /**
     * @brief DX12 深度模板缓冲区类 / DX12 depth stencil buffer class
     *
     * DepthStencilDX12 封装了 DX12 深度模板缓冲区的创建、绑定和管理功能。
     * 支持创建深度模板视图（DSV）和着色器资源视图（SRV），
     * 可用于深度测试、模板测试以及阴影贴图等高级渲染技术。
     * SRV 格式会根据深度格式自动进行类型转换（如 D24_UNORM_S8_UINT -> R24_UNORM_X8_TYPELESS）。
     *
     * 关键设计：DSV 索引从 DSV 描述符堆动态分配，而非硬编码为 0。
     * 这允许多个 DepthStencilDX12 实例（如主深度模板和场景深度模板）
     * 各自拥有独立的 DSV 描述符，避免互相覆盖。
     *
     * DepthStencilDX12 encapsulates the creation, binding, and management of
     * DX12 depth stencil buffers. Supports creating depth stencil views (DSV)
     * and shader resource views (SRV), and can be used for depth testing,
     * stencil testing, and advanced rendering techniques like shadow mapping.
     * SRV formats are automatically type-converted based on the depth format
     * (e.g., D24_UNORM_S8_UINT -> R24_UNORM_X8_TYPELESS).
     *
     * Key design: DSV index is dynamically allocated from the DSV descriptor
     * heap, not hardcoded to 0. This allows multiple DepthStencilDX12 instances
     * (e.g., main depth stencil and scene depth stencil) to each have their
     * own independent DSV descriptor, avoiding overwriting each other.
     */
    class DepthStencilDX12
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         */
        DepthStencilDX12();

        /**
         * @brief 析构函数 / Destructor
         */
        ~DepthStencilDX12();

        /**
         * @brief 初始化深度模板缓冲区 / Initialize the depth stencil buffer
         * @param core DX12Core 引用 / DX12Core reference
         * @param width 宽度 / Width
         * @param height 高度 / Height
         * @param format 深度模板格式 / Depth stencil format
         * @param msaaCount MSAA 采样数 / MSAA sample count
         * @param msaaQuality MSAA 质量等级 / MSAA quality level
         */
        void Initialize(
            DX12Core& core,
            int width,
            int height,
            ::DXGI_FORMAT format = ::DXGI_FORMAT_D24_UNORM_S8_UINT,
            UINT msaaCount = 1,
            UINT msaaQuality = 0
        );

        /**
         * @brief 关闭深度模板缓冲区 / Shutdown the depth stencil buffer
         */
        void Shutdown();

        /**
         * @brief 绑定为深度模板 / Bind as depth stencil
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Bind(::ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 清除深度模板缓冲区 / Clear the depth stencil buffer
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         * @param clearDepth 是否清除深度 / Whether to clear depth
         * @param clearStencil 是否清除模板 / Whether to clear stencil
         */
        void Clear(::ID3D12GraphicsCommandList* commandList, bool clearDepth = true, bool clearStencil = true);

        /**
         * @brief 转换到指定资源状态 / Transition to specific resource state
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         * @param newState 新的资源状态 / New resource state
         */
        void TransitionTo(
            ::ID3D12GraphicsCommandList* commandList,
            ::D3D12_RESOURCE_STATES newState
        );

        /**
         * @brief 获取底层资源指针 / Get the underlying resource pointer
         * @return 指向 ID3D12Resource 的指针 / Pointer to ID3D12Resource
         */
        ::ID3D12Resource* GetResource() const noexcept { return pResource.Get(); }

        /**
         * @brief 获取 DSV CPU 描述符句柄 / Get the DSV CPU descriptor handle
         * @return DSV CPU 句柄 / DSV CPU handle
         */
        ::D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept { return DSVHandle; }

        /**
         * @brief 获取 SRV CPU 描述符句柄（作为深度纹理读取用） / Get the SRV CPU descriptor handle (for reading depth as texture)
         * @return SRV CPU 句柄 / SRV CPU handle
         */
        ::D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const noexcept { return SRVHandle; }

        /**
         * @brief 获取 SRV GPU 描述符句柄 / Get the SRV GPU descriptor handle
         * @return SRV GPU 句柄 / SRV GPU handle
         */
        ::D3D12_GPU_DESCRIPTOR_HANDLE GetGPU_SRVHandle() const noexcept { return GPU_SRVHandle; }

        /**
         * @brief 获取宽度 / Get the width
         * @return 宽度值 / Width value
         */
        int GetWidth() const noexcept { return Width; }

        /**
         * @brief 获取高度 / Get the height
         * @return 高度值 / Height value
         */
        int GetHeight() const noexcept { return Height; }

        /**
         * @brief 获取深度模板格式 / Get the depth stencil format
         * @return DXGI_FORMAT 枚举值 / DXGI_FORMAT enum value
         */
        ::DXGI_FORMAT GetFormat() const noexcept { return Format; }

        /**
         * @brief 检查是否已初始化 / Check if initialized
         * @return true 表示已初始化 / true if initialized
         */
        bool IsInitialized() const noexcept { return pResource != nullptr; }

        /**
         * @brief 检查是否有 SRV（用于阴影贴图等） / Check if has SRV (for shadow mapping etc.)
         * @return true 表示有 SRV / true if has SRV
         */
        bool HasSRV() const noexcept { return hasSRV; }

    private:
        /**
         * @brief 创建深度模板视图 / Create the depth stencil view
         * @param core DX12Core 引用 / DX12Core reference
         */
        void CreateDSV(DX12Core& core);

        /**
         * @brief 创建着色器资源视图 / Create the shader resource view
         * @param core DX12Core 引用 / DX12Core reference
         */
        void CreateSRV(DX12Core& core);

        DX12Core* pCore;                 ///< DX12Core 指针（用于 Shutdown 时释放描述符）/ DX12Core pointer (for releasing descriptors in Shutdown)
        Microsoft::WRL::ComPtr<::ID3D12Resource> pResource;  ///< 深度模板资源指针 / Depth stencil resource pointer
        ::D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle;             ///< DSV CPU 描述符句柄 / DSV CPU descriptor handle
        ::D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;             ///< SRV CPU 描述符句柄 / SRV CPU descriptor handle
        ::D3D12_GPU_DESCRIPTOR_HANDLE GPU_SRVHandle;         ///< SRV GPU 描述符句柄 / SRV GPU descriptor handle

        int Width;                       ///< 宽度 / Width
        int Height;                      ///< 高度 / Height
        ::DXGI_FORMAT Format;            ///< 深度模板格式 / Depth stencil format
        UINT MSAACount;                  ///< MSAA 采样数 / MSAA sample count
        UINT MSAAQuality;                ///< MSAA 质量等级 / MSAA quality level

        UINT DSVHeapIndex;               ///< DSV 描述符堆索引（UINT_MAX 表示未分配）/ DSV descriptor heap index (UINT_MAX means unallocated)
        UINT SRVHeapIndex;               ///< SRV 描述符堆索引 / SRV descriptor heap index
        bool hasSRV;                     ///< 是否有 SRV / Whether has SRV
        ::D3D12_RESOURCE_STATES CurrentState;  ///< 当前资源状态 / Current resource state
    };
}

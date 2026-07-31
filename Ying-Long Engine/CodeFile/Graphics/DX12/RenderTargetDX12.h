/**
 * @file RenderTargetDX12.h
 * @brief DX12 渲染目标类定义 / DX12 render target class definition
 *
 * 本文件定义了 RenderTargetDX12 类和 RenderTargetTypeDX12 枚举，
 * 用于创建和管理 DX12 渲染目标资源，支持后台缓冲区、离屏纹理输出
 * 和 MSAA 多种类型，提供 RTV/SRV 创建、状态转换、MSAA 解析等功能。
 *
 * This file defines the RenderTargetDX12 class and RenderTargetTypeDX12 enum
 * for creating and managing DX12 render target resources. Supports multiple
 * types including back buffer, offscreen texture output, and MSAA, providing
 * RTV/SRV creation, state transitions, MSAA resolve, and more.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgiformat.h>
#include <wrl/client.h>
#include <cstdint>

// COM 接口前向声明 / COM interface forward declarations
interface ID3D12DepthStencilView;

namespace YingLong
{
    /**
     * @brief 渲染目标类型枚举 / Render target type enumeration
     *
     * 定义了三种渲染目标类型：后台缓冲区（由交换链创建）、
     * 纹理输出（离屏渲染用）和 MSAA（多重采样抗锯齿）。
     *
     * Defines three render target types: BackBuffer (created by swap chain),
     * TextureOutput (for offscreen rendering), and MSAA (multi-sampled
     * anti-aliasing).
     */
    enum class RenderTargetTypeDX12
    {
        BackBuffer,        ///< 交换链后台缓冲区 / Swap chain back buffer
        TextureOutput,     ///< 离屏渲染纹理输出 / Texture resource for offscreen rendering
        MSAA               ///< 多重采样抗锯齿 / Multi-sampled anti-aliasing
    };

    class DX12Core;

    /**
     * @brief DX12 渲染目标类 / DX12 render target class
     *
     * RenderTargetDX12 封装了 DX12 渲染目标的创建、绑定和管理功能。
     * 支持从交换链缓冲区初始化或手动创建离屏渲染目标，
     * 可创建渲染目标视图（RTV）和着色器资源视图（SRV），
     * 支持资源状态转换和 MSAA 解析。
     *
     * RenderTargetDX12 encapsulates the creation, binding, and management of
     * DX12 render targets. Supports initialization from swap chain buffers or
     * manual creation of offscreen render targets, can create render target
     * views (RTV) and shader resource views (SRV), and supports resource state
     * transitions and MSAA resolve.
     */
    class RenderTargetDX12
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         */
        RenderTargetDX12();

        /**
         * @brief 析构函数 / Destructor
         */
        ~RenderTargetDX12();

        /**
         * @brief 初始化渲染目标 / Initialize the render target
         * @param core DX12Core 引用 / DX12Core reference
         * @param type 渲染目标类型 / Render target type
         * @param width 宽度 / Width
         * @param height 高度 / Height
         * @param format 像素格式 / Pixel format
         * @param msaaCount MSAA 采样数 / MSAA sample count
         * @param msaaQuality MSAA 质量等级 / MSAA quality level
         */
        void Initialize(
            DX12Core& core,
            RenderTargetTypeDX12 type,
            int width,
            int height,
            DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
            UINT msaaCount = 1,
            UINT msaaQuality = 0
        );

        /**
         * @brief 从交换链缓冲区初始化 / Initialize from existing swap chain buffer
         * @param core DX12Core 引用 / DX12Core reference
         * @param bufferIndex 缓冲区索引 / Buffer index
         */
        void InitializeFromSwapChain(
            DX12Core& core,
            UINT bufferIndex
        );

        /**
         * @brief 关闭渲染目标 / Shutdown the render target
         */
        void Shutdown();

        /**
         * @brief 绑定为渲染目标 / Bind as render target
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         * @param dsv 深度模板视图句柄指针（可选） / Depth stencil view handle pointer (optional)
         */
        void Bind(::ID3D12GraphicsCommandList* commandList, const ::D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr);

        /**
         * @brief 清除渲染目标 / Clear the render target
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         * @param color 清除颜色（RGBA 四分量） / Clear color (RGBA four components)
         */
        void Clear(::ID3D12GraphicsCommandList* commandList, const float color[4]);

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
         * @brief Present 后重置状态追踪 / Reset state tracking after Present
         *
         * 使用 FLIP_DISCARD 交换效果时，DXGI 在 Present 后隐式地将
         * 后台缓冲区重置为 COMMON 状态。此方法同步 CPU 端追踪。
         *
         * With FLIP_DISCARD swap effect, DXGI implicitly resets the
         * back buffer to COMMON state after Present. This method
         * synchronizes the CPU-side tracking.
         */
        void OnPresented();

        /**
         * @brief 获取底层资源指针 / Get the underlying resource pointer
         * @return 指向 ID3D12Resource 的指针 / Pointer to ID3D12Resource
         */
        ::ID3D12Resource* GetResource() const noexcept { return pResource.Get(); }

        /**
         * @brief 获取 RTV CPU 描述符句柄 / Get the RTV CPU descriptor handle
         * @return RTV CPU 句柄 / RTV CPU handle
         */
        ::D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const noexcept { return RTVHandle; }

        /**
         * @brief 获取 SRV CPU 描述符句柄（作为纹理读取用） / Get the SRV CPU descriptor handle (for reading as texture)
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
         * @brief 获取像素格式 / Get the pixel format
         * @return DXGI_FORMAT 枚举值 / DXGI_FORMAT enum value
         */
        ::DXGI_FORMAT GetFormat() const noexcept { return Format; }

        /**
         * @brief 查询是否有 SRV / Query whether SRV exists
         * @return 如果有 SRV 返回 true / true if SRV exists
         */
        bool HasShaderResourceView() const noexcept { return HasSRV; }

        /**
         * @brief 检查是否已初始化 / Check if initialized
         * @return true 表示已初始化 / true if initialized
         */
        bool IsInitialized() const noexcept { return pResource != nullptr; }

        /// Get SRV heap index for diagnostic purposes
        UINT GetSRVHeapIndex() const noexcept { return SRVHeapIndex; }

        /**
         * @brief 解析 MSAA 到非 MSAA 纹理 / Resolve MSAA to non-MSAA texture
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         * @param destRenderTarget 目标渲染目标引用 / Destination render target reference
         */
        void Resolve(
            ::ID3D12GraphicsCommandList* commandList,
            RenderTargetDX12& destRenderTarget
        );

    private:
        /**
         * @brief 创建渲染目标视图 / Create the render target view
         * @param core DX12Core 引用 / DX12Core reference
         */
        void CreateRTV(DX12Core& core);

        /**
         * @brief 创建着色器资源视图 / Create the shader resource view
         * @param core DX12Core 引用 / DX12Core reference
         */
        void CreateSRV(DX12Core& core);

        DX12Core* pCore;                 ///< DX12Core 指针（用于 Shutdown 时释放描述符）/ DX12Core pointer (for releasing descriptors in Shutdown)
        Microsoft::WRL::ComPtr<::ID3D12Resource> pResource;  ///< 渲染目标资源指针 / Render target resource pointer
        ::D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;             ///< RTV CPU 描述符句柄 / RTV CPU descriptor handle
        ::D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;             ///< SRV CPU 描述符句柄 / SRV CPU descriptor handle
        ::D3D12_GPU_DESCRIPTOR_HANDLE GPU_SRVHandle;         ///< SRV GPU 描述符句柄 / SRV GPU descriptor handle

        RenderTargetTypeDX12 Type;       ///< 渲染目标类型 / Render target type
        int Width;                       ///< 宽度 / Width
        int Height;                      ///< 高度 / Height
        ::DXGI_FORMAT Format;            ///< 像素格式 / Pixel format
        UINT MSAACount;                  ///< MSAA 采样数 / MSAA sample count
        UINT MSAAQuality;                ///< MSAA 质量等级 / MSAA quality level

        UINT RTVHeapIndex;               ///< RTV 描述符堆索引（UINT_MAX 表示未分配/由 DX12Core 预分配）/ RTV descriptor heap index (UINT_MAX means unallocated/pre-allocated by DX12Core)
        UINT SRVHeapIndex;               ///< SRV 描述符堆索引 / SRV descriptor heap index
        bool HasSRV;                     ///< 是否有 SRV / Whether has SRV
        bool bOwnsRTV;                   ///< 是否拥有 RTV 索引所有权（TextureOutput/MSAA 为 true，BackBuffer 为 false）/ Whether owns RTV index (true for TextureOutput/MSAA, false for BackBuffer)
        ::D3D12_RESOURCE_STATES CurrentState;  ///< 当前资源状态 / Current resource state
    };
}

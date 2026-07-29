/**
 * @file DX12Core.h
 * @brief DX12 核心模块头文件 / DX12 Core Module Header
 *
 * 本文件定义了 DX12Core 类，负责 DirectX 12 渲染核心的初始化、
 * 管理和生命周期控制，包括设备、命令队列、交换链、描述符堆等核心对象。
 *
 * This file defines the DX12Core class, which is responsible for the initialization,
 * management, and lifecycle control of the DirectX 12 rendering core, including
 * core objects such as device, command queue, swap chain, and descriptor heaps.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgiformat.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>

#include "DX12DescriptorHeap.h"
#include "DX12Fence.h"
#include "DX12UploadBuffer.h"
#include "DX12RootSignature.h"
#include "DX12PipelineState.h"
#include "GBuffer.h"

namespace YingLong
{
    /**
     * @brief 帧资源缓冲数量 / Frame resource count for buffering
     *
     * 用于双缓冲或多缓冲机制，避免 CPU 和 GPU 同时访问同一资源。
     * Used for double or multi-buffering mechanism to prevent CPU and GPU
     * from accessing the same resource simultaneously.
     */
    constexpr UINT FRAME_COUNT = 2;

    /**
     * @brief DX12 核心类 / DX12 Core Class
     *
     * DX12Core 类是 DirectX 12 渲染系统的核心管理器，负责：
     * - 创建和管理 D3D12 设备、命令队列、交换链
     * - 管理描述符堆（RTV、DSV、CBV/SRV/UAV、Sampler）
     * - 管理帧资源和 GPU 同步
     * - 提供资源创建的辅助函数
     *
     * The DX12Core class is the core manager of the DirectX 12 rendering system,
     * responsible for:
     * - Creating and managing D3D12 device, command queue, and swap chain
     * - Managing descriptor heaps (RTV, DSV, CBV/SRV/UAV, Sampler)
     * - Managing frame resources and GPU synchronization
     * - Providing helper functions for resource creation
     */
    class DX12Core
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 初始化 DX12Core 对象的默认状态。
         * Initializes the default state of the DX12Core object.
         */
        DX12Core();

        /**
         * @brief 析构函数 / Destructor
         *
         * 自动调用 Shutdown() 释放所有资源。
         * Automatically calls Shutdown() to release all resources.
         */
        ~DX12Core();

        /**
         * @brief 初始化 DX12 核心 / Initialize DX12 Core
         *
         * 按顺序创建设备、命令队列、交换链、描述符堆、命令分配器、
         * 命令列表、渲染目标视图、围栏、根签名、上传缓冲区和管线状态。
         *
         * Sequentially creates device, command queue, swap chain, descriptor heaps,
         * command allocators, command list, render target views, fence,
         * root signature, upload buffer, and pipeline state.
         *
         * @param hWnd 窗口句柄 / Window handle
         * @param width 窗口宽度 / Window width
         * @param height 窗口高度 / Window height
         */
        void Initialize(HWND hWnd, int width, int height);

        /**
         * @brief 关闭并释放所有资源 / Shutdown and release all resources
         *
         * 等待 GPU 完成所有操作后，按顺序释放所有 DX12 资源。
         * Waits for GPU to complete all operations, then releases all DX12 resources in order.
         */
        void Shutdown();

        /**
         * @brief 开始一帧渲染 / Begin a frame of rendering
         *
         * 重置命令分配器和命令列表，设置描述符堆、视口和裁剪矩形。
         * Resets command allocator and command list, sets descriptor heaps,
         * viewport, and scissor rectangle.
         */
        void BeginFrame();

        /**
         * @brief 结束一帧渲染 / End a frame of rendering
         *
         * 关闭命令列表，执行命令队列，呈现交换链并移动到下一帧。
         * Closes command list, executes command queue, presents swap chain,
         * and moves to the next frame.
         */
        void EndFrame();

        /**
         * @brief 等待 GPU 完成当前帧操作 / Wait for GPU to complete current frame operations
         *
         * 阻塞 CPU 直到 GPU 完成当前帧的所有命令执行。
         * Blocks the CPU until the GPU completes all command execution for the current frame.
         */
        void WaitForGPU();

        /**
         * @brief 移动到下一帧 / Move to the next frame
         *
         * 信号围栏，更新当前后台缓冲区索引，并确保下一帧资源可用。
         * Signals the fence, updates the current back buffer index,
         * and ensures the next frame resources are available.
         */
        void MoveToNextFrame();

        /**
         * @brief 调整窗口大小 / Resize the window
         *
         * 调整交换链缓冲区大小并重新创建渲染目标视图。
         * Resizes swap chain buffers and recreates render target views.
         *
         * @param newWidth 新宽度 / New width
         * @param newHeight 新高度 / New height
         */
        void Resize(int newWidth, int newHeight);

        /**
         * @brief 获取 D3D12 设备指针 / Get D3D12 device pointer
         * @return ID3D12Device 指针 / ID3D12Device pointer
         */
        ::ID3D12Device* GetDevice() const noexcept { return pDevice.Get(); }

        /**
         * @brief 获取命令队列指针 / Get command queue pointer
         * @return ID3D12CommandQueue 指针 / ID3D12CommandQueue pointer
         */
        ::ID3D12CommandQueue* GetCommandQueue() const noexcept { return pCommandQueue.Get(); }

        /**
         * @brief 获取图形命令列表指针 / Get graphics command list pointer
         * @return ID3D12GraphicsCommandList 指针 / ID3D12GraphicsCommandList pointer
         */
        ::ID3D12GraphicsCommandList* GetCommandList() const noexcept { return pCommandList.Get(); }

        /**
         * @brief 获取交换链指针 / Get swap chain pointer
         * @return IDXGISwapChain3 指针 / IDXGISwapChain3 pointer
         */
        ::IDXGISwapChain3* GetSwapChain() const noexcept { return pSwapChain.Get(); }

        /**
         * @brief 获取渲染目标视图描述符堆 / Get render target view descriptor heap
         * @return RTV 描述符堆指针 / RTV descriptor heap pointer
         */
        DX12DescriptorHeap* GetRTVHeap() const noexcept { return RTVHeap.get(); }

        /**
         * @brief 获取深度模板视图描述符堆 / Get depth stencil view descriptor heap
         * @return DSV 描述符堆指针 / DSV descriptor heap pointer
         */
        DX12DescriptorHeap* GetDSVHeap() const noexcept { return DSVHeap.get(); }

        /**
         * @brief 获取 CBV/SRV/UAV 描述符堆 / Get CBV/SRV/UAV descriptor heap
         * @return CBV/SRV/UAV 描述符堆指针 / CBV/SRV/UAV descriptor heap pointer
         */
        DX12DescriptorHeap* GetCBVSRVUAVHeap() const noexcept { return CBVSRVUAVHeap.get(); }

        /**
         * @brief 获取采样器描述符堆 / Get sampler descriptor heap
         * @return 采样器描述符堆指针 / Sampler descriptor heap pointer
         */
        DX12DescriptorHeap* GetSamplerHeap() const noexcept { return SamplerHeap.get(); }

        /**
         * @brief 获取围栏对象 / Get fence object
         * @return 围栏指针 / Fence pointer
         */
        DX12Fence* GetFence() const noexcept { return Fence.get(); }

        /**
         * @brief 获取根签名对象 / Get root signature object
         * @return 根签名指针 / Root signature pointer
         */
        DX12RootSignature* GetRootSignature() const noexcept { return RootSignature.get(); }

        /**
         * @brief 获取管线状态对象 / Get pipeline state object
         * @return 管线状态指针 / Pipeline state pointer
         */
        DX12PipelineState* GetPipelineState() const noexcept { return PipelineState.get(); }

        /**
         * @brief 获取线管线状态对象 / Get line pipeline state object
         * @return 线管线状态指针 / Line pipeline state pointer
         */
        DX12PipelineState* GetLinePipelineState() const noexcept { return LinePipelineState.get(); }

        /**
         * @brief 获取 Geometry Pass 管线状态对象 / Get Geometry Pass pipeline state object
         * @return Geometry Pass 管线状态指针 / Geometry Pass pipeline state pointer
         */
        DX12PipelineState* GetGeometryPipelineState() const noexcept { return GeometryPipelineState.get(); }

        /**
         * @brief 获取 Lighting Pass 管线状态对象 / Get Lighting Pass pipeline state object
         * @return Lighting Pass 管线状态指针 / Lighting Pass pipeline state pointer
         */
        DX12PipelineState* GetLightingPipelineState() const noexcept { return LightingPipelineState.get(); }

        /**
         * @brief 获取当前渲染目标资源 / Get current render target resource
         * @return 当前后台缓冲区的 ID3D12Resource 指针
         *         ID3D12Resource pointer of current back buffer
         */
        ::ID3D12Resource* GetCurrentRenderTarget() const noexcept;

        /**
         * @brief 获取当前渲染目标视图 CPU 句柄 / Get current render target view CPU handle
         * @return 当前 RTV 的 CPU 描述符句柄
         *         CPU descriptor handle of current RTV
         */
        ::D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const noexcept;

        /**
         * @brief 获取深度模板视图 CPU 句柄 / Get depth stencil view CPU handle
         * @return DSV 的 CPU 描述符句柄
         *         CPU descriptor handle of DSV
         */
        ::D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept;

        /**
         * @brief 获取当前后台缓冲区索引 / Get current back buffer index
         * @return 当前后台缓冲区索引值
         *         Current back buffer index value
         */
        UINT GetCurrentBackBufferIndex() const noexcept { return CurrentBackBufferIndex; }

        /**
         * @brief 获取帧缓冲区数量 / Get frame buffer count
         * @return 帧缓冲区数量
         *         Frame buffer count
         */
        UINT GetFrameCount() const noexcept { return FRAME_COUNT; }

        /**
         * @brief 获取渲染宽度 / Get render width
         * @return 渲染宽度（像素）
         *         Render width in pixels
         */
        int GetWidth() const noexcept { return Width; }

        /**
         * @brief 获取渲染高度 / Get render height
         * @return 渲染高度（像素）
         *         Render height in pixels
         */
        int GetHeight() const noexcept { return Height; }

        /**
         * @brief 创建提交资源（辅助函数）/ Create committed resource (helper function)
         *
         * 使用默认堆属性创建一个已提交的 D3D12 资源。
         * Creates a committed D3D12 resource with default heap properties.
         *
         * @param dimension 资源维度（纹理1D/2D/3D、缓冲区等）
         *                  Resource dimension (texture 1D/2D/3D, buffer, etc.)
         * @param width 资源宽度 / Resource width
         * @param height 资源高度 / Resource height
         * @param depthOrArraySize 深度或数组大小 / Depth or array size
         * @param mipLevels Mip 贴图层级数 / Number of mip levels
         * @param format 像素格式 / Pixel format
         * @param flags 资源标志 / Resource flags
         * @param initialState 初始资源状态 / Initial resource state
         * @param clearValue 清除值（可选，用于渲染目标）
         *                   Clear value (optional, for render targets)
         * @return 已创建的资源 ComPtr / Created resource ComPtr
         */
        Microsoft::WRL::ComPtr<::ID3D12Resource> CreateCommittedResource(
            ::D3D12_RESOURCE_DIMENSION dimension,
            UINT64 width,
            UINT height,
            UINT16 depthOrArraySize,
            UINT16 mipLevels,
            ::DXGI_FORMAT format,
            ::D3D12_RESOURCE_FLAGS flags,
            ::D3D12_RESOURCE_STATES initialState,
            const ::D3D12_CLEAR_VALUE* clearValue = nullptr
        );

        /**
         * @brief 获取上传缓冲区对象 / Get upload buffer object
         * @return 上传缓冲区指针 / Upload buffer pointer
         */
        DX12UploadBuffer* GetUploadBuffer() const noexcept { return UploadBuffer.get(); }

    private:
        /**
         * @brief 创建 D3D12 设备 / Create D3D12 device
         *
         * 首先尝试创建硬件设备，如果失败则回退到 WARP 软件设备。
         * First tries to create a hardware device, falls back to WARP software device if failed.
         */
        void CreateDevice();

        /**
         * @brief 创建命令队列 / Create command queue
         *
         * 创建一个直接类型的命令队列，用于提交渲染命令。
         * Creates a direct type command queue for submitting rendering commands.
         */
        void CreateCommandQueue();

        /**
         * @brief 创建交换链 / Create swap chain
         *
         * 创建 DXGI 交换链用于显示渲染结果。
         * Creates DXGI swap chain for displaying rendering results.
         *
         * @param hWnd 窗口句柄 / Window handle
         * @param width 宽度 / Width
         * @param height 高度 / Height
         */
        void CreateSwapChain(HWND hWnd, int width, int height);

        /**
         * @brief 创建描述符堆 / Create descriptor heaps
         *
         * 创建 RTV、DSV、CBV/SRV/UAV 和 Sampler 四种描述符堆，
         * 并初始化占位描述符。
         *
         * Creates four types of descriptor heaps: RTV, DSV, CBV/SRV/UAV, and Sampler,
         * and initializes placeholder descriptors.
         *
         * @param width 宽度（用于占位资源）
         *              Width (for placeholder resources)
         * @param height 高度（用于占位资源）
         *               Height (for placeholder resources)
         */
        void CreateDescriptorHeaps(int width, int height);

        /**
         * @brief 初始化占位纹理 / Initialize placeholder textures
         *
         * 将占位纹理数据从上传堆复制到默认堆，并转换资源状态。
         * Copies placeholder texture data from upload heap to default heap and transitions resource states.
         */
        void InitializePlaceholderTextures();

        /**
         * @brief 创建命令分配器 / Create command allocators
         *
         * 为每一帧创建一个命令分配器。
         * Creates one command allocator for each frame.
         */
        void CreateCommandAllocators();

        /**
         * @brief 创建命令列表 / Create command list
         *
         * 创建一个直接类型的图形命令列表。
         * Creates a direct type graphics command list.
         */
        void CreateCommandList();

        /**
         * @brief 创建渲染目标视图 / Create render target views
         *
         * 为交换链的每个缓冲区创建 RTV。
         * Creates RTVs for each buffer of the swap chain.
         *
         * @param width 宽度 / Width
         * @param height 高度 / Height
         */
        void CreateRenderTargetViews(int width, int height);

        /**
         * @brief 创建深度模板视图（已弃用）/ Create depth stencil view (deprecated)
         *
         * @note 深度模板现在由 DX12Renderer 通过 DepthStencilDX12 管理。
         *       Depth stencil is now managed by DX12Renderer via DepthStencilDX12.
         *
         * @param width 宽度 / Width
         * @param height 高度 / Height
         */
        void CreateDepthStencilView(int width, int height);

        /**
         * @brief 创建围栏对象 / Create fence object
         *
         * 创建用于 CPU-GPU 同步的围栏。
         * Creates a fence for CPU-GPU synchronization.
         */
        void CreateFence();

        /**
         * @brief 创建根签名 / Create root signature
         *
         * 创建默认的根签名，定义着色器如何访问资源。
         * Creates the default root signature that defines how shaders access resources.
         */
        void CreateRootSignature();

        /**
         * @brief 创建上传缓冲区 / Create upload buffer
         *
         * 创建用于将数据上传到 GPU 的上传缓冲区。
         * Creates an upload buffer for uploading data to the GPU.
         */
        void CreateUploadBuffer();

        /**
         * @brief 创建管线状态对象 / Create pipeline state object
         *
         * 编译着色器并创建默认的 PBR 管线状态。
         * Compiles shaders and creates the default PBR pipeline state.
         */
        void CreatePipelineState();

        /**
         * @brief 创建线管线状态对象 / Create line pipeline state object
         *
         * 创建用于线列表（LINELIST）拓扑的管线状态，
         * 用于锥体线框等线框绘制。
         *
         * Creates pipeline state for line list (LINELIST) topology,
         * used for wireframe drawing such as cone wireframes.
         */
        void CreateLinePipelineState();

        /**
         * @brief创建 Geometry Pass 管线状态对象 / Create Geometry Pass pipeline state object
         *
         * 编译 GBuffer 顶点/像素着色器，创建 MRT (4 render targets) 管线状态。
         * Compiles GBuffer vertex/pixel shaders, creates MRT (4 render targets) pipeline state.
         */
        void CreateGeometryPipelineState();

        /**
         * @brief 创建 Lighting Pass 管线状态对象 / Create Lighting Pass pipeline state object
         *
         * 编译 LightingPass 顶点/像素着色器，创建全屏三角形管线状态（无输入布局）。
         * Compiles LightingPass vertex/pixel shaders, creates full-screen triangle pipeline state (no input layout).
         */
        void CreateLightingPipelineState();

        /**
         * @brief 释放渲染目标视图 / Release render target views
         *
         * 释放所有渲染目标资源引用。
         * Releases all render target resource references.
         */
        void ReleaseRenderTargetViews();

        // Core objects / 核心对象
        Microsoft::WRL::ComPtr<::ID3D12Device> pDevice;                ///< D3D12 设备指针 / D3D12 device pointer
        Microsoft::WRL::ComPtr<::ID3D12CommandQueue> pCommandQueue;    ///< 命令队列指针 / Command queue pointer
        Microsoft::WRL::ComPtr<::IDXGISwapChain3> pSwapChain;          ///< 交换链指针 / Swap chain pointer
        
        // Command objects / 命令对象
        Microsoft::WRL::ComPtr<::ID3D12CommandAllocator> pCommandAllocators[FRAME_COUNT];  ///< 命令分配器数组（每帧一个）/ Command allocator array (one per frame)
        Microsoft::WRL::ComPtr<::ID3D12GraphicsCommandList> pCommandList;                  ///< 图形命令列表 / Graphics command list

        // Descriptor heaps / 描述符堆
        std::unique_ptr<DX12DescriptorHeap> RTVHeap;              ///< 渲染目标视图描述符堆 / Render Target View heap
        std::unique_ptr<DX12DescriptorHeap> DSVHeap;              ///< 深度模板视图描述符堆 / Depth Stencil View heap
        std::unique_ptr<DX12DescriptorHeap> CBVSRVUAVHeap;        ///< 常量缓冲/着色器资源/UAV描述符堆 / Constant Buffer + Shader Resource + UAV heap
        std::unique_ptr<DX12DescriptorHeap> SamplerHeap;          ///< 采样器描述符堆 / Sampler heap

        // Render targets / 渲染目标
        Microsoft::WRL::ComPtr<::ID3D12Resource> pRenderTargets[FRAME_COUNT];  ///< 渲染目标资源数组 / Render target resource array

        // Synchronization / 同步
        std::unique_ptr<DX12Fence> Fence;              ///< GPU 同步围栏 / GPU synchronization fence
        UINT64 FrameFenceValues[FRAME_COUNT];          ///< 每帧的围栏值 / Fence values per frame
        UINT CurrentBackBufferIndex;                   ///< 当前后台缓冲区索引 / Current back buffer index

        // Root signature / 根签名
        std::unique_ptr<DX12RootSignature> RootSignature;  ///< 根签名对象 / Root signature object

        // Pipeline state / 管线状态
        std::unique_ptr<DX12PipelineState> PipelineState;      ///< 管线状态对象（三角形拓扑）/ Pipeline state object (triangle topology)
        std::unique_ptr<DX12PipelineState> LinePipelineState;  ///< 线管线状态对象（线拓扑）/ Line pipeline state object (line topology)
        std::unique_ptr<DX12PipelineState> GeometryPipelineState;  ///< Geometry Pass 管线状态（延迟渲染）/ Geometry Pass pipeline state (deferred)
        std::unique_ptr<DX12PipelineState> LightingPipelineState;  ///< Lighting Pass 管线状态（延迟渲染）/ Lighting Pass pipeline state (deferred)

        // Upload buffer for resource uploads / 资源上传缓冲区
        std::unique_ptr<DX12UploadBuffer> UploadBuffer;    ///< 上传缓冲区对象 / Upload buffer object

        // Dimensions / 尺寸
        int Width;    ///< 渲染宽度（像素）/ Render width in pixels
        int Height;   ///< 渲染高度（像素）/ Render height in pixels

        bool bInitialized = false;    ///< 是否已初始化标志 / Initialization flag

        // Debug layer / 调试层
        Microsoft::WRL::ComPtr<::ID3D12Debug> pDebugController;  ///< D3D12 调试控制器 / D3D12 debug controller
        bool EnableDebugLayer;                                    ///< 是否启用调试层 / Whether to enable debug layer

        // Placeholder textures for unused texture slots / 未使用纹理槽的占位纹理
        std::vector<Microsoft::WRL::ComPtr<::ID3D12Resource>> placeholderTextures;         ///< 占位纹理资源 / Placeholder texture resources
        std::vector<Microsoft::WRL::ComPtr<::ID3D12Resource>> placeholderUploadResources;  ///< 占位纹理上传资源 / Placeholder texture upload resources
    };
}

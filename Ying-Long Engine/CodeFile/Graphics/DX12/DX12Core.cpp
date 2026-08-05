/**
 * @file DX12Core.cpp
 * @brief DX12 核心模块实现文件 / DX12 Core Module Implementation
 *
 * 本文件实现了 DX12Core 类的所有方法，包括设备创建、
 * 交换链管理、描述符堆管理以及帧渲染生命周期管理。
 *
 * This file implements all methods of the DX12Core class, including
 * device creation, swap chain management, descriptor heap management,
 * and frame rendering lifecycle management.
 */

#include "DX12Core.h"
#include "DX12ShaderCompiler.h"
#include "DX12Primitives.h"
#include "../../Debug/DX12Log.h"
#include <dxgidebug.h>
#include <stdexcept>
#include <string>
#include <iostream>

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 初始化成员变量，设置默认值。
     * Initializes member variables with default values.
     */
    DX12Core::DX12Core()
        : CurrentBackBufferIndex(0)
        , Width(800)
        , Height(600)
        , EnableDebugLayer(true)
    {
        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            FrameFenceValues[i] = 0;
        }
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 调用 Shutdown() 确保所有资源被正确释放。
     * Calls Shutdown() to ensure all resources are properly released.
     */
    DX12Core::~DX12Core()
    {
        Shutdown();
    }

    /**
     * @brief 初始化 DX12 核心 / Initialize DX12 Core
     *
     * 按顺序执行12个初始化步骤，创建所有必要的 DX12 对象。
     * 如果任何步骤失败，抛出异常并终止初始化。
     *
     * Sequentially executes 12 initialization steps to create all necessary DX12 objects.
     * If any step fails, throws an exception and terminates initialization.
     *
     * @param hWnd 窗口句柄 / Window handle
     * @param width 窗口宽度 / Window width
     * @param height 窗口高度 / Window height
     */
    void DX12Core::Initialize(HWND hWnd, int width, int height)
    {
        // 保存窗口尺寸
        // Store window dimensions
        Width = width;
        Height = height;

        DX12Log("[DX12Core] === Starting DX12 Initialization ===\n");

        // 在调试构建中启用调试层
        // Enable debug layer in debug builds
#if defined(_DEBUG)
        if (EnableDebugLayer)
        {
            DX12Log("[DX12Core] Step 0: Enabling debug layer...\n");
            // 获取 D3D12 调试接口并启用调试层
            // Get D3D12 debug interface and enable debug layer
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
            {
                pDebugController->EnableDebugLayer();
                DX12LogSuccess("[DX12Core] Debug layer enabled successfully\n");
            }
            else
            {
                DX12LogWarning("[DX12Core] Failed to enable debug layer\n");
            }
        }
#endif

        // 步骤1：创建设备
        // Step 1: Create Device
        DX12Log("[DX12Core] Step 1: Creating device...\n");
        try
        {
            CreateDevice();
            DX12LogSuccess("[DX12Core] Device created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 1 (CreateDevice): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateDevice: " + std::string(e.what()));
        }

        // 步骤2：创建命令队列
        // Step 2: Create Command Queue
        DX12Log("[DX12Core] Step 2: Creating command queue...\n");
        try
        {
            CreateCommandQueue();
            DX12LogSuccess("[DX12Core] Command queue created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 2 (CreateCommandQueue): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateCommandQueue: " + std::string(e.what()));
        }

        // 步骤3：创建交换链
        // Step 3: Create Swap Chain
        DX12Log("[DX12Core] Step 3: Creating swap chain...\n");
        try
        {
            CreateSwapChain(hWnd, width, height);
            DX12LogSuccess("[DX12Core] Swap chain created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 3 (CreateSwapChain): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateSwapChain: " + std::string(e.what()));
        }

        // 步骤4：创建描述符堆
        // Step 4: Create Descriptor Heaps
        DX12Log("[DX12Core] Step 4: Creating descriptor heaps...\n");
        try
        {
            CreateDescriptorHeaps(width, height);
            DX12LogSuccess("[DX12Core] Descriptor heaps created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 4 (CreateDescriptorHeaps): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateDescriptorHeaps: " + std::string(e.what()));
        }

        // 步骤5：创建命令分配器
        // Step 5: Create Command Allocators
        DX12Log("[DX12Core] Step 5: Creating command allocators...\n");
        try
        {
            CreateCommandAllocators();
            DX12LogSuccess("[DX12Core] Command allocators created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 5 (CreateCommandAllocators): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateCommandAllocators: " + std::string(e.what()));
        }

        // 步骤6：创建命令列表
        // Step 6: Create Command List
        DX12Log("[DX12Core] Step 6: Creating command list...\n");
        try
        {
            CreateCommandList();
            DX12LogSuccess("[DX12Core] Command list created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 6 (CreateCommandList): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateCommandList: " + std::string(e.what()));
        }

        // 步骤6.5：初始化占位纹理
        // Step 6.5: Initialize placeholder textures
        DX12Log("[DX12Core] Step 6.5: Initializing placeholder textures...\n");
        try
        {
            InitializePlaceholderTextures();
            DX12LogSuccess("[DX12Core] Placeholder textures initialized successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 6.5 (InitializePlaceholderTextures): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at InitializePlaceholderTextures: " + std::string(e.what()));
        }

        // 步骤7：创建渲染目标视图
        // Step 7: Create Render Target Views
        DX12Log("[DX12Core] Step 7: Creating render target views...\n");
        try
        {
            CreateRenderTargetViews(width, height);
            DX12LogSuccess("[DX12Core] Render target views created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 7 (CreateRenderTargetViews): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateRenderTargetViews: " + std::string(e.what()));
        }

        // 步骤8：深度模板由 DX12Renderer 通过 DepthStencilDX12 管理
        // Step 8: Depth stencil is managed by DX12Renderer via DepthStencilDX12
        DX12Log("[DX12Core] Step 8: Depth stencil managed by DX12Renderer, skipping internal creation\n");

        // 步骤9：创建围栏
        // Step 9: Create Fence
        DX12Log("[DX12Core] Step 9: Creating fence...\n");
        try
        {
            CreateFence();
            DX12LogSuccess("[DX12Core] Fence created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 9 (CreateFence): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateFence: " + std::string(e.what()));
        }

        // 步骤10：创建根签名
        // Step 10: Create Root Signature
        DX12Log("[DX12Core] Step 10: Creating root signature...\n");
        try
        {
            CreateRootSignature();
            DX12LogSuccess("[DX12Core] Root signature created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 10 (CreateRootSignature): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateRootSignature: " + std::string(e.what()));
        }

        // 步骤11：创建上传缓冲区
        // Step 11: Create Upload Buffer
        DX12Log("[DX12Core] Step 11: Creating upload buffer...\n");
        try
        {
            CreateUploadBuffer();
            DX12LogSuccess("[DX12Core] Upload buffer created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 11 (CreateUploadBuffer): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreateUploadBuffer: " + std::string(e.what()));
        }

        // 步骤12：创建管线状态
        // Step 12: Create Pipeline State
        DX12Log("[DX12Core] Step 12: Creating pipeline state...\n");
        try
        {
            CreatePipelineState();
            DX12LogSuccess("[DX12Core] Pipeline state created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] FAILED Step 12 (CreatePipelineState): " + std::string(e.what()) + "\n").c_str());
            throw std::runtime_error("DX12Core::Initialize failed at CreatePipelineState: " + std::string(e.what()));
        }

        DX12Log("[DX12Core] === DX12 Initialization Complete ===\n");

        // 标记初始化完成
        // Mark initialization as complete
        bInitialized = true;
    }

    /**
     * @brief 关闭并释放所有资源 / Shutdown and release all resources
     *
     * 按逆序释放所有 DX12 资源，确保正确的资源释放顺序。
     * Releases all DX12 resources in reverse order to ensure proper resource release order.
     */
    void DX12Core::Shutdown()
    {
        // 如果未初始化则直接返回
        // Return directly if not initialized
        if (!bInitialized)
            return;

        DX12Log("[DX12Core] Shutting down...\n");
        bInitialized = false;

        // 先等待 GPU 完成所有待处理的命令
        // First wait for GPU to complete all pending commands
        if (Fence && pCommandQueue)
        {
            try { WaitForGPU(); } catch (...) {}
        }

        // 释放命令相关资源
        // Release command-related resources
        pCommandList.Reset();
        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            pCommandAllocators[i].Reset();
            pRenderTargets[i].Reset();
        }

        // 释放交换链和命令队列
        // Release swap chain and command queue
        pSwapChain.Reset();
        pCommandQueue.Reset();

        // 释放描述符堆和其他对象（必须在设备之前释放，
        // 否则 D3D12 调试层会报告这些对象为泄漏）
        // Release descriptor heaps and other objects BEFORE the device,
        // otherwise the D3D12 debug layer reports them as leaked.
        RTVHeap.reset();
        DSVHeap.reset();
        CBVSRVUAVHeap.reset();
        SamplerHeap.reset();
        Fence.reset();
        RootSignature.reset();
        UploadBuffer.reset();
        PipelineState.reset();
        LinePipelineState.reset();
        GeometryPipelineState.reset();
        LightingPipelineState.reset();

        // 释放占位纹理资源
        // Release placeholder texture resources
        placeholderTextures.clear();
        placeholderUploadResources.clear();

        // 释放光源剔除资源
        // Release light culling resources
        if (pLightCulling)
        {
            pLightCulling->Shutdown(*this);
            pLightCulling.reset();
        }

        // 清理静态图元的光源缓冲区资源
        // Clean up static primitive light buffer resources
        DX12Primitive::CleanupLightBuffers();

        // 设备必须在最后释放（所有 D3D12 对象都依赖设备）
        // Device must be released last (all D3D12 objects depend on it)
        pDevice.Reset();

        DX12Log("[DX12Core] Shutdown complete\n");
    }

    /**
     * @brief 创建 D3D12 设备 / Create D3D12 device
     *
     * 首先尝试使用默认硬件适配器创建设备。
     * 如果失败，则回退到 WARP（Windows Advanced Rasterization Platform）软件设备。
     *
     * First tries to create a device using the default hardware adapter.
     * If that fails, falls back to WARP (Windows Advanced Rasterization Platform) software device.
     *
     * @throws std::runtime_error 如果硬件和 WARP 设备都创建失败
     *                             If both hardware and WARP device creation fail
     */
    void DX12Core::CreateDevice()
    {
        DX12Log("[DX12Core] Creating device...\n");

        // 尝试创建硬件设备（nullptr 表示使用默认适配器）
        // Try to create hardware device (nullptr means use default adapter)
        HRESULT hr = D3D12CreateDevice(
            nullptr,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&pDevice)
        );

        if (FAILED(hr))
        {
            DX12LogWarning("[DX12Core] Hardware device failed, trying WARP...\n");
            // 回退到 WARP 适配器（软件光栅化器）
            // Fallback to WARP adapter (software rasterizer)
            Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
            Microsoft::WRL::ComPtr<IDXGIFactory4> factory;

            // 创建 DXGI 工厂
            // Create DXGI factory
            HRESULT hrFactory = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
            if (FAILED(hrFactory))
            {
                throw std::runtime_error("Failed to create DXGI factory");
            }
            
            // 枚举 WARP 适配器
            // Enumerate WARP adapter
            HRESULT hrWarp = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
            if (FAILED(hrWarp) || !warpAdapter)
            {
                throw std::runtime_error("Failed to get WARP adapter");
            }
            
            // 使用 WARP 适配器创建设备
            // Create device using WARP adapter
            hr = D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&pDevice)
            );
        }

        // 检查设备是否创建成功
        // Check if device creation succeeded
        if (FAILED(hr) || !pDevice)
        {
            throw std::runtime_error("Failed to create D3D12 device");
        }

        DX12LogSuccess("[DX12Core] Device created successfully\n");
    }

    /**
     * @brief 创建命令队列 / Create command queue
     *
     * 创建一个直接类型的命令队列，优先级为正常。
     * 直接类型的命令队列可以执行所有类型的 GPU 命令。
     *
     * Creates a direct type command queue with normal priority.
     * Direct type command queues can execute all types of GPU commands.
     *
     * @throws std::runtime_error 如果命令队列创建失败
     *                             If command queue creation fails
     */
    void DX12Core::CreateCommandQueue()
    {
        DX12Log("[DX12Core] Creating command queue...\n");

        // 配置命令队列描述
        // Configure command queue description
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;      ///< 直接命令列表类型 / Direct command list type
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;  ///< 正常优先级 / Normal priority
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;      ///< 无特殊标志 / No special flags
        queueDesc.NodeMask = 0;                               ///< 单 GPU / Single GPU

        // 创建命令队列
        // Create command queue
        HRESULT hr = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue));
        if (FAILED(hr) || !pCommandQueue)
        {
            throw std::runtime_error("Failed to create command queue");
        }

        DX12LogSuccess("[DX12Core] Command queue created successfully\n");
    }

    /**
     * @brief 创建交换链 / Create swap chain
     *
     * 创建 DXGI 交换链，使用翻转丢弃（Flip Discard）模式。
     * 翻转模式是现代 Windows 应用推荐的交换效果。
     *
     * Creates a DXGI swap chain using Flip Discard mode.
     * Flip mode is the recommended swap effect for modern Windows applications.
     *
     * @param hWnd 窗口句柄 / Window handle
     * @param width 宽度 / Width
     * @param height 高度 / Height
     * @throws std::runtime_error 如果交换链创建失败
     *                             If swap chain creation fails
     */
    void DX12Core::CreateSwapChain(HWND hWnd, int width, int height)
    {
        DX12Log("[DX12Core] Creating swap chain...\n");

        // 释放已有的交换链
        // Release existing swap chain if any
        pSwapChain.Reset();

        // 创建 DXGI 工厂（根据调试层设置决定是否启用调试）
        // Create DXGI factory (enable debug based on debug layer setting)
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory2(EnableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&factory));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create DXGI factory");
        }

        // 配置交换链描述
        // Configure swap chain description
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;                              ///< 缓冲区宽度 / Buffer width
        swapChainDesc.Height = height;                            ///< 缓冲区高度 / Buffer height
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;        ///< 像素格式（RGBA 8位无归一化）/ Pixel format (RGBA 8-bit unnormalized)
        swapChainDesc.Stereo = FALSE;                             ///< 非立体渲染 / Not stereo rendering
        swapChainDesc.SampleDesc.Count = 1;                       ///< 多重采样计数（1表示无MSAA）/ Multisample count (1 means no MSAA)
        swapChainDesc.SampleDesc.Quality = 0;                     ///< 多重采样质量 / Multisample quality
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  ///< 渲染目标输出用途 / Render target output usage
        swapChainDesc.BufferCount = FRAME_COUNT;                  ///< 缓冲区数量 / Buffer count
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;             ///< 拉伸缩放模式 / Stretch scaling mode
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; ///< 翻转丢弃交换效果 / Flip discard swap effect
        // 强制使用 IGNORE 模式，避免操作系统将后台缓冲区与桌面 Alpha 合成。
        // 在某些配置下，UNSPECIFIED 可能默认为 PREMULTIPLIED，
        // 这会将任何 Alpha 通道错误暴露为 ImGui 区域中的透明/重影效果。
        // Force IGNORE so the OS does not composite the backbuffer with desktop alpha.
        // UNSPECIFIED may default to PREMULTIPLIED on some configs, which exposes any alpha-channel
        // bugs as transparency/ghosting in ImGui regions.
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;         ///< 忽略 Alpha 模式 / Ignore alpha mode
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;  ///< 允许模式切换 / Allow mode switch

        // 创建交换链
        // Create swap chain
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        hr = factory->CreateSwapChainForHwnd(
            pCommandQueue.Get(),   ///< 关联的命令队列 / Associated command queue
            hWnd,                  ///< 窗口句柄 / Window handle
            &swapChainDesc,        ///< 交换链描述 / Swap chain description
            nullptr,               ///< 无全屏输出 / No fullscreen output
            nullptr,               ///< 无限制 / No restriction
            &swapChain             ///< 输出的交换链 / Output swap chain
        );
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create swap chain");
        }

        // 转换为 IDXGISwapChain3 接口
        // Convert to IDXGISwapChain3 interface
        hr = swapChain.As(&pSwapChain);
        if (FAILED(hr) || !pSwapChain)
        {
            throw std::runtime_error("Failed to cast swap chain");
        }

        // 获取当前后台缓冲区索引
        // Get current back buffer index
        CurrentBackBufferIndex = pSwapChain->GetCurrentBackBufferIndex();
        DX12LogSuccess("[DX12Core] Swap chain created successfully\n");
    }

    /**
     * @brief 创建描述符堆 / Create descriptor heaps
     *
     * 创建四种类型的描述符堆：
     * - RTV 堆：渲染目标视图
     * - DSV 堆：深度模板视图
     * - CBV/SRV/UAV 堆：着色器可见的资源描述符
     * - Sampler 堆：着色器可见的采样器描述符
     *
     * 同时创建占位描述符以满足 D3D12 的静态描述符要求。
     *
     * Creates four types of descriptor heaps:
     * - RTV heap: Render Target Views
     * - DSV heap: Depth Stencil Views
     * - CBV/SRV/UAV heap: Shader-visible resource descriptors
     * - Sampler heap: Shader-visible sampler descriptors
     *
     * Also creates placeholder descriptors to satisfy D3D12 static descriptor requirements.
     *
     * @param width 宽度（用于占位资源）
     *              Width (for placeholder resources)
     * @param height 高度（用于占位资源）
     *               Height (for placeholder resources)
     */
    void DX12Core::CreateDescriptorHeaps(int width, int height)
    {
        // RTV 堆 - 容量 = FRAME_COUNT(2) + G-Buffer(4) + 场景渲染目标(1) + 调整大小余量(1) = 8
        // 预分配 FRAME_COUNT 个索引供后台缓冲区使用（索引 0, 1），
        // 这样 CreateRenderTargetViews 可直接使用 GetCPUHandle(i)，
        // 而 SceneRenderTarget 和 G-Buffer 的 CreateRTV 调用 Allocate() 不会覆盖后台缓冲区。
        // RTV Heap - Capacity = FRAME_COUNT(2) + GBuffer(4) + scene RT(1) + resize spare(1) = 8.
        // Pre-allocate FRAME_COUNT indices for back buffers (indices 0, 1),
        // so CreateRenderTargetViews can directly use GetCPUHandle(i), and
        // SceneRenderTarget and G-Buffer's CreateRTV via Allocate() won't overwrite back buffer RTVs.
        RTVHeap = std::make_unique<DX12DescriptorHeap>(
            pDevice.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            FRAME_COUNT + GBUFFER_RT_COUNT + 2,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        );
        // 预分配 FRAME_COUNT 个索引供后台缓冲区 RTV 使用
        // Pre-allocate FRAME_COUNT indices for back buffer RTVs
        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            RTVHeap->Allocate();
        }

        // DSV 堆 - 容量 = 主深度模板(1) + 场景深度模板(1) + G-Buffer深度(1) + 调整大小余量(3) = 6
        // G-Buffer 使用独立的 D32_FLOAT 深度模板，需要额外的 DSV 描述符。
        // DSV Heap - Capacity = main DS(1) + scene DS(1) + GBuffer DS(1) + resize spare(3) = 6.
        // G-Buffer uses its own D32_FLOAT depth stencil, requiring an extra DSV descriptor.
        DSVHeap = std::make_unique<DX12DescriptorHeap>(
            pDevice.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            6,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        );

        // CBV/SRV/UAV 堆 - 分配100个描述符用于资源（着色器可见）
        // CBV/SRV/UAV Heap - Allocate 100 descriptors for resources (shader visible)
        CBVSRVUAVHeap = std::make_unique<DX12DescriptorHeap>(
            pDevice.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            100,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        );

        // Sampler 堆 - 分配10个描述符用于采样器（着色器可见）
        // Sampler Heap - Allocate 10 descriptors for samplers (shader visible)
        SamplerHeap = std::make_unique<DX12DescriptorHeap>(
            pDevice.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            10,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        );

        // 创建占位纹理资源（1x1像素，用于未使用纹理时的默认值）
        // Create placeholder texture resources (1x1 pixel, for default values when textures are not used)
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = 1;
        texDesc.Height = 1;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // 创建上传堆用于初始化纹理数据
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;

        // 创建4个占位纹理和对应的上传资源
        for (UINT i = 0; i < 4; ++i)
        {
            UINT nullSRVIndex = CBVSRVUAVHeap->Allocate();
            
            Microsoft::WRL::ComPtr<ID3D12Resource> placeholderTexture;
            Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
            
            HRESULT hr = pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&placeholderTexture)
            );
            if (FAILED(hr)) continue;

            D3D12_RESOURCE_DESC uploadDesc = {};
            uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadDesc.Width = 4; // 1x1 RGBA = 4 bytes
            uploadDesc.Height = 1;
            uploadDesc.DepthOrArraySize = 1;
            uploadDesc.MipLevels = 1;
            uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
            uploadDesc.SampleDesc.Count = 1;
            uploadDesc.SampleDesc.Quality = 0;
            uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            hr = pDevice->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &uploadDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&uploadResource)
            );
            if (FAILED(hr)) continue;

            // 初始化纹理数据（灰色像素）
            uint32_t pixel = 0xFF808080; // RGBA: 128,128,128,255 (gray)
            void* pData;
            D3D12_RANGE readRange = {};
            hr = uploadResource->Map(0, &readRange, &pData);
            if (SUCCEEDED(hr) && pData)
            {
                memcpy(pData, &pixel, sizeof(pixel));
                uploadResource->Unmap(0, nullptr);
            }

            // 创建 SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            pDevice->CreateShaderResourceView(placeholderTexture.Get(), &srvDesc, CBVSRVUAVHeap->GetCPUHandle(nullSRVIndex));

            // 将纹理从上传堆复制到默认堆（在命令列表执行时完成）
            placeholderTextures.push_back(placeholderTexture);
            placeholderUploadResources.push_back(uploadResource);
        }

        // 在 Sampler 堆的索引0处创建占位默认采样器
        // 这确保采样器的描述符表（根参数5）始终有一个有效的描述符
        // Create placeholder default sampler at index 0 of Sampler heap
        // This ensures the descriptor table for samplers (root param 5) always has a valid descriptor
        UINT defaultSamplerIndex = SamplerHeap->Allocate();
        D3D12_SAMPLER_DESC defaultSamplerDesc = {};
        defaultSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;           ///< 三线性过滤 / Trilinear filtering
        defaultSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;         ///< U 方向环绕寻址 / U direction wrap addressing
        defaultSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;         ///< V 方向环绕寻址 / V direction wrap addressing
        defaultSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;         ///< W 方向环绕寻址 / W direction wrap addressing
        defaultSamplerDesc.MipLODBias = 0.0f;                                   ///< Mip LOD 偏移 / Mip LOD bias
        defaultSamplerDesc.MaxAnisotropy = 1;                                   ///< 最大各向异性 / Maximum anisotropy
        defaultSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;        ///< 比较函数（从不）/ Comparison function (never)
        defaultSamplerDesc.BorderColor[0] = 0.0f;                               ///< 边框颜色R / Border color R
        defaultSamplerDesc.BorderColor[1] = 0.0f;                               ///< 边框颜色G / Border color G
        defaultSamplerDesc.BorderColor[2] = 0.0f;                               ///< 边框颜色B / Border color B
        defaultSamplerDesc.BorderColor[3] = 0.0f;                               ///< 边框颜色A / Border color A
        defaultSamplerDesc.MinLOD = 0.0f;                                       ///< 最小 LOD / Minimum LOD
        defaultSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                          ///< 最大 LOD / Maximum LOD
        pDevice->CreateSampler(&defaultSamplerDesc, SamplerHeap->GetCPUHandle(defaultSamplerIndex));
    }

    void DX12Core::InitializePlaceholderTextures()
    {
        if (placeholderTextures.empty() || !Fence || !pCommandQueue)
            return;

        // 重置命令分配器并关闭命令列表
        HRESULT hr = pCommandAllocators[0]->Reset();
        if (FAILED(hr))
            return;

        hr = pCommandList->Reset(pCommandAllocators[0].Get(), nullptr);
        if (FAILED(hr))
            return;

        // 记录资源状态转换：从 COPY_DEST 到 COPY_SOURCE（上传堆）
        for (size_t i = 0; i < placeholderUploadResources.size(); ++i)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = placeholderUploadResources[i].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            pCommandList->ResourceBarrier(1, &barrier);
        }

        // 记录复制命令：从上传堆复制到默认堆
        for (size_t i = 0; i < placeholderTextures.size() && i < placeholderUploadResources.size(); ++i)
        {
            pCommandList->CopyResource(placeholderTextures[i].Get(), placeholderUploadResources[i].Get());
        }

        // 记录资源状态转换：从 COPY_DEST 到 PIXEL_SHADER_RESOURCE
        for (size_t i = 0; i < placeholderTextures.size(); ++i)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = placeholderTextures[i].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            pCommandList->ResourceBarrier(1, &barrier);
        }

        // 关闭命令列表
        pCommandList->Close();

        // 提交命令列表到命令队列
        ID3D12CommandList* ppCommandLists[] = { pCommandList.Get() };
        pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // 等待命令执行完成
        UINT64 fenceValue = Fence->Increment();
        Fence->Signal(fenceValue);
        Fence->Wait(fenceValue);
    }

    /**
     * @brief 创建命令分配器 / Create command allocators
     *
     * 为每一帧创建一个命令分配器。命令分配器用于
     * 分配存储命令列表命令的内存。
     *
     * Creates one command allocator for each frame. Command allocators are used
     * to allocate memory for storing command list commands.
     */
    void DX12Core::CreateCommandAllocators()
    {
        // 为每一帧创建命令分配器
        // Create command allocators for each frame
        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            pDevice->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&pCommandAllocators[i])
            );
        }
    }

    /**
     * @brief 创建命令列表 / Create command list
     *
     * 创建一个直接类型的图形命令列表，初始时处于关闭状态。
     * Creates a direct type graphics command list, initially in closed state.
     *
     * @throws std::runtime_error 如果命令列表创建或关闭失败
     *                             If command list creation or closing fails
     */
    void DX12Core::CreateCommandList()
    {
        DX12Log("[DX12Core] Creating command list...\n");

        // 创建命令列表
        // Create command list
        HRESULT hr = pDevice->CreateCommandList(
            0,                                              ///< 节点掩码（单GPU）/ Node mask (single GPU)
            D3D12_COMMAND_LIST_TYPE_DIRECT,                 ///< 直接命令列表类型 / Direct command list type
            pCommandAllocators[CurrentBackBufferIndex].Get(),  ///< 关联的命令分配器 / Associated command allocator
            nullptr,                                        ///< 初始管线状态（无）/ Initial pipeline state (none)
            IID_PPV_ARGS(&pCommandList)                     ///< 输出命令列表 / Output command list
        );
        if (FAILED(hr) || !pCommandList)
        {
            throw std::runtime_error("Failed to create command list");
        }

        // 初始关闭命令列表（BeginFrame 时会重置）
        // Close command list initially (will be reset in BeginFrame)
        hr = pCommandList->Close();
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to close initial command list");
        }

        DX12LogSuccess("[DX12Core] Command list created successfully\n");
    }

    /**
     * @brief 创建渲染目标视图 / Create render target views
     *
     * 从交换链缓冲区获取每帧的渲染目标资源，
     * 并为每个缓冲区创建 RTV 描述符。
     *
     * Gets per-frame render target resources from swap chain buffers,
     * and creates RTV descriptors for each buffer.
     *
     * @param width 宽度 / Width
     * @param height 高度 / Height
     * @throws std::runtime_error 如果获取交换链缓冲区失败
     *                             If getting swap chain buffer fails
     */
    void DX12Core::CreateRenderTargetViews(int width, int height)
    {
        DX12Log("[DX12Core] Creating render target views...\n");

        // 先释放旧的渲染目标视图
        // First release old render target views
        ReleaseRenderTargetViews();

        // 配置 RTV 描述
        // Configure RTV description
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;          ///< 像素格式 / Pixel format
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  ///< 2D 纹理视图维度 / 2D texture view dimension
        rtvDesc.Texture2D.MipSlice = 0;                        ///< Mip 切片 / Mip slice
        rtvDesc.Texture2D.PlaneSlice = 0;                      ///< 平面切片 / Plane slice

        // 为每个帧缓冲区创建 RTV
        // Create RTV for each frame buffer
        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            // 从交换链获取缓冲区资源
            // Get buffer resource from swap chain
            HRESULT hr = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderTargets[i]));
            if (FAILED(hr) || !pRenderTargets[i])
            {
                throw std::runtime_error("Failed to get swap chain buffer");
            }
            
            // 创建渲染目标视图
            // Create render target view
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = RTVHeap->GetCPUHandle(i);
            pDevice->CreateRenderTargetView(pRenderTargets[i].Get(), &rtvDesc, rtvHandle);
        }

        DX12LogSuccess("[DX12Core] Render target views created successfully\n");
    }

    /**
     * @brief 创建深度模板视图（已弃用）/ Create depth stencil view (deprecated)
     *
     * @note 深度模板现在由 DX12Renderer 通过 DepthStencilDX12 管理。
     *       此方法保留用于兼容性但不执行任何操作。
     *       Depth stencil is now managed by DX12Renderer via DepthStencilDX12.
     *       This method is kept for compatibility but does nothing.
     *
     * @param width 宽度 / Width
     * @param height 高度 / Height
     */
    void DX12Core::CreateDepthStencilView(int width, int height)
    {
        // Depth stencil is now managed by DX12Renderer via DepthStencilDX12
        // This method is kept for compatibility but does nothing
    }

    /**
     * @brief 创建围栏对象 / Create fence object
     *
     * 创建用于 CPU-GPU 同步的围栏对象。
     * Creates a fence object for CPU-GPU synchronization.
     */
    void DX12Core::CreateFence()
    {
        DX12Log("[DX12Core] Creating fence...\n");
        Fence = std::make_unique<DX12Fence>(pDevice.Get(), pCommandQueue.Get());
        DX12LogSuccess("[DX12Core] Fence created successfully\n");
    }

    /**
     * @brief 创建根签名 / Create root signature
     *
     * 创建默认的根签名，定义着色器如何访问常量缓冲区、
     * 纹理、采样器等资源。
     *
     * Creates the default root signature that defines how shaders access
     * resources such as constant buffers, textures, and samplers.
     */
    void DX12Core::CreateRootSignature()
    {
        DX12Log("[DX12Core] Creating root signature...\n");
        RootSignature = std::make_unique<DX12RootSignature>(pDevice.Get());
        DX12LogSuccess("[DX12Core] Root signature created successfully\n");
    }

    /**
     * @brief 创建上传缓冲区 / Create upload buffer
     *
     * 创建一个大小为10MB的上传缓冲区，用于将数据从 CPU 上传到 GPU。
     * Creates a 10MB upload buffer for uploading data from CPU to GPU.
     */
    void DX12Core::CreateUploadBuffer()
    {
        DX12Log("[DX12Core] Creating upload buffer...\n");
        // 创建10MB大小的上传缓冲区
        // Create upload buffer with 10MB size
        UploadBuffer = std::make_unique<DX12UploadBuffer>(pDevice.Get(), 1024 * 1024 * 10); // 10MB
        DX12LogSuccess("[DX12Core] Upload buffer created successfully\n");
    }

    /**
     * @brief 创建管线状态对象 / Create pipeline state object
     *
     * 编译 PBR 顶点着色器和像素着色器，创建输入布局，
     * 并初始化管线状态对象。如果着色器编译失败，使用默认管线状态。
     *
     * Compiles PBR vertex shader and pixel shader, creates input layout,
     * and initializes the pipeline state object. If shader compilation fails,
     * uses the default pipeline state.
     */
    void DX12Core::CreatePipelineState()
    {
        PipelineState = std::make_unique<DX12PipelineState>();

        // 获取当前目录并构建着色器路径
        // Get current directory and build shader paths
        wchar_t basePath[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, basePath);
        std::wstring vsPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\PBRVertexShader.hlsl";
        std::wstring psPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\PBRPixelShader.hlsl";

        // 将宽路径转换为窄字符串用于日志记录
        // Convert wide paths to narrow for logging
        std::string vsPathNarrow(vsPath.begin(), vsPath.end());
        std::string psPathNarrow(psPath.begin(), psPath.end());
        DX12Log(("[DX12Core] Creating pipeline state (VS=" + vsPathNarrow + ")\n").c_str());

        try
        {
            // 编译顶点着色器和像素着色器
            // Compile vertex shader and pixel shader
            std::vector<uint8_t> vsBytecode = DX12ShaderCompiler::CompileVertexShader(vsPath);
            std::vector<uint8_t> psBytecode = DX12ShaderCompiler::CompilePixelShader(psPath);

            // 创建 PBR 输入布局
            // Create PBR input layout
            UINT elementCount = 0;
            D3D12_INPUT_ELEMENT_DESC* elements = PipelineState->CreatePBRInputLayout(elementCount);

            DX12Log("[DX12Core] Initializing PSO from shaders\n");

            // 从着色器初始化管线状态
            // Initialize pipeline state from shaders
            PipelineState->InitializeFromShaders(
                *this,
                RootSignature.get(),
                vsBytecode,
                psBytecode,
                elements,
                elementCount
            );

            // 释放输入布局数组
            // Delete input layout array
            delete[] elements;
            DX12LogSuccess("[DX12Core] Pipeline state created successfully\n");

            // 创建线管线状态（用于锥体线框等线列表绘制）
            // Create line pipeline state (for cone wireframe and other line list drawing)
            CreateLinePipelineState();
        }
        catch (const std::exception& e)
        {
            // 着色器编译失败时使用默认管线状态
            // Use default pipeline state when shader compilation fails
            DX12LogError(("[DX12Core] Failed to create pipeline state: " + std::string(e.what()) + "\n").c_str());
            DX12LogWarning("[DX12Core] Using default pipeline state\n");
            PipelineState->Initialize(*this, RootSignature.get());
        }

        // 创建延迟渲染所需的 Geometry Pass 和 Lighting Pass 管线状态
        // Create Geometry Pass and Lighting Pass pipeline states for deferred rendering
        CreateGeometryPipelineState();
        CreateLightingPipelineState();

        // 创建光源剔除管理器并初始化计算管线（Tile-Based Light Culling）
        // Create light culling manager and initialize compute pipeline
        pLightCulling = std::make_unique<DX12LightCullingManager>();
        pLightCulling->CreateRootSignature(*this);
        pLightCulling->CreateComputePSO(*this);
    }

    /**
     * @brief 创建线管线状态对象 / Create line pipeline state object
     *
     * 创建用于线列表（LINELIST）拓扑的管线状态，
     * 使用与主 PSO 相同的着色器和输入布局，
     * 但图元拓扑类型设置为 D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE。
     *
     * Creates pipeline state for line list (LINELIST) topology,
     * using the same shaders and input layout as the main PSO,
     * but with primitive topology type set to D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE.
     */
    void DX12Core::CreateLinePipelineState()
    {
        LinePipelineState = std::make_unique<DX12PipelineState>();

        DX12Log("[DX12Core] Creating line pipeline state...\n");

        try
        {
            // 使用与主 PSO 相同的着色器路径
            // Use the same shader paths as the main PSO
            wchar_t basePath[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, basePath);
            std::wstring vsPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\PBRVertexShader.hlsl";
            std::wstring psPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\PBRPixelShader.hlsl";

            // 编译着色器
            // Compile shaders
            std::vector<uint8_t> vsBytecode = DX12ShaderCompiler::CompileVertexShader(vsPath);
            std::vector<uint8_t> psBytecode = DX12ShaderCompiler::CompilePixelShader(psPath);

            // 创建 PBR 输入布局
            // Create PBR input layout
            UINT elementCount = 0;
            D3D12_INPUT_ELEMENT_DESC* elements = LinePipelineState->CreatePBRInputLayout(elementCount);

            // 设置线拓扑类型
            // Set line topology type
            LinePipelineState->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

            // 从着色器初始化管线状态
            // Initialize pipeline state from shaders
            LinePipelineState->InitializeFromShaders(
                *this,
                RootSignature.get(),
                vsBytecode,
                psBytecode,
                elements,
                elementCount
            );

            delete[] elements;
            DX12LogSuccess("[DX12Core] Line pipeline state created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] Failed to create line pipeline state: " + std::string(e.what()) + "\n").c_str());
            DX12LogWarning("[DX12Core] Line pipeline state not available, wireframe rendering will use default PSO\n");
        }
    }

    /**
     * @brief 开始一帧渲染 / Begin a frame of rendering
     *
     * 重置当前帧的命令分配器和命令列表，
     * 设置描述符堆、视口和裁剪矩形，为渲染做准备。
     *
     * Resets the command allocator and command list for the current frame,
     * sets descriptor heaps, viewport, and scissor rectangle to prepare for rendering.
     *
     * @throws std::runtime_error 如果命令分配器或命令列表重置失败
     *                             If command allocator or command list reset fails
     */
    void DX12Core::BeginFrame()
    {
        // 检查命令分配器是否存在
        // Check if command allocator exists
        if (!pCommandAllocators[CurrentBackBufferIndex])
        {
            throw std::runtime_error("DX12Core::BeginFrame - pCommandAllocators is null");
        }

        // 重置命令分配器（释放之前帧的所有命令内存）
        // Reset command allocator (frees all command memory from previous frame)
        HRESULT hr = pCommandAllocators[CurrentBackBufferIndex]->Reset();
        if (FAILED(hr))
        {
            throw std::runtime_error("DX12Core::BeginFrame - Failed to reset command allocator");
        }

        // 检查命令列表是否存在
        // Check if command list exists
        if (!pCommandList)
        {
            throw std::runtime_error("DX12Core::BeginFrame - pCommandList is null");
        }

        // 重置命令列表，准备记录新的命令
        // Reset command list to prepare for recording new commands
        hr = pCommandList->Reset(pCommandAllocators[CurrentBackBufferIndex].Get(), nullptr);
        if (FAILED(hr))
        {
            throw std::runtime_error("DX12Core::BeginFrame - Failed to reset command list");
        }

        // 设置着色器可见的描述符堆
        // Set shader-visible descriptor heaps
        ID3D12DescriptorHeap* heaps[] = {
            CBVSRVUAVHeap->GetHeap(),
            SamplerHeap->GetHeap()
        };
        pCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

        // 设置视口
        // Set viewport
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;                           ///< 视口左上角X / Viewport top-left X
        viewport.TopLeftY = 0;                           ///< 视口左上角Y / Viewport top-left Y
        viewport.Width = static_cast<float>(Width);      ///< 视口宽度 / Viewport width
        viewport.Height = static_cast<float>(Height);    ///< 视口高度 / Viewport height
        viewport.MinDepth = 0.0f;                        ///< 最小深度 / Minimum depth
        viewport.MaxDepth = 1.0f;                        ///< 最大深度 / Maximum depth

        // 设置裁剪矩形
        // Set scissor rectangle
        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;                             ///< 左边界 / Left boundary
        scissorRect.top = 0;                              ///< 上边界 / Top boundary
        scissorRect.right = Width;                        ///< 右边界 / Right boundary
        scissorRect.bottom = Height;                      ///< 下边界 / Bottom boundary

        // 将视口和裁剪矩形设置到光栅化阶段
        // Set viewport and scissor rectangle to rasterizer stage
        pCommandList->RSSetViewports(1, &viewport);
        pCommandList->RSSetScissorRects(1, &scissorRect);
    }

    /**
     * @brief 结束一帧渲染 / End a frame of rendering
     *
     * 关闭命令列表，将命令提交到命令队列执行，
     * 呈现交换链并前进到下一帧。
     *
     * Closes the command list, submits commands to the command queue for execution,
     * presents the swap chain, and advances to the next frame.
     */
    void DX12Core::EndFrame()
    {
        // 关闭命令列表（停止记录命令）
        // Close command list (stop recording commands)
        HRESULT hr = pCommandList->Close();
        if (FAILED(hr))
        {
            DX12LogError("[DX12Core] Failed to close command list\n");
        }

        // 将命令列表提交到命令队列执行
        // Submit command list to command queue for execution
        ID3D12CommandList* commandLists[] = { pCommandList.Get() };
        pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        // 呈现当前帧（交换前后台缓冲区）
        // Present current frame (swap front and back buffers)
        hr = pSwapChain->Present(1, 0);
        if (FAILED(hr))
        {
            DX12LogError("[DX12Core] Present failed\n");
        }

        // 移动到下一帧
        // Move to the next frame
        MoveToNextFrame();
    }

    /**
     * @brief 等待 GPU 完成当前帧操作 / Wait for GPU to complete current frame operations
     *
     * 阻塞 CPU 直到 GPU 完成当前帧对应的围栏值。
     * Blocks the CPU until the GPU completes the fence value corresponding to the current frame.
     */
    void DX12Core::WaitForGPU()
    {
        if (!Fence)
            return;

        for (UINT i = 0; i < FRAME_COUNT; i++)
        {
            Fence->Wait(FrameFenceValues[i]);
        }
    }

    /**
     * @brief 移动到下一帧 / Move to the next frame
     *
     * 为当前帧信号围栏，更新当前后台缓冲区索引，
     * 并确保下一帧的资源已经准备好（GPU已完成）。
     *
     * Signals the fence for the current frame, updates the current back buffer index,
     * and ensures the next frame's resources are ready (GPU has completed).
     */
    void DX12Core::MoveToNextFrame()
    {
        if (!Fence || !pSwapChain)
            return;

        // 为当前帧信号围栏（GPU完成此帧时设置此值）
        // Signal fence for current frame (GPU sets this value when done with this frame)
        FrameFenceValues[CurrentBackBufferIndex] = Fence->Increment();
        Fence->Signal(FrameFenceValues[CurrentBackBufferIndex]);

        // 获取新的当前后台缓冲区索引（交换链更新）
        // Get new current back buffer index (swap chain update)
        CurrentBackBufferIndex = pSwapChain->GetCurrentBackBufferIndex();

        // 检查下一帧是否已完成，如果没有则等待
        // Check if next frame is completed, wait if not
        if (Fence->GetCompletedValue() < FrameFenceValues[CurrentBackBufferIndex])
        {
            Fence->Wait(FrameFenceValues[CurrentBackBufferIndex]);
        }
    }

    /**
     * @brief 调整窗口大小 / Resize the window
     *
     * 等待 GPU 完成操作，调整交换链缓冲区大小，
     * 并重新创建渲染目标视图。
     *
     * Waits for GPU to complete operations, resizes swap chain buffers,
     * and recreates render target views.
     *
     * @param newWidth 新宽度 / New width
     * @param newHeight 新高度 / New height
     * @throws std::runtime_error 如果调整缓冲区大小失败
     *                             If resizing buffers fails
     */
    void DX12Core::Resize(int newWidth, int newHeight)
    {
        DX12Log(("[DX12Core] Resize: " + std::to_string(newWidth) + "x" + std::to_string(newHeight) + "\n").c_str());

        // 等待 GPU 完成所有操作
        // Wait for GPU to complete all operations
        WaitForGPU();

        // 更新尺寸
        // Update dimensions
        Width = newWidth;
        Height = newHeight;

        // 释放渲染目标
        // Release render targets
        ReleaseRenderTargetViews();

        // 调整交换链缓冲区大小
        // Resize swap chain buffers
        HRESULT hr = pSwapChain->ResizeBuffers(
            FRAME_COUNT,                                ///< 缓冲区数量 / Buffer count
            newWidth,                                   ///< 新宽度 / New width
            newHeight,                                  ///< 新高度 / New height
            DXGI_FORMAT_R8G8B8A8_UNORM,                 ///< 像素格式 / Pixel format
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH      ///< 标志 / Flags
        );
        if (FAILED(hr))
        {
            DX12LogError(("[DX12Core] ResizeBuffers FAILED! hr=" + std::to_string(hr) + "\n").c_str());
            throw std::runtime_error("ResizeBuffers failed: hr=" + std::to_string(hr));
        }

        // 获取新的当前后台缓冲区索引
        // Get new current back buffer index
        CurrentBackBufferIndex = pSwapChain->GetCurrentBackBufferIndex();

        // 重新创建渲染目标视图
        // Recreate render target views
        CreateRenderTargetViews(newWidth, newHeight);

        DX12Log("[DX12Core] Resize complete\n");

        // 注意：深度模板的重新创建由 DX12Renderer 通过 DepthStencilDX12 处理
        // Note: Depth stencil recreation is handled by DX12Renderer via DepthStencilDX12
    }

    /**
     * @brief 释放渲染目标视图 / Release render target views
     *
     * 释放所有渲染目标资源的 ComPtr 引用。
     * Releases ComPtr references to all render target resources.
     */
    void DX12Core::ReleaseRenderTargetViews()
    {
        // 逐个释放渲染目标资源
        // Release render target resources one by one
        for (UINT i = 0; i < FRAME_COUNT; ++i)
        {
            pRenderTargets[i].Reset();
        }
    }

    /**
     * @brief 获取当前渲染目标资源 / Get current render target resource
     * @return 当前后台缓冲区的 ID3D12Resource 指针
     *         ID3D12Resource pointer of current back buffer
     */
    ID3D12Resource* DX12Core::GetCurrentRenderTarget() const noexcept
    {
        return pRenderTargets[CurrentBackBufferIndex].Get();
    }

    /**
     * @brief 获取当前渲染目标视图 CPU 句柄 / Get current render target view CPU handle
     * @return 当前 RTV 的 CPU 描述符句柄
     *         CPU descriptor handle of current RTV
     */
    D3D12_CPU_DESCRIPTOR_HANDLE DX12Core::GetCurrentRTVHandle() const noexcept
    {
        return RTVHeap->GetCPUHandle(CurrentBackBufferIndex);
    }

    /**
     * @brief 获取深度模板视图 CPU 句柄 / Get depth stencil view CPU handle
     * @return DSV 的 CPU 描述符句柄（索引0）
     *         CPU descriptor handle of DSV (index 0)
     */
    D3D12_CPU_DESCRIPTOR_HANDLE DX12Core::GetDSVHandle() const noexcept
    {
        return DSVHeap->GetCPUHandle(0);
    }

    /**
     * @brief 创建提交资源（辅助函数）/ Create committed resource (helper function)
     *
     * 使用默认堆类型创建一个已提交的 D3D12 资源。
     * 已提交资源在物理内存中是连续的。
     *
     * Creates a committed D3D12 resource using default heap type.
     * Committed resources are contiguous in physical memory.
     *
     * @param dimension 资源维度 / Resource dimension
     * @param width 资源宽度 / Resource width
     * @param height 资源高度 / Resource height
     * @param depthOrArraySize 深度或数组大小 / Depth or array size
     * @param mipLevels Mip 级别数 / Number of mip levels
     * @param format 像素格式 / Pixel format
     * @param flags 资源标志 / Resource flags
     * @param initialState 初始资源状态 / Initial resource state
     * @param clearValue 清除值（可选）/ Clear value (optional)
     * @return 已创建资源的 ComPtr / ComPtr of created resource
     * @throws std::runtime_error 如果资源创建失败
     *                             If resource creation fails
     */
    Microsoft::WRL::ComPtr<ID3D12Resource> DX12Core::CreateCommittedResource(
        D3D12_RESOURCE_DIMENSION dimension,
        UINT64 width,
        UINT height,
        UINT16 depthOrArraySize,
        UINT16 mipLevels,
        DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* clearValue)
    {
        // 配置资源描述
        // Configure resource description
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = dimension;                ///< 资源维度 / Resource dimension
        resourceDesc.Alignment = 0;                        ///< 对齐（0表示默认）/ Alignment (0 means default)
        resourceDesc.Width = width;                        ///< 宽度 / Width
        resourceDesc.Height = height;                      ///< 高度 / Height
        resourceDesc.DepthOrArraySize = depthOrArraySize;  ///< 深度或数组大小 / Depth or array size
        resourceDesc.MipLevels = mipLevels;                ///< Mip 级别数 / Number of mip levels
        resourceDesc.Format = format;                      ///< 像素格式 / Pixel format
        resourceDesc.SampleDesc.Count = 1;                 ///< 采样计数 / Sample count
        resourceDesc.SampleDesc.Quality = 0;               ///< 采样质量 / Sample quality
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  ///< 纹理布局（未知）/ Texture layout (unknown)
        resourceDesc.Flags = flags;                        ///< 资源标志 / Resource flags

        // 配置堆属性（默认堆）
        // Configure heap properties (default heap)
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;          ///< 默认堆类型 / Default heap type
        heapProps.CreationNodeMask = 1;                    ///< 创建节点掩码 / Creation node mask
        heapProps.VisibleNodeMask = 1;                     ///< 可见节点掩码 / Visible node mask

        // 创建已提交资源
        // Create committed resource
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        HRESULT hr = pDevice->CreateCommittedResource(
            &heapProps,               ///< 堆属性 / Heap properties
            D3D12_HEAP_FLAG_NONE,     ///< 堆标志 / Heap flags
            &resourceDesc,            ///< 资源描述 / Resource description
            initialState,             ///< 初始状态 / Initial state
            clearValue,               ///< 清除值 / Clear value
            IID_PPV_ARGS(&resource)   ///< 输出资源 / Output resource
        );
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create committed resource");
        }

        return resource;
    }

    /**
     * @brief 创建 Geometry Pass 管线状态对象 / Create Geometry Pass pipeline state object
     *
     * 编译 GBuffer 顶点/像素着色器，配置 MRT (4 render targets) 管线状态。
     * G-Buffer 布局:
     *   RT0: Albedo(RGB) + AO(A)        - R8G8B8A8_UNORM
     *   RT1: Normal(RGB) + Roughness(A) - R16G16B16A16_FLOAT
     *   RT2: Position(RGB) + Metallic(A) - R16G16B16A16_FLOAT
     *   RT3: Emissive(RGB) + Unused(A)  - R8G8B8A8_UNORM (reserved)
     *   Depth: D32_FLOAT
     *
     * Compiles GBuffer vertex/pixel shaders, creates MRT (4 render targets) pipeline state.
     */
    void DX12Core::CreateGeometryPipelineState()
    {
        GeometryPipelineState = std::make_unique<DX12PipelineState>();

        DX12Log("[DX12Core] Creating Geometry Pass pipeline state...\n");

        try
        {
            // 构建着色器路径
            // Build shader paths
            wchar_t basePath[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, basePath);
            std::wstring vsPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\GBuffer\\GBufferVertexShader.hlsl";
            std::wstring psPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\GBuffer\\GBufferPixelShader.hlsl";

            // 编译 GBuffer 着色器
            // Compile GBuffer shaders
            std::vector<uint8_t> vsBytecode = DX12ShaderCompiler::CompileVertexShader(vsPath);
            std::vector<uint8_t> psBytecode = DX12ShaderCompiler::CompilePixelShader(psPath);

            // 创建 PBR 输入布局（GBufferVSInput 与 PBR 输入布局兼容：Position, Normal, TextureCoord）
            // Create PBR input layout (GBufferVSInput is compatible with PBR layout: Position, Normal, TextureCoord)
            UINT elementCount = 0;
            D3D12_INPUT_ELEMENT_DESC* elements = GeometryPipelineState->CreatePBRInputLayout(elementCount);

            // 配置 MRT 渲染目标格式
            // Configure MRT render target formats
            DXGI_FORMAT gbufferFormats[GBUFFER_RT_COUNT] = {
                DXGI_FORMAT_R8G8B8A8_UNORM,        ///< RT0: Albedo + AO
                DXGI_FORMAT_R16G16B16A16_FLOAT,    ///< RT1: Normal + Roughness
                DXGI_FORMAT_R16G16B16A16_FLOAT,    ///< RT2: Position + Metallic
                DXGI_FORMAT_R8G8B8A8_UNORM,         ///< RT3: Emissive (reserved)
            };
            GeometryPipelineState->SetRenderTargetFormats(
                gbufferFormats, GBUFFER_RT_COUNT, DXGI_FORMAT_D32_FLOAT);

            // 从着色器初始化管线状态
            // Initialize pipeline state from shaders
            GeometryPipelineState->InitializeFromShaders(
                *this,
                RootSignature.get(),
                vsBytecode,
                psBytecode,
                elements,
                elementCount
            );

            delete[] elements;
            DX12LogSuccess("[DX12Core] Geometry Pass pipeline state created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] Failed to create Geometry Pass pipeline state: " + std::string(e.what()) + "\n").c_str());
            DX12LogWarning("[DX12Core] Geometry Pass pipeline state not available, deferred rendering disabled\n");
        }
    }

    /**
     * @brief 创建 Lighting Pass 管线状态对象 / Create Lighting Pass pipeline state object
     *
     * 编译 LightingPass 顶点/像素着色器，创建全屏三角形管线状态。
     * - 无输入布局（使用 SV_VertexID 生成顶点）
     * - 禁用深度测试（全屏三角形覆盖整个屏幕）
     * - 无背面剔除
     * - 单渲染目标输出（场景渲染目标）
     *
     * Compiles LightingPass vertex/pixel shaders, creates full-screen triangle pipeline state.
     * - No input layout (uses SV_VertexID to generate vertices)
     * - Depth test disabled (full-screen triangle covers entire screen)
     * - No back-face culling
     * - Single render target output (scene render target)
     */
    void DX12Core::CreateLightingPipelineState()
    {
        LightingPipelineState = std::make_unique<DX12PipelineState>();

        DX12Log("[DX12Core] Creating Lighting Pass pipeline state...\n");

        try
        {
            // 构建着色器路径
            // Build shader paths
            wchar_t basePath[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, basePath);
            std::wstring vsPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\LightingPass\\LightingPassVertexShader.hlsl";
            std::wstring psPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\LightingPass\\LightingPassPixelShader.hlsl";

            // 编译 LightingPass 着色器
            // Compile LightingPass shaders
            std::vector<uint8_t> vsBytecode = DX12ShaderCompiler::CompileVertexShader(vsPath);
            std::vector<uint8_t> psBytecode = DX12ShaderCompiler::CompilePixelShader(psPath);

            // 配置单渲染目标格式（场景渲染目标）
            // Configure single render target format (scene render target)
            DXGI_FORMAT sceneFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            LightingPipelineState->SetRenderTargetFormats(&sceneFormat, 1, DXGI_FORMAT_UNKNOWN);

            // 禁用深度测试和深度写入（全屏三角形不需要深度）
            // Disable depth test and depth write (full-screen triangle doesn't need depth)
            D3D12_DEPTH_STENCIL_DESC dsDesc = DX12PipelineState::CreateDefaultDepthStencilState();
            dsDesc.DepthEnable = FALSE;
            dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
            LightingPipelineState->SetDepthStencilState(dsDesc);

            // 无背面剔除（全屏三角形覆盖整个屏幕，避免剔除问题）
            // No back-face culling (full-screen triangle covers entire screen, avoid culling issues)
            D3D12_RASTERIZER_DESC rasterDesc = DX12PipelineState::CreateDefaultRasterizerState();
            rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
            LightingPipelineState->SetRasterizerState(rasterDesc);

            // 从着色器初始化管线状态（无输入布局，使用 SV_VertexID）
            // Initialize pipeline state from shaders (no input layout, uses SV_VertexID)
            LightingPipelineState->InitializeFromShaders(
                *this,
                RootSignature.get(),
                vsBytecode,
                psBytecode,
                nullptr,    ///< 无输入布局 / No input layout
                0           ///< 0 个输入元素 / 0 input elements
            );

            DX12LogSuccess("[DX12Core] Lighting Pass pipeline state created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Core] Failed to create Lighting Pass pipeline state: " + std::string(e.what()) + "\n").c_str());
            DX12LogWarning("[DX12Core] Lighting Pass pipeline state not available, deferred rendering disabled\n");
        }
    }

    void DX12Core::CreateLightCullingResources()
    {
        if (!pLightCulling)
        {
            pLightCulling = std::make_unique<DX12LightCullingManager>();
            pLightCulling->CreateRootSignature(*this);
            pLightCulling->CreateComputePSO(*this);
        }
        pLightCulling->CreateResources(*this);
    }
}

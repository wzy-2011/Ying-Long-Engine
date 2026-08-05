/**
 * @file DX12Renderer.cpp
 * @brief DX12 渲染器实现文件 / DX12 Renderer Implementation
 *
 * 本文件实现了 DX12Renderer 类的所有方法，包括渲染器初始化、
 * 帧渲染生命周期、可绘制对象渲染以及窗口大小调整。
 *
 * This file implements all methods of the DX12Renderer class, including
 * renderer initialization, frame rendering lifecycle, drawable object
 * rendering, and window resizing.
 */

#include "DX12Renderer.h"
#include "DX12Primitives.h"
#include "../../Debug/DX12Log.h"
#include <cstring>
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 初始化所有成员变量为默认值。
     * Initializes all member variables with default values.
     */
    DX12Renderer::DX12Renderer()
        : pCamera(nullptr)              ///< 摄像机初始化为空 / Camera initialized to null
        , Width(0)                       ///< 宽度初始化为0 / Width initialized to 0
        , Height(0)                      ///< 高度初始化为0 / Height initialized to 0
        , SceneWidth(0)                  ///< 场景宽度初始化为0 / Scene width initialized to 0
        , SceneHeight(0)                 ///< 场景高度初始化为0 / Scene height initialized to 0
        , bInitialized(false)            ///< 未初始化 / Not initialized
        , bInFrame(false)                ///< 不在帧中 / Not in frame
        , bInImGuiFrame(false)           ///< 不在 ImGui 帧中 / Not in ImGui frame
        , bInGeometryPass(false)         ///< 不在 Geometry Pass 中 / Not in geometry pass
        , bUseDeferredRendering(false)   ///< 默认使用前向渲染 / Default to forward rendering
        , hWnd(nullptr)                  ///< 窗口句柄为空 / Window handle is null
        , bNeedsResize(false)            ///< 不需要调整大小 / No resize needed
        , bNeedsSceneResize(false)       ///< 不需要调整场景大小 / No scene resize needed
        , PendingWidth(0)                ///< 待处理宽度为0 / Pending width is 0
        , PendingHeight(0)               ///< 待处理高度为0 / Pending height is 0
        , PendingSceneWidth(0)           ///< 待处理场景宽度为0 / Pending scene width is 0
        , PendingSceneHeight(0)          ///< 待处理场景高度为0 / Pending scene height is 0
    {
        // 设置默认清除颜色为深灰色
        // Set default clear color to dark gray
        ClearColor[0] = 0.1f;    ///< 红色通道 / Red channel
        ClearColor[1] = 0.1f;    ///< 绿色通道 / Green channel
        ClearColor[2] = 0.1f;    ///< 蓝色通道 / Blue channel
        ClearColor[3] = 1.0f;    ///< Alpha 通道 / Alpha channel
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 调用 Shutdown() 释放所有资源，捕获所有异常以确保安全释放。
     * Calls Shutdown() to release all resources, catches all exceptions
     * to ensure safe release.
     */
    DX12Renderer::~DX12Renderer()
    {
        try
        {
            Shutdown();
        }
        catch (...)
        {
            // 捕获析构函数中的所有异常，避免程序崩溃
            // Catch all exceptions in destructor to avoid program crash
        }
    }

    /**
     * @brief 获取当前渲染目标 / Get current render target
     *
     * 根据当前后台缓冲区索引返回对应的渲染目标。
     * Returns the render target corresponding to the current back buffer index.
     *
     * @return 当前渲染目标指针，如果不可用返回 nullptr
     *         Current render target pointer, nullptr if not available
     */
    RenderTargetDX12* DX12Renderer::GetRenderTarget() const noexcept
    {
        // 检查核心对象是否存在
        // Check if core object exists
        if (!pCore)
            return nullptr;

        // 获取当前后台缓冲区索引
        // Get current back buffer index
        UINT idx = pCore->GetCurrentBackBufferIndex();

        // 检查索引范围和渲染目标是否存在
        // Check index range and if render target exists
        if (idx < FRAME_COUNT && pRenderTargets[idx])
            return pRenderTargets[idx].get();

        return nullptr;
    }

    /**
     * @brief 初始化渲染器 / Initialize the renderer
     *
     * 按四个步骤初始化渲染器：
     * 1. 初始化 DX12 核心
     * 2. 为交换链的两个后台缓冲区创建渲染目标
     * 3. 创建深度模板
     * 4. 初始化 ImGui
     *
     * Initializes the renderer in four steps:
     * 1. Initialize DX12 core
     * 2. Create render targets for both back buffers of the swap chain
     * 3. Create depth stencil
     * 4. Initialize ImGui
     *
     * @param hWnd 窗口句柄 / Window handle
     * @param width 渲染宽度 / Render width
     * @param height 渲染高度 / Render height
     * @throws std::invalid_argument 如果参数无效
     *                                If parameters are invalid
     * @throws std::runtime_error 如果任何初始化步骤失败
     *                             If any initialization step fails
     */
    void DX12Renderer::Initialize(HWND hWnd, int width, int height)
    {
        DX12Log("[DX12Renderer] === Starting Renderer Initialization ===\n");

        // 验证窗口句柄
        // Validate window handle
        if (hWnd == nullptr)
        {
            DX12LogError("[DX12Renderer] FAILED: Invalid HWND (nullptr)\n");
            throw std::invalid_argument("Invalid HWND provided to DX12Renderer::Initialize");
        }

        // 验证尺寸
        // Validate dimensions
        if (width <= 0 || height <= 0)
        {
            DX12LogError("[DX12Renderer] FAILED: Invalid dimensions\n");
            throw std::invalid_argument("Invalid dimensions provided to DX12Renderer::Initialize");
        }

        DX12Log(("[DX12Renderer] Window dimensions: " + std::to_string(width) + " x " + std::to_string(height) + "\n").c_str());

        // 保存尺寸和窗口句柄
        // Store dimensions and window handle
        Width = width;
        Height = height;
        this->hWnd = hWnd;

        // 步骤1：初始化 DX12 核心
        // Step 1: Initialize DX12 core
        DX12Log("[DX12Renderer] Step 1: Creating DX12Core...\n");
        pCore = std::make_unique<DX12Core>();
        try
        {
            pCore->Initialize(hWnd, width, height);
            DX12LogSuccess("[DX12Renderer] DX12Core initialized successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Renderer] FAILED Step 1 (DX12Core): " + std::string(e.what()) + "\n").c_str());
            pCore.reset();
            throw std::runtime_error("Failed to initialize DX12Core: " + std::string(e.what()));
        }

        // 步骤2：为两个交换链后台缓冲区创建渲染目标
        // Step 2: Cache both swap chain back buffers as render targets
        DX12Log("[DX12Renderer] Step 2: Creating render targets for both back buffers...\n");
        try
        {
            // 为每个帧缓冲区创建渲染目标
            // Create render target for each frame buffer
            for (int i = 0; i < FRAME_COUNT; i++)
            {
                pRenderTargets[i] = std::make_unique<RenderTargetDX12>();
                pRenderTargets[i]->InitializeFromSwapChain(*pCore, i);
            }
            DX12LogSuccess("[DX12Renderer] Render targets initialized successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Renderer] FAILED Step 2 (RenderTarget): " + std::string(e.what()) + "\n").c_str());
            // 清理已创建的渲染目标
            // Clean up created render targets
            for (int i = 0; i < FRAME_COUNT; i++)
                pRenderTargets[i].reset();
            throw std::runtime_error("Failed to initialize RenderTargetDX12: " + std::string(e.what()));
        }

        // 步骤3：创建深度模板
        // Step 3: Create depth stencil
        DX12Log("[DX12Renderer] Step 3: Creating depth stencil...\n");
        pDepthStencil = std::make_unique<DepthStencilDX12>();
        try
        {
            pDepthStencil->Initialize(*pCore, width, height);
            DX12LogSuccess("[DX12Renderer] Depth stencil initialized successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Renderer] FAILED Step 3 (DepthStencil): " + std::string(e.what()) + "\n").c_str());
            pDepthStencil.reset();
            throw std::runtime_error("Failed to initialize DepthStencilDX12: " + std::string(e.what()));
        }

        // 步骤4：初始化 ImGui
        // Step 4: Initialize ImGui
        DX12Log("[DX12Renderer] Step 4: Initializing ImGui...\n");
        pImGui = std::make_unique<ImGuiDX12>();
        try
        {
            pImGui->Initialize(*pCore, hWnd);
            DX12LogSuccess("[DX12Renderer] ImGui initialized successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Renderer] FAILED Step 4 (ImGui): " + std::string(e.what()) + "\n").c_str());
            pImGui.reset();
            throw std::runtime_error("Failed to initialize ImGuiDX12: " + std::string(e.what()));
        }

        // 标记初始化完成
        // Mark initialization as complete
        bInitialized = true;
        DX12Log("[DX12Renderer] === Renderer Initialization Complete ===\n");
    }

    /**
     * @brief 关闭渲染器 / Shutdown the renderer
     *
     * 等待 GPU 完成操作，然后按逆序释放所有资源。
     * Waits for GPU to complete operations, then releases all resources in reverse order.
     */
    void DX12Renderer::Shutdown()
    {
        // 如果未初始化则直接返回
        // Return directly if not initialized
        if (!bInitialized)
            return;

        DX12Log("[DX12Renderer] Shutting down...\n");
        bInitialized = false;

        // 等待 GPU 完成所有操作
        // Wait for GPU to complete all operations
        if (pCore)
        {
            try
            {
                pCore->WaitForGPU();
            }
            catch (...)
            {
                // 捕获等待 GPU 时的异常
                // Catch exceptions while waiting for GPU
            }
        }

        // 关闭并释放 ImGui
        // Shutdown and release ImGui
        if (pImGui)
        {
            try
            {
                pImGui->Shutdown();
            }
            catch (...)
            {
                // 捕获 ImGui 关闭时的异常
                // Catch exceptions during ImGui shutdown
            }
            pImGui.reset();
        }

        // 释放场景渲染目标和场景深度模板（必须在 pCore 之前释放，
        // 因为它们的 Shutdown 需要通过 pCore 释放描述符索引，
        // 否则 pCore 被销毁后 pCore 指针变为悬垂指针，Free() 会 use-after-free）
        // Release scene render target and scene depth stencil before pCore,
        // because their Shutdown() frees descriptor indices via pCore.
        // Without this, pCore is destroyed first and the pCore pointer
        // becomes dangling, causing use-after-free in Free().
        pGBuffer.reset();
        pSceneRenderTarget.reset();
        pSceneDepthStencil.reset();

        // 释放光源计数常量缓冲区（延迟渲染 Lighting Pass 使用，必须在 pCore 之前释放）
        // Release light count constant buffer (used by deferred Lighting Pass, before pCore)
        pLightCountBuffer.reset();

        // 释放深度模板
        // Release depth stencil
        pDepthStencil.reset();

        // 释放渲染目标
        // Release render targets
        for (int i = 0; i < FRAME_COUNT; i++)
            pRenderTargets[i].reset();

        // 释放 DX12 核心
        // Release DX12 core
        pCore.reset();

        // 重置状态变量
        // Reset state variables
        bInFrame = false;
        bInImGuiFrame = false;
        Width = 0;
        Height = 0;
        hWnd = nullptr;

        DX12Log("[DX12Renderer] Shutdown complete\n");
    }

    /**
     * @brief 开始渲染帧 / Begin rendering frame
     *
     * 执行以下操作：
     * 1. 执行待处理的窗口调整
     * 2. 调用 DX12Core::BeginFrame()
     * 3. 获取当前渲染目标和深度模板
     * 4. 将渲染目标转换为渲染目标状态
     * 5. 将深度模板转换为深度写入状态
     * 6. 清除渲染目标和深度模板
     * 7. 绑定渲染目标和深度模板
     *
     * Performs the following operations:
     * 1. Execute pending window resize
     * 2. Call DX12Core::BeginFrame()
     * 3. Get current render target and depth stencil
     * 4. Transition render target to render target state
     * 5. Transition depth stencil to depth write state
     * 6. Clear render target and depth stencil
     * 7. Bind render target and depth stencil
     *
     * @param clearColor 清除颜色，如果为 nullptr 则使用默认颜色
     *                   Clear color, uses default color if nullptr
     * @throws std::runtime_error 如果渲染器未初始化或关键对象为空
     *                             If renderer is not initialized or key objects are null
     */
    void DX12Renderer::BeginFrame(const float clearColor[4])
    {
        // 检查是否已初始化
        // Check if initialized
        if (!bInitialized)
            throw std::runtime_error("DX12Renderer::BeginFrame called on uninitialized renderer");

        // 检查核心对象
        // Check core object
        if (!pCore)
            throw std::runtime_error("DX12Renderer::BeginFrame - pCore is null");

        // 检查深度模板
        // Check depth stencil
        if (!pDepthStencil)
            throw std::runtime_error("DX12Renderer::BeginFrame - pDepthStencil is null");

        // 执行待处理的调整大小
        // Execute pending resize
        ExecuteResize();
        ExecuteSceneResize();

        // 确定使用的清除颜色
        // Determine clear color to use
        const float* color = clearColor ? clearColor : ClearColor;

        // 开始 DX12 核心帧
        // Begin DX12 core frame
        pCore->BeginFrame();

        // 获取当前后台缓冲区索引和渲染目标
        // Get current back buffer index and render target
        UINT backBufferIndex = pCore->GetCurrentBackBufferIndex();
        RenderTargetDX12* rt = pRenderTargets[backBufferIndex].get();
        if (!rt)
            throw std::runtime_error("DX12Renderer::BeginFrame - current render target is null");

        // 获取命令列表
        // Get command list
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
        {
            throw std::runtime_error("Failed to get command list in DX12Renderer::BeginFrame");
        }

        // 资源状态转换：渲染目标 -> 渲染目标状态
        // Resource state transition: render target -> render target state
        rt->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // 资源状态转换：深度模板 -> 深度写入状态
        // Resource state transition: depth stencil -> depth write state
        pDepthStencil->TransitionTo(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // 防御性检查：在 Clear 前验证 RTV 句柄和资源有效性
        // Defensive check: verify RTV handle and resource validity before Clear
        if (rt->GetRTVHandle().ptr == 0)
        {
            DX12LogError("[DX12Renderer::BeginFrame] Render target RTV handle is null, skipping clear/bind\n");
            bInFrame = true;
            return;
        }
        if (!rt->GetResource())
        {
            DX12LogError("[DX12Renderer::BeginFrame] Render target resource is null, skipping clear/bind\n");
            bInFrame = true;
            return;
        }

        // 清除渲染目标和深度模板
        // Clear render target and depth stencil
        rt->Clear(commandList, color);
        pDepthStencil->Clear(commandList, true, true);

        // 绑定渲染目标和深度模板到输出合并阶段
        // Bind render target and depth stencil to output merger stage
        auto dsvHandle = pDepthStencil->GetDSVHandle();
        rt->Bind(commandList, &dsvHandle);

        // 更新光源缓冲区（每帧只执行一次）
        // Update light buffers (executed once per frame)
        DX12Primitive::UpdateLightBuffers(commandList);

        // 标记进入帧状态
        // Mark in-frame state
        bInFrame = true;
    }

    /**
     * @brief 开始 ImGui 帧 / Begin ImGui frame
     *
     * 调用 ImGuiDX12::BeginFrame() 开始新的 ImGui 帧。
     * Calls ImGuiDX12::BeginFrame() to start a new ImGui frame.
     *
     * @throws std::runtime_error 如果渲染器未初始化或 ImGui 未初始化
     *                             If renderer is not initialized or ImGui is not initialized
     */
    void DX12Renderer::BeginImGuiFrame()
    {
        // 检查是否已初始化
        // Check if initialized
        if (!bInitialized)
            throw std::runtime_error("DX12Renderer::BeginImGuiFrame called on uninitialized renderer");

        // 检查 ImGui 是否已初始化
        // Check if ImGui is initialized
        if (!pImGui)
            throw std::runtime_error("ImGui not initialized in DX12Renderer");

        // 开始 ImGui 帧
        // Begin ImGui frame
        pImGui->BeginFrame();
        bInImGuiFrame = true;
    }

    /**
     * @brief 结束 ImGui 帧 / End ImGui frame
     *
     * 调用 ImGuiDX12::EndFrame() 结束 ImGui 帧并渲染 UI。
     * Calls ImGuiDX12::EndFrame() to end ImGui frame and render UI.
     */
    void DX12Renderer::EndImGuiFrame()
    {
        // 检查是否已初始化
        // Check if initialized
        if (!bInitialized)
            return;

        // 检查 ImGui 和帧状态
        // Check ImGui and frame state
        if (!pImGui || !bInImGuiFrame)
            return;

        // 获取命令列表
        // Get command list
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
            return;

        // 结束 ImGui 帧并渲染
        // End ImGui frame and render
        pImGui->EndFrame(commandList);
        bInImGuiFrame = false;
    }

    /**
     * @brief 结束渲染帧并呈现 / End rendering frame and present
     *
     * 执行以下操作：
     * 1. 如果 ImGui 帧仍在进行中，先结束它
     * 2. 将渲染目标转换为呈现状态
     * 3. 调用 DX12Core::EndFrame() 结束帧并呈现
     *
     * Performs the following operations:
     * 1. If ImGui frame is still in progress, end it first
     * 2. Transition render target to present state
     * 3. Call DX12Core::EndFrame() to end frame and present
     */
    void DX12Renderer::EndFrame()
    {
        // 检查是否已初始化且在帧中
        // Check if initialized and in frame
        if (!bInitialized || !bInFrame)
            return;

        // 如果 ImGui 帧仍在进行中，先结束它
        // If ImGui frame is still in progress, end it first
        if (bInImGuiFrame && pImGui)
        {
            EndImGuiFrame();
        }

        // 获取命令列表
        // Get command list
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
        {
            bInFrame = false;
            return;
        }

        // 获取当前渲染目标
        // Get current render target
        UINT backBufferIndex = pCore->GetCurrentBackBufferIndex();
        RenderTargetDX12* rt = pRenderTargets[backBufferIndex].get();
        if (rt)
        {
            // 资源状态转换：渲染目标 -> 呈现状态
            // Resource state transition: render target -> present state
            rt->TransitionTo(commandList, D3D12_RESOURCE_STATE_PRESENT);
        }

        // 结束 DX12 核心帧并呈现
        // End DX12 core frame and present
        pCore->EndFrame();

        // Present 后 DXGI 将 FLIP_DISCARD 缓冲区重置为 COMMON 状态。
        // 同步 CPU 端追踪，防止下一帧发出无效的 PRESENT→RENDER_TARGET 屏障。
        // After Present, DXGI resets FLIP_DISCARD buffers to COMMON state.
        // Sync CPU tracking to prevent invalid PRESENT→RENDER_TARGET barriers.
        if (rt)
        {
            rt->OnPresented();
        }

        // 标记离开帧状态
        // Mark out-of-frame state
        bInFrame = false;
    }

    /**
     * @brief 绘制一个可绘制对象 / Draw a drawable object
     *
     * 调用可绘制对象的 Draw 方法执行渲染。
     * Calls the Draw method of the drawable object to perform rendering.
     *
     * @param drawable 可绘制对象引用 / Drawable object reference
     * @throws std::runtime_error 如果未初始化或不在帧中
     *                             If not initialized or not in frame
     */
    void DX12Renderer::Draw(DX12Drawable& drawable)
    {
        // 检查是否已初始化
        // Check if initialized
        if (!bInitialized)
            throw std::runtime_error("DX12Renderer::Draw called on uninitialized renderer");

        // 检查是否在帧中
        // Check if in frame
        if (!bInFrame)
            throw std::runtime_error("DX12Renderer::Draw called outside of BeginFrame/EndFrame");

        // 获取命令列表
        // Get command list
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
            throw std::runtime_error("Failed to get command list in DX12Renderer::Draw");

        // 延迟渲染 Geometry Pass 期间，强制 drawable 使用 GeometryPipelineState
        // During deferred rendering Geometry Pass, force drawable to use GeometryPipelineState
        if (bInGeometryPass && pCore->GetGeometryPipelineState())
        {
            drawable.SetOverridePipelineState(pCore->GetGeometryPipelineState());
        }

        // 调用可绘制对象的绘制方法
        // Call draw method of drawable object
        drawable.Draw(commandList);

        // 清除覆盖 PSO，避免影响后续渲染
        // Clear override PSO to avoid affecting subsequent rendering
        if (bInGeometryPass)
        {
            drawable.SetOverridePipelineState(nullptr);
        }
    }

    /**
     * @brief 绘制多个可绘制对象 / Draw multiple drawables
     *
     * 遍历可绘制对象数组并依次绘制每个对象。
     * Iterates through the drawable array and draws each object in sequence.
     *
     * @param drawables 可绘制对象指针数组 / Array of drawable object pointers
     * @throws std::runtime_error 如果未初始化或不在帧中
     *                             If not initialized or not in frame
     */
    void DX12Renderer::Draw(const std::vector<DX12Drawable*>& drawables)
    {
        // 检查是否已初始化
        // Check if initialized
        if (!bInitialized)
            throw std::runtime_error("DX12Renderer::Draw called on uninitialized renderer");

        // 检查是否在帧中
        // Check if in frame
        if (!bInFrame)
            throw std::runtime_error("DX12Renderer::Draw called outside of BeginFrame/EndFrame");

        // 获取命令列表
        // Get command list
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
            throw std::runtime_error("Failed to get command list in DX12Renderer::Draw");

        // 遍历并绘制所有可绘制对象
        // Iterate and draw all drawable objects
        for (auto* drawable : drawables)
        {
            if (drawable)
            {
                // 延迟渲染 Geometry Pass 期间，强制使用 GeometryPipelineState
                // During deferred rendering Geometry Pass, force use of GeometryPipelineState
                if (bInGeometryPass && pCore->GetGeometryPipelineState())
                {
                    drawable->SetOverridePipelineState(pCore->GetGeometryPipelineState());
                }

                drawable->Draw(commandList);

                if (bInGeometryPass)
                {
                    drawable->SetOverridePipelineState(nullptr);
                }
            }
        }
    }

    /**
     * @brief 设置当前摄像机 / Set current camera
     * @param camera 摄像机指针 / Camera pointer
     */
    void DX12Renderer::SetCamera(Camera* camera)
    {
        pCamera = camera;
    }

    /**
     * @brief 调整渲染目标大小 / Resize the render target
     *
     * 设置待处理的调整大小请求。实际的调整大小操作
     * 会在下一个 BeginFrame 调用时执行，以避免阻塞 WM_SIZE 消息。
     *
     * Sets pending resize request. The actual resize operation
     * will be executed on the next BeginFrame call to avoid
     * blocking the WM_SIZE message.
     *
     * @param width 新宽度 / New width
     * @param height 新高度 / New height
     */
    void DX12Renderer::Resize(int width, int height)
    {
        // 检查是否已初始化
        // Check if initialized
        if (!bInitialized)
            return;

        // 验证尺寸
        // Validate dimensions
        if (width <= 0 || height <= 0)
            return;

        // 设置待处理的调整大小参数
        // Set pending resize parameters
        PendingWidth = width;
        PendingHeight = height;
        bNeedsResize = true;
    }

    /**
     * @brief 执行实际的调整大小 / Execute actual resize
     *
     * 执行实际的窗口调整大小操作，包括：
     * 1. 释放渲染目标和深度模板资源
     * 2. 调用 DX12Core::Resize() 调整交换链
     * 3. 重新创建渲染目标和深度模板
     *
     * 如果调整失败，恢复到原来的尺寸。
     *
     * Executes the actual window resize operation, including:
     * 1. Release render target and depth stencil resources
     * 2. Call DX12Core::Resize() to resize swap chain
     * 3. Recreate render targets and depth stencil
     *
     * If resize fails, restores original dimensions.
     */
    void DX12Renderer::ExecuteResize()
    {
        // 检查是否需要调整大小
        // Check if resize is needed
        if (!bNeedsResize)
            return;

        // 获取待处理的尺寸
        // Get pending dimensions
        int width = PendingWidth;
        int height = PendingHeight;

        // 验证尺寸
        // Validate dimensions
        if (width <= 0 || height <= 0)
        {
            bNeedsResize = false;
            return;
        }

        // 等待 GPU 完成所有操作
        // Wait for GPU to complete all operations
        WaitForGPU();

        // 保存旧尺寸以便失败时恢复
        // Save old dimensions for restoration on failure
        int oldWidth = Width;
        int oldHeight = Height;

        try
        {
            // 1. 在 ResizeBuffers 之前释放所有交换链缓冲区的引用
            // 1. Release all references to swap chain buffers BEFORE ResizeBuffers
            for (int i = 0; i < FRAME_COUNT; i++)
            {
                pRenderTargets[i]->Shutdown();
            }
            pDepthStencil->Shutdown();

            // 2. 现在可以安全地调整大小（内部调用 ResizeBuffers）
            // 2. Now safe to resize (internally calls ResizeBuffers)
            pCore->Resize(width, height);

            // 更新尺寸
            // Update dimensions
            Width = width;
            Height = height;

            // 3. 重新创建资源
            // 3. Recreate resources
            for (int i = 0; i < FRAME_COUNT; i++)
            {
                pRenderTargets[i]->InitializeFromSwapChain(*pCore, i);
            }
            pDepthStencil->Initialize(*pCore, width, height);

            // 调整完成，清除标志
            // Resize complete, clear flag
            bNeedsResize = false;
        }
        catch (const std::exception& e)
        {
            // 调整失败，恢复到原来的尺寸
            // Resize failed, restore original dimensions
            Width = oldWidth;
            Height = oldHeight;
            bNeedsResize = false;
            DX12LogError(("[DX12Renderer::ExecuteResize] FAILED: " + std::string(e.what()) + "\n").c_str());
        }
    }

    /**
     * @brief 设置清除颜色 / Set clear color
     * @param color 清除颜色数组（RGBA）/ Clear color array (RGBA)
     * @throws std::invalid_argument 如果颜色指针为空
     *                                If color pointer is null
     */
    void DX12Renderer::SetClearColor(const float color[4])
    {
        // 验证颜色指针
        // Validate color pointer
        if (!color)
            throw std::invalid_argument("Null color array provided to DX12Renderer::SetClearColor");

        // 复制颜色值
        // Copy color values
        ClearColor[0] = color[0];
        ClearColor[1] = color[1];
        ClearColor[2] = color[2];
        ClearColor[3] = color[3];
    }

    /**
     * @brief 等待 GPU 完成 / Wait for GPU to finish
     *
     * 委托给 DX12Core 的 WaitForGPU 方法。
     * Delegates to DX12Core's WaitForGPU method.
     */
    void DX12Renderer::WaitForGPU()
    {
        if (pCore)
        {
            pCore->WaitForGPU();
        }
    }

    /**
     * @brief 更新场景渲染尺寸 / Update scene render size
     *
     * 设置待处理的场景渲染尺寸变更请求。
     * Sets a pending scene render size change request.
     *
     * @param width 新宽度 / New width
     * @param height 新高度 / New height
     */
    void DX12Renderer::UpdateSceneSize(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;

        if (SceneWidth == width && SceneHeight == height && !bNeedsSceneResize)
            return;

        PendingSceneWidth = width;
        PendingSceneHeight = height;
        bNeedsSceneResize = true;
    }

    /**
     * @brief 执行待处理的场景尺寸调整 / Execute pending scene resize
     *
     * 创建或重建离屏场景渲染目标和深度模板缓冲区。
     * Creates or recreates the off-screen scene render target and depth stencil.
     */
    void DX12Renderer::ExecuteSceneResize()
    {
        if (!bNeedsSceneResize || !pCore)
            return;

        WaitForGPU();

        try
        {
            if (pSceneRenderTarget)
            {
                pSceneRenderTarget->Shutdown();
                pSceneRenderTarget.reset();
            }
            if (pSceneDepthStencil)
            {
                pSceneDepthStencil->Shutdown();
                pSceneDepthStencil.reset();
            }
            if (pGBuffer)
            {
                pGBuffer->Shutdown();
                pGBuffer.reset();
            }

            pSceneRenderTarget = std::make_unique<RenderTargetDX12>();
            pSceneRenderTarget->Initialize(
                *pCore,
                RenderTargetTypeDX12::TextureOutput,
                PendingSceneWidth,
                PendingSceneHeight,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                1, 0);

            pSceneDepthStencil = std::make_unique<DepthStencilDX12>();
            pSceneDepthStencil->Initialize(*pCore, PendingSceneWidth, PendingSceneHeight);

            // 仅在延迟渲染启用时创建 G-Buffer，避免不必要的资源分配
            // Only create G-Buffer when deferred rendering is enabled to avoid unnecessary allocation
            if (bUseDeferredRendering)
            {
                try
                {
                    pGBuffer = std::make_unique<GBuffer>();
                    pGBuffer->Initialize(*pCore, PendingSceneWidth, PendingSceneHeight);
                    DX12LogSuccess("[DX12Renderer::ExecuteSceneResize] G-Buffer created\n");
                }
                catch (const std::exception& e)
                {
                    // GBuffer 创建失败不影响场景 RT 和 DS 的创建
                    // GBuffer creation failure does not affect scene RT and DS creation
                    DX12LogError(("[DX12Renderer::ExecuteSceneResize] G-Buffer creation failed: " +
                                  std::string(e.what()) + "\n").c_str());
                    pGBuffer.reset();
                }
            }

            SceneWidth = PendingSceneWidth;
            SceneHeight = PendingSceneHeight;
            bNeedsSceneResize = false;
            DX12Log(("[DX12Renderer::ExecuteSceneResize] Scene resized to " +
                     std::to_string(SceneWidth) + "x" + std::to_string(SceneHeight) + "\n").c_str());
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12Renderer::ExecuteSceneResize] FAILED: " +
                          std::string(e.what()) + "\n").c_str());
            pSceneRenderTarget.reset();
            pSceneDepthStencil.reset();
            pGBuffer.reset();
            SceneWidth = 0;
            SceneHeight = 0;
            bNeedsSceneResize = false;
        }
    }

    /**
     * @brief 开始渲染到场景纹理 / Begin rendering to scene texture
     *
     * 将渲染目标从交换链后台缓冲区切换到离屏场景渲染目标，
     * 清除颜色和深度，绑定渲染目标和深度模板，设置视口。
     *
     * Switches render target from swap chain back buffer to off-screen
     * scene render target, clears color and depth, binds render target
     * and depth stencil, sets viewport.
     *
     * @param clearColor 清除颜色 / Clear color
     */
    void DX12Renderer::BeginSceneRender(const float clearColor[4])
    {
        if (!bInitialized || !pCore)
            return;

        // 诊断日志：记录 BeginSceneRender 入口状态
        // Diagnostic: log BeginSceneRender entry state
        {
            static int s_frameCounter = 0;
            if (s_frameCounter < 5)
            {
                s_frameCounter++;
                DX12Log(("[DX12Renderer::BeginSceneRender] Frame " + std::to_string(s_frameCounter) +
                         ": deferred=" + std::to_string(bUseDeferredRendering) +
                         " GBuffer=" + std::to_string(pGBuffer != nullptr) +
                         " GBufInit=" + std::to_string(pGBuffer ? pGBuffer->IsInitialized() : false) +
                         " SceneW=" + std::to_string(SceneWidth) +
                         " SceneH=" + std::to_string(SceneHeight) +
                         " GeoPSO=" + std::to_string(pCore->GetGeometryPipelineState() != nullptr) +
                         " GeoPSOInit=" + std::to_string(pCore->GetGeometryPipelineState() ? pCore->GetGeometryPipelineState()->IsInitialized() : false) +
                         "\n").c_str());
            }
        }

        if (!pSceneRenderTarget || !pSceneDepthStencil ||
            !pSceneRenderTarget->GetResource() || !pSceneDepthStencil->GetResource())
        {
            if (PendingSceneWidth > 0 && PendingSceneHeight > 0)
            {
                ExecuteSceneResize();
            }
            else if (SceneWidth > 0 && SceneHeight > 0)
            {
                ExecuteSceneResize();
            }
            else
            {
                return;
            }
        }

        if (!pSceneRenderTarget || !pSceneDepthStencil ||
            !pSceneRenderTarget->GetResource() || !pSceneDepthStencil->GetResource())
            return;

        const float* color = clearColor ? clearColor : ClearColor;

        // 延迟渲染路径：执行 Geometry Pass（写入 G-Buffer）
        // Deferred rendering path: execute Geometry Pass (writes to G-Buffer)
        if (bUseDeferredRendering &&
            pCore->GetGeometryPipelineState() && pCore->GetGeometryPipelineState()->IsInitialized())
        {
            // 按需创建 G-Buffer（首次启用延迟渲染时）
            // Lazily create G-Buffer on first deferred rendering frame
            if (!pGBuffer && SceneWidth > 0 && SceneHeight > 0)
            {
                DX12Log("[DX12Renderer] Attempting lazy G-Buffer creation...\n");
                try
                {
                    pGBuffer = std::make_unique<GBuffer>();
                    pGBuffer->Initialize(*pCore, SceneWidth, SceneHeight);
                    DX12LogSuccess("[DX12Renderer] G-Buffer created lazily for deferred rendering\n");
                }
                catch (const std::exception& e)
                {
                    DX12LogError(("[DX12Renderer] Failed to create G-Buffer lazily: " +
                                  std::string(e.what()) + "\n").c_str());
                    pGBuffer.reset();
                }
            }

            if (pGBuffer && pGBuffer->IsInitialized())
            {
                BeginGeometryPass(color);
                return;
            }
            else
            {
                // GBuffer 不可用，回退到前向渲染
                // GBuffer not available, fall back to forward rendering
                static bool s_bLoggedGBufferFallback = false;
                if (!s_bLoggedGBufferFallback)
                {
                    s_bLoggedGBufferFallback = true;
                    DX12LogWarning("[DX12Renderer] Deferred rendering enabled but GBuffer not available, falling back to forward rendering\n");
                }
            }
        }

        // 前向渲染路径（默认）
        // Forward rendering path (default)
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
            return;

        if (pCore->GetRootSignature() && pCore->GetRootSignature()->GetRootSignature())
        {
            commandList->SetGraphicsRootSignature(
                pCore->GetRootSignature()->GetRootSignature());
        }

        if (pCore->GetPipelineState() && pCore->GetPipelineState()->IsInitialized())
        {
            pCore->GetPipelineState()->Bind(commandList);
        }

        pSceneRenderTarget->TransitionTo(
            commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        pSceneDepthStencil->TransitionTo(
            commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        pSceneRenderTarget->Clear(commandList, color);
        pSceneDepthStencil->Clear(commandList, true, true);

        auto dsvHandle = pSceneDepthStencil->GetDSVHandle();
        pSceneRenderTarget->Bind(commandList, &dsvHandle);

        SetViewportAndScissor(commandList, SceneWidth, SceneHeight);
    }

    /**
     * @brief 结束渲染场景纹理 / End rendering to scene texture
     *
     * 将场景渲染目标转换为像素着色器资源状态以便 ImGui 读取，
     * 恢复交换链后台缓冲区作为渲染目标。
     *
     * Transitions the scene render target to pixel shader resource state
     * for ImGui reading, restores the swap chain back buffer as render target.
     */
    void DX12Renderer::EndSceneRender()
    {
        if (!bInitialized || !pCore || !pSceneRenderTarget)
            return;

        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
            return;

        // 保存延迟渲染状态（EndGeometryPass 会清除 bInGeometryPass）
        // Save deferred rendering state (EndGeometryPass clears bInGeometryPass)
        bool bWasDeferred = bInGeometryPass;

        // 延迟渲染路径：结束 Geometry Pass，执行 Lighting Pass
        // Deferred rendering path: end Geometry Pass, execute Lighting Pass
        if (bInGeometryPass)
        {
            // 诊断日志：验证延迟渲染路径是否被调用
            // Diagnostic: verify deferred rendering path is invoked
            {
                static bool s_bLoggedOnce = false;
                if (!s_bLoggedOnce)
                {
                    s_bLoggedOnce = true;
                    DX12Log("[DX12Renderer::EndSceneRender] Deferred path active: ending Geometry Pass, executing Lighting Pass\n");
                }
            }
            EndGeometryPass();
            ExecuteLightingPass();
        }

        // 延迟渲染路径：保持 scene RT 在 RENDER_TARGET 状态，
        // 以便后续渲染线框对象（前向通道）。
        // 调用者需在渲染完线框对象后调用 FinalizeDeferredSceneRender()。
        // Deferred rendering path: keep scene RT in RENDER_TARGET state
        // for rendering wireframe objects (forward pass).
        // Caller must call FinalizeDeferredSceneRender() after wireframe rendering.
        if (bWasDeferred)
        {
            return;
        }

        // 前向渲染路径：转换到 SRV 并设置后备缓冲区
        // Forward rendering path: transition to SRV and set up back buffer
        RestoreBackBufferAndViewport(commandList);
    }

    /**
     * @brief 完成延迟渲染场景（过渡到 SRV 并设置后备缓冲区）
     *        Finalize deferred scene rendering (transition to SRV and set up back buffer)
     *
     * 在延迟渲染的 Lighting Pass 和线框前向通道完成后调用。
     * 将场景渲染目标过渡到 PIXEL_SHADER_RESOURCE 状态供 ImGui 显示。
     * Called after the deferred rendering Lighting Pass and wireframe forward pass.
     * Transitions the scene render target to PIXEL_SHADER_RESOURCE for ImGui display.
     */
    void DX12Renderer::FinalizeDeferredSceneRender()
    {
        if (!bInitialized || !pCore || !pSceneRenderTarget)
            return;

        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList)
            return;

        RestoreBackBufferAndViewport(commandList);
    }

    /**
     * @brief 获取场景纹理的 GPU SRV 句柄 / Get GPU SRV handle for scene texture
     *
     * 返回场景渲染目标的 GPU 端 SRV 句柄，供 ImGui::Image 使用。
     * Returns the GPU-side SRV handle of the scene render target for ImGui::Image.
     *
     * @return GPU SRV 描述符句柄 / GPU SRV descriptor handle
     */
    D3D12_GPU_DESCRIPTOR_HANDLE DX12Renderer::GetSceneSRVHandle() const noexcept
    {
        if (pSceneRenderTarget && pSceneRenderTarget->HasShaderResourceView())
            return pSceneRenderTarget->GetGPU_SRVHandle();
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }

    // ============================================================================
    // 共享辅助方法 / Shared Helper Methods
    // ============================================================================

    void DX12Renderer::SetViewportAndScissor(ID3D12GraphicsCommandList* commandList, int width, int height)
    {
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(width);
        viewport.Height = static_cast<float>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = width;
        scissorRect.bottom = height;
        commandList->RSSetScissorRects(1, &scissorRect);
    }

    void DX12Renderer::RestoreBackBufferAndViewport(ID3D12GraphicsCommandList* commandList)
    {
        // 将场景渲染目标过渡到 SRV 供 ImGui 显示
        // Transition scene render target to SRV for ImGui display
        pSceneRenderTarget->TransitionTo(
            commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        pSceneDepthStencil->TransitionTo(
            commandList, D3D12_RESOURCE_STATE_COMMON);

        // 设置交换链后台缓冲区为渲染目标
        // Set swap chain back buffer as render target
        UINT backBufferIndex = pCore->GetCurrentBackBufferIndex();
        RenderTargetDX12* rt = pRenderTargets[backBufferIndex].get();
        if (rt)
        {
            rt->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

            if (pDepthStencil)
            {
                pDepthStencil->TransitionTo(
                    commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                auto dsvHandle = pDepthStencil->GetDSVHandle();
                rt->Bind(commandList, &dsvHandle);
            }
            else
            {
                rt->Bind(commandList, nullptr);
            }
        }

        // 恢复视口和裁剪矩形为窗口尺寸
        // Restore viewport and scissor to window dimensions
        SetViewportAndScissor(commandList, Width, Height);
    }

    // ============================================================================
    // 延迟渲染实现 / Deferred Rendering Implementation
    // ============================================================================

    /**
     * @brief 设置光源计数数据（用于延迟渲染 Lighting Pass）
     *
     * 上传光源计数和相机位置到内部常量缓冲区。在延迟渲染 Lighting Pass 中
     * 该缓冲区绑定到根参数 0（b0 寄存器），供 LightingPassPixelShader 读取。
     *
     * Uploads light counts and camera position to an internal constant buffer.
     * During the deferred rendering Lighting Pass, this buffer is bound to root
     * parameter 0 (b0 register) for LightingPassPixelShader to read.
     *
     * @param data 光源计数常量缓冲区数据 / Light count constant buffer data
     */
    void DX12Renderer::SetLightCountData(const DX12LightCountCB& data)
    {
        if (!pCore)
            return;

        // 惰性创建常量缓冲区（首次调用时）
        // Lazily create the constant buffer on first call
        if (!pLightCountBuffer)
        {
            try
            {
                pLightCountBuffer = std::make_unique<ConstantBufferDX12<DX12LightCountCB>>(
                    *pCore, 0, data);  // 根参数 0：b0 / Root param 0: b0
                DX12Log("[DX12Renderer] Light count constant buffer created for deferred rendering\n");
            }
            catch (const std::exception& e)
            {
                DX12LogError(("[DX12Renderer] Failed to create light count buffer: " +
                              std::string(e.what()) + "\n").c_str());
                return;
            }
        }
        else
        {
            pLightCountBuffer->Update(data);
        }
    }

    /**
     * @brief 开始 Geometry Pass（延迟渲染）/ Begin Geometry Pass (deferred rendering)
     *
     * 将渲染目标切换为 G-Buffer 的多渲染目标（MRT），清除 G-Buffer 和深度缓冲区，
     * 绑定根签名和 Geometry Pass 管线状态，设置视口和裁剪矩形。
     *
     * Switches render target to the G-Buffer MRT, clears G-Buffer and depth
     * stencil, binds root signature and Geometry Pass pipeline state, sets
     * viewport and scissor rectangle.
     *
     * @param clearColor 清除颜色（仅用于 G-Buffer Albedo 通道的背景）/ Clear color
     */
    void DX12Renderer::BeginGeometryPass(const float clearColor[4])
    {
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList || !pGBuffer)
            return;

        UNREFERENCED_PARAMETER(clearColor);  // G-Buffer 使用各自的优化清除值

        // 绑定根签名
        // Bind root signature
        if (pCore->GetRootSignature() && pCore->GetRootSignature()->GetRootSignature())
        {
            commandList->SetGraphicsRootSignature(
                pCore->GetRootSignature()->GetRootSignature());
        }

        // 绑定 Geometry Pass 管线状态
        // Bind Geometry Pass pipeline state
        if (pCore->GetGeometryPipelineState() && pCore->GetGeometryPipelineState()->IsInitialized())
        {
            pCore->GetGeometryPipelineState()->Bind(commandList);
        }

        // 设置全局静态覆盖 PSO，确保所有 drawable（包括通过 DemoScene 和
        // MeshRendererSystem 直接渲染的）都使用 Geometry Pass PSO
        // Set global static override PSO to ensure all drawables (including
        // those rendered directly via DemoScene and MeshRendererSystem) use Geometry Pass PSO
        DX12Drawable::SetStaticOverridePipelineState(pCore->GetGeometryPipelineState());

        // 转换 G-Buffer 资源状态：从 PIXEL_SHADER_RESOURCE（或初始）到 RENDER_TARGET
        // Transition G-Buffer resources: from PIXEL_SHADER_RESOURCE (or initial) to RENDER_TARGET
        pGBuffer->TransitionToRTV(commandList);

        // 清除 G-Buffer 各 RT 和深度缓冲区
        // Clear all G-Buffer RTs and depth stencil
        pGBuffer->Clear(commandList);

        // 绑定 G-Buffer 为 MRT（含 DSV）
        // Bind G-Buffer as MRT (with DSV)
        pGBuffer->BindAsMRT(commandList);

        // 设置视口和裁剪矩形为场景尺寸
        // Set viewport and scissor rectangle to scene dimensions
        SetViewportAndScissor(commandList, SceneWidth, SceneHeight);

        // 标记进入 Geometry Pass
        // Mark in-geometry-pass state
        bInGeometryPass = true;

        static bool s_bLoggedOnce = false;
        if (!s_bLoggedOnce)
        {
            s_bLoggedOnce = true;
            DX12Log(("[DX12Renderer] Deferred Geometry Pass active, G-Buffer size=" +
                     std::to_string(SceneWidth) + "x" + std::to_string(SceneHeight) + "\n").c_str());
            D3D12_GPU_DESCRIPTOR_HANDLE base = pGBuffer->GetGBufferSRVTableBase();
            DX12Log(("[DX12Renderer] G-Buffer SRV table base GPU handle = 0x" +
                     std::to_string(base.ptr) + "\n").c_str());
        }
    }

    /**
     * @brief 结束 Geometry Pass / End Geometry Pass
     *
     * 仅清除 Geometry Pass 状态标志。G-Buffer 资源状态转换由
     * ExecuteLightingPass() 完成（转换为 SRV 供 Lighting Pass 读取）。
     *
     * Only clears the Geometry Pass state flag. G-Buffer resource state
     * transitions are performed by ExecuteLightingPass() (transitioning
     * to SRV for the Lighting Pass to read).
     */
    void DX12Renderer::EndGeometryPass()
    {
        bInGeometryPass = false;
        // 清除全局静态覆盖 PSO，恢复前向渲染 PSO
        // Clear global static override PSO, restore forward rendering PSO
        DX12Drawable::SetStaticOverridePipelineState(nullptr);
    }

    /**
     * @brief 执行 Lighting Pass（延迟渲染）/ Execute Lighting Pass (deferred rendering)
     *
     * 读取 G-Buffer 数据并执行 PBR 光照计算，将最终光照结果写入场景渲染目标。
     * 步骤：
     *   1. 转换 G-Buffer RTs 从 RENDER_TARGET 到 PIXEL_SHADER_RESOURCE
     *   2. 转换场景渲染目标到 RENDER_TARGET，清除场景 RT
     *   3. 绑定 Lighting Pass PSO 和根签名
     *   4. 绑定根参数：b0 LightCountCB、t0-t3 G-Buffer SRV 表、t4-t5 光源缓冲区、s0 采样器
     *   5. 绘制全屏三角形（3 个顶点，通过 SV_VertexID 在着色器中生成）
     *
     * Reads G-Buffer data and performs PBR lighting calculations, writing the
     * final lit result to the scene render target.
     * Steps:
     *   1. Transition G-Buffer RTs from RENDER_TARGET to PIXEL_SHADER_RESOURCE
     *   2. Transition scene render target to RENDER_TARGET, clear scene RT
     *   3. Bind Lighting Pass PSO and root signature
     *   4. Bind root parameters: b0 LightCountCB, t0-t3 G-Buffer SRV table,
     *      t4-t5 light buffers, s0 sampler
     *   5. Draw full-screen triangle (3 vertices, generated in shader via SV_VertexID)
     */
    void DX12Renderer::ExecuteLightingPass()
    {
        ID3D12GraphicsCommandList* commandList = pCore->GetCommandList();
        if (!commandList || !pGBuffer || !pSceneRenderTarget)
            return;

        // 诊断日志：验证 Lighting Pass 是否被调用
        // Diagnostic log: verify Lighting Pass is being called
        {
            static bool s_bLoggedOnce = false;
            if (!s_bLoggedOnce)
            {
                s_bLoggedOnce = true;
                DX12Log("[DX12Renderer] ExecuteLightingPass called (first time)\n");
            }
        }

        // ========================================================================
        // Tile-Based Light Culling (Compute Shader) — delegated to DX12LightCullingManager
        // 基于 Tile 的光源剔除（计算着色器）— 委托给 DX12LightCullingManager
        // ========================================================================
        auto* lightCulling = pCore->GetLightCullingManager();
        if (lightCulling && lightCulling->GetRootSignature() && lightCulling->GetPSO())
        {
            // 惰性创建光源剔除资源
            // Lazily create light culling resources
            if (!pCore->IsLightCullingReady())
            {
                pCore->CreateLightCullingResources();
            }

            if (pCore->IsLightCullingReady())
            {
                // 更新光源剔除常量缓冲区（通过管理器写入）
                // Update light culling constant buffer via manager
                UINT pointLightCount = DX12Primitive::GetPointLightCount();
                UINT spotLightCount = DX12Primitive::GetSpotLightCount();

                if (pCamera)
                {
                    LightCullingConstantsCB cullingData = {};
                    cullingData.ScreenWidth = SceneWidth;
                    cullingData.ScreenHeight = SceneHeight;
                    cullingData.PointLightCount = pointLightCount;
                    cullingData.SpotLightCount = spotLightCount;

                    DirectX::XMMATRIX viewMatrix = pCamera->GetMatrix();
                    DirectX::XMMATRIX projMatrix = pCamera->GetProjection();
                    DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(viewMatrix, projMatrix);
                    DirectX::XMMATRIX viewProjT = DirectX::XMMatrixTranspose(viewProj);
                    DirectX::XMStoreFloat4x4(
                        reinterpret_cast<DirectX::XMFLOAT4X4*>(cullingData.ViewProjMatrix),
                        viewProjT);

                    uint8_t* cbPtr = lightCulling->GetConstantBufferCPU();
                    if (cbPtr)
                    {
                        memcpy(cbPtr, &cullingData, sizeof(LightCullingConstantsCB));
                    }
                }

                // Bind compute root signature and PSO (via manager)
                commandList->SetComputeRootSignature(lightCulling->GetRootSignature());
                commandList->SetPipelineState(lightCulling->GetPSO());

                // Bind CBV (b0): LightCullingConstants (via manager)
                D3D12_GPU_VIRTUAL_ADDRESS cbGPU = lightCulling->GetConstantBufferGPU();
                if (cbGPU != 0)
                {
                    commandList->SetComputeRootConstantBufferView(0, cbGPU);
                }

                // Bind SRV descriptor table (t4-t5): PointLight + SpotLight buffers
                DX12DescriptorHeap* cbvSrvHeap = pCore->GetCBVSRVUAVHeap();
                UINT pointLightSRVIndex = DX12Primitive::GetPointLightSRVIndex();
                if (cbvSrvHeap && pointLightSRVIndex != UINT_MAX)
                {
                    commandList->SetComputeRootDescriptorTable(
                        1, cbvSrvHeap->GetGPUHandle(pointLightSRVIndex));
                }

                // Bind UAV descriptor table (u0-u1): LightIndexList + LightCountPerTile (via manager)
                UINT uavIndex = lightCulling->GetLightIndexListUAVIndex();
                if (cbvSrvHeap && uavIndex != UINT_MAX)
                {
                    commandList->SetComputeRootDescriptorTable(
                        2, cbvSrvHeap->GetGPUHandle(uavIndex));
                }

                // Transition buffers from SRV (previous frame) to UAV for compute shader writes
                lightCulling->TransitionToUAV(commandList);

                // Dispatch compute shader
                UINT tilesX = (SceneWidth + TILE_SIZE_X - 1) / TILE_SIZE_X;
                UINT tilesY = (SceneHeight + TILE_SIZE_Y - 1) / TILE_SIZE_Y;
                commandList->Dispatch(tilesX, tilesY, 1);

                // UAV barrier: ensure compute shader writes are visible to pixel shader
                D3D12_RESOURCE_BARRIER uavBarrier = {};
                uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                uavBarrier.UAV.pResource = nullptr; // nullptr means all UAVs
                commandList->ResourceBarrier(1, &uavBarrier);

                // Transition buffers from UAV to SRV for pixel shader reading
                lightCulling->TransitionToSRV(commandList);
            }
        }

        // 步骤1：转换 G-Buffer RTs 从 RENDER_TARGET 到 PIXEL_SHADER_RESOURCE
        // Step 1: Transition G-Buffer RTs from RENDER_TARGET to PIXEL_SHADER_RESOURCE
        pGBuffer->TransitionToSRV(commandList);

        // 步骤2：转换场景渲染目标到 RENDER_TARGET 并清除
        // Step 2: Transition scene render target to RENDER_TARGET and clear
        pSceneRenderTarget->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // 清除场景渲染目标（光照结果将写入此处）
        // Clear scene render target (lighting result will be written here)
        float sceneClear[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        pSceneRenderTarget->Clear(commandList, sceneClear);

        // 绑定场景渲染目标（无 DSV，Lighting Pass 禁用深度测试）
        // Bind scene render target (no DSV, Lighting Pass disables depth testing)
        pSceneRenderTarget->Bind(commandList, nullptr);

        // 设置视口和裁剪矩形
        // Set viewport and scissor rectangle
        SetViewportAndScissor(commandList, SceneWidth, SceneHeight);

        // 步骤3：绑定根签名和 Lighting Pass 管线状态
        // Step 3: Bind root signature and Lighting Pass pipeline state
        if (pCore->GetRootSignature() && pCore->GetRootSignature()->GetRootSignature())
        {
            commandList->SetGraphicsRootSignature(
                pCore->GetRootSignature()->GetRootSignature());
        }

        if (pCore->GetLightingPipelineState() && pCore->GetLightingPipelineState()->IsInitialized())
        {
            pCore->GetLightingPipelineState()->Bind(commandList);
        }

        // 步骤4：绑定根参数
        // Step 4: Bind root parameters

        // 根参数 0：LightCountCB（b0，像素着色器）
        // Root param 0: LightCountCB (b0, pixel shader)
        if (pLightCountBuffer)
        {
            pLightCountBuffer->Bind(commandList);
        }

        // 根参数 3：G-Buffer SRV 描述符表（t0-t3，像素着色器）
        // Root param 3: G-Buffer SRV descriptor table (t0-t3, pixel shader)
        D3D12_GPU_DESCRIPTOR_HANDLE gbufferSRVBase = pGBuffer->GetGBufferSRVTableBase();
        if (gbufferSRVBase.ptr != 0)
        {
            commandList->SetGraphicsRootDescriptorTable(3, gbufferSRVBase);
        }

        // 根参数 4：光源缓冲区描述符表（t4-t5，像素着色器）
        // Root param 4: Light buffer descriptor table (t4-t5, pixel shader)
        DX12DescriptorHeap* cbvSrvHeap = pCore->GetCBVSRVUAVHeap();
        UINT pointLightSRVIndex = DX12Primitive::GetPointLightSRVIndex();
        if (cbvSrvHeap && pointLightSRVIndex != UINT_MAX)
        {
            commandList->SetGraphicsRootDescriptorTable(
                4, cbvSrvHeap->GetGPUHandle(pointLightSRVIndex));
        }

        // 根参数 5：采样器描述符表（s0，像素着色器）
        // Root param 5: Sampler descriptor table (s0, pixel shader)
        DX12DescriptorHeap* samplerHeap = pCore->GetSamplerHeap();
        if (samplerHeap)
        {
            commandList->SetGraphicsRootDescriptorTable(5, samplerHeap->GetGPUHandle(0));
        }

        // 根参数 6：Tile 光源列表 SRV 描述符表（t6-t7，像素着色器）
        // Root param 6: Tile light list SRV descriptor table (t6-t7, pixel shader)
        if (cbvSrvHeap && lightCulling && lightCulling->IsReady())
        {
            D3D12_GPU_DESCRIPTOR_HANDLE lightListHandle = lightCulling->GetLightIndexListSRVHandle(*cbvSrvHeap);
            if (lightListHandle.ptr != 0)
            {
                commandList->SetGraphicsRootDescriptorTable(6, lightListHandle);
            }
        }

        // 步骤5：设置图元拓扑并绘制全屏三角形
        // Step 5: Set primitive topology and draw full-screen triangle
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 全屏三角形：3 个顶点，通过 SV_VertexID 在着色器中生成，无需顶点缓冲区
        // Full-screen triangle: 3 vertices generated in shader via SV_VertexID,
        // no vertex buffer required
        commandList->DrawInstanced(3, 1, 0, 0);
    }
}

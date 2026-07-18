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
        , bInitialized(false)            ///< 未初始化 / Not initialized
        , bInFrame(false)                ///< 不在帧中 / Not in frame
        , bInImGuiFrame(false)           ///< 不在 ImGui 帧中 / Not in ImGui frame
        , hWnd(nullptr)                  ///< 窗口句柄为空 / Window handle is null
        , bNeedsResize(false)            ///< 不需要调整大小 / No resize needed
        , PendingWidth(0)                ///< 待处理宽度为0 / Pending width is 0
        , PendingHeight(0)               ///< 待处理高度为0 / Pending height is 0
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

        // 调用可绘制对象的绘制方法
        // Call draw method of drawable object
        drawable.Draw(commandList);
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
                drawable->Draw(commandList);
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
}

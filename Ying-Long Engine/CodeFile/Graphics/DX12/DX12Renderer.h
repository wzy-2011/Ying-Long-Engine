/**
 * @file DX12Renderer.h
 * @brief DX12 渲染器头文件 / DX12 Renderer Header
 *
 * 本文件定义了 DX12Renderer 类，提供高层的渲染接口，
 * 封装了 DX12Core、渲染目标、深度模板、ImGui 等组件。
 *
 * This file defines the DX12Renderer class, providing a high-level
 * rendering interface that encapsulates DX12Core, render targets,
 * depth stencil, ImGui, and other components.
 */

#pragma once

#include <memory>
#include <vector>
#include "DX12Core.h"
#include "RenderTargetDX12.h"
#include "DepthStencilDX12.h"
#include "DX12PipelineState.h"
#include "DX12Drawable.h"
#include "ImGuiDX12.h"
#include "../Camera/Camera.h"

namespace YingLong
{
    /**
     * @brief DX12 渲染器类 / DX12 Renderer Class
     *
     * DX12Renderer 是渲染系统的高层管理器，负责：
     * - 初始化和管理 DX12 核心
     * - 管理渲染目标和深度模板
     * - 管理 ImGui 集成
     * - 提供帧渲染的开始/结束接口
     * - 提供可绘制对象的渲染接口
     * - 管理摄像机
     *
     * DX12Renderer is the high-level manager of the rendering system, responsible for:
     * - Initializing and managing DX12 core
     * - Managing render targets and depth stencil
     * - Managing ImGui integration
     * - Providing begin/end frame rendering interfaces
     * - Providing rendering interfaces for drawable objects
     * - Managing camera
     */
    class DX12Renderer
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 初始化渲染器的默认状态。
         * Initializes the default state of the renderer.
         */
        DX12Renderer();

        /**
         * @brief 析构函数 / Destructor
         *
         * 自动调用 Shutdown() 释放所有资源。
         * Automatically calls Shutdown() to release all resources.
         */
        ~DX12Renderer();

        /**
         * @brief 初始化渲染器 / Initialize the renderer
         *
         * 按顺序初始化 DX12 核心、渲染目标、深度模板和 ImGui。
         * Sequentially initializes DX12 core, render targets, depth stencil, and ImGui.
         *
         * @param hWnd 窗口句柄 / Window handle
         * @param width 渲染宽度 / Render width
         * @param height 渲染高度 / Render height
         */
        void Initialize(HWND hWnd, int width, int height);

        /**
         * @brief 关闭渲染器 / Shutdown the renderer
         *
         * 按逆序释放所有渲染资源。
         * Releases all rendering resources in reverse order.
         */
        void Shutdown();

        /**
         * @brief 开始渲染帧 / Begin rendering frame
         *
         * 执行待处理的窗口调整，开始帧渲染，
         * 清除渲染目标和深度模板，并绑定渲染目标。
         *
         * Executes pending window resize, begins frame rendering,
         * clears render target and depth stencil, and binds render target.
         *
         * @param clearColor 清除颜色（RGBA），如果为 nullptr 则使用默认清除颜色
         *                   Clear color (RGBA), uses default clear color if nullptr
         */
        void BeginFrame(const float clearColor[4] = nullptr);

        /**
         * @brief 开始 ImGui 帧 / Begin ImGui frame
         *
         * 开始 ImGui 的新帧，准备接收 UI 命令。
         * Begins a new ImGui frame, ready to receive UI commands.
         */
        void BeginImGuiFrame();

        /**
         * @brief 结束 ImGui 帧 / End ImGui frame
         *
         * 结束 ImGui 帧并将 UI 渲染到命令列表。
         * Ends ImGui frame and renders UI to command list.
         */
        void EndImGuiFrame();

        /**
         * @brief 结束渲染帧并呈现 / End rendering frame and present
         *
         * 转换渲染目标状态为呈现状态，结束帧并呈现。
         * Transitions render target state to present state, ends frame and presents.
         */
        void EndFrame();

        /**
         * @brief 绘制一个可绘制对象 / Draw a drawable object
         * @param drawable 可绘制对象引用 / Drawable object reference
         */
        void Draw(DX12Drawable& drawable);

        /**
         * @brief 绘制多个可绘制对象 / Draw multiple drawables
         * @param drawables 可绘制对象指针数组 / Array of drawable object pointers
         */
        void Draw(const std::vector<DX12Drawable*>& drawables);

        /**
         * @brief 设置当前摄像机 / Set current camera
         * @param camera 摄像机指针 / Camera pointer
         */
        void SetCamera(Camera* camera);

        /**
         * @brief 获取当前摄像机 / Get camera
         * @return 摄像机指针 / Camera pointer
         */
        Camera* GetCamera() const noexcept { return pCamera; }

        /**
         * @brief 获取 DX12 核心 / Get core
         * @return DX12Core 指针 / DX12Core pointer
         */
        DX12Core* GetCore() const noexcept { return pCore.get(); }

        /**
         * @brief 获取当前渲染目标 / Get render target
         * @return 当前后台缓冲区对应的渲染目标指针
         *         Render target pointer corresponding to current back buffer
         */
        RenderTargetDX12* GetRenderTarget() const noexcept;

        /**
         * @brief 获取深度模板 / Get depth stencil
         * @return 深度模板指针 / Depth stencil pointer
         */
        DepthStencilDX12* GetDepthStencil() const noexcept { return pDepthStencil.get(); }

        /**
         * @brief 获取 ImGui 对象 / Get ImGui
         * @return ImGuiDX12 指针 / ImGuiDX12 pointer
         */
        ImGuiDX12* GetImGui() const noexcept { return pImGui.get(); }

        /**
         * @brief 调整渲染目标大小 / Resize the render target
         *
         * 设置待处理的调整大小请求，实际调整在下一个 BeginFrame 时执行。
         * Sets pending resize request, actual resize is executed in next BeginFrame.
         *
         * @param width 新宽度 / New width
         * @param height 新高度 / New height
         */
        void Resize(int width, int height);

        /**
         * @brief 获取渲染宽度 / Get width
         * @return 渲染宽度（像素）/ Render width in pixels
         */
        int GetWidth() const noexcept { return Width; }

        /**
         * @brief 获取渲染高度 / Get height
         * @return 渲染高度（像素）/ Render height in pixels
         */
        int GetHeight() const noexcept { return Height; }

        /**
         * @brief 检查是否已初始化 / Check if initialized
         * @return 是否已初始化 / Whether initialized
         */
        bool IsInitialized() const noexcept { return bInitialized; }

        /**
         * @brief 设置清除颜色 / Set clear color
         * @param color 清除颜色数组（RGBA）/ Clear color array (RGBA)
         */
        void SetClearColor(const float color[4]);

        /**
         * @brief 获取清除颜色 / Get clear color
         * @return 清除颜色数组指针（RGBA）
         *         Clear color array pointer (RGBA)
         */
        const float* GetClearColor() const noexcept { return ClearColor; }

        /**
         * @brief 等待 GPU 完成 / Wait for GPU to finish
         *
         * 阻塞 CPU 直到 GPU 完成所有待处理命令。
         * Blocks the CPU until GPU completes all pending commands.
         */
        void WaitForGPU();

    private:
        static constexpr int FRAME_COUNT = 2;    ///< 帧缓冲数量 / Frame buffer count

        // Core components / 核心组件
        std::unique_ptr<DX12Core> pCore;                          ///< DX12 核心对象 / DX12 core object
        std::unique_ptr<RenderTargetDX12> pRenderTargets[FRAME_COUNT];  ///< 渲染目标数组（每帧一个）/ Render target array (one per frame)
        std::unique_ptr<DepthStencilDX12> pDepthStencil;          ///< 深度模板对象 / Depth stencil object
        std::unique_ptr<ImGuiDX12> pImGui;                        ///< ImGui 对象 / ImGui object

        // Camera / 摄像机
        Camera* pCamera;    ///< 当前摄像机指针 / Current camera pointer

        // Dimensions / 尺寸
        int Width;     ///< 渲染宽度（像素）/ Render width in pixels
        int Height;    ///< 渲染高度（像素）/ Render height in pixels

        // Clear color / 清除颜色
        float ClearColor[4];    ///< 帧缓冲清除颜色（RGBA）/ Frame buffer clear color (RGBA)

        // State / 状态
        bool bInitialized;     ///< 是否已初始化标志 / Initialization flag
        bool bInFrame;         ///< 是否在帧中标志 / In-frame flag
        bool bInImGuiFrame;    ///< 是否在 ImGui 帧中标志 / In-ImGui-frame flag
        HWND hWnd;             ///< 窗口句柄 / Window handle

        // Pending resize (executed in BeginFrame to avoid blocking WM_SIZE)
        // 待处理的调整大小（在 BeginFrame 中执行以避免阻塞 WM_SIZE）
        bool bNeedsResize;     ///< 是否需要调整大小标志 / Needs resize flag
        int PendingWidth;      ///< 待处理的宽度 / Pending width
        int PendingHeight;     ///< 待处理的高度 / Pending height

        /**
         * @brief 执行实际的调整大小 / Execute actual resize
         *
         * 从 BeginFrame 中调用，执行实际的窗口调整大小操作。
         * Called from BeginFrame to execute the actual window resize operation.
         */
        void ExecuteResize();
    };
}

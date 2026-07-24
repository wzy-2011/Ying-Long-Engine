/**
 * @file ImGuiDX12.h
 * @brief ImGui DX12 渲染后端类定义 / ImGui DX12 rendering backend class definition
 *
 * 本文件定义了 ImGuiDX12 类，用于在 DX12 中集成 ImGui 调试 UI。
 * 支持双缓冲顶点/索引缓冲区以避免 GPU/CPU 资源争用，
 * 提供自定义字体加载和字体缩放功能。
 *
 * This file defines the ImGuiDX12 class for integrating ImGui debug UI in DX12.
 * Supports double-buffered vertex/index buffers to avoid GPU/CPU resource
 * contention, and provides custom font loading and font scaling functionality.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <array>

namespace YingLong
{
    class DX12Core;

    /// 帧缓冲数量，必须与 DX12Core::FRAME_COUNT 一致
    /// 定义在此处以便每帧资源数组可以使用固定大小的 std::array
    /// FRAME_COUNT must match DX12Core::FRAME_COUNT. Defined here so the
    /// per-frame resource array can be a fixed-size std::array.
    constexpr UINT IMGUI_FRAME_COUNT = 2;

    /**
     * @brief ImGui DX12 渲染后端类 / ImGui DX12 rendering backend class
     *
     * ImGuiDX12 封装了 ImGui 在 DX12 环境下的初始化、渲染和资源管理。
     * 使用双缓冲（每帧独立的顶点/索引缓冲区）来避免 CPU 写入与 GPU 读取
     * 之间的资源争用，从而解决 ImGui 重影/闪烁问题。
     * 支持从 DX11 上下文切换时保存/恢复 DX11 多视口回调。
     *
     * ImGuiDX12 encapsulates ImGui initialization, rendering, and resource
     * management in a DX12 environment. Uses double buffering (per-frame
     * independent vertex/index buffers) to avoid resource contention between
     * CPU writes and GPU reads, thus resolving ImGui ghosting/flickering issues.
     * Supports saving/restoring DX11 multi-viewport callbacks when switching
     * from a DX11 context.
     */
    class ImGuiDX12
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         */
        ImGuiDX12();

        /**
         * @brief 析构函数 / Destructor
         */
        ~ImGuiDX12();

        /**
         * @brief 使用 DX12 初始化 ImGui / Initialize ImGui with DX12
         * @param core DX12Core 引用 / DX12Core reference
         * @param hWnd 窗口句柄 / Window handle
         */
        void Initialize(DX12Core& core, HWND hWnd);

        /**
         * @brief 关闭 ImGui / Shutdown ImGui
         */
        void Shutdown();

        /**
         * @brief 开始 ImGui 帧 / Begin ImGui frame
         */
        void BeginFrame();

        /**
         * @brief 结束 ImGui 帧并渲染 / End ImGui frame and render
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void EndFrame(ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 检查是否已初始化 / Check if initialized
         * @return true 表示已初始化 / true if initialized
         */
        bool IsInitialized() const noexcept { return bInitialized; }

        /**
         * @brief 设置字体缩放 / Set font scale
         * @param scale 缩放比例 / Scale factor
         */
        void SetFontScale(float scale) noexcept { FontScale = scale; }

        /**
         * @brief 获取字体缩放 / Get font scale
         * @return 缩放比例 / Scale factor
         */
        float GetFontScale() const noexcept { return FontScale; }

        /**
         * @brief 加载自定义字体 / Load custom font
         * @param fontPath 字体文件路径 / Font file path
         * @param sizePixels 字体大小（像素） / Font size in pixels
         */
        void LoadFont(const std::string& fontPath, float sizePixels = 16.0f);

        /**
         * @brief 获取默认字体大小 / Get default font size
         * @return 默认字体大小（像素） / Default font size in pixels
         */
        float GetDefaultFontSize() const noexcept { return DefaultFontSize; }

    private:
        /**
         * @brief 创建 ImGui 资源 / Create ImGui resources
         * @param core DX12Core 引用 / DX12Core reference
         */
        void CreateResources(DX12Core& core);

        /**
         * @brief 更新 ImGui 资源（窗口大小改变时调用） / Update ImGui resources (called on resize)
         * @param core DX12Core 引用 / DX12Core reference
         */
        void UpdateResources(DX12Core& core);

        // DX12 资源 / DX12 resources
        Microsoft::WRL::ComPtr<ID3D12Resource> pFontTexture;  ///< 字体纹理资源 / Font texture resource

        /**
         * @brief 每帧资源结构体 / Per-frame resource structure
         *
         * 每帧独立的顶点/索引缓冲区。DX12Core 使用 FRAME_COUNT=2 的双缓冲，
         * 因此每帧需要自己的缓冲区以防止 CPU 覆盖 GPU 仍在读取的缓冲区
         * （这会导致严重的 ImGui 重影/闪烁）。
         *
         * Per-frame vertex/index buffers. DX12Core uses FRAME_COUNT=2 double
         * buffering, so each frame needs its own buffers to prevent the CPU
         * from overwriting a buffer the GPU is still reading (which caused
         * severe ImGui ghosting/flicker).
         */
        struct ImGuiFrameResource
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;  ///< 顶点缓冲区 / Vertex buffer
            Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;   ///< 索引缓冲区 / Index buffer
            UINT VertexBufferSize = 0;                             ///< 顶点缓冲区大小 / Vertex buffer size
            UINT IndexBufferSize = 0;                              ///< 索引缓冲区大小 / Index buffer size
        };
        std::array<ImGuiFrameResource, IMGUI_FRAME_COUNT> FrameResources;  ///< 每帧资源数组 / Per-frame resource array
        UINT CurrentFrameIndex = 0;                                         ///< 当前帧索引 / Current frame index

        // 缓存的指针 / Cached pointers
        ID3D12Device* pDevice;               ///< D3D12 设备指针 / D3D12 device pointer
        ID3D12DescriptorHeap* pSRVHeap;      ///< SRV 描述符堆指针 / SRV descriptor heap pointer
        DX12Core* pCore = nullptr;           ///< DX12Core 指针 / DX12Core pointer

        // 描述符句柄 / Descriptor handles
        D3D12_CPU_DESCRIPTOR_HANDLE FontTextureSRV;       ///< 字体纹理 SRV CPU 句柄 / Font texture SRV CPU handle
        D3D12_GPU_DESCRIPTOR_HANDLE FontTextureSRV_GPU;   ///< 字体纹理 SRV GPU 句柄 / Font texture SRV GPU handle
        UINT FontSRVHeapIndex = UINT_MAX;                 ///< 字体纹理 SRV 堆索引（UINT_MAX 表示未分配）/ Font SRV heap index (UINT_MAX = unallocated)

        // 管线状态 / Pipeline state
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pPSO;          ///< 管线状态对象 / Pipeline state object
        Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;  ///< 根签名 / Root signature

        // 状态 / State
        bool bInitialized;          ///< 初始化标志 / Initialization flag
        bool bCreatedContext;       ///< 是否创建了 ImGui 上下文 / Whether ImGui context was created
        float FontScale;            ///< 字体缩放比例 / Font scale factor
        float DefaultFontSize;      ///< 默认字体大小 / Default font size
        HWND WindowHandle = nullptr; ///< 窗口句柄 / Window handle
    };
}

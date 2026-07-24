/**
 * @file ImGuiDX12.cpp
 * @brief ImGui DX12 渲染后端类实现 / ImGui DX12 rendering backend class implementation
 *
 * 本文件实现了 ImGuiDX12 类的所有方法，包括 ImGui 的初始化、
 * 关闭、帧开始/结束、资源创建、字体纹理上传、管线状态创建等。
 * 实现了与 DX11 上下文切换时的回调保存/恢复机制。
 *
 * This file implements all methods of the ImGuiDX12 class, including ImGui
 * initialization, shutdown, frame begin/end, resource creation, font texture
 * upload, pipeline state creation, and more. Implements callback save/restore
 * mechanism when switching with DX11 context.
 */

#include "ImGuiDX12.h"
#include "DX12Core.h"
#include "DX12DescriptorHeap.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"
#include "DirectXTex.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include "../../Debug/DX12Log.h"
#include <string>

namespace
{
    // ========================================================================
    // 保存的 DX11 多视口回调
    // 进入 DX12 模式时保存，离开时恢复。
    // ImGui_ImplDX11_Init（由 Graphics::Graphics 调用）会在 ImGuiPlatformIO 中
    // 注册 Renderer_CreateWindow / Renderer_DestroyWindow / Renderer_SetWindowSize
    // / Renderer_RenderWindow / Renderer_SwapBuffers。如果保留这些回调，
    // ImGuiDX12::EndFrame 的 RenderPlatformWindowsDefault() 会调用这些
    // DX11 回调，创建会泄漏的 DX11 交换链/设备对象。
    // 之前尝试在此处调用 ImGui_ImplDX11_Shutdown()，但会在启动时导致内存泄漏。
    // 因此我们只将回调设空并在 Shutdown() 中恢复它们。
    // DX11 后端数据保持活跃但处于休眠状态。
    //
    // DX11 renderer multi-viewport callbacks saved when entering DX12 mode and
    // restored when leaving. ImGui_ImplDX11_Init (called by Graphics::Graphics)
    // registers Renderer_CreateWindow / Renderer_DestroyWindow / Renderer_SetWindowSize
    // / Renderer_RenderWindow / Renderer_SwapBuffers in ImGuiPlatformIO. If left
    // in place, ImGuiDX12::EndFrame's RenderPlatformWindowsDefault() would invoke
    // these DX11 callbacks, creating DX11 swap chains/device objects that leak.
    // We previously tried calling ImGui_ImplDX11_Shutdown() here, but that caused
    // a memory leak at startup. Instead we just null the callbacks and restore
    // them in Shutdown(). The DX11 backend data stays alive but dormant.
    // ========================================================================
    struct SavedDX11Callbacks
    {
        void (*Renderer_CreateWindow)(ImGuiViewport*)      = nullptr;      ///< 创建窗口回调 / Create window callback
        void (*Renderer_DestroyWindow)(ImGuiViewport*)     = nullptr;      ///< 销毁窗口回调 / Destroy window callback
        void (*Renderer_SetWindowSize)(ImGuiViewport*, ImVec2) = nullptr;  ///< 设置窗口大小回调 / Set window size callback
        void (*Renderer_RenderWindow)(ImGuiViewport*, void*)  = nullptr;    ///< 渲染窗口回调 / Render window callback
        void (*Renderer_SwapBuffers)(ImGuiViewport*, void*)   = nullptr;    ///< 交换缓冲区回调 / Swap buffers callback
        ImGuiBackendFlags SavedBackendFlags = 0;                             ///< 保存的后端标志 / Saved backend flags
        bool bSaved = false;                                                  ///< 是否已保存标志 / Whether saved flag
    };
    static SavedDX11Callbacks g_SavedDX11;  ///< 全局保存的 DX11 回调 / Global saved DX11 callbacks
}

namespace YingLong
{
    // ========================================================================
    // 构造函数 / Constructor
    // ========================================================================
    ImGuiDX12::ImGuiDX12()
        : FontScale(1.0f)
        , DefaultFontSize(16.0f)
        , pDevice(nullptr)
        , pSRVHeap(nullptr)
        , bInitialized(false)
        , bCreatedContext(false)
    {
        FontTextureSRV.ptr = 0;
        FontTextureSRV_GPU.ptr = 0;
    }

    // ========================================================================
    // 析构函数 / Destructor
    // ========================================================================
    ImGuiDX12::~ImGuiDX12()
    {
        Shutdown();
    }

    // ========================================================================
    // 初始化 ImGui / Initialize ImGui
    // ========================================================================
    void ImGuiDX12::Initialize(DX12Core& core, HWND hWnd)
    {
        WindowHandle = hWnd;
        pCore = &core;

        DX12Log("==================================================\n");
        DX12Log("[ImGuiDX12] Initialize ENTERED\n");
        DX12Log("==================================================\n");

        IMGUI_CHECKVERSION();

        ImGuiIO& io = ImGui::GetIO();
        DX12Log(("[ImGuiDX12] BackendPlatformUserData = " + std::to_string((uintptr_t)io.BackendPlatformUserData) + "\n").c_str());

        // 检查 ImGui 上下文是否已创建（即 DX11 Graphics 构造函数已创建上下文 + Win32 后端 + DX11 后端）
        // Check if ImGui context is already created (i.e. the DX11 Graphics
        // constructor already created the context + Win32 backend + DX11 backend).
        if (io.BackendPlatformUserData != nullptr)
        {
            DX12Log("[ImGuiDX12] ImGui Win32 backend already initialized, skipping context creation\n");
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

            // 保存并清空 DX11 渲染器多视口回调。
            // Graphics::Graphics() 调用了 ImGui_ImplDX11_Init，它注册了
            // Renderer_CreateWindow / Renderer_RenderWindow / Renderer_SwapBuffers
            // / Renderer_DestroyWindow 到 ImGuiPlatformIO。如果保留这些，
            // DX12 模式下的 RenderPlatformWindowsDefault() 会调用这些 DX11
            // 回调，创建会泄漏的 DX11 交换链和设备对象。
            // 之前尝试在此处调用 ImGui_ImplDX11_Shutdown()，但会在启动时导致内存泄漏。
            // 因此，我们只将回调设空并在切回 DX11 时在 Shutdown() 中恢复它们。
            // DX11 后端数据（设备/上下文引用、字体纹理等）保持活跃但休眠 ——
            // 返回 DX11 时无需重新初始化。
            //
            // Save and null out the DX11 renderer multi-viewport callbacks.
            // Graphics::Graphics() called ImGui_ImplDX11_Init which registered
            // Renderer_CreateWindow / Renderer_RenderWindow / Renderer_SwapBuffers
            // / Renderer_DestroyWindow in ImGuiPlatformIO. If left in place,
            // RenderPlatformWindowsDefault() in DX12 mode would invoke these DX11
            // callbacks, creating DX11 swap chains and device objects that leak.
            // We previously tried calling ImGui_ImplDX11_Shutdown() here, but that
            // caused a memory leak at startup. Instead, we just null out the
            // callbacks and restore them in Shutdown() when switching back to DX11.
            // The DX11 backend data (device/context refs, font texture, etc.)
            // remains alive but dormant — no re-init needed when returning to DX11.
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            if (!g_SavedDX11.bSaved)
            {
                DX12Log("[ImGuiDX12] Saving + nulling DX11 renderer multi-viewport callbacks\n");
                g_SavedDX11.Renderer_CreateWindow   = platform_io.Renderer_CreateWindow;
                g_SavedDX11.Renderer_DestroyWindow  = platform_io.Renderer_DestroyWindow;
                g_SavedDX11.Renderer_SetWindowSize  = platform_io.Renderer_SetWindowSize;
                g_SavedDX11.Renderer_RenderWindow   = platform_io.Renderer_RenderWindow;
                g_SavedDX11.Renderer_SwapBuffers    = platform_io.Renderer_SwapBuffers;
                g_SavedDX11.SavedBackendFlags        = io.BackendFlags & ImGuiBackendFlags_RendererHasViewports;
                g_SavedDX11.bSaved                   = true;

                platform_io.Renderer_CreateWindow   = nullptr;
                platform_io.Renderer_DestroyWindow  = nullptr;
                platform_io.Renderer_SetWindowSize  = nullptr;
                platform_io.Renderer_RenderWindow   = nullptr;
                platform_io.Renderer_SwapBuffers    = nullptr;
                io.BackendFlags &= ~ImGuiBackendFlags_RendererHasViewports;
            }

            // 只创建 DX12 资源，不重新初始化上下文
            // Just create DX12 resources, don't reinitialize context
            CreateResources(core);
            bInitialized = true;
            bCreatedContext = false;
            DX12Log("[ImGuiDX12] Initialize DONE (reuse context)\n");
            return;
        }

        DX12Log("[ImGuiDX12] Creating new ImGui context\n");
        // 初始化 ImGui 上下文 / Initialize ImGui context
        ImGui::CreateContext();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // 当启用 ViewportsEnable 时，窗口可以拖到主窗口外。
        // 平台后端期望方形窗口（无圆角）和不透明背景以正确合成。
        // When ViewportsEnable is on, windows may be dragged outside the main
        // window. Platform backends expect square windows (no rounding) and
        // opaque backgrounds for correct compositing.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // 设置字体缩放 / Set font scale
        io.FontGlobalScale = FontScale;

        // 初始化 Win32 后端 / Initialize Win32 backend
        ImGui_ImplWin32_Init(hWnd);

        // 创建 ImGui 资源 / Create ImGui resources
        CreateResources(core);

        bInitialized = true;
        bCreatedContext = true;
        DX12Log("[ImGuiDX12] Initialize DONE (new context)\n");
    }

    // ========================================================================
    // 关闭 ImGui / Shutdown ImGui
    // ========================================================================
    void ImGuiDX12::Shutdown()
    {
        if (!bInitialized)
            return;

        // 关闭 DX12 后端 / Shutdown DX12 backend
        ImGui_ImplDX12_Shutdown();

        // 恢复在 Initialize() 中被设空的 DX11 渲染器多视口回调。
        // 通过 F5 切回 DX11 时必需，这样 Graphics::ClearBuffer/EndFrame
        // 才能再次创建/渲染平台窗口。在应用退出时这是无害的空操作（回调不会再次触发）。
        // Restore DX11 renderer multi-viewport callbacks that were nulled in
        // Initialize(). Necessary when switching back to DX11 via F5 so that
        // Graphics::ClearBuffer/EndFrame can create/render platform windows
        // again. At app exit this is a harmless no-op (callbacks won't fire again).
        if (g_SavedDX11.bSaved)
        {
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            ImGuiIO& io = ImGui::GetIO();
            platform_io.Renderer_CreateWindow   = g_SavedDX11.Renderer_CreateWindow;
            platform_io.Renderer_DestroyWindow  = g_SavedDX11.Renderer_DestroyWindow;
            platform_io.Renderer_SetWindowSize  = g_SavedDX11.Renderer_SetWindowSize;
            platform_io.Renderer_RenderWindow   = g_SavedDX11.Renderer_RenderWindow;
            platform_io.Renderer_SwapBuffers    = g_SavedDX11.Renderer_SwapBuffers;
            io.BackendFlags |= g_SavedDX11.SavedBackendFlags;
            g_SavedDX11 = SavedDX11Callbacks{};
            DX12Log("[ImGuiDX12] Restored DX11 renderer multi-viewport callbacks\n");
        }

        // 只有在我们创建了 Win32 后端和上下文时才关闭它们
        // Only shutdown Win32 backend and destroy context if we created them
        if (bCreatedContext)
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        // 释放字体纹理 SRV 描述符索引 / Free font texture SRV descriptor index
        if (pCore && FontSRVHeapIndex != UINT_MAX)
        {
            pCore->GetCBVSRVUAVHeap()->Free(FontSRVHeapIndex);
            FontSRVHeapIndex = UINT_MAX;
        }

        // 释放 DX12 资源 / Release DX12 resources
        pFontTexture.Reset();
        for (auto& res : FrameResources)
        {
            res.VertexBuffer.Reset();
            res.IndexBuffer.Reset();
            res.VertexBufferSize = 0;
            res.IndexBufferSize = 0;
        }
        pPSO.Reset();
        pRootSignature.Reset();
        pCore = nullptr;

        bInitialized = false;
        bCreatedContext = false;
    }

    // ========================================================================
    // 开始 ImGui 帧 / Begin ImGui frame
    // ========================================================================
    void ImGuiDX12::BeginFrame()
    {
        if (!bInitialized)
        {
            DX12LogError("[ImGuiDX12] BeginFrame called but not initialized!\n");
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        // 始终从窗口句柄同步 DisplaySize，以便立即反映大小变化
        // Always sync DisplaySize from the window handle so resize is reflected immediately
        RECT rect;
        if (WindowHandle && GetClientRect(WindowHandle, &rect) && rect.right > rect.left && rect.bottom > rect.top)
            io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
        else if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0)
            io.DisplaySize = ImVec2(1750.0f, 900.0f);

        // 开始 ImGui 帧 / Start ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    // ========================================================================
    // 结束 ImGui 帧并渲染 / End ImGui frame and render
    // ========================================================================
    void ImGuiDX12::EndFrame(ID3D12GraphicsCommandList* commandList)
    {
        if (!bInitialized)
            return;

        ImGuiIO& io = ImGui::GetIO();

        // 渲染 ImGui 绘制数据 / Render ImGui draw data
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        // 当设置了 ViewportsEnable 时，必须每帧无条件调用 UpdatePlatformWindows，
        // 否则下一个 NewFrame() 会断言失败
        // UpdatePlatformWindows must be called unconditionally every frame
        // when ViewportsEnable is set, or the next NewFrame() will assert
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        if (!draw_data || draw_data->CmdListsCount == 0)
        {
            DX12LogWarning("[ImGuiDX12] EndFrame: no draw data\n");
            return;
        }

        if (!pDevice || !pSRVHeap || !pPSO || !pRootSignature || !commandList)
        {
            static int s_logged = 0;
            if (s_logged < 3)
            {
                s_logged++;
                DX12LogError("[ImGuiDX12] Missing resources:");
                if (!pDevice) DX12LogError(" pDevice");
                if (!pSRVHeap) DX12LogError(" pSRVHeap");
                if (!pPSO) DX12LogError(" pPSO");
                if (!pRootSignature) DX12LogError(" pRootSignature");
                if (!commandList) DX12LogError(" commandList");
                DX12LogError("\n");
            }
            return;
        }

        // 从 DX12Core 同步每帧索引。
        // DX12Core::MoveToNextFrame 已经用围栏等待了此帧之前的 GPU 工作，
        // 因此覆盖此帧的 VB/IB 是安全的（无释放后使用）。
        // 这是 ImGui 重影/闪烁的关键修复：之前单个 VB/IB 在两帧之间共享，
        // CPU 在另一帧的 GPU 仍在读取时覆盖了它。
        //
        // Sync the per-frame index from DX12Core. DX12Core::MoveToNextFrame
        // already fence-stalled on this frame's previous GPU work, so it is
        // safe to overwrite this frame's VB/IB (no use-after-free). This is
        // the key fix for the ImGui ghosting/flicker: previously a single VB/IB
        // was shared across both frames and the CPU overwrote it while the GPU
        // for the other frame was still reading.
        if (pCore)
            CurrentFrameIndex = pCore->GetCurrentBackBufferIndex();
        if (CurrentFrameIndex >= IMGUI_FRAME_COUNT)
            CurrentFrameIndex = 0;
        auto& frameRes = FrameResources[CurrentFrameIndex];

        // DPI 缩放 / Scale for DPI
        draw_data->ScaleClipRects(io.DisplayFramebufferScale);

        // 创建/更新顶点缓冲区 / Create/update vertex buffer
        UINT vertexBufferSize = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        UINT indexBufferSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

        if (!frameRes.VertexBuffer || frameRes.VertexBufferSize < vertexBufferSize)
        {
            // 每帧拥有自己的缓冲区；此帧索引的 GPU 已经在
            // DX12Core::MoveToNextFrame 中被围栏等待过，
            // 因此旧缓冲区可以立即释放（不需要延迟队列）。
            //
            // Each frame owns its own buffer; the GPU for this frame index has
            // already been fence-waited in DX12Core::MoveToNextFrame, so the
            // old buffer can be released immediately (no deferred queue needed).
            if (frameRes.VertexBuffer)
            {
                DX12LogWarning(("[ImGuiDX12] VB REBUILD[frame=" + std::to_string(CurrentFrameIndex)
                    + "]: old=" + std::to_string(frameRes.VertexBufferSize)
                    + " bytes, new=" + std::to_string(vertexBufferSize + 5000)
                    + " bytes (need=" + std::to_string(vertexBufferSize) + ")\n").c_str());
            }
            else
            {
                DX12LogSuccess(("[ImGuiDX12] VB CREATE[frame=" + std::to_string(CurrentFrameIndex)
                    + "]: size=" + std::to_string(vertexBufferSize + 5000) + " bytes\n").c_str());
            }

            frameRes.VertexBufferSize = vertexBufferSize + 5000;

            D3D12_RESOURCE_DESC vbDesc = {};
            vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            vbDesc.Width = frameRes.VertexBufferSize;
            vbDesc.Height = 1;
            vbDesc.DepthOrArraySize = 1;
            vbDesc.MipLevels = 1;
            vbDesc.SampleDesc.Count = 1;
            vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            HRESULT hrVB = pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &vbDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&frameRes.VertexBuffer)
            );
            if (FAILED(hrVB) || !frameRes.VertexBuffer)
            {
                DX12LogError(("[ImGuiDX12] VB CreateCommittedResource FAILED: 0x" + std::to_string(hrVB) + "\n").c_str());
                return;
            }
        }

        if (!frameRes.IndexBuffer || frameRes.IndexBufferSize < indexBufferSize)
        {
            if (frameRes.IndexBuffer)
            {
                DX12LogWarning(("[ImGuiDX12] IB REBUILD[frame=" + std::to_string(CurrentFrameIndex)
                    + "]: old=" + std::to_string(frameRes.IndexBufferSize)
                    + " bytes, new=" + std::to_string(indexBufferSize + 2000)
                    + " bytes (need=" + std::to_string(indexBufferSize) + ")\n").c_str());
            }
            else
            {
                DX12LogSuccess(("[ImGuiDX12] IB CREATE[frame=" + std::to_string(CurrentFrameIndex)
                    + "]: size=" + std::to_string(indexBufferSize + 2000) + " bytes\n").c_str());
            }

            frameRes.IndexBufferSize = indexBufferSize + 2000;

            D3D12_RESOURCE_DESC ibDesc = {};
            ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            ibDesc.Width = frameRes.IndexBufferSize;
            ibDesc.Height = 1;
            ibDesc.DepthOrArraySize = 1;
            ibDesc.MipLevels = 1;
            ibDesc.SampleDesc.Count = 1;
            ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            HRESULT hrIB = pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &ibDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&frameRes.IndexBuffer)
            );
            if (FAILED(hrIB) || !frameRes.IndexBuffer)
            {
                DX12LogError(("[ImGuiDX12] IB CreateCommittedResource FAILED: 0x" + std::to_string(hrIB) + "\n").c_str());
                return;
            }
        }

        if (!frameRes.VertexBuffer || !frameRes.IndexBuffer)
        {
            DX12LogError("[ImGuiDX12] VB or IB is null after create\n");
            return;
        }

        // 上传顶点数据 / Upload vertex data
        ImDrawVert* vtx_dst = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        HRESULT hrMap = frameRes.VertexBuffer->Map(0, &readRange, (void**)&vtx_dst);
        if (FAILED(hrMap) || !vtx_dst)
        {
            DX12LogError(("[ImGuiDX12] VB Map FAILED: 0x" + std::to_string(hrMap) + "\n").c_str());
            return;
        }
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            vtx_dst += cmd_list->VtxBuffer.Size;
        }
        frameRes.VertexBuffer->Unmap(0, nullptr);

        // 上传索引数据 / Upload index data
        ImDrawIdx* idx_dst = nullptr;
        hrMap = frameRes.IndexBuffer->Map(0, &readRange, (void**)&idx_dst);
        if (FAILED(hrMap) || !idx_dst)
        {
            DX12LogError(("[ImGuiDX12] IB Map FAILED: 0x" + std::to_string(hrMap) + "\n").c_str());
            return;
        }
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        frameRes.IndexBuffer->Unmap(0, nullptr);

        // 设置管线状态 / Set pipeline state
        commandList->SetPipelineState(pPSO.Get());
        commandList->SetGraphicsRootSignature(pRootSignature.Get());

        // 设置描述符堆 / Set descriptor heap
        ID3D12DescriptorHeap* heaps[] = { pSRVHeap };
        commandList->SetDescriptorHeaps(1, heaps);

        // 设置视口 / Set viewport
        D3D12_VIEWPORT viewport = {};
        viewport.Width = draw_data->DisplaySize.x;
        viewport.Height = draw_data->DisplaySize.y;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        // 设置投影矩阵为根常量（寄存器 b0）/ Set projection matrix as root constants (register b0)
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f / (R - L),   0.0f,               0.0f,       0.0f },
            { 0.0f,             2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,             0.0f,               0.5f,       0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.5f,       1.0f },
        };
        commandList->SetGraphicsRoot32BitConstants(0, 16, mvp, 0);

        // 设置顶点/索引缓冲区 / Set vertex/index buffers
        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = frameRes.VertexBuffer->GetGPUVirtualAddress();
        vbv.SizeInBytes = vertexBufferSize;
        vbv.StrideInBytes = sizeof(ImDrawVert);
        commandList->IASetVertexBuffers(0, 1, &vbv);

        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = frameRes.IndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = indexBufferSize;
        ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        commandList->IASetIndexBuffer(&ibv);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 渲染绘制命令 / Render draw commands.
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    // 当启用 ViewportsEnable 时，裁剪矩形是绝对屏幕坐标
                    // （draw_data->DisplayPos != 0,0）。
                    // 减去 DisplayPos 转换为相对于渲染目标的坐标用于裁剪矩形。
                    //
                    // Clip rects are in absolute screen coordinates when
                    // ViewportsEnable is on (draw_data->DisplayPos != 0,0).
                    // Subtract DisplayPos to convert to render-target-relative
                    // coordinates for the scissor rect.
                    D3D12_RECT scissor = {
                        (LONG)(pcmd->ClipRect.x - draw_data->DisplayPos.x),
                        (LONG)(pcmd->ClipRect.y - draw_data->DisplayPos.y),
                        (LONG)(pcmd->ClipRect.z - draw_data->DisplayPos.x),
                        (LONG)(pcmd->ClipRect.w - draw_data->DisplayPos.y)
                    };
                    commandList->RSSetScissorRects(1, &scissor);

                    // 根据 ImTextureID 绑定正确的纹理 SRV（根参数索引 1）。
                    // ImGui 文字命令的 TextureId 为 0（未调用 SetTexID），使用字体纹理。
                    // ImGui::Image 命令的 TextureId 为用户指定的 GPU 描述符句柄（如场景渲染目标）。
                    // 之前始终绑定 FontTextureSRV_GPU，导致场景视口显示字体纹理而非 3D 场景。
                    // Bind correct texture SRV based on ImTextureID (root param index 1).
                    // Text commands have TextureId = 0 (SetTexID not called), use font texture.
                    // ImGui::Image commands have TextureId = user-specified GPU descriptor handle
                    // (e.g., scene render target). Previously always bound FontTextureSRV_GPU,
                    // causing the scene viewport to show font texture glyphs instead of 3D scene.
                    ImTextureID texID = pcmd->GetTexID();
                    if (texID)
                    {
                        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
                        gpuHandle.ptr = (UINT64)texID;
                        commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);
                    }
                    else
                    {
                        commandList->SetGraphicsRootDescriptorTable(1, FontTextureSRV_GPU);
                    }

                    // 绘制索引化实例 / Draw indexed instanced
                    commandList->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_vtx_offset += cmd_list->VtxBuffer.Size;
            global_idx_offset += cmd_list->IdxBuffer.Size;
        }
    }

    // ========================================================================
    // 加载自定义字体 / Load custom font
    // ========================================================================
    void ImGuiDX12::LoadFont(const std::string& fontPath, float sizePixels)
    {
        ImGuiIO& io = ImGui::GetIO();
        
        // 清除现有字体 / Clear existing fonts
        io.Fonts->Clear();

        // 添加自定义字体 / Add custom font
        ImFontConfig config;
        config.SizePixels = sizePixels * FontScale;
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), sizePixels * FontScale, &config);

        // 存储默认字体大小 / Store default font size
        DefaultFontSize = sizePixels;
    }

    // ========================================================================
    // 创建 ImGui 资源 / Create ImGui resources
    // ========================================================================
    void ImGuiDX12::CreateResources(DX12Core& core)
    {
        DX12Log("==================================================\n");
        DX12Log("[ImGuiDX12] CreateResources ENTERED\n");
        DX12Log("==================================================\n");

        ID3D12Device* device = core.GetDevice();
        DX12DescriptorHeap* srvHeap = core.GetCBVSRVUAVHeap();
        HRESULT hr;

        if (!device)
        {
            DX12LogError("[ImGuiDX12] FAILED to get D3D12 device\n");
            return;
        }
        DX12LogSuccess("[ImGuiDX12] Got D3D12 device\n");

        if (!srvHeap)
        {
            DX12LogError("[ImGuiDX12] FAILED to get SRV heap\n");
            return;
        }

        DX12LogSuccess("[ImGuiDX12] Got SRV heap\n");

        // 缓存设备和 SRV 堆用于渲染 / Cache device and SRV heap for rendering
        pDevice = device;
        pSRVHeap = srvHeap->GetHeap();

        // 描述并创建根签名
        // RootParam[0]: 投影矩阵的根常量（寄存器 b0，16 个 float = 4x4 矩阵）
        // RootParam[1]: 字体纹理 SRV 的描述符表（寄存器 t0）
        //
        // Describe and create the root signature
        // RootParam[0]: RootConstants for projection matrix (register b0, 16 floats = 4x4 matrix)
        // RootParam[1]: DescriptorTable for font texture SRV (register t0)
        CD3DX12_ROOT_PARAMETER rootParams[2];
        rootParams[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        
        CD3DX12_DESCRIPTOR_RANGE descRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParams[1].InitAsDescriptorTable(1, &descRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(
            2,
            rootParams,
            1, &staticSampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        );

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        D3D12SerializeRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signature,
            &error
        );

        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature));

        // 首先创建字体纹理（不需要 PSO 或着色器）
        // Create font texture first (doesn't need PSO or shaders)
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        if (!pixels || width <= 0 || height <= 0)
        {
            DX12LogError("[ImGuiDX12] FAILED to get font texture data\n");
            return;
        }

        DX12LogSuccess(("[ImGuiDX12] Font texture data OK: " + std::to_string(width) + "x" + std::to_string(height) + "\n").c_str());

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&pFontTexture)
        );

        if (!pFontTexture)
        {
            DX12LogError("[ImGuiDX12] FAILED to create font texture\n");
            return;
        }

        DX12LogSuccess("[ImGuiDX12] Font texture created\n");

        // 上传字体纹理 / Upload font texture
        DX12Log("[ImGuiDX12] Creating upload buffer...\n");
        UINT uploadBufferSize = ((width * 4 + 255) & ~255) * height;
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer;
        
        D3D12_RESOURCE_DESC uploadDesc = {};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Alignment = 0;
        uploadDesc.Width = uploadBufferSize;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.SampleDesc.Quality = 0;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;

        hr = device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pUploadBuffer)
        );
        if (FAILED(hr) || !pUploadBuffer)
        {
            DX12LogError(("[ImGuiDX12] FAILED to create upload buffer: HRESULT 0x" + std::to_string(hr) + "\n").c_str());
            return;
        }
        DX12LogSuccess("[ImGuiDX12] Upload buffer created\n");

        void* mappedData = nullptr;
        hr = pUploadBuffer->Map(0, nullptr, &mappedData);
        if (FAILED(hr) || !mappedData)
        {
            DX12LogError(("[ImGuiDX12] FAILED to map upload buffer: HRESULT 0x" + std::to_string(hr) + "\n").c_str());
            return;
        }
        // 按行复制并对齐行间距（D3D12 要求 256 字节对齐的行）
        // Copy row by row with aligned row pitch (D3D12 requires 256-byte aligned rows)
        UINT rowPitch = (width * 4 + 255) & ~255;
        for (int y = 0; y < height; y++)
        {
            memcpy((BYTE*)mappedData + y * rowPitch, pixels + y * width * 4, width * 4);
        }
        pUploadBuffer->Unmap(0, nullptr);

        // 使用临时命令列表复制到字体纹理 / Copy to font texture using temporary command list
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pTempAllocator;
        hr = device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&pTempAllocator)
        );
        if (FAILED(hr) || !pTempAllocator)
        {
            DX12LogError("[ImGuiDX12] FAILED to create temporary command allocator\n");
            return;
        }
        DX12LogSuccess("[ImGuiDX12] Command allocator created\n");

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pTempCommandList;
        hr = device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            pTempAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&pTempCommandList)
        );
        if (FAILED(hr) || !pTempCommandList)
        {
            DX12LogError("[ImGuiDX12] FAILED to create temporary command list\n");
            return;
        }
        DX12LogSuccess("[ImGuiDX12] Command list created\n");

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = pFontTexture.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = pUploadBuffer.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Width = width;
        src.PlacedFootprint.Footprint.Height = height;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = (width * 4 + 255) & ~255;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Offset = 0;

        pTempCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = pFontTexture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        pTempCommandList->ResourceBarrier(1, &barrier);

        pTempCommandList->Close();

        ID3D12CommandQueue* commandQueue = core.GetCommandQueue();
        DX12Fence* fence = core.GetFence();
        if (commandQueue && fence)
        {
            ID3D12CommandList* ppCommandLists[] = { pTempCommandList.Get() };
            commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

            // 用新值信号围栏并等待 GPU 完成。
            // 初始化时的 core.WaitForGPU() 等待已经到达的围栏值 0，
            // 因此它会立即返回而不实际等待此命令列表。
            //
            // Signal the fence with a new value and wait for the GPU to complete.
            // core.WaitForGPU() at init time waits for fence value 0 which is already reached,
            // so it returns immediately without actually waiting for this command list.
            UINT64 waitValue = fence->Increment();
            fence->Signal(waitValue);
            fence->Wait(waitValue);
        }
        else
        {
            DX12LogError("[ImGuiDX12] No command queue or fence available, texture upload may not complete\n");
        }
        DX12LogSuccess("[ImGuiDX12] Font texture uploaded to GPU\n");

        // 为字体纹理创建 SRV / Create SRV for font texture
        FontSRVHeapIndex = srvHeap->Allocate();
        FontTextureSRV = srvHeap->GetCPUHandle(FontSRVHeapIndex);
        FontTextureSRV_GPU = srvHeap->GetGPUHandle(FontSRVHeapIndex);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(pFontTexture.Get(), &srvDesc, FontTextureSRV);
        DX12LogSuccess("[ImGuiDX12] Font SRV created\n");

        DX12Log("[ImGuiDX12] About to call ImGui_ImplDX12_Init...\n");

        // 初始化 ImGui DX12 后端（编译着色器）/ Initialize ImGui DX12 backend (compiles shaders)
        ImGui_ImplDX12_Init(
            device,
            core.GetFrameCount(),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            srvHeap->GetHeap(),
            FontTextureSRV_GPU,
            FontTextureSRV_GPU
        );

        DX12LogSuccess("[ImGuiDX12] ImGui_ImplDX12_Init OK\n");

        // 创建管线状态（在着色器编译后）/ Create pipeline state (after shaders are compiled)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = pRootSignature.Get();

            const void* vertexShader = nullptr;
            size_t vertexShaderSize = 0;
            const void* pixelShader = nullptr;
            size_t pixelShaderSize = 0;
            ImGui_ImplDX12_GetShaders(&vertexShader, &vertexShaderSize, &pixelShader, &pixelShaderSize);

            Microsoft::WRL::ComPtr<ID3DBlob> pVertexBlob;
            Microsoft::WRL::ComPtr<ID3DBlob> pPixelBlob;

            if (!vertexShader || !pixelShader)
            {
                DX12LogWarning("[ImGuiDX12] GetShaders returned null, using D3DCompile fallback\n");
                const char* imguiVS = R"(
                    cbuffer vertexBuffer : register(b0)
                    {
                        float4x4 ProjectionMatrix;
                    };
                    struct VS_INPUT
                    {
                        float2 pos : POSITION;
                        float2 uv  : TEXCOORD0;
                        float4 col : COLOR0;
                    };
                    struct PS_INPUT
                    {
                        float4 pos : SV_POSITION;
                        float2 uv  : TEXCOORD0;
                        float4 col : COLOR0;
                    };
                    PS_INPUT main(VS_INPUT input)
                    {
                        PS_INPUT output;
                        output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
                        output.uv = input.uv;
                        output.col = input.col;
                        return output;
                    };
                )";

                const char* imguiPS = R"(
                    struct PS_INPUT
                    {
                        float4 pos : SV_POSITION;
                        float2 uv  : TEXCOORD0;
                        float4 col : COLOR0;
                    };
                    sampler sampler0 : register(s0);
                    Texture2D texture0 : register(t0);
                    float4 main(PS_INPUT input) : SV_Target
                    {
                        float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
                        return out_col;
                    };
                )";

                Microsoft::WRL::ComPtr<ID3DBlob> pVSError;
                HRESULT hr = D3DCompile(imguiVS, strlen(imguiVS), "imgui_vs", nullptr, nullptr, "main", "vs_5_0", 0, 0, &pVertexBlob, &pVSError);
                if (FAILED(hr))
                {
                    if (pVSError) DX12LogError(("[ImGuiDX12] VS compile failed: " + std::string((char*)pVSError->GetBufferPointer(), pVSError->GetBufferSize()) + "\n").c_str());
                    else DX12LogError("[ImGuiDX12] VS compile failed with no error message\n");
                    return;
                }

                Microsoft::WRL::ComPtr<ID3DBlob> pPSError;
                hr = D3DCompile(imguiPS, strlen(imguiPS), "imgui_ps", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pPixelBlob, &pPSError);
                if (FAILED(hr))
                {
                    if (pPSError) DX12LogError(("[ImGuiDX12] PS compile failed: " + std::string((char*)pPSError->GetBufferPointer(), pPSError->GetBufferSize()) + "\n").c_str());
                    else DX12LogError("[ImGuiDX12] PS compile failed with no error message\n");
                    return;
                }

                vertexShader = pVertexBlob->GetBufferPointer();
                vertexShaderSize = pVertexBlob->GetBufferSize();
                pixelShader = pPixelBlob->GetBufferPointer();
                pixelShaderSize = pPixelBlob->GetBufferSize();
                DX12LogSuccess("[ImGuiDX12] D3DCompile succeeded (VS + PS)\n");
            }

            psoDesc.VS.pShaderBytecode = vertexShader;
            psoDesc.VS.BytecodeLength = vertexShaderSize;
            psoDesc.PS.pShaderBytecode = pixelShader;
            psoDesc.PS.BytecodeLength = pixelShaderSize;

            const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            // Alpha 通道混合必须使用 ONE/INV_SRC_ALPHA（与标准 ImGui DX11 后端匹配）。
            // 在此处使用 INV_SRC_ALPHA/ZERO 会导致 RT alpha 衰减到 0；如果交换链
            // 用 alpha 合成，ImGui 区域会变得透明并闪烁（切换活动窗口时出现蓝色闪烁，
            // 因为标题栏是蓝色且 alpha 混合会累积）。
            //
            // Alpha channel blend must use ONE/INV_SRC_ALPHA (matching standard ImGui DX11 backend).
            // Using INV_SRC_ALPHA/ZERO here causes RT alpha to decay toward 0; if the swap chain
            // composites with alpha, ImGui regions become transparent and flicker (blue flash
            // when switching active windows, since title bars are blue and alpha-blend accumulates).
            psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = FALSE;

            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleMask = UINT_MAX;

            DX12LogHeader("[ImGuiDX12] BlendState config:\n");
            DX12Log("  SrcBlend       = SRC_ALPHA\n");
            DX12Log("  DestBlend      = INV_SRC_ALPHA\n");
            DX12Log("  SrcBlendAlpha  = ONE\n");
            DX12Log("  DestBlendAlpha = INV_SRC_ALPHA\n");
            DX12Log("  DepthEnable    = FALSE\n");
            DX12Log("  RTVFormat       = R8G8B8A8_UNORM\n");

            HRESULT hrPSO = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPSO));
            if (FAILED(hrPSO) || !pPSO)
            {
                DX12LogError(("[ImGuiDX12] PSO creation FAILED: HRESULT 0x" + std::to_string(hrPSO) + "\n").c_str());
            }
            else
            {
                DX12LogSuccess("[ImGuiDX12] PSO created successfully\n");
            }
        }

        // 存储字体纹理 ID。
        // 必须使用 GPU 描述符句柄（FontTextureSRV_GPU.ptr），而非堆索引。
        // 渲染代码将 ImTextureID 视为 D3D12_GPU_DESCRIPTOR_HANDLE，
        // 若使用堆索引（小整数如 4）会被当作无效 GPU 地址，导致文字渲染读取垃圾数据
        // （表现为视口中出现放大拉伸的奇怪图案）。
        // Store font texture ID. Must use the GPU descriptor handle
        // (FontTextureSRV_GPU.ptr), not the heap index. The render code treats
        // ImTextureID as a D3D12_GPU_DESCRIPTOR_HANDLE; using the heap index
        // (a small integer like 4) would be interpreted as an invalid GPU address,
        // causing text rendering to read garbage (manifesting as enlarged
        // stretched weird patterns in the viewport).
        io.Fonts->TexID = (ImTextureID)FontTextureSRV_GPU.ptr;
    }
}

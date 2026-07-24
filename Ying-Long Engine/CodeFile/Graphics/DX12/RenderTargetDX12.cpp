/**
 * @file RenderTargetDX12.cpp
 * @brief DX12 渲染目标类实现 / DX12 render target class implementation
 *
 * 本文件实现了 RenderTargetDX12 类的所有方法，包括
 * 渲染目标的初始化、关闭、绑定、清除、状态转换、
 * MSAA 解析以及 RTV/SRV 的创建。
 *
 * This file implements all methods of the RenderTargetDX12 class, including
 * render target initialization, shutdown, binding, clearing, state
 * transitions, MSAA resolve, and RTV/SRV creation.
 */

#include "RenderTargetDX12.h"
#include "DX12Core.h"
#include "../../Debug/DX12Log.h"

namespace YingLong
{
    // ========================================================================
    // 构造函数 / Constructor
    // ========================================================================
    RenderTargetDX12::RenderTargetDX12()
        : pCore(nullptr)
        , Width(0)
        , Height(0)
        , Format(DXGI_FORMAT_R8G8B8A8_UNORM)
        , MSAACount(1)
        , MSAAQuality(0)
        , RTVHeapIndex(UINT_MAX)
        , SRVHeapIndex(UINT_MAX)
        , HasSRV(false)
        , bOwnsRTV(false)
        , CurrentState(D3D12_RESOURCE_STATE_COMMON)
    {
        RTVHandle.ptr = 0;
        SRVHandle.ptr = 0;
        GPU_SRVHandle.ptr = 0;
    }

    // ========================================================================
    // 析构函数 / Destructor
    // ========================================================================
    RenderTargetDX12::~RenderTargetDX12()
    {
        Shutdown();
    }

    // ========================================================================
    // 初始化渲染目标 / Initialize render target
    // ========================================================================
    void RenderTargetDX12::Initialize(
        DX12Core& core,
        RenderTargetTypeDX12 type,
        int width,
        int height,
        DXGI_FORMAT format,
        UINT msaaCount,
        UINT msaaQuality)
    {
        pCore = &core;
        Type = type;
        Width = width;
        Height = height;
        Format = format;
        MSAACount = msaaCount;
        MSAAQuality = msaaQuality;
        // TextureOutput/MSAA 类型通过 CreateRTV 动态分配 RTV 索引，拥有所有权
        // TextureOutput/MSAA types allocate RTV index dynamically via CreateRTV, owning it
        bOwnsRTV = true;

        // 后台缓冲区由交换链创建，此处直接返回
        // Back buffer is created by swap chain, return directly here
        if (type == RenderTargetTypeDX12::BackBuffer)
        {
            return;
        }

        // --- 描述纹理资源 / Describe the texture resource ---
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = static_cast<UINT64>(width);
        resourceDesc.Height = static_cast<UINT>(height);
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = format;
        resourceDesc.SampleDesc.Count = msaaCount;
        resourceDesc.SampleDesc.Quality = msaaQuality;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 设置优化清除值（用于更快地清除渲染目标）
        // Set optimized clear value (for faster render target clears)
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        // 创建提交资源（默认堆 + 渲染目标初始状态）
        // Create committed resource (default heap + render target initial state)
        core.GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            IID_PPV_ARGS(&pResource)
        );

        // 创建渲染目标视图
        // Create render target view
        CreateRTV(core);

        // 纹理输出类型需要创建 SRV 以便作为纹理读取
        // TextureOutput type needs SRV for reading as texture
        if (type == RenderTargetTypeDX12::TextureOutput)
        {
            CreateSRV(core);
        }

        CurrentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    // ========================================================================
    // 从交换链缓冲区初始化 / Initialize from swap chain buffer
    // ========================================================================
    void RenderTargetDX12::InitializeFromSwapChain(DX12Core& core, UINT bufferIndex)
    {
        pCore = &core;
        Type = RenderTargetTypeDX12::BackBuffer;

        // 从交换链获取缓冲区资源
        // Get buffer resource from swap chain
        core.GetSwapChain()->GetBuffer(bufferIndex, IID_PPV_ARGS(&pResource));

        // 从核心的 RTV 堆获取 RTV 句柄。
        // 索引 0..FRAME_COUNT-1 由 DX12Core::CreateDescriptorHeaps 预分配，
        // 后台缓冲区不拥有 RTV 索引所有权（bOwnsRTV = false），
        // 因此 Shutdown 不会释放这些索引。
        // Get RTV handle from core's RTV heap.
        // Indices 0..FRAME_COUNT-1 are pre-allocated by DX12Core::CreateDescriptorHeaps.
        // Back buffers do not own the RTV index (bOwnsRTV = false),
        // so Shutdown will not free these indices.
        RTVHandle = core.GetRTVHeap()->GetCPUHandle(bufferIndex);
        RTVHeapIndex = bufferIndex;
        bOwnsRTV = false;

        CurrentState = D3D12_RESOURCE_STATE_COMMON;
    }

    // ========================================================================
    // 关闭渲染目标 / Shutdown render target
    // ========================================================================
    void RenderTargetDX12::Shutdown()
    {
        // 释放 RTV 描述符索引（仅当拥有所有权时，即 TextureOutput/MSAA 类型）。
        // 后台缓冲区的 RTV 索引由 DX12Core 预分配，不在此释放。
        // Release RTV descriptor index (only if owned, i.e., TextureOutput/MSAA types).
        // Back buffer RTV indices are pre-allocated by DX12Core, not freed here.
        if (pCore && bOwnsRTV && RTVHeapIndex != UINT_MAX)
        {
            pCore->GetRTVHeap()->Free(RTVHeapIndex);
        }

        // 释放 SRV 描述符索引（若已分配）
        // Release SRV descriptor index (if allocated)
        if (pCore && SRVHeapIndex != UINT_MAX)
        {
            pCore->GetCBVSRVUAVHeap()->Free(SRVHeapIndex);
        }

        pResource.Reset();
        RTVHandle.ptr = 0;
        SRVHandle.ptr = 0;
        GPU_SRVHandle.ptr = 0;
        Width = 0;
        Height = 0;
        HasSRV = false;
        bOwnsRTV = false;
        RTVHeapIndex = UINT_MAX;
        SRVHeapIndex = UINT_MAX;
        CurrentState = D3D12_RESOURCE_STATE_COMMON;
    }

    // ========================================================================
    // 绑定为渲染目标 / Bind as render target
    // ========================================================================
    void RenderTargetDX12::Bind(::ID3D12GraphicsCommandList* commandList, const ::D3D12_CPU_DESCRIPTOR_HANDLE* dsv)
    {
        // 防御性检查：RTV 句柄必须有效
        // Defensive check: RTV handle must be valid
        if (RTVHandle.ptr == 0)
        {
            DX12LogError("[RenderTargetDX12::Bind] Invalid RTV handle (null), skipping bind\n");
            return;
        }

        // 设置输出合并阶段的渲染目标（可选深度模板视图）
        // Set render targets on the output merger stage (optional depth stencil view)
        commandList->OMSetRenderTargets(1, &RTVHandle, FALSE, dsv);
    }

    // ========================================================================
    // 清除渲染目标 / Clear render target
    // ========================================================================
    void RenderTargetDX12::Clear(::ID3D12GraphicsCommandList* commandList, const float color[4])
    {
        // 防御性检查：RTV 句柄和资源必须有效，避免 SDK 层参数错误导致崩溃
        // Defensive check: RTV handle and resource must be valid to avoid SDK layer
        // parameter errors that cause crashes
        if (RTVHandle.ptr == 0)
        {
            DX12LogError("[RenderTargetDX12::Clear] Invalid RTV handle (null), skipping clear\n");
            return;
        }
        if (!pResource)
        {
            DX12LogError("[RenderTargetDX12::Clear] Invalid resource (null), skipping clear\n");
            return;
        }

        commandList->ClearRenderTargetView(RTVHandle, color, 0, nullptr);
    }

    // ========================================================================
    // 转换资源状态 / Transition resource state
    // ========================================================================
    void RenderTargetDX12::TransitionTo(
        ::ID3D12GraphicsCommandList* commandList,
        ::D3D12_RESOURCE_STATES newState)
    {
        if (!pResource)
            return;

        // 跳过冗余屏障：如果状态未变则不发出屏障
        // Skip redundant barrier: don't emit if state hasn't changed
        // 注意：OnPresented() 在 Present 后将状态追踪同步为 COMMON，
        // 所以 PRESENT→RENDER_TARGET 不会发生，而是 COMMON→RENDER_TARGET。
        // Note: OnPresented() syncs state tracking to COMMON after Present,
        // so PRESENT→RENDER_TARGET doesn't occur; it's COMMON→RENDER_TARGET.
        if (CurrentState == newState)
            return;

        // 创建资源屏障（状态转换）
        // Create resource barrier (state transition)
        ::D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = ::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = ::D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pResource.Get();
        barrier.Transition.StateBefore = CurrentState;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);
        CurrentState = newState;
    }

    // ========================================================================
    // Present 后重置状态追踪 / Reset state tracking after Present
    // ========================================================================
    void RenderTargetDX12::OnPresented()
    {
        // 使用 FLIP_DISCARD 时，DXGI 在 Present 后隐式将后台缓冲区重置为
        // COMMON 状态。同步 CPU 端追踪以避免无效的 PRESENT→RENDER_TARGET 屏障。
        // With FLIP_DISCARD, DXGI implicitly resets back buffers to COMMON
        // state after Present. Sync CPU tracking to avoid invalid barriers.
        CurrentState = D3D12_RESOURCE_STATE_COMMON;
    }

    // ========================================================================
    // MSAA 解析 / MSAA resolve
    // ========================================================================
    void RenderTargetDX12::Resolve(::ID3D12GraphicsCommandList* commandList, RenderTargetDX12& destRenderTarget)
    {
        if (!pResource || !destRenderTarget.GetResource())
            return;

        // 将多重采样渲染目标解析为非多重采样纹理
        // Resolve multi-sampled render target to non-multi-sampled texture
        commandList->ResolveSubresource(
            destRenderTarget.GetResource(),
            0,
            pResource.Get(),
            0,
            Format
        );
    }

    // ========================================================================
    // 创建渲染目标视图 / Create render target view
    // ========================================================================
    void RenderTargetDX12::CreateRTV(DX12Core& core)
    {
        // 从 RTV 堆分配描述符
        // Allocate descriptor from RTV heap
        RTVHeapIndex = core.GetRTVHeap()->Allocate();
        RTVHandle = core.GetRTVHeap()->GetCPUHandle(RTVHeapIndex);

        ::D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = Format;

        // 根据 MSAA 选择视图维度
        // Select view dimension based on MSAA
        if (MSAACount > 1)
        {
            rtvDesc.ViewDimension = ::D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            rtvDesc.ViewDimension = ::D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;
        }

        core.GetDevice()->CreateRenderTargetView(pResource.Get(), &rtvDesc, RTVHandle);
    }

    // ========================================================================
    // 创建着色器资源视图 / Create shader resource view
    // ========================================================================
    void RenderTargetDX12::CreateSRV(DX12Core& core)
    {
        // 从 CBV/SRV/UAV 堆分配描述符
        // Allocate descriptor from CBV/SRV/UAV heap
        SRVHeapIndex = core.GetCBVSRVUAVHeap()->Allocate();
        SRVHandle = core.GetCBVSRVUAVHeap()->GetCPUHandle(SRVHeapIndex);
        GPU_SRVHandle = core.GetCBVSRVUAVHeap()->GetGPUHandle(SRVHeapIndex);

        ::D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = Format;

        // 根据 MSAA 选择视图维度
        // Select view dimension based on MSAA
        if (MSAACount > 1)
        {
            srvDesc.ViewDimension = ::D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            srvDesc.ViewDimension = ::D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        }

        // 使用默认的四分量映射（RGBA -> RGBA）
        // Use default four-component mapping (RGBA -> RGBA)
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        core.GetDevice()->CreateShaderResourceView(pResource.Get(), &srvDesc, SRVHandle);
        HasSRV = true;
    }
}

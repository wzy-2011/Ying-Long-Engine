/**
 * @file DepthStencilDX12.cpp
 * @brief DX12 深度模板缓冲区类实现 / DX12 depth stencil buffer class implementation
 *
 * 本文件实现了 DepthStencilDX12 类的所有方法，包括
 * 深度模板缓冲区的初始化、关闭、绑定、清除、状态转换
 * 以及 DSV/SRV 的创建。
 *
 * This file implements all methods of the DepthStencilDX12 class, including
 * depth stencil buffer initialization, shutdown, binding, clearing, state
 * transitions, and DSV/SRV creation.
 */

#include "DepthStencilDX12.h"
#include "DX12Core.h"

namespace YingLong
{
    // ========================================================================
    // 构造函数 / Constructor
    // ========================================================================
    DepthStencilDX12::DepthStencilDX12()
        : pCore(nullptr)
        , Width(0)
        , Height(0)
        , Format(::DXGI_FORMAT_D24_UNORM_S8_UINT)
        , MSAACount(1)
        , MSAAQuality(0)
        , DSVHeapIndex(UINT_MAX)
        , SRVHeapIndex(UINT_MAX)
        , hasSRV(false)
        , CurrentState(::D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        DSVHandle.ptr = 0;
        SRVHandle.ptr = 0;
        GPU_SRVHandle.ptr = 0;
    }

    // ========================================================================
    // 析构函数 / Destructor
    // ========================================================================
    DepthStencilDX12::~DepthStencilDX12()
    {
        Shutdown();
    }

    // ========================================================================
    // 初始化深度模板缓冲区 / Initialize depth stencil buffer
    // ========================================================================
    void DepthStencilDX12::Initialize(
        DX12Core& core,
        int width,
        int height,
        ::DXGI_FORMAT format,
        UINT msaaCount,
        UINT msaaQuality)
    {
        pCore = &core;
        Width = width;
        Height = height;
        Format = format;
        MSAACount = msaaCount;
        MSAAQuality = msaaQuality;

        // --- 描述深度纹理资源 / Describe the depth texture resource ---
        ::D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = static_cast<UINT64>(width);
        resourceDesc.Height = static_cast<UINT>(height);
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = format;
        resourceDesc.SampleDesc.Count = msaaCount;
        resourceDesc.SampleDesc.Quality = msaaQuality;
        resourceDesc.Layout = ::D3D12_TEXTURE_LAYOUT_UNKNOWN;
        // 必须添加深度模板标志 / Must add depth stencil flag
        resourceDesc.Flags = ::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        ::D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = ::D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 设置优化清除值（用于更快地清除深度模板）
        // Set optimized clear value (for faster depth stencil clears)
        ::D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        // 创建提交资源（默认堆 + 深度写入初始状态）
        // Create committed resource (default heap + depth write initial state)
        core.GetDevice()->CreateCommittedResource(
            &heapProps,
            ::D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            ::D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&pResource)
        );

        CurrentState = ::D3D12_RESOURCE_STATE_DEPTH_WRITE;

        // 创建深度模板视图
        // Create depth stencil view
        CreateDSV(core);
    }

    // ========================================================================
    // 关闭深度模板缓冲区 / Shutdown depth stencil buffer
    // ========================================================================
    void DepthStencilDX12::Shutdown()
    {
        // 释放 DSV 描述符索引（若已分配），使其可被后续分配复用。
        // 这对于支持窗口调整大小至关重要：每次调整都会 Shutdown+Initialize，
        // 若不释放索引会耗尽 DSV 堆（当前容量仅为 4）。
        // Release the DSV descriptor index (if allocated) so it can be reused by
        // subsequent allocations. This is critical for supporting window resize:
        // each resize does Shutdown+Initialize, and not freeing the index would
        // exhaust the DSV heap (current capacity is only 4).
        if (pCore && DSVHeapIndex != UINT_MAX)
        {
            pCore->GetDSVHeap()->Free(DSVHeapIndex);
        }

        // 释放 SRV 描述符索引（若已分配）
        // Release the SRV descriptor index (if allocated)
        if (pCore && SRVHeapIndex != UINT_MAX)
        {
            pCore->GetCBVSRVUAVHeap()->Free(SRVHeapIndex);
        }

        pResource.Reset();
        DSVHandle.ptr = 0;
        SRVHandle.ptr = 0;
        GPU_SRVHandle.ptr = 0;
        Width = 0;
        Height = 0;
        hasSRV = false;
        DSVHeapIndex = UINT_MAX;
        SRVHeapIndex = UINT_MAX;
        CurrentState = ::D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    // ========================================================================
    // 绑定为深度模板 / Bind as depth stencil
    // ========================================================================
    void DepthStencilDX12::Bind(::ID3D12GraphicsCommandList* commandList)
    {
        // 深度模板通常在 OMSetRenderTargets 中与渲染目标一起绑定
        // 此方法为保持接口一致性而保留
        // Depth stencil is typically bound together with render target in OMSetRenderTargets
        // This method is kept for consistency
    }

    // ========================================================================
    // 清除深度模板缓冲区 / Clear depth stencil buffer
    // ========================================================================
    void DepthStencilDX12::Clear(::ID3D12GraphicsCommandList* commandList, bool clearDepth, bool clearStencil)
    {
        ::D3D12_CLEAR_FLAGS clearFlags = (::D3D12_CLEAR_FLAGS)0;
        float depth = 1.0f;
        UINT8 stencil = 0;

        // 根据参数设置清除标志
        // Set clear flags based on parameters
        if (clearDepth)
        {
            clearFlags |= ::D3D12_CLEAR_FLAG_DEPTH;
        }
        if (clearStencil)
        {
            clearFlags |= ::D3D12_CLEAR_FLAG_STENCIL;
        }

        commandList->ClearDepthStencilView(DSVHandle, clearFlags, depth, stencil, 0, nullptr);
    }

    // ========================================================================
    // 转换资源状态 / Transition resource state
    // ========================================================================
    void DepthStencilDX12::TransitionTo(
        ::ID3D12GraphicsCommandList* commandList,
        ::D3D12_RESOURCE_STATES newState)
    {
        if (!pResource)
            return;

        // 跳过冗余屏障：如果状态未变则不发出屏障
        // Skip redundant barrier: don't emit if state hasn't changed
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
    // 创建深度模板视图 / Create depth stencil view
    // ========================================================================
    void DepthStencilDX12::CreateDSV(DX12Core& core)
    {
        // 从 DSV 堆动态分配描述符索引。
        // 关键：不能硬编码为 0，因为存在多个 DepthStencilDX12 实例
        //（主深度模板 + 场景深度模板），硬编码会导致它们共享同一 DSV 描述符，
        // 互相覆盖，引发深度测试使用错误的深度缓冲区。
        // Dynamically allocate descriptor index from DSV heap.
        // Critical: must not hardcode to 0 because multiple DepthStencilDX12
        // instances exist (main depth stencil + scene depth stencil); hardcoding
        // would cause them to share the same DSV descriptor, overwriting each
        // other and causing depth tests to use the wrong depth buffer.
        DSVHeapIndex = core.GetDSVHeap()->Allocate();
        DSVHandle = core.GetDSVHeap()->GetCPUHandle(DSVHeapIndex);

        ::D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = Format;

        // 根据 MSAA 选择视图维度
        // Select view dimension based on MSAA
        if (MSAACount > 1)
        {
            dsvDesc.ViewDimension = ::D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            dsvDesc.ViewDimension = ::D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;
        }

        dsvDesc.Flags = ::D3D12_DSV_FLAG_NONE;

        core.GetDevice()->CreateDepthStencilView(pResource.Get(), &dsvDesc, DSVHandle);
    }

    // ========================================================================
    // 创建着色器资源视图 / Create shader resource view
    // ========================================================================
    void DepthStencilDX12::CreateSRV(DX12Core& core)
    {
        if (!pResource)
            return;

        // 防止重复分配导致描述符泄漏：若已分配 SRV 索引，先释放旧索引
        // Guard against re-allocation causing descriptor leak: if an SRV index
        // was already allocated, free the old one first
        if (SRVHeapIndex != UINT_MAX)
        {
            core.GetCBVSRVUAVHeap()->Free(SRVHeapIndex);
        }

        // 从 CBV/SRV/UAV 堆分配描述符
        // Allocate descriptor from CBV/SRV/UAV heap
        SRVHeapIndex = core.GetCBVSRVUAVHeap()->Allocate();
        SRVHandle = core.GetCBVSRVUAVHeap()->GetCPUHandle(SRVHeapIndex);
        GPU_SRVHandle = core.GetCBVSRVUAVHeap()->GetGPUHandle(SRVHeapIndex);

        ::D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        
        // 为 SRV 选择适当的格式（深度格式与 RTV/DSV 不同，需要类型转换）
        // Choose the appropriate format for SRV (depth formats are different from RTV/DSV,
        // typeless conversion is needed)
        switch (Format)
        {
        case ::DXGI_FORMAT_D32_FLOAT:
            srvDesc.Format = ::DXGI_FORMAT_R32_FLOAT;
            break;
        case ::DXGI_FORMAT_D24_UNORM_S8_UINT:
            srvDesc.Format = ::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            break;
        case ::DXGI_FORMAT_D16_UNORM:
            srvDesc.Format = ::DXGI_FORMAT_R16_UNORM;
            break;
        default:
            srvDesc.Format = ::DXGI_FORMAT_R32_FLOAT;
            break;
        }

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

        // 使用默认的四分量映射
        // Use default four-component mapping
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        core.GetDevice()->CreateShaderResourceView(pResource.Get(), &srvDesc, SRVHandle);
        hasSRV = true;
    }
}

/**
 * @file GBuffer.cpp
 * @brief G-Buffer implementation for Deferred Rendering
 */

#include "GBuffer.h"
#include "DX12Core.h"
#include "RenderTargetDX12.h"
#include "DepthStencilDX12.h"
#include "../../Debug/DX12Log.h"
#include <stdexcept>

namespace YingLong
{
    GBuffer::GBuffer()
        : pCore(nullptr)
        , DSVFormat(DXGI_FORMAT_D32_FLOAT)
        , Width(0)
        , Height(0)
        , bInitialized(false)
    {
        RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;       // Albedo + AO
        RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;   // Normal + Roughness
        RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;   // Position + Metallic
        RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM;       // Emissive (reserved)
    }

    GBuffer::~GBuffer()
    {
        Shutdown();
    }

    void GBuffer::Initialize(DX12Core& core, int width, int height)
    {
        if (bInitialized)
            Shutdown();

        pCore = &core;
        Width = width;
        Height = height;

        DX12Log("[GBuffer] Initializing G-Buffer\n");

        // Create the 4 G-Buffer render targets
        for (UINT i = 0; i < GBUFFER_RT_COUNT; ++i)
        {
            RenderTargets[i] = std::make_unique<RenderTargetDX12>();
            RenderTargets[i]->Initialize(
                core,
                RenderTargetTypeDX12::TextureOutput,
                width, height,
                RTVFormats[i],
                1, 0  // No MSAA
            );
        }

        // Create the G-Buffer depth stencil (D32, no stencil for simplicity)
        pDepthStencil = std::make_unique<DepthStencilDX12>();
        pDepthStencil->Initialize(core, width, height, DXGI_FORMAT_D32_FLOAT);

        bInitialized = true;
        DX12LogSuccess("[GBuffer] G-Buffer initialized successfully\n");

        // 诊断日志：验证 SRV 描述符表是否连续
        // Diagnostic: verify SRV descriptor table contiguity
        {
            DX12Log(("[GBuffer] SRV heap indices: " +
                     std::to_string(RenderTargets[0]->GetSRVHeapIndex()) + ", " +
                     std::to_string(RenderTargets[1]->GetSRVHeapIndex()) + ", " +
                     std::to_string(RenderTargets[2]->GetSRVHeapIndex()) + ", " +
                     std::to_string(RenderTargets[3]->GetSRVHeapIndex()) + "\n").c_str());

            bool bContiguous = true;
            for (UINT i = 1; i < GBUFFER_RT_COUNT; ++i)
            {
                if (RenderTargets[i]->GetSRVHeapIndex() != RenderTargets[i - 1]->GetSRVHeapIndex() + 1)
                {
                    bContiguous = false;
                    break;
                }
            }
            if (!bContiguous)
            {
                DX12LogError("[GBuffer] WARNING: G-Buffer SRV descriptors are NOT contiguous! Lighting Pass will read wrong data!\n");
            }
            else
            {
                DX12Log("[GBuffer] G-Buffer SRV descriptors are contiguous (OK)\n");
            }
        }
    }

    void GBuffer::Shutdown()
    {
        if (!bInitialized)
            return;

        DX12Log("[GBuffer] Shutting down G-Buffer\n");

        // Release depth stencil first (frees DSV descriptor)
        pDepthStencil.reset();

        // Release render targets (frees RTV/SRV descriptors)
        for (UINT i = 0; i < GBUFFER_RT_COUNT; ++i)
        {
            RenderTargets[i].reset();
        }

        pCore = nullptr;
        bInitialized = false;
        DX12Log("[GBuffer] G-Buffer shutdown complete\n");
    }

    void GBuffer::BindAsMRT(ID3D12GraphicsCommandList* commandList)
    {
        if (!bInitialized || !commandList)
            return;

        // Collect all RTV handles
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[GBUFFER_RT_COUNT];
        for (UINT i = 0; i < GBUFFER_RT_COUNT; ++i)
        {
            rtvHandles[i] = RenderTargets[i]->GetRTVHandle();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = pDepthStencil->GetDSVHandle();

        // Bind all RTs + DSV to OM stage
        commandList->OMSetRenderTargets(GBUFFER_RT_COUNT, rtvHandles, FALSE, &dsvHandle);
    }

    void GBuffer::Clear(ID3D12GraphicsCommandList* commandList)
    {
        if (!bInitialized || !commandList)
            return;

        // Clear all RTs
        static const float clearColor[GBUFFER_RT_COUNT][4] = {
            { 0.0f, 0.0f, 0.0f, 1.0f },  // Albedo + AO
            { 0.0f, 0.0f, 0.0f, 0.0f },  // Normal + Roughness (normal=0 means invalid)
            { 0.0f, 0.0f, 0.0f, 0.0f },  // Position + Metallic
            { 0.0f, 0.0f, 0.0f, 0.0f },  // Emissive
        };

        for (UINT i = 0; i < GBUFFER_RT_COUNT; ++i)
        {
            commandList->ClearRenderTargetView(
                RenderTargets[i]->GetRTVHandle(),
                clearColor[i],
                0, nullptr
            );
        }

        // Clear depth (no stencil since D32)
        pDepthStencil->Clear(commandList, true, false);
    }

    void GBuffer::TransitionToSRV(ID3D12GraphicsCommandList* commandList)
    {
        if (!bInitialized || !commandList)
            return;

        for (UINT i = 0; i < GBUFFER_RT_COUNT; ++i)
        {
            RenderTargets[i]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        // Depth stays as depth for now (Lighting pass doesn't need depth SRV unless doing SSAO)
    }

    void GBuffer::TransitionToRTV(ID3D12GraphicsCommandList* commandList)
    {
        if (!bInitialized || !commandList)
            return;

        for (UINT i = 0; i < GBUFFER_RT_COUNT; ++i)
        {
            RenderTargets[i]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        pDepthStencil->TransitionTo(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GBuffer::GetGPU_SRVHandle(UINT index) const noexcept
    {
        if (index < GBUFFER_RT_COUNT && RenderTargets[index])
            return RenderTargets[index]->GetGPU_SRVHandle();
        return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GBuffer::GetGBufferSRVTableBase() const noexcept
    {
        // GBuffer::Initialize creates all 4 RTs in a tight loop, so their SRV
        // descriptors are allocated sequentially from the same heap and are
        // guaranteed to be contiguous. Return RT0's GPU SRV handle as the base.
        if (!bInitialized || !RenderTargets[0])
            return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

        return RenderTargets[0]->GetGPU_SRVHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GBuffer::GetRTVHandle(UINT index) const noexcept
    {
        if (index < GBUFFER_RT_COUNT && RenderTargets[index])
            return RenderTargets[index]->GetRTVHandle();
        return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GBuffer::GetDSVHandle() const noexcept
    {
        if (pDepthStencil)
            return pDepthStencil->GetDSVHandle();
        return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }

    void GBuffer::Resize(DX12Core& core, int width, int height)
    {
        if (!bInitialized)
        {
            Initialize(core, width, height);
            return;
        }

        DX12Log(("[GBuffer] Resizing to " + std::to_string(width) + "x" + std::to_string(height) + "\n").c_str());

        // Release and recreate
        Shutdown();
        Initialize(core, width, height);
    }
}

/**
 * @file GBuffer.h
 * @brief G-Buffer (Geometry Buffer) for Deferred Rendering
 *
 * Encapsulates the multiple render targets used in the Geometry Pass
 * of deferred rendering. Provides MRT binding, clearing, and state
 * transitions for the G-Buffer textures.
 *
 * G-Buffer layout:
 *   RT0: Albedo(RGB) + AO(A)        - R8G8B8A8_UNORM
 *   RT1: Normal(RGB) + Roughness(A) - R16G16B16A16_FLOAT
 *   RT2: Position(RGB) + Metallic(A) - R16G16B16A16_FLOAT
 *   RT3: Emissive(RGB) + Unused(A)  - R8G8B8A8_UNORM (reserved)
 *   Depth: D32_FLOAT
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>
#include <cstdint>

namespace YingLong
{
    class DX12Core;
    class RenderTargetDX12;
    class DepthStencilDX12;

    /// Number of G-Buffer render targets (excluding depth)
    constexpr UINT GBUFFER_RT_COUNT = 4;

    /**
     * @brief G-Buffer for deferred rendering
     *
     * Manages the multi-render-target (MRT) G-Buffer resources used in
     * the Geometry Pass. Each RT stores specific surface attributes
     * that the Lighting Pass reads back to compute final lighting.
     */
    class GBuffer
    {
    public:
        GBuffer();
        ~GBuffer();

        /// Initialize G-Buffer render targets and depth stencil
        void Initialize(DX12Core& core, int width, int height);

        /// Release all G-Buffer resources
        void Shutdown();

        /// Bind all G-Buffer RTs + depth as MRT for writing (Geometry Pass)
        void BindAsMRT(ID3D12GraphicsCommandList* commandList);

        /// Clear all G-Buffer RTs and depth
        void Clear(ID3D12GraphicsCommandList* commandList);

        /// Transition all G-Buffer RTs from RENDER_TARGET to PIXEL_SHADER_RESOURCE
        void TransitionToSRV(ID3D12GraphicsCommandList* commandList);

        /// Transition all G-Buffer RTs from PIXEL_SHADER_RESOURCE to RENDER_TARGET
        void TransitionToRTV(ID3D12GraphicsCommandList* commandList);

        /// Get the GPU descriptor handle for a G-Buffer RT's SRV
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPU_SRVHandle(UINT index) const noexcept;

        /// Get the base GPU descriptor handle for the contiguous G-Buffer SRV table.
        /// The 4 G-Buffer SRVs are allocated contiguously in the CBV/SRV/UAV heap,
        /// so this base handle can be used with SetGraphicsRootDescriptorTable for
        /// root parameter 3 (t0-t3 descriptor table) during the Lighting Pass.
        /// Returns 0 if G-Buffer is not initialized or SRVs are not contiguous.
        D3D12_GPU_DESCRIPTOR_HANDLE GetGBufferSRVTableBase() const noexcept;

        /// Get the CPU descriptor handle for a G-Buffer RT's RTV
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT index) const noexcept;

        /// Get the DSV CPU handle for the G-Buffer's depth stencil
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept;

        /// Get render target formats array (for PSO creation)
        const DXGI_FORMAT* GetRTVFormats() const noexcept { return RTVFormats; }

        /// Get the depth stencil format
        DXGI_FORMAT GetDSVFormat() const noexcept { return DSVFormat; }

        /// Get width
        int GetWidth() const noexcept { return Width; }

        /// Get height
        int GetHeight() const noexcept { return Height; }

        /// Check if initialized
        bool IsInitialized() const noexcept { return bInitialized; }

        /// Resize G-Buffer (releases and recreates resources)
        void Resize(DX12Core& core, int width, int height);

    private:
        DX12Core* pCore;
        std::unique_ptr<RenderTargetDX12> RenderTargets[GBUFFER_RT_COUNT];
        std::unique_ptr<DepthStencilDX12> pDepthStencil;

        DXGI_FORMAT RTVFormats[GBUFFER_RT_COUNT];
        DXGI_FORMAT DSVFormat;

        int Width;
        int Height;
        bool bInitialized;
    };
}

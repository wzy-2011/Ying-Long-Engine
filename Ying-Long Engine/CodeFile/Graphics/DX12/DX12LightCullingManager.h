/**
 * @file DX12LightCullingManager.h
 * @brief DX12 光源剔除管理器 / DX12 Light Culling Manager
 *
 * 管理 Tile-Based Light Culling 所需的资源，包括：
 * - 光源索引列表和 Tile 光源计数缓冲区（UAV/SRV）
 * - 计算根签名和管线状态（PSO）
 * - 常量缓冲区（CBV）
 * - 资源状态转换辅助方法
 *
 * Manages resources for Tile-Based Light Culling, including:
 * - Light index list and per-tile light count buffers (UAV/SRV)
 * - Compute root signature and pipeline state (PSO)
 * - Constant buffer (CBV)
 * - Resource state transition helpers
 */
#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

#include "DX12DescriptorHeap.h"

namespace YingLong
{
    // Forward declarations
    class DX12Core;

    /**
     * @brief 光源剔除管理器 / Light Culling Manager
     *
     * 封装 Tile-Based Light Culling 相关的所有资源管理。
     * Encapsulates all resource management for Tile-Based Light Culling.
     */
    class DX12LightCullingManager
    {
    public:
        DX12LightCullingManager();
        ~DX12LightCullingManager();

        /// @brief 创建计算根签名 / Create compute root signature
        void CreateRootSignature(DX12Core& core);

        /// @brief 创建计算管线状态 / Create compute pipeline state
        void CreateComputePSO(DX12Core& core);

        /// @brief 创建光源剔除缓冲区资源 / Create light culling buffer resources
        void CreateResources(DX12Core& core);

        /// @brief 释放所有资源 / Release all resources
        void Shutdown(DX12Core& core);

        /// @brief 检查资源是否就绪 / Check if resources are ready
        bool IsReady() const noexcept { return bReady; }

        // Accessors / 访问器
        ID3D12RootSignature* GetRootSignature() const noexcept { return RootSig.Get(); }
        ID3D12PipelineState* GetPSO() const noexcept { return PSO.Get(); }

        D3D12_GPU_DESCRIPTOR_HANDLE GetLightIndexListSRVHandle(const DX12DescriptorHeap& heap) const noexcept;
        D3D12_GPU_DESCRIPTOR_HANDLE GetLightCountSRVHandle(const DX12DescriptorHeap& heap) const noexcept;
        UINT GetLightIndexListUAVIndex() const noexcept { return LightIndexListUAVIndex; }
        UINT GetLightCountUAVIndex() const noexcept { return LightCountUAVIndex; }

        /// @brief 获取常量缓冲区 CPU 可写指针 / Get constant buffer CPU writable pointer
        uint8_t* GetConstantBufferCPU() const noexcept { return pConstantBufferCPU; }

        /// @brief 获取常量缓冲区 GPU 地址 / Get constant buffer GPU address
        D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferGPU() const noexcept { return ConstantBufferGPU; }

        /// @brief 过渡缓冲区到 SRV 状态 / Transition buffers to SRV state
        void TransitionToSRV(ID3D12GraphicsCommandList* commandList);

        /// @brief 过渡缓冲区到 UAV 状态 / Transition buffers to UAV state
        void TransitionToUAV(ID3D12GraphicsCommandList* commandList);

    private:
        bool bReady = false;

        // Light culling buffers / 光源剔除缓冲区
        Microsoft::WRL::ComPtr<::ID3D12Resource> pLightIndexListBuffer;
        Microsoft::WRL::ComPtr<::ID3D12Resource> pLightCountPerTileBuffer;

        // SRV/UAV descriptor indices / 描述符索引
        UINT LightIndexListSRVIndex = UINT_MAX;
        UINT LightCountSRVIndex = UINT_MAX;
        UINT LightIndexListUAVIndex = UINT_MAX;
        UINT LightCountUAVIndex = UINT_MAX;

        // Compute pipeline / 计算管线
        Microsoft::WRL::ComPtr<::ID3D12RootSignature> RootSig;
        Microsoft::WRL::ComPtr<::ID3D12PipelineState> PSO;

        // Constant buffer / 常量缓冲区
        Microsoft::WRL::ComPtr<::ID3D12Resource> pConstantBuffer;
        uint8_t* pConstantBufferCPU = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferGPU = {};
        UINT CBVIndex = UINT_MAX;
    };
}
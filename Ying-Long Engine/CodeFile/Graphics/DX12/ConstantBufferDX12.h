/**
 * @file ConstantBufferDX12.h
 * @brief DX12 常量缓冲区模板类定义 / DX12 constant buffer template class definition
 *
 * 本文件定义了 ConstantBufferDX12 模板类，用于创建和管理
 * 可更新的 DX12 常量缓冲区资源，支持上传堆和内存映射更新。
 *
 * 关键设计：使用 FRAME_COUNT 个独立缓冲区（每帧一个），
 * 避免 CPU/GPU 竞态。在双缓冲渲染中，CPU 最多领先 GPU 1 帧，
 * 单缓冲区会导致 GPU 读取时被 CPU 覆写，引发闪烁和残影。
 *
 * This file defines the ConstantBufferDX12 template class for creating and
 * managing updatable DX12 constant buffer resources with upload heap and
 * memory-mapped updates.
 *
 * Key design: Uses FRAME_COUNT independent buffers (one per frame) to avoid
 * CPU/GPU race. In double-buffered rendering, CPU can be up to 1 frame ahead
 * of GPU; a single buffer would be overwritten by CPU while GPU is still
 * reading it, causing flickering and ghosting.
 */

#pragma once

#include "BindableDX12.h"
#include "DX12Core.h"
#include <vector>
#include <stdexcept>
#include <array>

namespace YingLong
{
    /**
     * @brief DX12 常量缓冲区模板类 / DX12 constant buffer template class
     *
     * ConstantBufferDX12 封装了 DX12 常量缓冲区的创建、更新和绑定功能。
     * 它使用上传堆（UPLOAD heap）来存储常量数据，通过内存映射（Map/Unmap）
     * 进行数据更新。缓冲区大小会自动对齐到 256 字节边界（D3D12 常量缓冲区要求）。
     *
     * 每帧使用独立的缓冲区，消除 CPU/GPU 竞态：
     * - Update() 写入当前帧索引对应的缓冲区
     * - Bind() 绑定当前帧索引对应的缓冲区
     * - 当 GPU 执行帧 N 的命令时，CPU 可以安全地写入帧 N+1 的缓冲区
     *
     * ConstantBufferDX12 encapsulates the creation, update, and binding of
     * DX12 constant buffers. It uses an upload heap to store constant data
     * and updates data via memory mapping (Map/Unmap). The buffer size is
     * automatically aligned to 256-byte boundaries as required by D3D12
     * constant buffers.
     *
     * Each frame uses an independent buffer, eliminating CPU/GPU race:
     * - Update() writes to the buffer corresponding to the current frame index
     * - Bind() binds the buffer corresponding to the current frame index
     * - While GPU executes frame N's commands, CPU can safely write to frame N+1's buffer
     *
     * @tparam ConstantType 常量数据结构类型 / Constant data structure type
     */
    template<typename ConstantType>
    class ConstantBufferDX12 : public BindableDX12
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12Core 引用 / DX12Core reference
         * @param rootParameterIndex 根参数索引 / Root parameter index
         * @param initialData 初始常量数据 / Initial constant data
         */
        ConstantBufferDX12(DX12Core& core, UINT rootParameterIndex, const ConstantType& initialData = {})
            : BindableDX12()
            , RootParameterIndex(rootParameterIndex)
        {
            pCore = &core;
            Data = initialData;

            CreateConstantBuffers();
        }

        /**
         * @brief 将常量缓冲区绑定到图形命令列表 / Bind the constant buffer to the graphics command list
         *
         * 绑定当前帧索引对应的缓冲区。必须在 Update() 之后调用，
         * 以确保绑定的是最新写入的缓冲区。
         *
         * Binds the buffer corresponding to the current frame index.
         * Must be called after Update() to ensure the latest written buffer is bound.
         *
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        virtual void Bind(::ID3D12GraphicsCommandList* commandList) override
        {
            if (!pCore)
                return;

            UINT frameIndex = pCore->GetCurrentBackBufferIndex();
            if (frameIndex >= FRAME_COUNT || !pConstantBuffers[frameIndex])
                return;

            commandList->SetGraphicsRootConstantBufferView(
                RootParameterIndex,
                pConstantBuffers[frameIndex]->GetGPUVirtualAddress());
        }

        /**
         * @brief 获取类型名称 / Get the type name
         * @return 类型名称字符串 / Type name string
         */
        virtual const char* GetTypeName() const override { return "ConstantBufferDX12"; }

        /**
         * @brief 更新常量缓冲区数据 / Update constant buffer data
         *
         * 将数据写入当前帧索引对应的缓冲区。由于每帧使用独立缓冲区，
         * CPU 写入时不会影响 GPU 正在读取的上一帧缓冲区。
         *
         * Writes data to the buffer corresponding to the current frame index.
         * Since each frame uses an independent buffer, CPU writes do not
         * affect the previous frame's buffer that GPU may still be reading.
         *
         * @param newData 新的常量数据 / New constant data
         */
        void Update(const ConstantType& newData)
        {
            Data = newData;

            if (!pCore)
                return;

            UINT frameIndex = pCore->GetCurrentBackBufferIndex();
            if (frameIndex >= FRAME_COUNT || !pConstantBuffers[frameIndex])
                return;

            // 设置读取范围为空，表示 CPU 不会读回数据（优化性能）
            // Set read range to empty, indicating CPU won't read back data (performance optimization)
            ::D3D12_RANGE readRange = {};
            readRange.Begin = 0;
            readRange.End = 0;

            void* pData;
            HRESULT hr = pConstantBuffers[frameIndex]->Map(0, &readRange, &pData);
            if (SUCCEEDED(hr) && pData)
            {
                memcpy(pData, &Data, sizeof(ConstantType));
                pConstantBuffers[frameIndex]->Unmap(0, nullptr);
            }
        }

        /**
         * @brief 获取常量缓冲区的 GPU 虚拟地址 / Get the GPU virtual address of the constant buffer
         *
         * 返回当前帧索引对应的缓冲区地址。用于需要在 Bind() 之外
         * 获取地址的场景。
         *
         * Returns the GPU virtual address of the buffer corresponding to
         * the current frame index. Used when the address is needed
         * outside of Bind().
         *
         * @return GPU 虚拟地址 / GPU virtual address
         */
        ::D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept
        {
            if (!pCore)
                return 0;

            UINT frameIndex = pCore->GetCurrentBackBufferIndex();
            if (frameIndex >= FRAME_COUNT || !pConstantBuffers[frameIndex])
                return 0;

            return pConstantBuffers[frameIndex]->GetGPUVirtualAddress();
        }

    private:
        /**
         * @brief 创建所有帧的常量缓冲区资源 / Create constant buffer resources for all frames
         *
         * 为每个帧索引创建一个独立的上传堆常量缓冲区，
         * 并将初始数据复制到所有缓冲区中。
         *
         * Creates an independent upload heap constant buffer for each
         * frame index, and copies initial data into all buffers.
         */
        void CreateConstantBuffers()
        {
            auto device = pCore->GetDevice();
            if (!device)
            {
                throw std::runtime_error("Null device in ConstantBufferDX12::CreateConstantBuffers");
            }

            // 计算缓冲区大小并对齐到 256 字节（D3D12 常量缓冲区对齐要求）
            // Calculate buffer size and align to 256 bytes (D3D12 constant buffer alignment requirement)
            UINT bufferSize = sizeof(ConstantType);
            bufferSize = (bufferSize + 255) & ~255;

            ::D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = ::D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            ::D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = bufferSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = ::DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = ::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = ::D3D12_RESOURCE_FLAG_NONE;

            // 为每个帧创建独立的缓冲区，避免 CPU/GPU 竞态
            // Create an independent buffer for each frame to avoid CPU/GPU race
            for (UINT i = 0; i < FRAME_COUNT; ++i)
            {
                HRESULT hr = device->CreateCommittedResource(
                    &heapProps,
                    ::D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    ::D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&pConstantBuffers[i])
                );
                if (FAILED(hr) || !pConstantBuffers[i])
                {
                    throw std::runtime_error("Failed to create constant buffer for frame");
                }

                // 将初始数据映射并复制到缓冲区
                // Map and copy initial data to the buffer
                ::D3D12_RANGE readRange = {};
                readRange.Begin = 0;
                readRange.End = 0;

                void* pData;
                hr = pConstantBuffers[i]->Map(0, &readRange, &pData);
                if (SUCCEEDED(hr) && pData)
                {
                    memcpy(pData, &Data, sizeof(ConstantType));
                    pConstantBuffers[i]->Unmap(0, nullptr);
                }
            }

            bInitialized = true;
        }

        std::array<Microsoft::WRL::ComPtr<::ID3D12Resource>, FRAME_COUNT> pConstantBuffers;  ///< 每帧的常量缓冲区资源 / Per-frame constant buffer resources
        ConstantType Data;                                                                    ///< 常量数据副本 / Constant data copy
        UINT RootParameterIndex;                                                             ///< 根参数索引 / Root parameter index
    };
}

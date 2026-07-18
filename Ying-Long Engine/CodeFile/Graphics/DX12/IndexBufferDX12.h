/**
 * @file IndexBufferDX12.h
 * @brief DX12 索引缓冲区模板类定义 / DX12 index buffer template class definition
 *
 * 本文件定义了 IndexBufferDX12 模板类，用于创建和管理
 * DX12 索引缓冲区资源，支持从索引数据向量创建缓冲区并
 * 通过上传堆将数据传输到 GPU 默认堆。自动根据索引类型
 * 选择 R16_UINT 或 R32_UINT 格式。
 *
 * This file defines the IndexBufferDX12 template class for creating and
 * managing DX12 index buffer resources. It supports creating buffers from
 * index data vectors and transferring data to the GPU default heap via an
 * upload heap. Automatically selects R16_UINT or R32_UINT format based on
 * the index type.
 */

#pragma once

#include "BindableDX12.h"
#include "DX12Core.h"
#include <vector>
#include <d3d12.h>
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief DX12 索引缓冲区模板类 / DX12 index buffer template class
     *
     * IndexBufferDX12 封装了 DX12 索引缓冲区的创建和绑定功能。
     * 它使用默认堆（DEFAULT heap）存储索引数据以获得最佳 GPU 访问性能，
     * 并通过上传堆（UPLOAD heap）和临时命令列表将数据从 CPU 传输到 GPU。
     * 自动根据索引类型大小选择 DXGI_FORMAT_R16_UINT（16位）或
     * DXGI_FORMAT_R32_UINT（32位）格式。
     *
     * IndexBufferDX12 encapsulates the creation and binding of DX12 index
     * buffers. It uses a default heap for storing index data for optimal GPU
     * access performance, and transfers data from CPU to GPU via an upload heap
     * and temporary command list. Automatically selects DXGI_FORMAT_R16_UINT
     * (16-bit) or DXGI_FORMAT_R32_UINT (32-bit) format based on the index type size.
     *
     * @tparam IndexType 索引数据类型（通常为 uint16_t 或 uint32_t） / Index data type (typically uint16_t or uint32_t)
     */
    template<typename IndexType>
    class IndexBufferDX12 : public BindableDX12
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12Core 引用 / DX12Core reference
         * @param indices 索引数据向量 / Index data vector
         */
        IndexBufferDX12(DX12Core& core, const std::vector<IndexType>& indices)
            : BindableDX12()
        {
            pCore = &core;
            IndexCount = static_cast<UINT>(indices.size());
            // 根据索引类型大小自动选择格式 / Auto-select format based on index type size
            Format = (sizeof(IndexType) == 2) ? ::DXGI_FORMAT_R16_UINT : ::DXGI_FORMAT_R32_UINT;
            CreateIndexBuffer(indices);
        }

        /**
         * @brief 将索引缓冲区绑定到输入装配阶段 / Bind the index buffer to the input assembler stage
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        virtual void Bind(::ID3D12GraphicsCommandList* commandList) override
        {
            commandList->IASetIndexBuffer(&View);
        }

        /**
         * @brief 获取类型名称 / Get the type name
         * @return 类型名称字符串 / Type name string
         */
        virtual const char* GetTypeName() const override { return "IndexBufferDX12"; }

        /**
         * @brief 获取索引数量 / Get the index count
         * @return 索引数量 / Index count
         */
        UINT GetIndexCount() const noexcept { return IndexCount; }

        /**
         * @brief 获取索引格式 / Get the index format
         * @return DXGI_FORMAT 枚举值 / DXGI_FORMAT enum value
         */
        ::DXGI_FORMAT GetFormat() const noexcept { return Format; }

    private:
        /**
         * @brief 创建索引缓冲区资源 / Create the index buffer resource
         *
         * 创建默认堆索引缓冲区，并通过上传堆和临时命令列表
         * 将索引数据从 CPU 复制到 GPU。完成后使用围栏同步。
         *
         * Creates a default heap index buffer and copies index data from CPU
         * to GPU via an upload heap and temporary command list. Synchronizes
         * using a fence after completion.
         *
         * @param indices 索引数据向量 / Index data vector
         */
        void CreateIndexBuffer(const std::vector<IndexType>& indices)
        {
            auto device = pCore->GetDevice();
            UINT bufferSize = sizeof(IndexType) * indices.size();

            // --- 创建默认堆索引缓冲区 / Create default heap index buffer ---
            ::D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = ::D3D12_HEAP_TYPE_DEFAULT;
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

            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                ::D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                ::D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&pIndexBuffer)
            );
            if (FAILED(hr) || !pIndexBuffer)
            {
                throw std::runtime_error("Failed to create index buffer");
            }

            // --- 创建上传堆缓冲区 / Create upload heap buffer ---
            ::D3D12_HEAP_PROPERTIES uploadHeapProps = {};
            uploadHeapProps.Type = ::D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapProps.CreationNodeMask = 1;
            uploadHeapProps.VisibleNodeMask = 1;

            ::D3D12_RESOURCE_DESC uploadDesc = {};
            uploadDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadDesc.Alignment = 0;
            uploadDesc.Width = bufferSize;
            uploadDesc.Height = 1;
            uploadDesc.DepthOrArraySize = 1;
            uploadDesc.MipLevels = 1;
            uploadDesc.Format = ::DXGI_FORMAT_UNKNOWN;
            uploadDesc.SampleDesc.Count = 1;
            uploadDesc.SampleDesc.Quality = 0;
            uploadDesc.Layout = ::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadDesc.Flags = ::D3D12_RESOURCE_FLAG_NONE;

            Microsoft::WRL::ComPtr<::ID3D12Resource> pUploadBuffer;
            hr = device->CreateCommittedResource(
                &uploadHeapProps,
                ::D3D12_HEAP_FLAG_NONE,
                &uploadDesc,
                ::D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pUploadBuffer)
            );
            if (FAILED(hr) || !pUploadBuffer)
            {
                throw std::runtime_error("Failed to create upload buffer for index data");
            }

            // --- 将索引数据映射并复制到上传堆 / Map and copy index data to upload heap ---
            void* pData;
            hr = pUploadBuffer->Map(0, nullptr, &pData);
            if (FAILED(hr) || !pData)
            {
                throw std::runtime_error("Failed to map upload buffer");
            }
            memcpy(pData, indices.data(), bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // --- 创建临时命令分配器和命令列表 / Create temporary command allocator and command list ---
            Microsoft::WRL::ComPtr<::ID3D12CommandAllocator> pTempAllocator;
            hr = device->CreateCommandAllocator(
                ::D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&pTempAllocator)
            );
            if (FAILED(hr) || !pTempAllocator)
            {
                throw std::runtime_error("Failed to create temporary command allocator");
            }

            Microsoft::WRL::ComPtr<::ID3D12GraphicsCommandList> pTempCommandList;
            hr = device->CreateCommandList(
                0,
                ::D3D12_COMMAND_LIST_TYPE_DIRECT,
                pTempAllocator.Get(),
                nullptr,
                IID_PPV_ARGS(&pTempCommandList)
            );
            if (FAILED(hr) || !pTempCommandList)
            {
                throw std::runtime_error("Failed to create temporary command list");
            }

            // --- 执行复制操作并转换资源状态 / Execute copy operation and transition resource state ---
            pTempCommandList->CopyResource(pIndexBuffer.Get(), pUploadBuffer.Get());

            ::D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = ::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = pIndexBuffer.Get();
            barrier.Transition.StateBefore = ::D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = ::D3D12_RESOURCE_STATE_INDEX_BUFFER;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            pTempCommandList->ResourceBarrier(1, &barrier);

            hr = pTempCommandList->Close();
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to close temporary command list");
            }

            // --- 执行命令列表并等待 GPU 完成 / Execute command list and wait for GPU completion ---
            auto commandQueue = pCore->GetCommandQueue();
            ::ID3D12CommandList* commandLists[] = { pTempCommandList.Get() };
            commandQueue->ExecuteCommandLists(1, commandLists);

            // 创建围栏用于同步 / Create fence for synchronization
            Microsoft::WRL::ComPtr<::ID3D12Fence> pFence;
            hr = device->CreateFence(0, ::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
            if (FAILED(hr) || !pFence)
            {
                throw std::runtime_error("Failed to create fence");
            }

            HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!eventHandle)
            {
                throw std::runtime_error("Failed to create fence event");
            }

            commandQueue->Signal(pFence.Get(), 1);
            pFence->SetEventOnCompletion(1, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);

            // --- 设置索引缓冲区视图 / Set up index buffer view ---
            View.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
            View.SizeInBytes = bufferSize;
            View.Format = Format;
        }

        Microsoft::WRL::ComPtr<::ID3D12Resource> pIndexBuffer;  ///< 索引缓冲区资源指针 / Index buffer resource pointer
        ::D3D12_INDEX_BUFFER_VIEW View = {};                    ///< 索引缓冲区视图 / Index buffer view
        UINT IndexCount = 0;                                     ///< 索引数量 / Index count
        ::DXGI_FORMAT Format = ::DXGI_FORMAT_UNKNOWN;            ///< 索引格式 / Index format
    };
}

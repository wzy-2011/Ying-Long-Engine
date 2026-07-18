/**
 * @file VertexBufferDX12.h
 * @brief DX12 顶点缓冲区模板类定义 / DX12 vertex buffer template class definition
 *
 * 本文件定义了 VertexBufferDX12 模板类，用于创建和管理
 * DX12 顶点缓冲区资源，支持从顶点数据向量创建缓冲区并
 * 通过上传堆将数据传输到 GPU 默认堆。
 *
 * This file defines the VertexBufferDX12 template class for creating and
 * managing DX12 vertex buffer resources. It supports creating buffers from
 * vertex data vectors and transferring data to the GPU default heap via
 * an upload heap.
 */

#pragma once

#include "BindableDX12.h"
#include "DX12Core.h"
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
#include <stdexcept>

using namespace DirectX;

namespace YingLong
{
    /**
     * @brief DX12 顶点缓冲区模板类 / DX12 vertex buffer template class
     *
     * VertexBufferDX12 封装了 DX12 顶点缓冲区的创建和绑定功能。
     * 它使用默认堆（DEFAULT heap）存储顶点数据以获得最佳 GPU 访问性能，
     * 并通过上传堆（UPLOAD heap）和临时命令列表将数据从 CPU 传输到 GPU。
     * 数据传输完成后，使用围栏（fence）同步等待 GPU 操作完成。
     *
     * VertexBufferDX12 encapsulates the creation and binding of DX12 vertex
     * buffers. It uses a default heap for storing vertex data for optimal GPU
     * access performance, and transfers data from CPU to GPU via an upload heap
     * and temporary command list. After data transfer completes, a fence is used
     * to synchronize and wait for GPU operations to finish.
     *
     * @tparam VertexType 顶点数据结构类型 / Vertex data structure type
     */
    template<typename VertexType>
    class VertexBufferDX12 : public BindableDX12
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12Core 引用 / DX12Core reference
         * @param vertices 顶点数据向量 / Vertex data vector
         */
        VertexBufferDX12(DX12Core& core, const std::vector<VertexType>& vertices)
            : BindableDX12()
        {
            pCore = &core;
            VertexCount = static_cast<UINT>(vertices.size());
            StrideSize = sizeof(VertexType);
            CreateVertexBuffer(vertices);
        }

        /**
         * @brief 将顶点缓冲区绑定到输入装配阶段 / Bind the vertex buffer to the input assembler stage
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        virtual void Bind(::ID3D12GraphicsCommandList* commandList) override
        {
            commandList->IASetVertexBuffers(0, 1, &View);
        }

        /**
         * @brief 获取类型名称 / Get the type name
         * @return 类型名称字符串 / Type name string
         */
        virtual const char* GetTypeName() const override { return "VertexBufferDX12"; }

        /**
         * @brief 获取顶点数量 / Get the vertex count
         * @return 顶点数量 / Vertex count
         */
        UINT GetVertexCount() const noexcept { return VertexCount; }

        /**
         * @brief 获取顶点跨度大小 / Get the vertex stride size
         * @return 每个顶点的字节大小 / Size in bytes per vertex
         */
        UINT GetStrideSize() const noexcept { return StrideSize; }

    private:
        /**
         * @brief 创建顶点缓冲区资源 / Create the vertex buffer resource
         *
         * 创建默认堆顶点缓冲区，并通过上传堆和临时命令列表
         * 将顶点数据从 CPU 复制到 GPU。完成后使用围栏同步。
         *
         * Creates a default heap vertex buffer and copies vertex data from CPU
         * to GPU via an upload heap and temporary command list. Synchronizes
         * using a fence after completion.
         *
         * @param vertices 顶点数据向量 / Vertex data vector
         */
        void CreateVertexBuffer(const std::vector<VertexType>& vertices)
        {
            auto device = pCore->GetDevice();
            UINT bufferSize = sizeof(VertexType) * vertices.size();

            // --- 创建默认堆顶点缓冲区 / Create default heap vertex buffer ---
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
                IID_PPV_ARGS(&pVertexBuffer)
            );
            if (FAILED(hr) || !pVertexBuffer)
            {
                throw std::runtime_error("Failed to create vertex buffer");
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
                throw std::runtime_error("Failed to create upload buffer for vertex data");
            }

            // --- 将顶点数据映射并复制到上传堆 / Map and copy vertex data to upload heap ---
            void* pData;
            hr = pUploadBuffer->Map(0, nullptr, &pData);
            if (FAILED(hr) || !pData)
            {
                throw std::runtime_error("Failed to map upload buffer");
            }
            memcpy(pData, vertices.data(), bufferSize);
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
            pTempCommandList->CopyResource(pVertexBuffer.Get(), pUploadBuffer.Get());

            ::D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = ::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = pVertexBuffer.Get();
            barrier.Transition.StateBefore = ::D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = ::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
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

            // --- 设置顶点缓冲区视图 / Set up vertex buffer view ---
            View.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
            View.SizeInBytes = bufferSize;
            View.StrideInBytes = sizeof(VertexType);
        }

        Microsoft::WRL::ComPtr<::ID3D12Resource> pVertexBuffer;  ///< 顶点缓冲区资源指针 / Vertex buffer resource pointer
        ::D3D12_VERTEX_BUFFER_VIEW View = {};                   ///< 顶点缓冲区视图 / Vertex buffer view
        UINT VertexCount = 0;                                    ///< 顶点数量 / Vertex count
        UINT StrideSize = 0;                                     ///< 顶点跨度大小（字节） / Vertex stride size in bytes
    };
}

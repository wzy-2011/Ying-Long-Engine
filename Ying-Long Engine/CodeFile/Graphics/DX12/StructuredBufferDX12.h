#pragma once

#include "DX12Core.h"
#include "DX12DescriptorHeap.h"
#include <vector>

namespace YingLong
{
    template<typename ElementType>
    class StructuredBufferDX12
    {
    public:
        StructuredBufferDX12() = default;
        
        StructuredBufferDX12(DX12Core& core, UINT initialCapacity = 100)
            : pCore(&core)
        {
            CreateBuffer(initialCapacity);
        }

        ~StructuredBufferDX12() = default;

        void Initialize(DX12Core& core, UINT initialCapacity = 100)
        {
            pCore = &core;
            CreateBuffer(initialCapacity);
        }

        void InitializeWithSRVIndex(DX12Core& core, UINT initialCapacity, UINT srvIndex)
        {
            pCore = &core;
            this->srvIndex = srvIndex;
            CreateBuffer(initialCapacity);
        }

        void Update(const std::vector<ElementType>& data)
        {
            if (!pBuffer || !pCore || !pUploadBuffer)
                return;

            size_t dataSize = data.size() * sizeof(ElementType);
            
            if (dataSize > bufferSize)
            {
                UINT newCapacity = static_cast<UINT>(data.size()) * 2;
                ReleaseResources();
                CreateBuffer(newCapacity);
                lastDataSize = 0;
            }

            if (data.empty())
                return;

            D3D12_RANGE readRange = {};
            readRange.Begin = 0;
            readRange.End = 0;

            void* pData;
            HRESULT hr = pUploadBuffer->Map(0, &readRange, &pData);
            if (FAILED(hr) || !pData)
                return;

            if (lastDataSize == dataSize && memcmp(pData, data.data(), dataSize) == 0)
            {
                pUploadBuffer->Unmap(0, nullptr);
                return;
            }

            memcpy(pData, data.data(), dataSize);
            pUploadBuffer->Unmap(0, nullptr);

            lastDataSize = dataSize;
            needsUpdate = true;
            currentDataSize = static_cast<UINT>(dataSize);
        }

        void ApplyUpdate(ID3D12GraphicsCommandList* commandList)
        {
            if (!needsUpdate || !pBuffer || !pUploadBuffer)
                return;

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = pBuffer.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            commandList->ResourceBarrier(1, &barrier);

            commandList->CopyBufferRegion(pBuffer.Get(), 0, pUploadBuffer.Get(), 0, currentDataSize);

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
            commandList->ResourceBarrier(1, &barrier);

            needsUpdate = false;
        }

        void ReleaseResources()
        {
            pBuffer.Reset();
            pUploadBuffer.Reset();
            capacity = 0;
            bufferSize = 0;
            needsUpdate = false;
            lastDataSize = 0;
        }

        void Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex)
        {
            if (!pBuffer || !pCore)
                return;
            
            if (srvIndex == UINT_MAX)
                AllocateSRV();

            commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, 
                pCore->GetCBVSRVUAVHeap()->GetGPUHandle(srvIndex));
        }

        UINT GetSRVIndex() const { return srvIndex; }

        ID3D12Resource* GetBuffer() const { return pBuffer.Get(); }

        bool NeedsUpdate() const { return needsUpdate; }

    private:
        void CreateBuffer(UINT capacity)
        {
            if (!pCore)
                return;

            auto device = pCore->GetDevice();
            if (!device)
                return;

            UINT elementSize = sizeof(ElementType);
            bufferSize = capacity * elementSize;

            UINT alignedSize = ((bufferSize + 255) & ~255);

            D3D12_HEAP_PROPERTIES defaultHeapProps = {};
            defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
            defaultHeapProps.CreationNodeMask = 1;
            defaultHeapProps.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = alignedSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            HRESULT hr = device->CreateCommittedResource(
                &defaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pBuffer)
            );

            if (FAILED(hr))
                return;

            D3D12_HEAP_PROPERTIES uploadHeapProps = {};
            uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapProps.CreationNodeMask = 1;
            uploadHeapProps.VisibleNodeMask = 1;

            hr = device->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pUploadBuffer)
            );

            if (SUCCEEDED(hr))
            {
                this->capacity = capacity;
                if (srvIndex == UINT_MAX)
                    AllocateSRV();
                else
                    CreateSRV();
            }
            else
            {
                pBuffer.Reset();
            }
        }

        void AllocateSRV()
        {
            if (!pCore || !pBuffer)
                return;

            srvIndex = pCore->GetCBVSRVUAVHeap()->Allocate();
            
            CreateSRV();
        }

        void CreateSRV()
        {
            if (!pCore || !pBuffer || srvIndex == UINT_MAX)
                return;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = capacity;
            srvDesc.Buffer.StructureByteStride = sizeof(ElementType);
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            pCore->GetDevice()->CreateShaderResourceView(pBuffer.Get(), &srvDesc, 
                pCore->GetCBVSRVUAVHeap()->GetCPUHandle(srvIndex));
        }

        DX12Core* pCore = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer;
        UINT capacity = 0;
        UINT bufferSize = 0;
        UINT srvIndex = UINT_MAX;
        bool needsUpdate = false;
        UINT currentDataSize = 0;
        size_t lastDataSize = 0;
    };
}
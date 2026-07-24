#include "DX12DescriptorHeap.h"
#include <stdexcept>

namespace YingLong
{
    DX12DescriptorHeap::DX12DescriptorHeap(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        UINT numDescriptors,
        D3D12_DESCRIPTOR_HEAP_FLAGS flags)
        : Type(type)
        , Flags(flags)
        , NumDescriptors(numDescriptors)
        , CurrentIndex(0)
        , DescriptorSize(0)
    {
        if (!device)
        {
            throw std::runtime_error("Null device passed to DX12DescriptorHeap constructor");
        }

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = type;
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Flags = flags;
        heapDesc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pHeap));
        if (FAILED(hr) || !pHeap)
        {
            throw std::runtime_error("Failed to create descriptor heap");
        }

        DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

        CPUHeapStart = pHeap->GetCPUDescriptorHandleForHeapStart();

        if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            GPUHeapStart = pHeap->GetGPUDescriptorHandleForHeapStart();
        }
        else
        {
            GPUHeapStart.ptr = 0;
        }
    }

    DX12DescriptorHeap::~DX12DescriptorHeap()
    {
        pHeap.Reset();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetCPUHandle(UINT index) const noexcept
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = CPUHeapStart;
        handle.ptr += static_cast<UINT64>(index) * DescriptorSize;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetGPUHandle(UINT index) const noexcept
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = GPUHeapStart;
        handle.ptr += static_cast<UINT64>(index) * DescriptorSize;
        return handle;
    }

    UINT DX12DescriptorHeap::Allocate()
    {
        if (!FreeList.empty())
        {
            UINT index = FreeList.back();
            FreeList.pop_back();
            return index;
        }

        if (CurrentIndex >= NumDescriptors)
        {
            throw std::runtime_error("DX12DescriptorHeap::Allocate() - Heap is full");
        }

        UINT allocatedIndex = CurrentIndex;
        CurrentIndex++;
        return allocatedIndex;
    }

    void DX12DescriptorHeap::Free(UINT index)
    {
        if (index >= NumDescriptors)
            return;

        FreeList.push_back(index);
    }

    bool DX12DescriptorHeap::HasSpace() const noexcept
    {
        return !FreeList.empty() || CurrentIndex < NumDescriptors;
    }

    UINT DX12DescriptorHeap::GetFreeCount() const noexcept
    {
        return static_cast<UINT>(FreeList.size()) + (NumDescriptors - CurrentIndex);
    }

    void DX12DescriptorHeap::ResetAllocation()
    {
        CurrentIndex = 0;
        FreeList.clear();
    }
}
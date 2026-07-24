#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace YingLong
{
    class DX12DescriptorHeap
    {
    public:
        DX12DescriptorHeap(
            ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            UINT numDescriptors,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        );

        ~DX12DescriptorHeap();

        ID3D12DescriptorHeap* GetHeap() const noexcept { return pHeap.Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const noexcept;

        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const noexcept;

        UINT GetDescriptorSize() const noexcept { return DescriptorSize; }

        UINT GetNumDescriptors() const noexcept { return NumDescriptors; }

        UINT Allocate();

        void Free(UINT index);

        bool HasSpace() const noexcept;

        UINT GetFreeCount() const noexcept;

        void ResetAllocation();

        bool IsShaderVisible() const noexcept { return Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; }

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
        D3D12_DESCRIPTOR_HEAP_TYPE Type;
        D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
        UINT NumDescriptors;
        UINT DescriptorSize;
        UINT CurrentIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapStart;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHeapStart;
        std::vector<UINT> FreeList;
    };
}
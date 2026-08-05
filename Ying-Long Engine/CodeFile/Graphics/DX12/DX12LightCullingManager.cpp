/**
 * @file DX12LightCullingManager.cpp
 * @brief DX12 光源剔除管理器实现 / DX12 Light Culling Manager Implementation
 */
#include "DX12LightCullingManager.h"
#include "DX12Core.h"
#include "DX12ShaderCompiler.h"
#include "DX12Primitives.h"
#include "../../Debug/DX12Log.h"
#include <stdexcept>
#include <string>

namespace YingLong
{
    DX12LightCullingManager::DX12LightCullingManager()
    {
    }

    DX12LightCullingManager::~DX12LightCullingManager()
    {
    }

    void DX12LightCullingManager::CreateRootSignature(DX12Core& core)
    {
        DX12Log("[DX12LightCullingManager] Creating light culling compute root signature...\n");

        ID3D12Device* pDevice = core.GetDevice();
        if (!pDevice)
        {
            DX12LogError("[DX12LightCullingManager] Device is null\n");
            return;
        }

        try
        {
            std::vector<D3D12_ROOT_PARAMETER1> parameters;

            // Param 0: LightCullingConstants CBV (b0)
            D3D12_ROOT_PARAMETER1 param0 = {};
            param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param0.Descriptor.ShaderRegister = 0;
            param0.Descriptor.RegisterSpace = 0;
            parameters.push_back(param0);

            // Param 1: Light buffers SRV descriptor table (t4-t5)
            D3D12_DESCRIPTOR_RANGE1 srvRange = {};
            srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            srvRange.NumDescriptors = 2;
            srvRange.BaseShaderRegister = 4;
            srvRange.RegisterSpace = 0;
            srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_PARAMETER1 param1 = {};
            param1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param1.DescriptorTable.NumDescriptorRanges = 1;
            param1.DescriptorTable.pDescriptorRanges = &srvRange;
            parameters.push_back(param1);

            // Param 2: Light culling output UAV descriptor table (u0-u1)
            D3D12_DESCRIPTOR_RANGE1 uavRange = {};
            uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            uavRange.NumDescriptors = 2;
            uavRange.BaseShaderRegister = 0;
            uavRange.RegisterSpace = 0;
            uavRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_PARAMETER1 param2 = {};
            param2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param2.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param2.DescriptorTable.NumDescriptorRanges = 1;
            param2.DescriptorTable.pDescriptorRanges = &uavRange;
            parameters.push_back(param2);

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
            rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(parameters.size());
            rootSigDesc.Desc_1_1.pParameters = parameters.data();
            rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
            rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;
            rootSigDesc.Desc_1_1.Flags =
                D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

            Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

            HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &serializedRootSig, &errorBlob);
            if (FAILED(hr))
            {
                if (errorBlob)
                {
                    DX12LogError(("[DX12LightCullingManager] Root signature serialization error: " +
                                  std::string((const char*)errorBlob->GetBufferPointer()) + "\n").c_str());
                }
                throw std::runtime_error("Failed to serialize light culling root signature");
            }

            hr = pDevice->CreateRootSignature(
                0,
                serializedRootSig->GetBufferPointer(),
                serializedRootSig->GetBufferSize(),
                IID_PPV_ARGS(&RootSig));
            if (FAILED(hr) || !RootSig)
            {
                throw std::runtime_error("Failed to create light culling root signature");
            }

            DX12LogSuccess("[DX12LightCullingManager] Compute root signature created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12LightCullingManager] Failed to create root signature: " + std::string(e.what()) + "\n").c_str());
            RootSig.Reset();
        }
    }

    void DX12LightCullingManager::CreateComputePSO(DX12Core& core)
    {
        DX12Log("[DX12LightCullingManager] Creating light culling compute PSO...\n");

        if (!RootSig)
        {
            DX12LogWarning("[DX12LightCullingManager] Root signature not available, skipping compute PSO\n");
            return;
        }

        ID3D12Device* pDevice = core.GetDevice();
        if (!pDevice)
            return;

        try
        {
            wchar_t basePath[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, basePath);
            std::wstring csPath = std::wstring(basePath) + L"\\CodeFile\\Shader\\LightCulling\\LightCullingCS.hlsl";

            std::vector<uint8_t> csBytecode = DX12ShaderCompiler::CompileComputeShader(csPath);

            D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
            computeDesc.pRootSignature = RootSig.Get();
            computeDesc.CS.pShaderBytecode = csBytecode.data();
            computeDesc.CS.BytecodeLength = csBytecode.size();
            computeDesc.NodeMask = 0;
            computeDesc.CachedPSO.pCachedBlob = nullptr;
            computeDesc.CachedPSO.CachedBlobSizeInBytes = 0;
            computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            HRESULT hr = pDevice->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&PSO));
            if (FAILED(hr) || !PSO)
            {
                throw std::runtime_error("Failed to create light culling compute PSO");
            }

            DX12LogSuccess("[DX12LightCullingManager] Compute PSO created successfully\n");
        }
        catch (const std::exception& e)
        {
            DX12LogError(("[DX12LightCullingManager] Failed to create compute PSO: " + std::string(e.what()) + "\n").c_str());
            PSO.Reset();
        }
    }

    void DX12LightCullingManager::CreateResources(DX12Core& core)
    {
        if (bReady)
            return;

        ID3D12Device* pDevice = core.GetDevice();
        DX12DescriptorHeap* heap = core.GetCBVSRVUAVHeap();
        if (!pDevice || !heap)
            return;

        DX12Log("[DX12LightCullingManager] Creating light culling resources...\n");

        int w = core.GetWidth();
        int h = core.GetHeight();
        if (w <= 0 || h <= 0) return;

        uint32_t tilesX = (w + TILE_SIZE_X - 1) / TILE_SIZE_X;
        uint32_t tilesY = (h + TILE_SIZE_Y - 1) / TILE_SIZE_Y;
        uint32_t numTiles = tilesX * tilesY;

        uint64_t indexListSize = (uint64_t)numTiles * MAX_LIGHTS_PER_TILE * sizeof(uint32_t);
        uint64_t countPerTileSize = (uint64_t)numTiles * sizeof(uint32_t);

        // Create LightIndexList buffer (UAV)
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = indexListSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = pDevice->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&pLightIndexListBuffer));
        if (FAILED(hr))
        {
            DX12LogError("[DX12LightCullingManager] Failed to create light index list buffer\n");
            return;
        }

        // Create LightCountPerTile buffer (UAV)
        bufferDesc.Width = countPerTileSize;
        hr = pDevice->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&pLightCountPerTileBuffer));
        if (FAILED(hr))
        {
            DX12LogError("[DX12LightCullingManager] Failed to create light count per tile buffer\n");
            return;
        }

        // Allocate SRV indices (t6, t7) for pixel shader reading
        LightIndexListSRVIndex = heap->Allocate();
        LightCountSRVIndex = heap->Allocate();

        // Create SRV for LightIndexList
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_UINT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = numTiles * MAX_LIGHTS_PER_TILE;
        srvDesc.Buffer.StructureByteStride = 0;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        pDevice->CreateShaderResourceView(pLightIndexListBuffer.Get(), &srvDesc,
            heap->GetCPUHandle(LightIndexListSRVIndex));

        // Create SRV for LightCountPerTile
        srvDesc.Buffer.NumElements = numTiles;
        pDevice->CreateShaderResourceView(pLightCountPerTileBuffer.Get(), &srvDesc,
            heap->GetCPUHandle(LightCountSRVIndex));

        // Allocate UAV indices (u0, u1) for compute shader writing
        LightIndexListUAVIndex = heap->Allocate();
        LightCountUAVIndex = heap->Allocate();

        // Create UAV for LightIndexList
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_UINT;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = numTiles * MAX_LIGHTS_PER_TILE;
        uavDesc.Buffer.StructureByteStride = 0;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        pDevice->CreateUnorderedAccessView(pLightIndexListBuffer.Get(), nullptr, &uavDesc,
            heap->GetCPUHandle(LightIndexListUAVIndex));

        // Create UAV for LightCountPerTile
        uavDesc.Buffer.NumElements = numTiles;
        pDevice->CreateUnorderedAccessView(pLightCountPerTileBuffer.Get(), nullptr, &uavDesc,
            heap->GetCPUHandle(LightCountUAVIndex));

        // Create light culling constant buffer (upload heap)
        CBVIndex = heap->Allocate();

        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC cbDesc = {};
        cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.Width = 256;
        cbDesc.Height = 1;
        cbDesc.DepthOrArraySize = 1;
        cbDesc.MipLevels = 1;
        cbDesc.Format = DXGI_FORMAT_UNKNOWN;
        cbDesc.SampleDesc.Count = 1;
        cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = pDevice->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pConstantBuffer));
        if (FAILED(hr))
        {
            DX12LogError("[DX12LightCullingManager] Failed to create constant buffer\n");
            return;
        }

        // Map constant buffer for CPU writes
        D3D12_RANGE readRange = {};
        hr = pConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pConstantBufferCPU));
        if (FAILED(hr))
        {
            DX12LogError("[DX12LightCullingManager] Failed to map constant buffer\n");
            return;
        }

        ConstantBufferGPU = pConstantBuffer->GetGPUVirtualAddress();

        // Create CBV
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = ConstantBufferGPU;
        cbvDesc.SizeInBytes = 256;
        pDevice->CreateConstantBufferView(&cbvDesc,
            heap->GetCPUHandle(CBVIndex));

        bReady = true;
        DX12LogSuccess("[DX12LightCullingManager] Resources created successfully\n");
    }

    void DX12LightCullingManager::Shutdown(DX12Core& core)
    {
        DX12DescriptorHeap* heap = core.GetCBVSRVUAVHeap();

        // Unmap constant buffer
        if (pConstantBufferCPU && pConstantBuffer)
        {
            pConstantBuffer->Unmap(0, nullptr);
            pConstantBufferCPU = nullptr;
        }

        // Release resources
        pLightIndexListBuffer.Reset();
        pLightCountPerTileBuffer.Reset();
        pConstantBuffer.Reset();
        PSO.Reset();
        RootSig.Reset();

        // Free descriptor indices
        if (heap)
        {
            if (LightIndexListSRVIndex != UINT_MAX) { heap->Free(LightIndexListSRVIndex); LightIndexListSRVIndex = UINT_MAX; }
            if (LightCountSRVIndex != UINT_MAX) { heap->Free(LightCountSRVIndex); LightCountSRVIndex = UINT_MAX; }
            if (LightIndexListUAVIndex != UINT_MAX) { heap->Free(LightIndexListUAVIndex); LightIndexListUAVIndex = UINT_MAX; }
            if (LightCountUAVIndex != UINT_MAX) { heap->Free(LightCountUAVIndex); LightCountUAVIndex = UINT_MAX; }
            if (CBVIndex != UINT_MAX) { heap->Free(CBVIndex); CBVIndex = UINT_MAX; }
        }

        bReady = false;
        ConstantBufferGPU = {};
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DX12LightCullingManager::GetLightIndexListSRVHandle(const DX12DescriptorHeap& heap) const noexcept
    {
        if (LightIndexListSRVIndex != UINT_MAX)
            return heap.GetGPUHandle(LightIndexListSRVIndex);
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DX12LightCullingManager::GetLightCountSRVHandle(const DX12DescriptorHeap& heap) const noexcept
    {
        if (LightCountSRVIndex != UINT_MAX)
            return heap.GetGPUHandle(LightCountSRVIndex);
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }

    void DX12LightCullingManager::TransitionToSRV(ID3D12GraphicsCommandList* commandList)
    {
        if (!commandList || !bReady)
            return;

        D3D12_RESOURCE_BARRIER barriers[2] = {};

        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = pLightIndexListBuffer.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.pResource = pLightCountPerTileBuffer.Get();
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(2, barriers);
    }

    void DX12LightCullingManager::TransitionToUAV(ID3D12GraphicsCommandList* commandList)
    {
        if (!commandList || !bReady)
            return;

        D3D12_RESOURCE_BARRIER barriers[2] = {};

        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = pLightIndexListBuffer.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.pResource = pLightCountPerTileBuffer.Get();
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(2, barriers);
    }
}
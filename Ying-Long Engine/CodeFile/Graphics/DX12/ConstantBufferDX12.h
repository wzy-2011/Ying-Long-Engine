/**
 * @file ConstantBufferDX12.h
 * @brief DX12 常量缓冲区模板类定义 / DX12 constant buffer template class definition
 *
 * 本文件定义了 ConstantBufferDX12 模板类，用于创建和管理
 * 可更新的 DX12 常量缓冲区资源，支持上传堆和内存映射更新。
 *
 * This file defines the ConstantBufferDX12 template class for creating and
 * managing updatable DX12 constant buffer resources with upload heap and
 * memory-mapped updates.
 */

#pragma once

#include "BindableDX12.h"
#include "DX12Core.h"
#include <vector>
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief DX12 常量缓冲区模板类 / DX12 constant buffer template class
     *
     * ConstantBufferDX12 封装了 DX12 常量缓冲区的创建、更新和绑定功能。
     * 它使用上传堆（UPLOAD heap）来存储常量数据，通过内存映射（Map/Unmap）
     * 进行数据更新。缓冲区大小会自动对齐到 256 字节边界（D3D12 常量缓冲区要求）。
     *
     * ConstantBufferDX12 encapsulates the creation, update, and binding of
     * DX12 constant buffers. It uses an upload heap to store constant data
     * and updates data via memory mapping (Map/Unmap). The buffer size is
     * automatically aligned to 256-byte boundaries as required by D3D12
     * constant buffers.
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

            CreateConstantBuffer();
        }

        /**
         * @brief 将常量缓冲区绑定到图形命令列表 / Bind the constant buffer to the graphics command list
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        virtual void Bind(::ID3D12GraphicsCommandList* commandList) override
        {
            if (!pConstantBuffer)
                return;
            commandList->SetGraphicsRootConstantBufferView(RootParameterIndex, pConstantBuffer->GetGPUVirtualAddress());
        }

        /**
         * @brief 获取类型名称 / Get the type name
         * @return 类型名称字符串 / Type name string
         */
        virtual const char* GetTypeName() const override { return "ConstantBufferDX12"; }

        /**
         * @brief 更新常量缓冲区数据 / Update constant buffer data
         * @param newData 新的常量数据 / New constant data
         */
        void Update(const ConstantType& newData)
        {
            Data = newData;

            if (!pConstantBuffer)
                return;

            // 设置读取范围为空，表示 CPU 不会读回数据（优化性能）
            // Set read range to empty, indicating CPU won't read back data (performance optimization)
            ::D3D12_RANGE readRange = {};
            readRange.Begin = 0;
            readRange.End = 0;

            void* pData;
            HRESULT hr = pConstantBuffer->Map(0, &readRange, &pData);
            if (SUCCEEDED(hr) && pData)
            {
                memcpy(pData, &Data, sizeof(ConstantType));
                pConstantBuffer->Unmap(0, nullptr);
            }
        }

        /**
         * @brief 获取常量缓冲区的 GPU 虚拟地址 / Get the GPU virtual address of the constant buffer
         * @return GPU 虚拟地址 / GPU virtual address
         */
        ::D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept
        {
            if (!pConstantBuffer)
                return 0;
            return pConstantBuffer->GetGPUVirtualAddress();
        }

    private:
        /**
         * @brief 创建常量缓冲区资源 / Create the constant buffer resource
         *
         * 创建上传堆类型的常量缓冲区，并将初始数据复制到缓冲区中。
         * Creates an upload heap type constant buffer and copies initial data into it.
         */
        void CreateConstantBuffer()
        {
            auto device = pCore->GetDevice();
            if (!device)
            {
                throw std::runtime_error("Null device in ConstantBufferDX12::CreateConstantBuffer");
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

            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                ::D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                ::D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pConstantBuffer)
            );
            if (FAILED(hr) || !pConstantBuffer)
            {
                throw std::runtime_error("Failed to create constant buffer");
            }

            // 将初始数据映射并复制到缓冲区
            // Map and copy initial data to the buffer
            ::D3D12_RANGE readRange = {};
            readRange.Begin = 0;
            readRange.End = 0;

            void* pData;
            hr = pConstantBuffer->Map(0, &readRange, &pData);
            if (SUCCEEDED(hr) && pData)
            {
                memcpy(pData, &Data, sizeof(ConstantType));
                pConstantBuffer->Unmap(0, nullptr);
            }
        }

        Microsoft::WRL::ComPtr<::ID3D12Resource> pConstantBuffer;  ///< 常量缓冲区资源指针 / Constant buffer resource pointer
        ConstantType Data;                                          ///< 常量数据副本 / Constant data copy
        UINT RootParameterIndex;                                    ///< 根参数索引 / Root parameter index
    };
}

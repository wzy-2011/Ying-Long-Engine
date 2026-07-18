/**
 * @file BindableDX12.h
 * @brief DX12 可绑定资源基类和辅助函数定义 / DX12 bindable resource base class and helper function definitions
 *
 * 本文件定义了所有 DX12 可绑定资源的抽象基类 BindableDX12，
 * 以及设置常量缓冲区视图和着色器资源视图的辅助函数。
 *
 * This file defines the abstract base class BindableDX12 for all DX12
 * bindable resources, and helper functions for setting constant buffer
 * views and shader resource views.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <cstdint>

namespace YingLong
{
    class DX12Core;

    /**
     * @brief DX12 可绑定资源抽象基类 / Abstract base class for DX12 bindable resources
     *
     * 所有可绑定到渲染管线的 DX12 资源（如常量缓冲区、顶点缓冲区、
     * 索引缓冲区、纹理、采样器等）都应继承此类。它提供了统一的
     * 绑定接口、类型名称查询、根参数索引管理等功能。
     *
     * All DX12 resources that can be bound to the rendering pipeline (such as
     * constant buffers, vertex buffers, index buffers, textures, samplers, etc.)
     * should inherit from this class. It provides a unified binding interface,
     * type name querying, root parameter index management, and more.
     */
    class BindableDX12
    {
    public:
        BindableDX12() = default;
        virtual ~BindableDX12() = default;

        /**
         * @brief 将资源绑定到图形命令列表 / Bind the resource to the graphics command list
         * @param commandList 指向 ID3D12GraphicsCommandList 的指针 / Pointer to ID3D12GraphicsCommandList
         */
        virtual void Bind(::ID3D12GraphicsCommandList* commandList) = 0;

        /**
         * @brief 获取资源类型名称 / Get the resource type name
         * @return 资源类型的 C 风格字符串 / C-style string of the resource type
         */
        virtual const char* GetTypeName() const = 0;

        /**
         * @brief 检查资源是否已初始化 / Check if the resource is initialized
         * @return true 表示已初始化，false 表示未初始化 / true if initialized, false otherwise
         */
        bool IsInitialized() const noexcept { return bInitialized; }

        /**
         * @brief 设置根参数索引 / Set the root parameter index
         * @param index 根参数索引值 / Root parameter index value
         */
        void SetRootParameterIndex(UINT index) noexcept { RootParameterIndex = index; }

        /**
         * @brief 获取根参数索引 / Get the root parameter index
         * @return 根参数索引值 / Root parameter index value
         */
        UINT GetRootParameterIndex() const noexcept { return RootParameterIndex; }

        /**
         * @brief 获取 GPU 描述符句柄 / Get the GPU descriptor handle
         * @return D3D12_GPU_DESCRIPTOR_HANDLE 结构体 / D3D12_GPU_DESCRIPTOR_HANDLE structure
         */
        virtual ::D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return {}; }

        /**
         * @brief 获取 CPU 描述符句柄 / Get the CPU descriptor handle
         * @return D3D12_CPU_DESCRIPTOR_HANDLE 结构体 / D3D12_CPU_DESCRIPTOR_HANDLE structure
         */
        virtual ::D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return {}; }

        /**
         * @brief 获取描述符范围类型 / Get the descriptor range type
         * @return D3D12_DESCRIPTOR_RANGE_TYPE 枚举值 / D3D12_DESCRIPTOR_RANGE_TYPE enum value
         */
        virtual ::D3D12_DESCRIPTOR_RANGE_TYPE GetType() const noexcept { return ::D3D12_DESCRIPTOR_RANGE_TYPE_SRV; }

        /**
         * @brief 获取底层 ID3D12Resource 指针 / Get the underlying ID3D12Resource pointer
         * @return 指向 ID3D12Resource 的指针 / Pointer to ID3D12Resource
         */
        virtual ::ID3D12Resource* GetResource() const noexcept { return nullptr; }

    protected:
        DX12Core* pCore = nullptr;          ///< 指向 DX12Core 的指针 / Pointer to DX12Core
        bool bInitialized = false;          ///< 初始化标志 / Initialization flag
        UINT RootParameterIndex = 0;        ///< 根签名中的参数索引 / Parameter index in root signature
    };

    /**
     * @brief 设置图形根常量缓冲区视图 / Set the graphics root constant buffer view
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     * @param gpuAddress GPU 虚拟地址 / GPU virtual address
     * @param rootParameterIndex 根参数索引 / Root parameter index
     */
    inline void SetConstantBufferView(
        ::ID3D12GraphicsCommandList* commandList,
        ::D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
        UINT rootParameterIndex)
    {
        commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
    }

    /**
     * @brief 设置图形根着色器资源视图 / Set the graphics root shader resource view
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     * @param gpuAddress GPU 虚拟地址 / GPU virtual address
     * @param rootParameterIndex 根参数索引 / Root parameter index
     */
    inline void SetShaderResourceView(
        ::ID3D12GraphicsCommandList* commandList,
        ::D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
        UINT rootParameterIndex)
    {
        commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
    }
}

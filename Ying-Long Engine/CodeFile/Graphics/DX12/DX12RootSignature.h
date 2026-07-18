/**
 * @file DX12RootSignature.h
 * @brief DX12 根签名头文件 / DX12 Root Signature Header
 *
 * 本文件定义了 DX12RootSignature 类和 DX12RootSignatureBuilder 类，
 * 用于创建和管理 D3D12 根签名。根签名定义了着色器如何访问资源。
 *
 * This file defines the DX12RootSignature class and DX12RootSignatureBuilder class,
 * used for creating and managing D3D12 root signatures. The root signature
 * defines how shaders access resources.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

namespace YingLong
{
    /**
     * @brief 根参数类型枚举 / Root parameter type enum
     *
     * 定义根签名参数的类型。
     * Defines the types of root signature parameters.
     */
    enum class RootParameterType
    {
        ConstantBuffer,     ///< 常量缓冲视图（b-寄存器）/ CBV (b-register)
        ShaderResource,     ///< 着色器资源视图（t-寄存器）/ SRV (t-register)
        UnorderedAccess,    ///< 无序访问视图（u-寄存器）/ UAV (u-register)
        Sampler,            ///< 采样器（s-寄存器）/ Sampler (s-register)
        DescriptorTable,    ///< 描述符表 / Table of descriptors
        Constants           ///< 根签名中的32位常量 / 32-bit constants directly in root signature
    };

    /**
     * @brief DX12 根签名类 / DX12 Root Signature Class
     *
     * DX12RootSignature 类封装了 D3D12 根签名对象，提供：
     * - 默认 PBR 根签名的创建
     * - 自定义根签名的创建
     * - 寄存器到根参数索引的映射查询
     * - 绑定到命令列表
     *
     * The DX12RootSignature class encapsulates the D3D12 root signature object, providing:
     * - Default PBR root signature creation
     * - Custom root signature creation
     * - Register to root parameter index mapping queries
     * - Binding to command list
     */
    class DX12RootSignature
    {
    public:
        /**
         * @brief 构造函数（创建默认根签名）/ Constructor (create default root signature)
         *
         * 基于现有的 PBR 着色器创建默认根签名。
         * Creates default root signature based on existing PBR shaders.
         *
         * @param device D3D12 设备指针 / D3D12 device pointer
         */
        DX12RootSignature(ID3D12Device* device);

        /**
         * @brief 构造函数（创建自定义根签名）/ Constructor (create custom root signature)
         *
         * 根据提供的参数数组创建自定义根签名。
         * Creates custom root signature based on provided parameter array.
         *
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @param parameters 根参数数组 / Root parameter array
         */
        DX12RootSignature(ID3D12Device* device, const std::vector<D3D12_ROOT_PARAMETER1>& parameters);

        /**
         * @brief 析构函数 / Destructor
         *
         * 释放根签名资源。
         * Releases root signature resources.
         */
        ~DX12RootSignature();

        /**
         * @brief 获取根签名对象 / Get the root signature object
         * @return ID3D12RootSignature 指针 / ID3D12RootSignature pointer
         */
        ID3D12RootSignature* GetRootSignature() const noexcept { return pRootSignature.Get(); }

        /**
         * @brief 获取根参数数量 / Get number of root parameters
         * @return 根参数数量 / Number of root parameters
         */
        UINT GetNumParameters() const noexcept { return NumParameters; }

        /**
         * @brief 获取指定 CBV 寄存器对应的根参数索引 / Get CBV root parameter index for specific register
         * @param registerIndex 寄存器索引 / Register index
         * @return 根参数索引 / Root parameter index
         */
        UINT GetCBVRootParameterIndex(UINT registerIndex) const noexcept;

        /**
         * @brief 获取指定 SRV 寄存器对应的根参数索引 / Get SRV root parameter index for specific register
         * @param registerIndex 寄存器索引 / Register index
         * @return 根参数索引 / Root parameter index
         */
        UINT GetSRVRootParameterIndex(UINT registerIndex) const noexcept;

        /**
         * @brief 获取指定采样器寄存器对应的根参数索引 / Get sampler root parameter index for specific register
         * @param registerIndex 寄存器索引 / Register index
         * @return 根参数索引 / Root parameter index
         */
        UINT GetSamplerRootParameterIndex(UINT registerIndex) const noexcept;

        /**
         * @brief 将根签名绑定到命令列表 / Bind root signature to command list
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Bind(ID3D12GraphicsCommandList* commandList) noexcept;

    private:
        /**
         * @brief 创建默认根签名 / Create default root signature
         * @param device D3D12 设备指针 / D3D12 device pointer
         */
        void CreateDefaultRootSignature(ID3D12Device* device);

        /**
         * @brief 从参数创建根签名 / Create root signature from parameters
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @param parameters 根参数数组 / Root parameter array
         */
        void CreateFromParameters(ID3D12Device* device, const std::vector<D3D12_ROOT_PARAMETER1>& parameters);

        Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;  ///< D3D12 根签名对象 / D3D12 root signature object
        UINT NumParameters;                                            ///< 根参数数量 / Number of root parameters

        // Mapping from register to root parameter index
        // 寄存器到根参数索引的映射
        std::vector<UINT> CBVRegisterMapping;     ///< CBV 寄存器映射（b0, b1, b2...）/ CBV register mapping (b0, b1, b2...)
        std::vector<UINT> SRVRegisterMapping;     ///< SRV 寄存器映射（t0, t1, t2...）/ SRV register mapping (t0, t1, t2...)
        std::vector<UINT> SamplerRegisterMapping; ///< 采样器寄存器映射（s0, s1...）/ Sampler register mapping (s0, s1...)
    };

    /**
     * @brief DX12 根签名构建器类 / DX12 Root Signature Builder Class
     *
     * DX12RootSignatureBuilder 是一个辅助类，用于以流式方式
     * 构建自定义根签名，支持添加各种类型的根参数。
     *
     * DX12RootSignatureBuilder is a helper class for building custom
     * root signatures in a fluent way, supporting adding various types
     * of root parameters.
     */
    class DX12RootSignatureBuilder
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         */
        DX12RootSignatureBuilder();

        /**
         * @brief 添加常量缓冲视图（CBV）/ Add a constant buffer view (CBV) at b-register
         * @param registerIndex 寄存器索引 / Register index
         * @param space 寄存器空间 / Register space
         */
        void AddConstantBuffer(UINT registerIndex, UINT space = 0);

        /**
         * @brief 添加着色器资源视图（SRV）/ Add a shader resource view (SRV) at t-register
         * @param registerIndex 寄存器索引 / Register index
         * @param space 寄存器空间 / Register space
         */
        void AddShaderResource(UINT registerIndex, UINT space = 0);

        /**
         * @brief 添加多个 SRV 的描述符表 / Add a descriptor table for multiple SRVs
         * @param baseRegister 起始寄存器 / Base register
         * @param count 描述符数量 / Number of descriptors
         * @param space 寄存器空间 / Register space
         */
        void AddDescriptorTableSRV(UINT baseRegister, UINT count, UINT space = 0);

        /**
         * @brief 添加多个 CBV 的描述符表 / Add a descriptor table for multiple CBVs
         * @param baseRegister 起始寄存器 / Base register
         * @param count 描述符数量 / Number of descriptors
         * @param space 寄存器空间 / Register space
         */
        void AddDescriptorTableCBV(UINT baseRegister, UINT count, UINT space = 0);

        /**
         * @brief 添加多个采样器的描述符表 / Add a descriptor table for multiple Samplers
         * @param baseRegister 起始寄存器 / Base register
         * @param count 描述符数量 / Number of descriptors
         * @param space 寄存器空间 / Register space
         */
        void AddDescriptorTableSampler(UINT baseRegister, UINT count, UINT space = 0);

        /**
         * @brief 添加根签名中的32位常量 / Add 32-bit constants directly in root signature
         * @param num32BitValues 32位值的数量 / Number of 32-bit values
         * @param registerIndex 寄存器索引 / Register index
         * @param space 寄存器空间 / Register space
         */
        void Add32BitConstants(UINT num32BitValues, UINT registerIndex, UINT space = 0);

        /**
         * @brief 添加静态采样器 / Add static sampler (no need for descriptor heap)
         * @param registerIndex 寄存器索引 / Register index
         * @param filter 过滤模式 / Filter mode
         * @param addressMode 寻址模式 / Address mode
         * @param space 寄存器空间 / Register space
         */
        void AddStaticSampler(
            UINT registerIndex,
            D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            UINT space = 0
        );

        /**
         * @brief 构建根签名 / Build the root signature
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @return 创建的根签名对象指针 / Created root signature object pointer
         */
        DX12RootSignature* Build(ID3D12Device* device);

        /**
         * @brief 重置构建器 / Reset the builder
         *
         * 清除所有已添加的参数，准备构建新的根签名。
         * Clears all added parameters, ready to build a new root signature.
         */
        void Reset();

    private:
        std::vector<D3D12_ROOT_PARAMETER1> Parameters;      ///< 根参数列表 / Root parameter list
        std::vector<D3D12_STATIC_SAMPLER_DESC1> StaticSamplers;  ///< 静态采样器列表 / Static sampler list

        // Track register mappings
        // 跟踪寄存器映射
        std::vector<UINT> CBVRegisters;        ///< CBV 寄存器列表 / CBV register list
        std::vector<UINT> SRVRegisters;        ///< SRV 寄存器列表 / SRV register list
        std::vector<UINT> SamplerRegisters;    ///< 采样器寄存器列表 / Sampler register list

        UINT CurrentParameterIndex;            ///< 当前参数索引 / Current parameter index
    };
}

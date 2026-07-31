/**
 * @file DX12RootSignature.cpp
 * @brief DX12 根签名实现文件 / DX12 Root Signature Implementation
 *
 * 本文件实现了 DX12RootSignature 类和 DX12RootSignatureBuilder 类，
 * 包括默认 PBR 根签名的创建、自定义根签名的构建等功能。
 *
 * This file implements the DX12RootSignature class and DX12RootSignatureBuilder class,
 * including default PBR root signature creation, custom root signature building, etc.
 */

#include "DX12RootSignature.h"
#include "d3dx12.h"
#include "../../Debug/DX12Log.h"
#include <stdexcept>

namespace YingLong
{
    // Default root signature based on PBR shaders:
    // b0: PointLightConstantBuffer (50 lights + count + camera pos)
    // b1: SpotLightConstantBuffer (50 lights + count)
    // b2: MaterialConstantBuffer (albedo, metallic, roughness, AO, texture flags)
    // t0-t3: Albedo, Metallic, Roughness, Normal textures
    // s0: Sampler
    // 基于 PBR 着色器的默认根签名：
    // b0: 点光源常量缓冲区（50个光源 + 数量 + 摄像机位置）
    // b1: 聚光源常量缓冲区（50个光源 + 数量）
    // b2: 材质常量缓冲区（反照率、金属度、粗糙度、AO、纹理标志）
    // t0-t3: 反照率、金属度、粗糙度、法线纹理
    // s0: 采样器

    /**
     * @brief 构造函数（创建默认根签名）/ Constructor (create default root signature)
     *
     * 基于 PBR 着色器结构创建默认根签名。
     * Creates default root signature based on PBR shader structure.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @throws std::runtime_error 如果设备为空 / If device is null
     */
    DX12RootSignature::DX12RootSignature(ID3D12Device* device)
    {
        // 验证设备指针
        // Validate device pointer
        if (!device)
        {
            throw std::runtime_error("Null device passed to DX12RootSignature constructor");
        }

        // 创建默认 PBR 根签名
        // Create default PBR root signature
        CreateDefaultRootSignature(device);
    }

    /**
     * @brief 构造函数（创建自定义根签名）/ Constructor (create custom root signature)
     *
     * 根据提供的参数数组创建自定义根签名。
     * Creates custom root signature based on provided parameter array.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @param parameters 根参数数组 / Root parameter array
     */
    DX12RootSignature::DX12RootSignature(ID3D12Device* device, const std::vector<D3D12_ROOT_PARAMETER1>& parameters)
    {
        CreateFromParameters(device, parameters);
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 释放根签名资源。
     * Releases root signature resources.
     */
    DX12RootSignature::~DX12RootSignature()
    {
        pRootSignature.Reset();
    }

    /**
     * @brief 创建默认根签名 / Create default root signature
     *
     * 创建基于 PBR 着色器的默认根签名，包含6个根参数：
     * - 参数0: 光源计数常量缓冲区 CBV (b0, 像素着色器)
     * - 参数1: 材质常量缓冲区 CBV (b1, 像素着色器)
     * - 参数2: 变换常量缓冲区 CBV (b2, 顶点着色器)
     * - 参数3: 纹理描述符表 SRV (t0-t3, 像素着色器)
     * - 参数4: 光源缓冲区描述符表 SRV (t4-t5, 像素着色器)
     * - 参数5: 采样器描述符表 Sampler (s0, 像素着色器)
     *
     * Creates default root signature based on PBR shaders, containing 6 root parameters:
     * - Param 0: Light count constant buffer CBV (b0, pixel shader)
     * - Param 1: Material constant buffer CBV (b1, pixel shader)
     * - Param 2: Transform constant buffer CBV (b2, vertex shader)
     * - Param 3: Texture descriptor table SRV (t0-t3, pixel shader)
     * - Param 4: Light buffer descriptor table SRV (t4-t5, pixel shader)
     * - Param 5: Sampler descriptor table Sampler (s0, pixel shader)
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @throws std::runtime_error 如果序列化或创建失败
     *                             If serialization or creation fails
     */
    void DX12RootSignature::CreateDefaultRootSignature(ID3D12Device* device)
    {
        std::vector<D3D12_ROOT_PARAMETER1> parameters;

        D3D12_ROOT_PARAMETER1 param0 = {};
        param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param0.Descriptor.ShaderRegister = 0;
        param0.Descriptor.RegisterSpace = 0;
        parameters.push_back(param0);
        CBVRegisterMapping.push_back(0);

        D3D12_ROOT_PARAMETER1 param1 = {};
        param1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param1.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param1.Descriptor.ShaderRegister = 1;
        param1.Descriptor.RegisterSpace = 0;
        parameters.push_back(param1);
        CBVRegisterMapping.push_back(1);

        D3D12_ROOT_PARAMETER1 param2 = {};
        param2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param2.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        param2.Descriptor.ShaderRegister = 2;
        param2.Descriptor.RegisterSpace = 0;
        parameters.push_back(param2);

        D3D12_DESCRIPTOR_RANGE1 textureRange = {};
        textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange.NumDescriptors = 4;
        textureRange.BaseShaderRegister = 0;
        textureRange.RegisterSpace = 0;
        textureRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER1 param3 = {};
        param3.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param3.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param3.DescriptorTable.NumDescriptorRanges = 1;
        param3.DescriptorTable.pDescriptorRanges = &textureRange;
        parameters.push_back(param3);
        SRVRegisterMapping.push_back(3);

        D3D12_DESCRIPTOR_RANGE1 lightBufferRange = {};
        lightBufferRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        lightBufferRange.NumDescriptors = 2;
        lightBufferRange.BaseShaderRegister = 4;
        lightBufferRange.RegisterSpace = 0;
        lightBufferRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        lightBufferRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER1 param4 = {};
        param4.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param4.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param4.DescriptorTable.NumDescriptorRanges = 1;
        param4.DescriptorTable.pDescriptorRanges = &lightBufferRange;
        parameters.push_back(param4);
        SRVRegisterMapping.push_back(4);

        D3D12_DESCRIPTOR_RANGE1 samplerRange = {};
        samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        samplerRange.NumDescriptors = 1;
        samplerRange.BaseShaderRegister = 0;
        samplerRange.RegisterSpace = 0;
        samplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER1 param5 = {};
        param5.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param5.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param5.DescriptorTable.NumDescriptorRanges = 1;
        param5.DescriptorTable.pDescriptorRanges = &samplerRange;
        parameters.push_back(param5);
        SamplerRegisterMapping.push_back(5);

        // 参数6：Tile-Based Light Culling 光源列表描述符表 SRV (t6-t7, 像素着色器)
        // Param 6: Tile-Based Light Culling light list descriptor table SRV (t6-t7, pixel shader)
        D3D12_DESCRIPTOR_RANGE1 lightListRange = {};
        lightListRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        lightListRange.NumDescriptors = 2;
        lightListRange.BaseShaderRegister = 6;
        lightListRange.RegisterSpace = 0;
        lightListRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        lightListRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER1 param6 = {};
        param6.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param6.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param6.DescriptorTable.NumDescriptorRanges = 1;
        param6.DescriptorTable.pDescriptorRanges = &lightListRange;
        parameters.push_back(param6);
        SRVRegisterMapping.push_back(6);

        // 保存参数数量
        // Save parameter count
        NumParameters = static_cast<UINT>(parameters.size());

        // 创建根签名描述
        // Create root signature description
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;                                  ///< 根签名版本1.1 / Root signature version 1.1
        rootSigDesc.Desc_1_1.NumParameters = NumParameters;                                     ///< 参数数量 / Parameter count
        rootSigDesc.Desc_1_1.pParameters = parameters.data();                                   ///< 参数数组指针 / Parameter array pointer
        rootSigDesc.Desc_1_1.NumStaticSamplers = 0;                                             ///< 无静态采样器 / No static samplers
        rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;                                         ///< 静态采样器指针 / Static sampler pointer
        rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;  ///< 允许输入装配器输入布局 / Allow input assembler input layout

        // 序列化根签名
        // Serialize root signature
        Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &serializedRootSig, &errorBlob);
        if (FAILED(hr))
        {
            // 如果有错误信息，记录日志
            // If there is error info, log it
            if (errorBlob)
            {
                DX12LogError("[DX12RootSignature] Root signature serialization error\n");
            }
            throw std::runtime_error("Failed to serialize root signature");
        }

        // 验证序列化结果
        // Validate serialization result
        if (!serializedRootSig)
        {
            throw std::runtime_error("Serialized root signature is null");
        }

        // 创建根签名
        // Create root signature
        hr = device->CreateRootSignature(
            0,                                                          ///< 节点掩码 / Node mask
            serializedRootSig->GetBufferPointer(),                      ///< 序列化数据指针 / Serialized data pointer
            serializedRootSig->GetBufferSize(),                         ///< 序列化数据大小 / Serialized data size
            IID_PPV_ARGS(&pRootSignature)                               ///< 输出根签名 / Output root signature
        );
        if (FAILED(hr) || !pRootSignature)
        {
            throw std::runtime_error("Failed to create root signature");
        }
    }

    /**
     * @brief 从参数创建根签名 / Create root signature from parameters
     *
     * 根据提供的根参数数组创建自定义根签名。
     * Creates custom root signature based on provided root parameter array.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @param parameters 根参数数组 / Root parameter array
     */
    void DX12RootSignature::CreateFromParameters(ID3D12Device* device, const std::vector<D3D12_ROOT_PARAMETER1>& parameters)
    {
        // 保存参数数量
        // Save parameter count
        NumParameters = static_cast<UINT>(parameters.size());

        // 配置根签名描述
        // Configure root signature description
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSigDesc.Desc_1_1.NumParameters = NumParameters;
        rootSigDesc.Desc_1_1.pParameters = parameters.data();
        rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
        rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;
        rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 序列化根签名
        // Serialize root signature
        Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        D3D12SerializeVersionedRootSignature(&rootSigDesc, &serializedRootSig, &errorBlob);

        // 创建根签名
        // Create root signature
        device->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&pRootSignature)
        );
    }

    /**
     * @brief 获取指定 CBV 寄存器对应的根参数索引 / Get CBV root parameter index for specific register
     * @param registerIndex 寄存器索引 / Register index
     * @return 根参数索引，如果超出范围返回0
     *         Root parameter index, returns 0 if out of range
     */
    UINT DX12RootSignature::GetCBVRootParameterIndex(UINT registerIndex) const noexcept
    {
        if (registerIndex < CBVRegisterMapping.size())
        {
            return CBVRegisterMapping[registerIndex];
        }
        return 0;
    }

    /**
     * @brief 获取指定 SRV 寄存器对应的根参数索引 / Get SRV root parameter index for specific register
     * @param registerIndex 寄存器索引 / Register index
     * @return 根参数索引，如果超出范围返回4（默认纹理表索引）
     *         Root parameter index, returns 4 (default texture table index) if out of range
     */
    UINT DX12RootSignature::GetSRVRootParameterIndex(UINT registerIndex) const noexcept
    {
        if (registerIndex < SRVRegisterMapping.size())
        {
            return SRVRegisterMapping[registerIndex];
        }
        return 4; // Default texture table index / 默认纹理表索引
    }

    /**
     * @brief 获取指定采样器寄存器对应的根参数索引 / Get sampler root parameter index for specific register
     * @param registerIndex 寄存器索引 / Register index
     * @return 根参数索引，如果超出范围返回5（默认采样器表索引）
     *         Root parameter index, returns 5 (default sampler table index) if out of range
     */
    UINT DX12RootSignature::GetSamplerRootParameterIndex(UINT registerIndex) const noexcept
    {
        if (registerIndex < SamplerRegisterMapping.size())
        {
            return SamplerRegisterMapping[registerIndex];
        }
        return 5; // Default sampler table index / 默认采样器表索引
    }

    /**
     * @brief 将根签名绑定到命令列表 / Bind root signature to command list
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     */
    void DX12RootSignature::Bind(ID3D12GraphicsCommandList* commandList) noexcept
    {
        commandList->SetGraphicsRootSignature(pRootSignature.Get());
    }

    // ============================================================================
    // DX12RootSignatureBuilder implementation
    // DX12RootSignatureBuilder 实现
    // ============================================================================

    /**
     * @brief 构建器构造函数 / Builder constructor
     */
    DX12RootSignatureBuilder::DX12RootSignatureBuilder()
        : CurrentParameterIndex(0)
    {
    }

    /**
     * @brief 添加常量缓冲视图（CBV）/ Add a constant buffer view (CBV)
     * @param registerIndex 寄存器索引 / Register index
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::AddConstantBuffer(UINT registerIndex, UINT space)
    {
        // 配置 CBV 根参数
        // Configure CBV root parameter
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                    ///< CBV 类型 / CBV type
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;                    ///< 所有着色器可见 / All shaders visible
        param.Descriptor.ShaderRegister = registerIndex;                          ///< 寄存器索引 / Register index
        param.Descriptor.RegisterSpace = space;                                   ///< 寄存器空间 / Register space
        Parameters.push_back(param);

        // 跟踪 CBV 寄存器映射
        // Track CBV register mapping
        CBVRegisters.push_back(registerIndex);
        CurrentParameterIndex++;
    }

    /**
     * @brief 添加着色器资源视图（SRV）/ Add a shader resource view (SRV)
     * @param registerIndex 寄存器索引 / Register index
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::AddShaderResource(UINT registerIndex, UINT space)
    {
        // 配置 SRV 根参数
        // Configure SRV root parameter
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;                    ///< SRV 类型 / SRV type
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.Descriptor.ShaderRegister = registerIndex;
        param.Descriptor.RegisterSpace = space;
        Parameters.push_back(param);

        // 跟踪 SRV 寄存器映射
        // Track SRV register mapping
        SRVRegisters.push_back(registerIndex);
        CurrentParameterIndex++;
    }

    /**
     * @brief 添加多个 SRV 的描述符表 / Add a descriptor table for multiple SRVs
     * @param baseRegister 起始寄存器 / Base register
     * @param count 描述符数量 / Number of descriptors
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::AddDescriptorTableSRV(UINT baseRegister, UINT count, UINT space)
    {
        // 配置 SRV 描述符范围
        // Configure SRV descriptor range
        D3D12_DESCRIPTOR_RANGE1 range = {};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                        ///< SRV 范围类型 / SRV range type
        range.NumDescriptors = count;                                              ///< 描述符数量 / Descriptor count
        range.BaseShaderRegister = baseRegister;                                   ///< 起始寄存器 / Base register
        range.RegisterSpace = space;                                               ///< 寄存器空间 / Register space
        range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;                            ///< 无标志 / No flags
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;  ///< 追加偏移 / Append offset

        // 配置描述符表根参数
        // Configure descriptor table root parameter
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;        ///< 描述符表类型 / Descriptor table type
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;                             ///< 1个范围 / 1 range
        param.DescriptorTable.pDescriptorRanges = &range;                          ///< 范围指针 / Range pointer
        Parameters.push_back(param);

        // 跟踪所有 SRV 寄存器
        // Track all SRV registers
        for (UINT i = 0; i < count; ++i)
        {
            SRVRegisters.push_back(baseRegister + i);
        }
        CurrentParameterIndex++;
    }

    /**
     * @brief 添加多个 CBV 的描述符表 / Add a descriptor table for multiple CBVs
     * @param baseRegister 起始寄存器 / Base register
     * @param count 描述符数量 / Number of descriptors
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::AddDescriptorTableCBV(UINT baseRegister, UINT count, UINT space)
    {
        // 配置 CBV 描述符范围
        // Configure CBV descriptor range
        D3D12_DESCRIPTOR_RANGE1 range = {};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        range.NumDescriptors = count;
        range.BaseShaderRegister = baseRegister;
        range.RegisterSpace = space;
        range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // 配置描述符表根参数
        // Configure descriptor table root parameter
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &range;
        Parameters.push_back(param);

        // 跟踪所有 CBV 寄存器
        // Track all CBV registers
        for (UINT i = 0; i < count; ++i)
        {
            CBVRegisters.push_back(baseRegister + i);
        }
        CurrentParameterIndex++;
    }

    /**
     * @brief 添加多个采样器的描述符表 / Add a descriptor table for multiple Samplers
     * @param baseRegister 起始寄存器 / Base register
     * @param count 描述符数量 / Number of descriptors
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::AddDescriptorTableSampler(UINT baseRegister, UINT count, UINT space)
    {
        // 配置采样器描述符范围
        // Configure sampler descriptor range
        D3D12_DESCRIPTOR_RANGE1 range = {};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        range.NumDescriptors = count;
        range.BaseShaderRegister = baseRegister;
        range.RegisterSpace = space;
        range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // 配置描述符表根参数
        // Configure descriptor table root parameter
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &range;
        Parameters.push_back(param);

        // 跟踪所有采样器寄存器
        // Track all sampler registers
        for (UINT i = 0; i < count; ++i)
        {
            SamplerRegisters.push_back(baseRegister + i);
        }
        CurrentParameterIndex++;
    }

    /**
     * @brief 添加根签名中的32位常量 / Add 32-bit constants directly in root signature
     * @param num32BitValues 32位值的数量 / Number of 32-bit values
     * @param registerIndex 寄存器索引 / Register index
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::Add32BitConstants(UINT num32BitValues, UINT registerIndex, UINT space)
    {
        // 配置 32 位常量根参数
        // Configure 32-bit constants root parameter
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;          ///< 32位常量类型 / 32-bit constants type
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.Constants.Num32BitValues = num32BitValues;                            ///< 32位值数量 / Number of 32-bit values
        param.Constants.ShaderRegister = registerIndex;                             ///< 寄存器索引 / Register index
        param.Constants.RegisterSpace = space;                                      ///< 寄存器空间 / Register space
        Parameters.push_back(param);

        CurrentParameterIndex++;
    }

    /**
     * @brief 添加静态采样器 / Add static sampler
     *
     * 静态采样器直接嵌入根签名中，不需要描述符堆。
     * Static samplers are embedded directly in the root signature and don't need a descriptor heap.
     *
     * @param registerIndex 寄存器索引 / Register index
     * @param filter 过滤模式 / Filter mode
     * @param addressMode 寻址模式 / Address mode
     * @param space 寄存器空间 / Register space
     */
    void DX12RootSignatureBuilder::AddStaticSampler(
        UINT registerIndex,
        D3D12_FILTER filter,
        D3D12_TEXTURE_ADDRESS_MODE addressMode,
        UINT space)
    {
        // 配置静态采样器描述
        // Configure static sampler description
        D3D12_STATIC_SAMPLER_DESC1 samplerDesc = {};
        samplerDesc.Filter = filter;                                                    ///< 过滤模式 / Filter mode
        samplerDesc.AddressU = addressMode;                                              ///< U方向寻址模式 / U direction address mode
        samplerDesc.AddressV = addressMode;                                              ///< V方向寻址模式 / V direction address mode
        samplerDesc.AddressW = addressMode;                                              ///< W方向寻址模式 / W direction address mode
        samplerDesc.MipLODBias = 0.0f;                                                   ///< Mip LOD 偏移 / Mip LOD bias
        samplerDesc.MaxAnisotropy = 16;                                                  ///< 最大各向异性 / Maximum anisotropy
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;                        ///< 比较函数（从不）/ Comparison function (never)
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;           ///< 边框颜色（透明黑）/ Border color (transparent black)
        samplerDesc.MinLOD = 0.0f;                                                       ///< 最小 LOD / Minimum LOD
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                                         ///< 最大 LOD / Maximum LOD
        samplerDesc.ShaderRegister = registerIndex;                                      ///< 寄存器索引 / Register index
        samplerDesc.RegisterSpace = space;                                               ///< 寄存器空间 / Register space
        samplerDesc.Flags = D3D12_SAMPLER_FLAG_NONE;                                     ///< 无标志 / No flags

        StaticSamplers.push_back(samplerDesc);
    }

    /**
     * @brief 构建根签名 / Build the root signature
     *
     * 使用已添加的参数构建并创建 DX12RootSignature 对象。
     * Builds and creates a DX12RootSignature object using the added parameters.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @return 创建的根签名对象指针，失败返回 nullptr
     *         Created root signature object pointer, nullptr on failure
     */
    DX12RootSignature* DX12RootSignatureBuilder::Build(ID3D12Device* device)
    {
        // 配置版本化根签名描述
        // Configure versioned root signature description
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(Parameters.size());
        rootSigDesc.Desc_1_1.pParameters = Parameters.empty() ? nullptr : Parameters.data();
        rootSigDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(StaticSamplers.size());
        rootSigDesc.Desc_1_1.pStaticSamplers = StaticSamplers.empty() ? nullptr : reinterpret_cast<const ::D3D12_STATIC_SAMPLER_DESC*>(StaticSamplers.data());
        rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // 序列化根签名
        // Serialize root signature
        Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &serializedRootSig, &errorBlob);
        if (FAILED(hr))
        {
            // 如果有错误信息，记录日志
            // If there is error info, log it
            if (errorBlob)
            {
                DX12LogError("[DX12RootSignatureBuilder] Serialization failed\n");
            }
            return nullptr;
        }

        // 返回新创建的根签名对象
        // Return newly created root signature object
        return new DX12RootSignature(device, Parameters);
    }

    /**
     * @brief 重置构建器 / Reset the builder
     *
     * 清除所有已添加的参数和状态，准备构建新的根签名。
     * Clears all added parameters and state, ready to build a new root signature.
     */
    void DX12RootSignatureBuilder::Reset()
    {
        Parameters.clear();
        StaticSamplers.clear();
        CBVRegisters.clear();
        SRVRegisters.clear();
        SamplerRegisters.clear();
        CurrentParameterIndex = 0;
    }
}

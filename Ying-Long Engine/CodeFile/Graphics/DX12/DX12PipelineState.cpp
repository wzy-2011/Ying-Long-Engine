/**
 * @file DX12PipelineState.cpp
 * @brief DX12 管线状态对象实现文件 / DX12 Pipeline State Implementation
 *
 * 本文件实现了 DX12PipelineState 类的功能，包括 PSO 的创建、
 * 着色器加载、各种状态配置等。
 *
 * This file implements the DX12PipelineState class functionality, including
 * PSO creation, shader loading, various state configurations, etc.
 */

#include "DX12PipelineState.h"
#include "DX12Core.h"
#include "DX12RootSignature.h"
#include "../../Debug/DX12Log.h"
#include <d3dcompiler.h>
#include <fstream>
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 初始化 PSO 的各个状态组件为零，并设置默认的图元拓扑。
     * Initializes each PSO state component to zero and sets default primitive topology.
     */
    DX12PipelineState::DX12PipelineState()
        : PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)  ///< 默认三角形列表 / Default triangle list
        , PrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)  ///< 默认三角形拓扑 / Default triangle topology
        , RTVCount(0)
        , DSVFormat(DXGI_FORMAT_D24_UNORM_S8_UINT)
        , bUseCustomRTVFormats(false)
        , IsCustomPSO(false)                                       ///< 非自定义 PSO / Not custom PSO
    {
        // 初始化描述和布局为零
        // Initialize desc and layout to zero
        ZeroMemory(&PSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        ZeroMemory(&InputLayout, sizeof(D3D12_INPUT_LAYOUT_DESC));
        ZeroMemory(RTVFormatsArr, sizeof(RTVFormatsArr));

        // 设置默认渲染状态（而非清零，避免无效状态）
        // Set default render states (instead of zeroing, to avoid invalid states)
        RasterizerState = CreateDefaultRasterizerState();
        BlendState = CreateDefaultBlendState();
        DepthStencilState = CreateDefaultDepthStencilState();
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 释放 PSO 对象和着色器字节码资源。
     * Releases PSO object and shader bytecode resources.
     */
    DX12PipelineState::~DX12PipelineState()
    {
        // 释放 PSO 对象
        // Release PSO object
        pPSO.Reset();

        // 清除着色器字节码
        // Clear shader bytecode
        VertexShaderBytecode.clear();
        PixelShaderBytecode.clear();
        InputElements.clear();
    }

    /**
     * @brief 使用默认设置初始化 PSO / Initialize PSO with default settings
     *
     * 设置默认的光栅化、混合、深度模板状态，创建 PBR 输入布局，
     * 设置占位符着色器，最后创建 PSO 对象。
     *
     * Sets default rasterizer, blend, depth stencil states, creates PBR input layout,
     * sets placeholder shaders, and finally creates the PSO object.
     *
     * @param core DX12 核心对象引用 / DX12 core object reference
     * @param rootSignature 根签名指针 / Root signature pointer
     */
    void DX12PipelineState::Initialize(DX12Core& core, DX12RootSignature* rootSignature)
    {
        // 设置默认状态
        // Set default states
        RasterizerState = CreateDefaultRasterizerState();
        BlendState = CreateDefaultBlendState();
        DepthStencilState = CreateDefaultDepthStencilState();

        // 创建默认 PBR 输入布局
        // Create default PBR input layout
        UINT elementCount = 0;
        D3D12_INPUT_ELEMENT_DESC* elements = CreatePBRInputLayout(elementCount);
        InputElements.assign(elements, elements + elementCount);
        delete[] elements;

        // 配置输入布局描述
        // Configure input layout description
        InputLayout.pInputElementDescs = InputElements.data();
        InputLayout.NumElements = static_cast<UINT>(InputElements.size());

        // 使用占位符着色器作为备用
        // Use placeholder shaders as fallback
        SetPlaceholderShaders();

        // 创建 PSO 对象
        // Create PSO object
        CreatePSO(core, rootSignature);
    }

    /**
     * @brief 使用自定义设置初始化 PSO / Initialize PSO with custom settings
     *
     * 直接使用提供的 PSO 描述结构体创建 PSO 对象。
     * Creates the PSO object directly using the provided PSO description structure.
     *
     * @param core DX12 核心对象引用 / DX12 core object reference
     * @param rootSignature 根签名指针 / Root signature pointer
     * @param psoDesc PSO 描述结构体 / PSO description structure
     */
    void DX12PipelineState::Initialize(
        DX12Core& core,
        DX12RootSignature* rootSignature,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
    {
        PSODesc = psoDesc;
        IsCustomPSO = true;

        // 直接创建图形管线状态
        // Create graphics pipeline state directly
        core.GetDevice()->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&pPSO));
    }

    /**
     * @brief 从着色器初始化 PSO / Initialize PSO from shaders
     *
     * 使用提供的顶点着色器和像素着色器字节码，
     * 以及输入布局来创建 PSO。
     *
     * Creates the PSO using the provided vertex and pixel shader bytecode,
     * and input layout.
     *
     * @param core DX12 核心对象引用 / DX12 core object reference
     * @param rootSignature 根签名指针 / Root signature pointer
     * @param vertexShaderBytecode 顶点着色器字节码 / Vertex shader bytecode
     * @param pixelShaderBytecode 像素着色器字节码 / Pixel shader bytecode
     * @param inputLayout 输入布局描述数组 / Input layout description array
     * @param inputLayoutSize 输入布局元素数量 / Number of input layout elements
     */
    void DX12PipelineState::InitializeFromShaders(
        DX12Core& core,
        DX12RootSignature* rootSignature,
        const std::vector<uint8_t>& vertexShaderBytecode,
        const std::vector<uint8_t>& pixelShaderBytecode,
        const D3D12_INPUT_ELEMENT_DESC* inputLayout,
        UINT inputLayoutSize)
    {
        // 保存着色器字节码
        // Save shader bytecode
        VertexShaderBytecode = vertexShaderBytecode;
        PixelShaderBytecode = pixelShaderBytecode;

        // 如果提供了输入布局，设置它
        // If input layout is provided, set it
        if (inputLayout && inputLayoutSize > 0)
        {
            InputElements.assign(inputLayout, inputLayout + inputLayoutSize);
            InputLayout.pInputElementDescs = InputElements.data();
            InputLayout.NumElements = inputLayoutSize;
        }

        // 不重置光栅化/混合/深度状态，保留构造函数设置的默认值或调用者通过
        // SetRasterizerState/SetBlendState/SetDepthStencilState 设置的自定义值
        // Do NOT reset rasterizer/blend/depth states - preserve defaults from constructor
        // or custom values set by caller via SetRasterizerState/SetBlendState/SetDepthStencilState

        // 创建 PSO 对象
        // Create PSO object
        CreatePSO(core, rootSignature);
    }

    /**
     * @brief 从文件加载着色器字节码 / Load shader bytecode from files
     *
     * 从二进制文件加载顶点着色器和像素着色器字节码（.cso 文件）。
     * Loads vertex and pixel shader bytecode from binary files (.cso files).
     *
     * @param vertexShaderPath 顶点着色器文件路径 / Vertex shader file path
     * @param pixelShaderPath 像素着色器文件路径 / Pixel shader file path
     */
    void DX12PipelineState::LoadShadersFromFile(
        const std::string& vertexShaderPath,
        const std::string& pixelShaderPath)
    {
        // 加载顶点着色器
        // Load vertex shader
        std::ifstream vsFile(vertexShaderPath, std::ios::binary);
        if (vsFile)
        {
            VertexShaderBytecode.assign(
                std::istreambuf_iterator<char>(vsFile),
                std::istreambuf_iterator<char>()
            );
            vsFile.close();
        }

        // 加载像素着色器
        // Load pixel shader
        std::ifstream psFile(pixelShaderPath, std::ios::binary);
        if (psFile)
        {
            PixelShaderBytecode.assign(
                std::istreambuf_iterator<char>(psFile),
                std::istreambuf_iterator<char>()
            );
            psFile.close();
        }
    }

    /**
     * @brief 将 PSO 绑定到命令列表 / Bind PSO to command list
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     */
    void DX12PipelineState::Bind(ID3D12GraphicsCommandList* commandList) noexcept
    {
        if (pPSO)
        {
            commandList->SetPipelineState(pPSO.Get());
        }
    }

    /**
     * @brief 设置输入布局 / Set input layout
     * @param inputElements 输入元素描述数组 / Input element description array
     * @param count 元素数量 / Element count
     */
    void DX12PipelineState::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* inputElements, UINT count)
    {
        InputElements.assign(inputElements, inputElements + count);
        InputLayout.pInputElementDescs = InputElements.data();
        InputLayout.NumElements = count;
    }

    /**
     * @brief 设置光栅化状态 / Set rasterizer state
     * @param rasterizerDesc 光栅化状态描述 / Rasterizer state description
     */
    void DX12PipelineState::SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
    {
        RasterizerState = rasterizerDesc;
    }

    /**
     * @brief 设置混合状态 / Set blend state
     * @param blendDesc 混合状态描述 / Blend state description
     */
    void DX12PipelineState::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
    {
        BlendState = blendDesc;
    }

    /**
     * @brief 设置深度模板状态 / Set depth stencil state
     * @param depthStencilDesc 深度模板状态描述 / Depth stencil state description
     */
    void DX12PipelineState::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
    {
        DepthStencilState = depthStencilDesc;
    }

    /**
     * @brief 设置图元拓扑类型 / Set primitive topology type
     * @param topology 图元拓扑 / Primitive topology
     */
    void DX12PipelineState::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
    {
        PrimitiveTopology = topology;
    }

    /**
     * @brief 设置 PSO 图元拓扑类型 / Set PSO primitive topology type
     * @param topologyType PSO 图元拓扑类型 / PSO primitive topology type
     */
    void DX12PipelineState::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
    {
        PrimitiveTopologyType = topologyType;
    }

    void DX12PipelineState::SetRenderTargetFormats(const DXGI_FORMAT* formats, UINT count, DXGI_FORMAT dsvFormat)
    {
        if (count > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
            count = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

        for (UINT i = 0; i < count; ++i)
            RTVFormatsArr[i] = formats[i];
        RTVCount = count;
        DSVFormat = dsvFormat;
        bUseCustomRTVFormats = true;
    }

    /**
     * @brief 创建 PSO 对象 / Create the PSO object
     *
     * 填充 PSO 描述结构体的各个字段，然后调用 D3D12 设备
     * 创建图形管线状态对象。
     *
     * Fills each field of the PSO description structure, then calls the D3D12
     * device to create the graphics pipeline state object.
     *
     * @param core DX12 核心对象引用 / DX12 core object reference
     * @param rootSignature 根签名指针 / Root signature pointer
     * @throws std::runtime_error 如果着色器字节码为空 / If shader bytecode is empty
     */
    void DX12PipelineState::CreatePSO(DX12Core& core, DX12RootSignature* rootSignature)
    {
        // 验证顶点着色器字节码
        // Validate vertex shader bytecode
        if (VertexShaderBytecode.empty())
        {
            throw std::runtime_error("DX12PipelineState::CreatePSO - Vertex shader bytecode is empty");
        }
        // 验证像素着色器字节码
        // Validate pixel shader bytecode
        if (PixelShaderBytecode.empty())
        {
            throw std::runtime_error("DX12PipelineState::CreatePSO - Pixel shader bytecode is empty");
        }

        // 填充 PSO 描述结构体
        // Fill out PSO description
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        // 根签名
        // Root signature
        psoDesc.pRootSignature = rootSignature->GetRootSignature();

        // 输入布局
        // Input layout
        psoDesc.InputLayout = InputLayout;

        // 顶点着色器
        // Vertex shader
        psoDesc.VS.pShaderBytecode = VertexShaderBytecode.data();
        psoDesc.VS.BytecodeLength = VertexShaderBytecode.size();

        // 像素着色器
        // Pixel shader
        psoDesc.PS.pShaderBytecode = PixelShaderBytecode.data();
        psoDesc.PS.BytecodeLength = PixelShaderBytecode.size();

        // 光栅化状态
        // Rasterizer state
        psoDesc.RasterizerState = RasterizerState;

        // 混合状态
        // Blend state
        psoDesc.BlendState = BlendState;

        // 深度模板状态
        // Depth stencil state
        psoDesc.DepthStencilState = DepthStencilState;

        // 图元拓扑类型（使用成员变量，支持三角形和线拓扑）
        // Primitive topology type (uses member variable, supports triangle and line topology)
        psoDesc.PrimitiveTopologyType = PrimitiveTopologyType;

        // 渲染目标格式
        // Render target format - use custom formats if set (for MRT), otherwise default single RT
        if (bUseCustomRTVFormats && RTVCount > 0)
        {
            psoDesc.NumRenderTargets = RTVCount;
            for (UINT i = 0; i < RTVCount; ++i)
                psoDesc.RTVFormats[i] = RTVFormatsArr[i];
            psoDesc.DSVFormat = DSVFormat;
        }
        else
        {
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        }

        // 采样描述
        // Sample description
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.SampleMask = UINT_MAX;

        DX12Log("[DX12PipelineState] Creating PSO\n");

        // 创建图形管线状态对象
        // Create graphics pipeline state object
        HRESULT hr = core.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPSO));
        if (FAILED(hr))
        {
            DX12LogError("[DX12PipelineState] CreatePSO failed\n");
            pPSO.Reset();
        }
        else
        {
            DX12LogSuccess("[DX12PipelineState] PSO created successfully\n");
        }
    }

    /**
     * @brief 创建默认光栅化状态 / Create default rasterizer state
     *
     * 创建默认的光栅化状态配置：
     * - 实体填充模式
     * - 背面剔除
     * - 顺时针为正面
     * - 启用深度裁剪
     * - 禁用多重采样
     *
     * Creates default rasterizer state configuration:
     * - Solid fill mode
     * - Back face culling
     * - Clockwise front face
     * - Depth clip enabled
     * - Multisample disabled
     *
     * @return 默认光栅化状态描述 / Default rasterizer state description
     */
    D3D12_RASTERIZER_DESC DX12PipelineState::CreateDefaultRasterizerState()
    {
        D3D12_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;                    ///< 实体填充 / Solid fill
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;                     ///< 背面剔除 / Back face culling
        rasterizerDesc.FrontCounterClockwise = FALSE;                        ///< 顺时针为正面 / Clockwise is front
        rasterizerDesc.DepthBias = 0;                                        ///< 深度偏移 / Depth bias
        rasterizerDesc.DepthBiasClamp = 0.0f;                                ///< 深度偏移钳制 / Depth bias clamp
        rasterizerDesc.SlopeScaledDepthBias = 0.0f;                          ///< 斜率缩放深度偏移 / Slope scaled depth bias
        rasterizerDesc.DepthClipEnable = TRUE;                               ///< 启用深度裁剪 / Enable depth clip
        rasterizerDesc.MultisampleEnable = FALSE;                            ///< 禁用多重采样 / Disable multisample
        rasterizerDesc.AntialiasedLineEnable = FALSE;                        ///< 禁用抗锯齿线 / Disable antialiased lines
        rasterizerDesc.ForcedSampleCount = 0;                                ///< 强制采样数 / Forced sample count
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;  ///< 保守光栅化关闭 / Conservative rasterization off

        return rasterizerDesc;
    }

    /**
     * @brief 创建默认混合状态 / Create default blend state
     *
     * 创建默认的混合状态配置：
     * - 禁用 alpha-to-coverage
     * - 禁用独立混合
     * - 禁用混合
     * - 所有颜色通道可写
     *
     * Creates default blend state configuration:
     * - Alpha-to-coverage disabled
     * - Independent blend disabled
     * - Blending disabled
     * - All color channels writable
     *
     * @return 默认混合状态描述 / Default blend state description
     */
    D3D12_BLEND_DESC DX12PipelineState::CreateDefaultBlendState()
    {
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;                             ///< 禁用 alpha to coverage / Disable alpha to coverage
        blendDesc.IndependentBlendEnable = FALSE;                            ///< 禁用独立混合 / Disable independent blend

        D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
        rtBlendDesc.BlendEnable = FALSE;                                     ///< 禁用混合 / Disable blending
        rtBlendDesc.LogicOpEnable = FALSE;                                   ///< 禁用逻辑操作 / Disable logic op
        rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;                              ///< 源混合因子（1）/ Source blend factor (one)
        rtBlendDesc.DestBlend = D3D12_BLEND_ZERO;                            ///< 目标混合因子（0）/ Dest blend factor (zero)
        rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;                            ///< 混合操作（相加）/ Blend op (add)
        rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;                         ///< 源 alpha 混合因子 / Source alpha blend factor
        rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;                       ///< 目标 alpha 混合因子 / Dest alpha blend factor
        rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;                       ///< alpha 混合操作（相加）/ Alpha blend op (add)
        rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;                           ///< 逻辑操作（无操作）/ Logic op (no-op)
        rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;   ///< 所有颜色通道可写 / All color channels writable

        blendDesc.RenderTarget[0] = rtBlendDesc;

        return blendDesc;
    }

    /**
     * @brief 创建默认深度模板状态 / Create default depth stencil state
     *
     * 创建默认的深度模板状态配置：
     * - 启用深度测试
     * - 启用深度写入
     * - 深度测试函数：小于
     * - 禁用模板测试
     *
     * Creates default depth stencil state configuration:
     * - Depth test enabled
     * - Depth write enabled
     * - Depth test function: less
     * - Stencil test disabled
     *
     * @return 默认深度模板状态描述 / Default depth stencil state description
     */
    D3D12_DEPTH_STENCIL_DESC DX12PipelineState::CreateDefaultDepthStencilState()
    {
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;                                 ///< 启用深度测试 / Enable depth test
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;        ///< 启用深度写入 / Enable depth write
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;             ///< 深度测试函数：小于 / Depth func: less
        depthStencilDesc.StencilEnable = FALSE;                              ///< 禁用模板测试 / Disable stencil test
        depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;  ///< 模板读取掩码 / Stencil read mask
        depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; ///< 模板写入掩码 / Stencil write mask

        // 正面模板操作
        // Front face stencil operations
        D3D12_DEPTH_STENCILOP_DESC frontFace = {};
        frontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;                     ///< 模板失败：保持 / Stencil fail: keep
        frontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;                ///< 深度失败：保持 / Depth fail: keep
        frontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;                     ///< 模板通过：保持 / Stencil pass: keep
        frontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;                 ///< 模板函数：总是 / Stencil func: always

        // 背面模板操作
        // Back face stencil operations
        D3D12_DEPTH_STENCILOP_DESC backFace = {};
        backFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        backFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        backFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        backFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        depthStencilDesc.FrontFace = frontFace;
        depthStencilDesc.BackFace = backFace;

        return depthStencilDesc;
    }

    /**
     * @brief 创建 PBR 着色器的默认输入布局 / Create default input layout for PBR shaders
     *
     * 创建包含三个元素的 PBR 输入布局：
     * - Position: float3（位置）
     * - Normal: float3（法线）
     * - TextureCoord: float2（纹理坐标）
     *
     * Creates a PBR input layout containing three elements:
     * - Position: float3 (position)
     * - Normal: float3 (normal)
     * - TextureCoord: float2 (texture coordinate)
     *
     * @param outElementCount 输出元素数量 / Output element count
     * @return 输入元素描述数组指针（调用者负责用 delete[] 释放）
     *         Input element description array pointer (caller responsible for delete[])
     */
    D3D12_INPUT_ELEMENT_DESC* DX12PipelineState::CreatePBRInputLayout(UINT& outElementCount)
    {
        outElementCount = 3;

        D3D12_INPUT_ELEMENT_DESC* elements = new D3D12_INPUT_ELEMENT_DESC[outElementCount];

        // Position - 顶点位置
        // Position - vertex position
        elements[0].SemanticName = "Position";
        elements[0].SemanticIndex = 0;
        elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        elements[0].InputSlot = 0;
        elements[0].AlignedByteOffset = 0;
        elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[0].InstanceDataStepRate = 0;

        // Normal - 顶点法线
        // Normal - vertex normal
        elements[1].SemanticName = "Normal";
        elements[1].SemanticIndex = 0;
        elements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        elements[1].InputSlot = 0;
        elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[1].InstanceDataStepRate = 0;

        // TextureCoord - 纹理坐标
        // TextureCoord - texture coordinate
        elements[2].SemanticName = "TextureCoord";
        elements[2].SemanticIndex = 0;
        elements[2].Format = DXGI_FORMAT_R32G32_FLOAT;
        elements[2].InputSlot = 0;
        elements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[2].InstanceDataStepRate = 0;

        return elements;
    }

    /**
     * @brief 设置占位符着色器 / Set placeholder shaders
     *
     * 编译并设置简单的占位符着色器，在没有加载实际着色器时
     * 作为备用。顶点着色器实现基本的 MVP 变换，像素着色器
     * 输出固定的灰色。
     *
     * Compiles and sets simple placeholder shaders as fallback when no actual
     * shaders are loaded. The vertex shader implements basic MVP transform,
     * and the pixel shader outputs a fixed gray color.
     */
    void DX12PipelineState::SetPlaceholderShaders()
    {
        // 占位符顶点着色器源码
        // Placeholder vertex shader source
        const char* vsSource =
            "cbuffer TransformCB : register(b3) { matrix Model; matrix MVP; }\n"
            "struct VS_IN { float3 Pos : Position; float3 Norm : Normal; float2 Tex : TextureCoord; };\n"
            "struct VS_OUT { float3 WPos : POSITION; float3 Norm : NORMAL; float4 Pos : SV_Position; float2 Tex : TextureCoord; };\n"
            "VS_OUT main(VS_IN i) { VS_OUT o; o.Pos = mul(float4(i.Pos,1), MVP); o.Norm = mul(float4(i.Norm,0), Model).xyz; o.Tex = i.Tex; o.WPos = mul(float4(i.Pos,1), Model).xyz; return o; }\n";

        // 占位符像素着色器源码（输出灰色）
        // Placeholder pixel shader source (outputs gray)
        const char* psSource =
            "struct PS_IN { float3 WPos : POSITION; float3 Norm : NORMAL; float4 Pos : SV_Position; float2 Tex : TextureCoord; };\n"
            "float4 main(PS_IN i) : SV_Target { return float4(0.6, 0.6, 0.6, 1.0); }\n";

        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

        // 编译顶点着色器
        // Compile vertex shader
        HRESULT hr = D3DCompile(vsSource, strlen(vsSource), "placeholder_vs",
            nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
        if (SUCCEEDED(hr) && vsBlob)
        {
            VertexShaderBytecode.assign(
                (uint8_t*)vsBlob->GetBufferPointer(),
                (uint8_t*)vsBlob->GetBufferPointer() + vsBlob->GetBufferSize());
        }

        // 编译像素着色器
        // Compile pixel shader
        errorBlob.Reset();
        hr = D3DCompile(psSource, strlen(psSource), "placeholder_ps",
            nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (SUCCEEDED(hr) && psBlob)
        {
            PixelShaderBytecode.assign(
                (uint8_t*)psBlob->GetBufferPointer(),
                (uint8_t*)psBlob->GetBufferPointer() + psBlob->GetBufferSize());
        }
    }
}

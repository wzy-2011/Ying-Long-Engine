/**
 * @file DX12PipelineState.h
 * @brief DX12 管线状态对象头文件 / DX12 Pipeline State Header
 *
 * 本文件定义了 DX12PipelineState 类，用于创建和管理
 * D3D12 图形管线状态对象（PSO），包括输入布局、光栅化状态、
 * 混合状态、深度模板状态等配置。
 *
 * This file defines the DX12PipelineState class, which is used to create and
 * manage D3D12 graphics pipeline state objects (PSO), including input layout,
 * rasterizer state, blend state, depth stencil state, etc.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace YingLong
{
    class DX12Core;
    class DX12RootSignature;

    /**
     * @brief DX12 管线状态类 / DX12 Pipeline State Class
     *
     * DX12PipelineState 类封装了 D3D12 图形管线状态对象（PSO），提供：
     * - 默认 PBR 管线状态的创建
     * - 自定义管线状态的创建和配置
     * - 着色器字节码的加载
     * - 各种管线状态组件的设置和查询
     * - 绑定到命令列表
     *
     * The DX12PipelineState class encapsulates the D3D12 graphics pipeline state
     * object (PSO), providing:
     * - Default PBR pipeline state creation
     * - Custom pipeline state creation and configuration
     * - Shader bytecode loading
     * - Various pipeline state component settings and queries
     * - Binding to command list
     */
    class DX12PipelineState
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 创建一个未初始化的管线状态对象。
         * Creates an uninitialized pipeline state object.
         */
        DX12PipelineState();

        /**
         * @brief 析构函数 / Destructor
         *
         * 释放管线状态资源。
         * Releases pipeline state resources.
         */
        ~DX12PipelineState();

        /**
         * @brief 使用默认设置初始化 PSO / Initialize PSO with default settings
         *
         * 使用默认的光栅化状态、混合状态、深度模板状态和
         * PBR 输入布局初始化管线状态对象。
         *
         * Initializes the pipeline state object with default rasterizer state,
         * blend state, depth stencil state, and PBR input layout.
         *
         * @param core DX12 核心对象引用 / DX12 core object reference
         * @param rootSignature 根签名指针 / Root signature pointer
         */
        void Initialize(DX12Core& core, DX12RootSignature* rootSignature);

        /**
         * @brief 使用自定义设置初始化 PSO / Initialize PSO with custom settings
         *
         * 根据提供的 PSO 描述结构体直接创建管线状态对象。
         *
         * Creates the pipeline state object directly from the provided PSO description.
         *
         * @param core DX12 核心对象引用 / DX12 core object reference
         * @param rootSignature 根签名指针 / Root signature pointer
         * @param psoDesc PSO 描述结构体 / PSO description structure
         */
        void Initialize(
            DX12Core& core,
            DX12RootSignature* rootSignature,
            const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc
        );

        /**
         * @brief 从着色器初始化 PSO / Initialize PSO from shaders
         *
         * 使用提供的顶点着色器和像素着色器字节码初始化 PSO。
         *
         * Initializes the PSO using the provided vertex and pixel shader bytecode.
         *
         * @param core DX12 核心对象引用 / DX12 core object reference
         * @param rootSignature 根签名指针 / Root signature pointer
         * @param vertexShaderBytecode 顶点着色器字节码 / Vertex shader bytecode
         * @param pixelShaderBytecode 像素着色器字节码 / Pixel shader bytecode
         * @param inputLayout 输入布局描述数组 / Input layout description array
         * @param inputLayoutSize 输入布局元素数量 / Number of input layout elements
         */
        void InitializeFromShaders(
            DX12Core& core,
            DX12RootSignature* rootSignature,
            const std::vector<uint8_t>& vertexShaderBytecode,
            const std::vector<uint8_t>& pixelShaderBytecode,
            const D3D12_INPUT_ELEMENT_DESC* inputLayout,
            UINT inputLayoutSize
        );

        /**
         * @brief 从文件加载着色器字节码 / Load shader bytecode from files
         *
         * 从编译后的着色器文件（.cso）加载顶点着色器和像素着色器字节码。
         *
         * Loads vertex and pixel shader bytecode from compiled shader files (.cso).
         *
         * @param vertexShaderPath 顶点着色器文件路径 / Vertex shader file path
         * @param pixelShaderPath 像素着色器文件路径 / Pixel shader file path
         */
        void LoadShadersFromFile(
            const std::string& vertexShaderPath,
            const std::string& pixelShaderPath
        );

        /**
         * @brief 获取 PSO 对象 / Get the PSO object
         * @return ID3D12PipelineState 指针 / ID3D12PipelineState pointer
         */
        ID3D12PipelineState* GetPSO() const noexcept { return pPSO.Get(); }

        /**
         * @brief 将 PSO 绑定到命令列表 / Bind PSO to command list
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Bind(ID3D12GraphicsCommandList* commandList) noexcept;

        /**
         * @brief 设置输入布局 / Set input layout
         * @param inputElements 输入元素描述数组 / Input element description array
         * @param count 元素数量 / Element count
         */
        void SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* inputElements, UINT count);

        /**
         * @brief 设置光栅化状态 / Set rasterizer state
         * @param rasterizerDesc 光栅化状态描述 / Rasterizer state description
         */
        void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc);

        /**
         * @brief 设置混合状态 / Set blend state
         * @param blendDesc 混合状态描述 / Blend state description
         */
        void SetBlendState(const D3D12_BLEND_DESC& blendDesc);

        /**
         * @brief 设置深度模板状态 / Set depth stencil state
         * @param depthStencilDesc 深度模板状态描述 / Depth stencil state description
         */
        void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);

        /**
         * @brief 设置图元拓扑类型 / Set primitive topology type
         * @param topology 图元拓扑 / Primitive topology
         */
        void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);

        /**
         * @brief 设置 PSO 图元拓扑类型 / Set PSO primitive topology type
         * @param topologyType PSO 图元拓扑类型 / PSO primitive topology type
         */
        void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);

        /**
         * @brief 设置渲染目标格式 / Set render target formats
         *
         * 配置 PSO 的渲染目标格式。支持单 RT 和多 RT (MRT)。
         * Configures the PSO's render target formats. Supports single and multiple RTs (MRT).
         *
         * @param formats 格式数组指针 / Pointer to format array
         * @param count RT 数量 / Number of render targets
         * @param dsvFormat 深度模板格式 / Depth stencil format
         */
        void SetRenderTargetFormats(const DXGI_FORMAT* formats, UINT count, DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

        /**
         * @brief 获取输入布局描述 / Get input layout description
         * @return 输入布局描述常量引用 / Input layout description const reference
         */
        const D3D12_INPUT_LAYOUT_DESC& GetInputLayout() const noexcept { return InputLayout; }

        /**
         * @brief 获取光栅化状态描述 / Get rasterizer state description
         * @return 光栅化状态描述常量引用 / Rasterizer state description const reference
         */
        const D3D12_RASTERIZER_DESC& GetRasterizerState() const noexcept { return RasterizerState; }

        /**
         * @brief 检查是否已初始化 / Check if initialized
         * @return 是否已初始化 / Whether initialized
         */
        bool IsInitialized() const noexcept { return pPSO != nullptr; }

        /**
         * @brief 创建默认光栅化状态 / Create default rasterizer state
         *
         * 创建默认的光栅化状态（实体填充、背面剔除、启用深度裁剪）。
         * Creates default rasterizer state (solid fill, back face culling, depth clip enabled).
         *
         * @return 默认光栅化状态描述 / Default rasterizer state description
         */
        static D3D12_RASTERIZER_DESC CreateDefaultRasterizerState();

        /**
         * @brief 创建默认混合状态 / Create default blend state
         *
         * 创建默认的混合状态（禁用混合，所有颜色通道可写）。
         * Creates default blend state (blending disabled, all color channels writable).
         *
         * @return 默认混合状态描述 / Default blend state description
         */
        static D3D12_BLEND_DESC CreateDefaultBlendState();

        /**
         * @brief 创建默认深度模板状态 / Create default depth stencil state
         *
         * 创建默认的深度模板状态（启用深度写入、深度测试函数为小于、禁用模板）。
         * Creates default depth stencil state (depth write enabled, depth test less, stencil disabled).
         *
         * @return 默认深度模板状态描述 / Default depth stencil state description
         */
        static D3D12_DEPTH_STENCIL_DESC CreateDefaultDepthStencilState();

        /**
         * @brief 创建 PBR 着色器的默认输入布局 / Create default input layout for PBR shaders
         *
         * 创建包含位置、法线、纹理坐标的 PBR 输入布局。
         * Creates a PBR input layout containing position, normal, and texture coordinates.
         *
         * @param outElementCount 输出元素数量 / Output element count
         * @return 输入元素描述数组指针（调用者负责释放）/ Input element description array pointer (caller responsible for release)
         */
        static D3D12_INPUT_ELEMENT_DESC* CreatePBRInputLayout(UINT& outElementCount);

    private:
        /**
         * @brief 创建 PSO 对象 / Create the PSO object
         *
         * 根据当前配置的状态创建实际的 D3D12 管线状态对象。
         * Creates the actual D3D12 pipeline state object based on currently configured states.
         *
         * @param core DX12 核心对象引用 / DX12 core object reference
         * @param rootSignature 根签名指针 / Root signature pointer
         */
        void CreatePSO(DX12Core& core, DX12RootSignature* rootSignature);

        /**
         * @brief 设置占位符着色器 / Set placeholder shaders
         *
         * 设置占位符着色器作为备用，在没有加载实际着色器时使用。
         * Sets placeholder shaders as fallback when no actual shaders are loaded.
         */
        void SetPlaceholderShaders();

        Microsoft::WRL::ComPtr<ID3D12PipelineState> pPSO;  ///< D3D12 管线状态对象 / D3D12 pipeline state object

        // PSO components
        // PSO 组件
        D3D12_INPUT_LAYOUT_DESC InputLayout;                  ///< 输入布局描述 / Input layout description
        D3D12_RASTERIZER_DESC RasterizerState;                ///< 光栅化状态 / Rasterizer state
        D3D12_BLEND_DESC BlendState;                          ///< 混合状态 / Blend state
        D3D12_DEPTH_STENCIL_DESC DepthStencilState;           ///< 深度模板状态 / Depth stencil state
        D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology;             ///< 图元拓扑 / Primitive topology
D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType; ///< PSO 图元拓扑类型 / PSO primitive topology type

        // Shader bytecode
        // 着色器字节码
        std::vector<uint8_t> VertexShaderBytecode;            ///< 顶点着色器字节码 / Vertex shader bytecode
        std::vector<uint8_t> PixelShaderBytecode;             ///< 像素着色器字节码 / Pixel shader bytecode

        // Input layout elements (owned)
        // 输入布局元素（拥有所有权）
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;  ///< 输入元素描述数组 / Input element description array

        // PSO desc
        // PSO 描述
        D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;           ///< PSO 描述结构体 / PSO description structure

        // Render target formats (for MRT support)
        // 渲染目标格式（用于 MRT 支持）
        DXGI_FORMAT RTVFormatsArr[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];  ///< 渲染目标格式数组 / RT format array
        UINT RTVCount;                                         ///< 渲染目标数量 / RT count
        DXGI_FORMAT DSVFormat;                                 ///< 深度模板格式 / DSV format
        bool bUseCustomRTVFormats;                             ///< 是否使用自定义 RTV 格式 / Whether using custom RTV formats

        bool IsCustomPSO;                                     ///< 是否为自定义 PSO / Whether it's a custom PSO
    };
}

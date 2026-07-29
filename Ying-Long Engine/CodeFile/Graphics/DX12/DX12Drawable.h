/**
 * @file DX12Drawable.h
 * @brief DX12 可绘制对象头文件 / DX12 Drawable Header
 *
 * 本文件定义了 DX12Drawable 类，是 DX12 渲染系统中的可绘制对象基类。
 * 它使用 Bindable 模式管理各种渲染资源（顶点缓冲区、索引缓冲区、
 * 常量缓冲区等），并提供绑定和绘制功能。
 *
 * This file defines the DX12Drawable class, which is the drawable object base class
 * in the DX12 rendering system. It manages various rendering resources (vertex buffer,
 * index buffer, constant buffer, etc.) using the Bindable pattern, and provides
 * binding and drawing functionality.
 */

#pragma once

#include <memory>
#include <vector>
#include "BindableDX12.h"
#include "DX12PipelineState.h"

namespace YingLong
{
    class DX12Core;

    /**
     * @brief DX12 可绘制对象类 / DX12 Drawable Class
     *
     * DX12Drawable 是 DX12 渲染系统中的可绘制对象类，提供：
     * - Bindable 资源的管理（添加、绑定）
     * - 管线状态对象的设置
     * - 图元拓扑的设置
     * - 绑定和绘制功能
     * - 支持索引绘制和非索引绘制
     *
     * 该类采用组合模式，通过添加各种 Bindable 资源来构建
     * 完整的可绘制对象。
     *
     * DX12Drawable is the drawable object class in the DX12 rendering system, providing:
     * - Bindable resource management (add, bind)
     * - Pipeline state object setting
     * - Primitive topology setting
     * - Binding and drawing functionality
     * - Support for indexed and non-indexed drawing
     *
     * This class uses the composition pattern, building complete drawable objects
     * by adding various Bindable resources.
     */
    class DX12Drawable
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 创建一个未初始化的可绘制对象。
         * Creates an uninitialized drawable object.
         */
        DX12Drawable();

        /**
         * @brief 析构函数 / Destructor
         *
         * 释放所有绑定的资源。
         * Releases all bound resources.
         */
        ~DX12Drawable();

        /**
         * @brief 初始化可绘制对象 / Initialize the drawable
         * @param core DX12 核心对象引用 / DX12 core object reference
         */
        void Initialize(DX12Core& core);

        /**
         * @brief 添加可绑定资源 / Add a bindable resource
         * @param bindable 可绑定资源的唯一指针 / Unique pointer to bindable resource
         */
        void AddBindable(std::unique_ptr<BindableDX12> bindable);

        /**
         * @brief 设置管线状态 / Set the pipeline state
         * @param pso 管线状态对象指针 / Pipeline state object pointer
         */
        void SetPipelineState(DX12PipelineState* pso);

        /**
         * @brief 设置覆盖管线状态 / Set override pipeline state
         *
         * 设置后将覆盖 drawable 自身的 PSO。用于延迟渲染 Geometry Pass
         * 时强制使用 GeometryPipelineState。传入 nullptr 可清除覆盖。
         *
         * When set, overrides the drawable's own PSO. Used during the
         * deferred rendering Geometry Pass to force GeometryPipelineState.
         * Pass nullptr to clear the override.
         *
         * @param pso 覆盖管线状态对象指针 / Override pipeline state pointer
         */
        void SetOverridePipelineState(DX12PipelineState* pso) noexcept { pOverridePSO = pso; }

        /**
         * @brief 绑定所有资源并绘制 / Bind all resources and draw
         *
         * 先绑定所有资源，然后执行绘制调用。
         * First binds all resources, then executes the draw call.
         *
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Draw(ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 仅绑定（用于实例化渲染）/ Bind only (for instanced rendering)
         *
         * 仅绑定所有资源，不执行绘制调用，用于实例化渲染等场景。
         * Only binds all resources without executing draw call, used for
         * instanced rendering and other scenarios.
         *
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Bind(ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 获取索引数量 / Get the number of indices
         *
         * 如果设置了索引缓冲区，返回索引数量。
         * Returns the number of indices if an index buffer is set.
         *
         * @return 索引数量 / Number of indices
         */
        UINT GetIndexCount() const noexcept { return IndexCount; }

        /**
         * @brief 设置索引数量 / Set index count
         *
         * 用于仅顶点渲染（无索引缓冲区）的情况。
         * Used for vertex-only rendering (no index buffer).
         *
         * @param count 索引/顶点数量 / Index/vertex count
         */
        void SetIndexCount(UINT count) noexcept { IndexCount = count; }

        /**
         * @brief 获取图元拓扑 / Get primitive topology
         * @return 图元拓扑 / Primitive topology
         */
        D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const noexcept { return PrimitiveTopology; }

        /**
         * @brief 设置图元拓扑 / Set primitive topology
         * @param topology 图元拓扑 / Primitive topology
         */
        void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology) noexcept { PrimitiveTopology = topology; }

        /**
         * @brief 检查是否已初始化 / Check if initialized
         * @return 是否已初始化 / Whether initialized
         */
        bool IsInitialized() const noexcept { return bInitialized; }

    private:
        // Bindables
        // 可绑定资源
        std::vector<std::unique_ptr<BindableDX12>> Bindables;  ///< 可绑定资源列表 / Bindable resource list

        // Pipeline state
        // 管线状态
        DX12PipelineState* pPSO;                               ///< 管线状态对象指针 / Pipeline state object pointer
        DX12PipelineState* pOverridePSO;                        ///< 覆盖管线状态指针（延迟渲染用）/ Override PSO (for deferred rendering)

        // Draw parameters
        // 绘制参数
        UINT IndexCount;                                       ///< 索引数量 / Index count
        D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology;              ///< 图元拓扑 / Primitive topology
        bool bInitialized;                                     ///< 是否已初始化 / Whether initialized
        bool bUseIndexBuffer;                                  ///< 是否使用索引缓冲区 / Whether to use index buffer
    };
}

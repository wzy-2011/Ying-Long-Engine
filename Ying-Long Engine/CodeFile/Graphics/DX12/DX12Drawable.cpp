/**
 * @file DX12Drawable.cpp
 * @brief DX12 可绘制对象实现文件 / DX12 Drawable Implementation
 *
 * 本文件实现了 DX12Drawable 类的功能，包括可绑定资源的管理、
 * 管线状态设置、绑定和绘制等。
 *
 * This file implements the DX12Drawable class functionality, including
 * bindable resource management, pipeline state setting, binding and drawing.
 */

#include "DX12Drawable.h"
#include "DX12Core.h"

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 初始化所有成员变量为默认值。
     * Initializes all member variables to default values.
     */
    DX12Drawable::DX12Drawable()
        : pPSO(nullptr)                       ///< 管线状态为空 / PSO is null
        , pOverridePSO(nullptr)               ///< 覆盖管线状态为空 / Override PSO is null
        , IndexCount(0)                       ///< 索引数量为0 / Index count is 0
        , PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)  ///< 默认三角形列表 / Default triangle list
        , bInitialized(false)                 ///< 未初始化 / Not initialized
        , bUseIndexBuffer(false)              ///< 不使用索引缓冲区 / Not using index buffer
    {
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 清除所有可绑定资源并释放管线状态引用。
     * Clears all bindable resources and releases PSO reference.
     */
    DX12Drawable::~DX12Drawable()
    {
        // 清除所有可绑定资源
        // Clear all bindable resources
        Bindables.clear();
        pPSO = nullptr;
        pOverridePSO = nullptr;
    }

    /**
     * @brief 初始化可绘制对象 / Initialize the drawable
     * @param core DX12 核心对象引用 / DX12 core object reference
     */
    void DX12Drawable::Initialize(DX12Core& core)
    {
        UNREFERENCED_PARAMETER(core);
        bInitialized = true;
    }

    /**
     * @brief 添加可绑定资源 / Add a bindable resource
     *
     * 将一个可绑定资源添加到可绘制对象中。
     * Adds a bindable resource to the drawable object.
     *
     * @param bindable 可绑定资源的唯一指针 / Unique pointer to bindable resource
     */
    void DX12Drawable::AddBindable(std::unique_ptr<BindableDX12> bindable)
    {
        Bindables.push_back(std::move(bindable));
    }

    /**
     * @brief 设置管线状态 / Set the pipeline state
     * @param pso 管线状态对象指针 / Pipeline state object pointer
     */
    void DX12Drawable::SetPipelineState(DX12PipelineState* pso)
    {
        pPSO = pso;
    }

    /**
     * @brief 绑定所有资源并绘制 / Bind all resources and draw
     *
     * 先调用 Bind() 绑定所有资源，然后根据是否使用索引缓冲区
     * 执行相应的绘制调用。
     *
     * First calls Bind() to bind all resources, then executes the appropriate
     * draw call based on whether an index buffer is used.
     *
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     */
    void DX12Drawable::Draw(ID3D12GraphicsCommandList* commandList)
    {
        // 先绑定所有资源
        // First bind all resources
        Bind(commandList);

        // 根据是否使用索引缓冲区选择绘制方式
        // Choose draw method based on whether index buffer is used
        if (bUseIndexBuffer && IndexCount > 0)
        {
            // 索引绘制 / Indexed draw
            commandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
        }
        else if (IndexCount > 0)
        {
            // 非索引绘制 / Non-indexed draw
            commandList->DrawInstanced(IndexCount, 1, 0, 0);
        }
    }

    /**
     * @brief 仅绑定资源 / Bind only
     *
     * 绑定管线状态、所有可绑定资源，并设置图元拓扑。
     * 不执行绘制调用，用于实例化渲染等场景。
     *
     * Binds pipeline state, all bindable resources, and sets primitive topology.
     * Does not execute draw call, used for instanced rendering and other scenarios.
     *
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     */
    void DX12Drawable::Bind(ID3D12GraphicsCommandList* commandList)
    {
        // 绑定管线状态（优先使用覆盖 PSO，用于延迟渲染 Geometry Pass）
        // Bind pipeline state (override PSO takes priority, for deferred rendering Geometry Pass)
        DX12PipelineState* activePSO = pOverridePSO ? pOverridePSO : pPSO;
        if (activePSO)
        {
            activePSO->Bind(commandList);
        }

        // 绑定所有可绑定资源
        // Bind all bindable resources
        for (auto& bindable : Bindables)
        {
            bindable->Bind(commandList);
        }

        // 设置图元拓扑
        // Set primitive topology
        commandList->IASetPrimitiveTopology(PrimitiveTopology);
    }
}

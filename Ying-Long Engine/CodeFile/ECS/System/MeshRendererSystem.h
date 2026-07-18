/**
 * @file MeshRendererSystem.h
 * @brief 网格渲染系统 / Mesh renderer system
 *
 * 渲染所有拥有 TransformComponent + MeshComponent 的实体。
 *
 * DX11 路径（RenderDX11）：
 *   - 如果设置了 MeshComponent::ModelPath，懒加载 Model 并调用 Model::Draw
 *   - 如果 ModelPath 为空，使用缓存的 SolidBoxDrawable 或 SolidSphereDrawable
 *     作为占位符，尺寸来自 ColliderComponent，颜色来自 MeshComponent::TintColor
 *
 * DX12 路径（RenderDX12）：
 *   - 完整的 DX12 Mesh 类尚不存在，因此每个实体使用 DX12Box 占位符
 *   - 盒子使用 TransformComponent 确定位置/旋转，碰撞体尺寸（或 Scale）确定大小
 *
 * Renders entities that have TransformComponent + MeshComponent.
 *
 * DX11 path (RenderDX11):
 *   - If MeshComponent::ModelPath is set, lazily loads Model and calls Model::Draw
 *   - If ModelPath is empty, falls back to a cached SolidBoxDrawable or
 *     SolidSphereDrawable placeholder sized from ColliderComponent and
 *     tinted from MeshComponent::TintColor.
 *
 * DX12 path (RenderDX12):
 *   - Full DX12 Mesh class doesn't exist yet, so we render a DX12Box
 *     placeholder per entity.
 */
#pragma once
#include "System.h"
#include "../Scene/Scene.h"
#include "../Components/Components.h"
#include "../Entity/Entity.h"
#include "../../entt-master/include/entt.hpp"
#include "../../Graphics/Drawable/SolidSphere.h"
#include "../../Graphics/Drawable/SolidBoxDrawable.h"
#include <unordered_map>
#include <memory>

// 前置声明以避免沉重的头文件包含
// Forward declarations to avoid heavy includes in header
struct ID3D12GraphicsCommandList;  // 全局 D3D12 类型，不在任何命名空间中 / global D3D12 type, not in any namespace

namespace YingLong
{
    class Graphics;
    class DX12Core;
    class DX12Box;

    struct DX12LightCountCB;      // 定义在 Graphics/DX12/DX12Primitives.h / defined in Graphics/DX12/DX12Primitives.h
    struct DX12PointLightData;    // 定义在 Graphics/DX12/DX12Primitives.h / defined in Graphics/DX12/DX12Primitives.h
    struct DX12SpotLightData;     // 定义在 Graphics/DX12/DX12Primitives.h / defined in Graphics/DX12/DX12Primitives.h

    /**
     * @brief 网格渲染系统 / Mesh renderer system
     *
     * 遍历所有拥有 TransformComponent + MeshComponent 的实体并渲染它们。
     * 支持 DX11 和 DX12 两种渲染路径，内部使用缓存避免重复创建渲染资源。
     *
     * Iterates all entities with TransformComponent + MeshComponent and renders them.
     * Supports both DX11 and DX12 render paths, uses internal caching to avoid
     * recreating render resources.
     */
    class MeshRendererSystem : public System
    {
    public:
        /**
         * @brief 默认构造函数 / Default constructor
         */
        MeshRendererSystem() = default;

        /**
         * @brief 析构函数 / Destructor
         *
         * 调用 ShutDown() 释放所有缓存的渲染资源。
         * Calls ShutDown() to release all cached render resources.
         */
        ~MeshRendererSystem() override;

        void Initialize() override {}

        /**
         * @brief 场景级更新 / Scene-level update
         *
         * 不进行模拟工作，仅从缓存中清理已销毁的实体。
         * No simulation work; just prunes destroyed entities from the caches.
         *
         * @param scene 场景引用 / Scene reference
         * @param DeltaTime 增量时间 / Delta time
         */
        void UpdateScene(Scene& scene, float DeltaTime) override;

        /**
         * @brief 系统关闭 / System shutdown
         *
         * 释放所有缓存的 DX11 和 DX12 渲染资源。
         * Releases all cached DX11 and DX12 render resources.
         */
        void ShutDown() override;

        /**
         * @brief DX11 渲染 / DX11 render
         *
         * 通过 Model::Draw 或纯色占位符渲染 MeshComponent 实体。
         * Renders MeshComponent entities via Model::Draw or solid-color placeholder.
         *
         * @param scene 场景引用 / Scene reference
         * @param gfx 图形设备 / Graphics device
         */
        void RenderDX11(Scene& scene, Graphics& gfx);

        /**
         * @brief DX12 渲染 / DX12 render
         *
         * 通过缓存的 DX12Box 占位符渲染 MeshComponent 实体。
         * viewMatrix / projMatrix 是 float[16]（行优先），来自 Camera。
         *
         * Renders MeshComponent entities via cached DX12Box placeholders.
         * viewMatrix / projMatrix are float[16] (row-major) from Camera.
         *
         * @param scene 场景引用 / Scene reference
         * @param core DX12 核心 / DX12 core
         * @param cmdList 命令列表 / Command list
         * @param viewMatrix 视图矩阵 / View matrix
         * @param projMatrix 投影矩阵 / Projection matrix
         * @param dt 增量时间 / Delta time
         */
        void RenderDX12(Scene& scene, DX12Core& core, ID3D12GraphicsCommandList* cmdList,
                        const float* viewMatrix, const float* projMatrix, float dt);

        /**
         * @brief 设置 DX12 光源计数数据 / Set DX12 light count data
         *
         * 将光源计数数据推送到每个缓存的 DX12Box，使 PBR 着色器（b0 cbuffer）
         * 能够获取光源数量和相机位置。在 RenderDX12 之前调用。
         *
         * Push light count data into every cached DX12Box so the PBR shader
         * (b0 cbuffer) can get light counts and camera position. Call before RenderDX12.
         *
         * @param data 光源计数常量缓冲区数据 / Light count constant buffer data
         */
        void SetDX12LightCountData(const DX12LightCountCB& data);

        /**
         * @brief 设置 DX12 点光源结构化缓冲区 / Set DX12 point light structured buffer
         *
         * 更新全局点光源结构化缓冲区。在 RenderDX12 之前调用。
         *
         * Update global point light structured buffer. Call before RenderDX12.
         *
         * @param data 点光源数据向量 / Point light data vector
         */
        void SetDX12PointLightBuffer(const std::vector<DX12PointLightData>& data);

        /**
         * @brief 设置 DX12 聚光源结构化缓冲区 / Set DX12 spot light structured buffer
         *
         * 更新全局聚光源结构化缓冲区。在 RenderDX12 之前调用。
         *
         * Update global spot light structured buffer. Call before RenderDX12.
         *
         * @param data 聚光源数据向量 / Spot light data vector
         */
        void SetDX12SpotLightBuffer(const std::vector<DX12SpotLightData>& data);

        /**
         * @brief 遗留单实体更新（System ABI）/ Legacy single-entity update (System ABI)
         *
         * 空实现，渲染由 RenderDX11/RenderDX12 驱动。
         * No-op; rendering is driven by RenderDX11/RenderDX12.
         *
         * @param entity 实体 / Entity
         * @param DeltaTime 增量时间 / Delta time
         */
        void Update(Entity& entity, float DeltaTime) override {}

    private:
        /**
         * @brief DX12 占位符缓存 / DX12 placeholder cache
         *
         * 实体 → DX12Box 的映射。在首次 RenderDX12 调用时惰性创建；
         * 实体失效时销毁。
         *
         * Entity → DX12Box map. Lazily created on first RenderDX12 call
         * for each entity; destroyed on entity invalidation.
         */
        std::unordered_map<entt::entity, std::unique_ptr<DX12Box>> m_dx12Boxes;

        /**
         * @brief DX11 占位符缓存 / DX11 placeholder caches
         *
         * 当实体没有 ModelPath 时惰性创建。
         * Lazily created when an entity has no ModelPath.
         */
        std::unordered_map<entt::entity, std::unique_ptr<SolidBoxDrawable>>   m_dx11Boxes;
        std::unordered_map<entt::entity, std::unique_ptr<SolidSphereDrawable>> m_dx11Spheres;

        /**
         * @brief 清理无效缓存 / Prune invalid cache entries
         *
         * 移除注册表中已不存在的实体对应的缓存项。
         * Remove cache entries whose entities no longer exist in the registry.
         *
         * @param scene 场景引用 / Scene reference
         */
        void PruneInvalid(Scene& scene);
    };
}

/**
 * @file MeshRendererSystem.cpp
 * @brief 网格渲染系统实现 / Mesh renderer system implementation
 *
 * 实现 MeshRendererSystem 的所有功能，包括 DX11 和 DX12 两种渲染路径、
 * 渲染资源缓存管理、光源数据传播等。
 *
 * Implements all MeshRendererSystem functionality, including DX11 and DX12
 * render paths, render resource cache management, light data propagation, etc.
 */
#include "MeshRendererSystem.h"
#include "../Scene/Scene.h"
#include "../Components/Components.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/DX12/DX12Core.h"
#include "../../Graphics/DX12/DX12Primitives.h"
#include "../../Debug/DX12Log.h"

#include <DirectXMath.h>

namespace YingLong
{
    MeshRendererSystem::~MeshRendererSystem()
    {
        // 析构时确保所有缓存资源被释放
        // Ensure all cached resources are released on destruction
        ShutDown();
    }

    void MeshRendererSystem::ShutDown()
    {
        // 清空所有 DX12 和 DX11 缓存
        // Clear all DX12 and DX11 caches
        m_dx12Boxes.clear();
        m_dx11Boxes.clear();
        m_dx11Spheres.clear();
    }

    void MeshRendererSystem::UpdateScene(Scene& scene, float DeltaTime)
    {
        // 不进行模拟工作，仅从缓存中清理已销毁的实体
        // No simulation work; just prune destroyed entities from the caches.
        PruneInvalid(scene);
    }

    void MeshRendererSystem::SetDX12LightCountData(const DX12LightCountCB& data)
    {
        // 将光源计数数据传播到所有缓存的 DX12Box
        // Propagate light count data to all cached DX12Boxes
        for (auto& pair : m_dx12Boxes)
        {
            if (pair.second)
                pair.second->SetLightCountData(data);
        }
    }

    void MeshRendererSystem::SetDX12PointLightBuffer(const std::vector<DX12PointLightData>& data)
    {
        // 更新全局点光源结构化缓冲区
        // Update global point light structured buffer
        DX12Primitive::UpdatePointLightBuffer(data);
    }

    void MeshRendererSystem::SetDX12SpotLightBuffer(const std::vector<DX12SpotLightData>& data)
    {
        // 更新全局聚光源结构化缓冲区
        // Update global spot light structured buffer
        DX12Primitive::UpdateSpotLightBuffer(data);
    }

    void MeshRendererSystem::PruneInvalid(Scene& scene)
    {
        auto& reg = scene.GetRegistry();

        // 清理 DX12 盒子缓存中已失效的实体
        // Prune invalid entities from DX12 box cache
        for (auto it = m_dx12Boxes.begin(); it != m_dx12Boxes.end(); )
        {
            if (!reg.valid(it->first))
                it = m_dx12Boxes.erase(it);
            else
                ++it;
        }

        // 清理 DX11 盒子缓存中已失效的实体
        // Prune invalid entities from DX11 box cache
        for (auto it = m_dx11Boxes.begin(); it != m_dx11Boxes.end(); )
        {
            if (!reg.valid(it->first))
                it = m_dx11Boxes.erase(it);
            else
                ++it;
        }

        // 清理 DX11 球体缓存中已失效的实体
        // Prune invalid entities from DX11 sphere cache
        for (auto it = m_dx11Spheres.begin(); it != m_dx11Spheres.end(); )
        {
            if (!reg.valid(it->first))
                it = m_dx11Spheres.erase(it);
            else
                ++it;
        }
    }

    void MeshRendererSystem::RenderDX11(Scene& scene, Graphics& gfx)
    {
        auto& reg = scene.GetRegistry();
        // 遍历所有拥有 TransformComponent + MeshComponent 的实体
        // Iterate all entities with TransformComponent + MeshComponent
        auto view = reg.view<TransformComponent, MeshComponent>();
        for (auto e : view)
        {
            auto& mesh = reg.get<MeshComponent>(e);
            auto& tr = reg.get<TransformComponent>(e);
            // 跳过不可见实体
            // Skip invisible entities
            if (!mesh.IsVisible)
                continue;

            // 路径 A：设置了 ModelPath → 加载 Model 并调用 Model::Draw
            // Path A: ModelPath is set → load Model and use Model::Draw.
            if (!mesh.ModelPath.empty())
            {
                if (!mesh.ModelPtr)
                {
                    try
                    {
                        // 懒加载模型
                        // Lazy-load model
                        mesh.ModelPtr = std::make_shared<Model>(gfx, mesh.ModelPath);
                    }
                    catch (const std::exception& ex)
                    {
                        // 加载失败，记录错误并清空路径以避免每帧重试
                        // Load failed, log error and clear path to avoid retrying every frame
                        DX12LogError(("[MeshRendererSystem] Failed to load model '" +
                                      mesh.ModelPath + "': " + std::string(ex.what()) + "\n").c_str());
                        mesh.ModelPath.clear();  // don't retry every frame
                    }
                }
                if (mesh.ModelPtr)
                {
                    // 同步变换并绘制
                    // Sync transform and draw
                    mesh.ModelPtr->Position = tr.Position;
                    mesh.ModelPtr->Rotation = tr.Rotation;
                    mesh.ModelPtr->Scale = tr.Scale;
                    mesh.ModelPtr->Draw(gfx);
                }
                continue;  // Model handled; skip placeholder path
            }

            // 路径 B：没有 ModelPath → 使用缓存的纯色占位符
            //   尺寸来自 ColliderComponent（或 TransformComponent::Scale 作为后备）
            //   颜色来自 MeshComponent::TintColor
            // Path B: no ModelPath → use a cached solid-color placeholder sized
            //   from ColliderComponent (or TransformComponent.Scale as fallback)
            //   and tinted from MeshComponent::TintColor.
            auto* col = reg.try_get<ColliderComponent>(e);
            XMFLOAT3 color = { mesh.TintColor.x, mesh.TintColor.y, mesh.TintColor.z };

            // 球形碰撞体 → 使用 SolidSphereDrawable
            // Sphere collider → use SolidSphereDrawable
            if (col && col->Shape == ColliderShape::Sphere)
            {
                auto it = m_dx11Spheres.find(e);
                if (it == m_dx11Spheres.end())
                {
                    // 惰性创建球体占位符
                    // Lazily create sphere placeholder
                    auto s = std::make_unique<SolidSphereDrawable>(gfx, std::fabs(col->Radius), color);
                    it = m_dx11Spheres.emplace(e, std::move(s)).first;
                }
                it->second->SetPosition(tr.Position);
                it->second->SetColor(color);
                it->second->Update(0.0f, 1.0f);
                it->second->Draw(gfx);
            }
            else
            {
                // 盒子/胶囊体（胶囊体使用盒子近似，仅作占位符）
                // Box / Capsule (Capsule uses Box approximation, just a placeholder).
                XMFLOAT3 halfExtents = { 0.5f, 0.5f, 0.5f };
                if (col)
                {
                    if (col->Shape == ColliderShape::Box)
                    {
                        // 盒子：直接使用半尺寸
                        // Box: use half extents directly
                        halfExtents = { std::fabs(col->HalfExtents.x),
                                        std::fabs(col->HalfExtents.y),
                                        std::fabs(col->HalfExtents.z) };
                    }
                    else if (col->Shape == ColliderShape::Capsule)
                    {
                        // 胶囊体：近似为匹配包围盒大小的盒子
                        // Capsule approximated as a box of matching bounding size.
                        halfExtents = { std::fabs(col->Radius),
                                        std::fabs(col->HalfHeight) + std::fabs(col->Radius),
                                        std::fabs(col->Radius) };
                    }
                }
                else
                {
                    // 没有碰撞体；使用 Transform.Scale 作为完整尺寸 → 半尺寸 = scale/2
                    // No collider; use Transform.Scale as full size → half = scale/2.
                    halfExtents = { std::fabs(tr.Scale.x) * 0.5f,
                                    std::fabs(tr.Scale.y) * 0.5f,
                                    std::fabs(tr.Scale.z) * 0.5f };
                }

                auto it = m_dx11Boxes.find(e);
                if (it == m_dx11Boxes.end())
                {
                    // 惰性创建盒子占位符
                    // Lazily create box placeholder
                    auto b = std::make_unique<SolidBoxDrawable>(gfx, halfExtents, color);
                    it = m_dx11Boxes.emplace(e, std::move(b)).first;
                }
                it->second->SetPosition(tr.Position);
                // TransformComponent::Rotation 使用度；SolidBoxDrawable
                // 将旋转传递给 XMMatrixRotationRollPitchYaw（弧度）
                // TransformComponent::Rotation is in degrees; SolidBoxDrawable
                // passes rotation to XMMatrixRotationRollPitchYaw (radians).
                constexpr float deg2rad = XM_PI / 180.0f;
                it->second->SetRotation({
                    tr.Rotation.x * deg2rad,
                    tr.Rotation.y * deg2rad,
                    tr.Rotation.z * deg2rad });
                it->second->SetColor(color);
                it->second->Update(0.0f, 1.0f);
                it->second->Draw(gfx);
            }
        }
    }

    void MeshRendererSystem::RenderDX12(Scene& scene, DX12Core& core,
                                       ID3D12GraphicsCommandList* cmdList,
                                       const float* viewMatrix,
                                       const float* projMatrix, float dt)
    {
        if (!cmdList)
            return;

        auto& reg = scene.GetRegistry();
        // 遍历所有拥有 TransformComponent + MeshComponent 的实体
        // Iterate all entities with TransformComponent + MeshComponent
        auto view = reg.view<TransformComponent, MeshComponent>();
        for (auto e : view)
        {
            auto& mesh = reg.get<MeshComponent>(e);
            auto& tr = reg.get<TransformComponent>(e);
            // 跳过不可见实体
            // Skip invisible entities
            if (!mesh.IsVisible)
                continue;

            // 惰性创建该实体的 DX12Box 占位符
            // Lazy-create DX12Box placeholder for this entity.
            auto it = m_dx12Boxes.find(e);
            if (it == m_dx12Boxes.end())
            {
                try
                {
                    auto box = std::make_unique<DX12Box>(core);
                    box->SetColor(mesh.TintColor.x, mesh.TintColor.y,
                                  mesh.TintColor.z, mesh.TintColor.w);
                    // DX12Box::CreateGeometry 设置了演示用的自动旋转速度。
                    // 归零，使物理旋转（通过 SetRotation）成为唯一旋转源。
                    // DX12Box::CreateGeometry sets a demo auto-rotation speed.
                    // Zero it so physics rotation (via SetRotation) is the only
                    // source of rotation.
                    float* rotSpeed = box->GetRotationSpeed();
                    rotSpeed[0] = 0.0f;
                    rotSpeed[1] = 0.0f;
                    rotSpeed[2] = 0.0f;
                    it = m_dx12Boxes.emplace(e, std::move(box)).first;
                }
                catch (const std::exception& ex)
                {
                    DX12LogError(("[MeshRendererSystem] Failed to create DX12Box: " +
                                  std::string(ex.what()) + "\n").c_str());
                    continue;
                }
            }

            auto& box = it->second;
            // 设置位置
            // Set position
            box->SetPosition(tr.Position.x, tr.Position.y, tr.Position.z);
            // TransformComponent::Rotation 使用度；DX12Primitive
            // 存储弧度（传递给 XMMatrixRotationRollPitchYaw）
            // TransformComponent::Rotation is in degrees; DX12Primitive stores
            // rotation in radians (passed to XMMatrixRotationRollPitchYaw).
            constexpr float deg2rad = XM_PI / 180.0f;
            box->SetRotation(
                tr.Rotation.x * deg2rad,
                tr.Rotation.y * deg2rad,
                tr.Rotation.z * deg2rad);

            // 当有碰撞体时从碰撞体尺寸导出均匀缩放，否则回退到
            // TransformComponent.Scale 的平均值。DX12Box 仅支持
            // 均匀缩放；完整的 DX12 Mesh 类会支持各轴独立缩放。
            // Derive a uniform scale from collider extents when available, else
            // fall back to the average of TransformComponent.Scale. DX12Box only
            // supports uniform scaling; a proper DX12 Mesh class would do per-axis.
            float scale = (tr.Scale.x + tr.Scale.y + tr.Scale.z) / 3.0f;
            if (auto* col = reg.try_get<ColliderComponent>(e))
            {
                if (col->Shape == ColliderShape::Box)
                {
                    // 匹配盒子尺寸（半尺寸 * 2 = 完整尺寸；DX12Box
                    // 单位立方体 → 按最大半尺寸 * 2 缩放以紧密贴合）
                    // Match box extents (half-extents * 2 = full size; DX12Box
                    // unit cube → scale by max half-extent * 2 for a tight fit).
                    scale = (std::fabs(col->HalfExtents.x) +
                             std::fabs(col->HalfExtents.y) +
                             std::fabs(col->HalfExtents.z)) / 3.0f * 2.0f;
                }
                else if (col->Shape == ColliderShape::Sphere)
                {
                    // 球体：直径 = 半径 * 2
                    // Sphere: diameter = radius * 2
                    scale = std::fabs(col->Radius) * 2.0f;
                }
            }
            box->SetScale(scale);

            // 设置视图/投影矩阵并更新、绘制
            // Set view/projection matrices and update, then draw
            box->SetViewMatrix(viewMatrix);
            box->SetProjectionMatrix(projMatrix);
            box->Update(dt);
            box->Draw(cmdList);
        }
    }
}

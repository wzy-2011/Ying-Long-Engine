/**
 * @file Components.h
 * @brief ECS 所有组件定义 / All ECS component definitions
 *
 * 定义引擎中使用的所有 ECS 组件结构体，包括：
 *   - TransformComponent: 位置、旋转、缩放
 *   - SpriteRendererComponent: 2D 精灵渲染
 *   - RigidbodyComponent: PhysX 刚体物理
 *   - ColliderComponent: 碰撞体几何与材质
 *   - TagComponent: 实体名称标签
 *   - MeshComponent: 3D 网格渲染
 *
 * Defines all ECS component structs used in the engine, including:
 *   - TransformComponent: position, rotation, scale
 *   - SpriteRendererComponent: 2D sprite rendering
 *   - RigidbodyComponent: PhysX rigid body physics
 *   - ColliderComponent: collider geometry and material
 *   - TagComponent: entity name tag
 *   - MeshComponent: 3D mesh rendering
 */
#pragma once
#include <DirectXMath.h>
#include <string>
#include <memory>
#include "../../Physics/Physics.h"
#include "../../Graphics/Drawable/Model.h"

using namespace DirectX;

namespace YingLong
{
    /**
     * @brief 变换组件 / Transform component
     *
     * 存储实体在 3D 空间中的位置、旋转和缩放。
     * 旋转单位为度（degrees），与 Mesh::GetTransformXM 保持一致。
     *
     * Stores the entity's position, rotation, and scale in 3D space.
     * Rotation is in degrees, consistent with Mesh::GetTransformXM.
     */
    struct TransformComponent
    {
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;

        /**
         * @brief 从位置构造 / Construct from position
         * @param pos 初始位置 / Initial position
         */
        TransformComponent(XMFLOAT3 pos) : Position(pos)
        {

        };

        /**
         * @brief 从坐标值构造 / Construct from coordinate values
         * @param x X 坐标 / X coordinate
         * @param y Y 坐标 / Y coordinate
         * @param z Z 坐标 / Z coordinate
         */
        TransformComponent(float x, float y, float z) : Position({ x, y, z })
        {

        }

        XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  ///< 位置 / Position
        XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };  ///< 旋转（度） / Rotation (degrees)
        XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };     ///< 缩放 / Scale
    };

    /**
     * @brief 精灵渲染组件 / Sprite renderer component
     *
     * 2D 精灵渲染数据，包含纹理路径、颜色和渲染层级。
     * 用于 2D 游戏或 UI 元素的渲染。
     *
     * 2D sprite rendering data, including texture path, color, and render layer.
     * Used for 2D game or UI element rendering.
     */
    struct SpriteRendererComponent
    {
        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;

        std::string TexturePath;                      ///< 纹理文件路径 / Texture file path
        XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f }; ///< 颜色（RGBA） / Color (RGBA)
        float ZIndex = 0.0f;                          ///< 渲染层级 / Render layer
        bool IsVisible = true;                        ///< 是否可见 / Is visible
    };

    /**
     * @brief 刚体组件 / Rigidbody component
     *
     * 由 PhysX 驱动的刚体。PxRigidActor* Actor 由 PhysicsSystem::UpdateScene
     * 在实体同时拥有 ColliderComponent 时惰性创建。
     *
     * - Mass=0 创建 PxRigidStatic（静态刚体）
     * - Mass>0 创建 PxRigidDynamic（动态刚体）
     *
     * Mass/UseGravity/IsKinematic 是初始化参数；Actor 创建后应通过
     * PhysX API 进行修改（PhysicsSystem 编辑器会实时修改）。
     *
     * Rigid body driven by PhysX. The PxRigidActor* Actor is created lazily by
     * PhysicsSystem::UpdateScene when the entity also has a ColliderComponent.
     *
     * - Mass=0 creates PxRigidStatic (static body)
     * - Mass>0 creates PxRigidDynamic (dynamic body)
     *
     * Mass/UseGravity/IsKinematic are initialization parameters; once the actor
     * exists they should be mutated through PhysX APIs (PhysicsSystem editor does
     * this live; runtime code can also call actor->setMass etc. directly).
     */
    struct RigidbodyComponent
    {
        RigidbodyComponent() = default;
        RigidbodyComponent(const RigidbodyComponent&) = default;

        float Mass = 1.0f;            ///< 质量（0=静态，>0=动态） / Mass (0=static, >0=dynamic)
        bool UseGravity = true;       ///< 是否受重力影响 / Affected by gravity
        bool IsKinematic = false;     ///< 是否为运动学刚体 / Is kinematic body

        /**
         * @brief 线性阻尼 / Linear damping
         *
         * 仅适用于 PxRigidDynamic（Mass > 0）。值越高速度衰减越快。
         * PhysX 默认值为 0（无阻尼）。0.5 可获得稳定的沉降效果。
         *
         * Applies only to PxRigidDynamic (Mass > 0). Higher values cause
         * velocity to decay faster. PhysX default is 0 (no damping).
         * 0.5 gives stable settling without feeling like moving through syrup.
         */
        float LinearDamping = 0.5f;

        /**
         * @brief 角阻尼 / Angular damping
         *
         * 仅适用于 PxRigidDynamic。值越高角速度衰减越快。
         * Applies only to PxRigidDynamic. Higher values cause angular
         * velocity to decay faster.
         */
        float AngularDamping = 0.5f;

        /**
         * @brief 运行时 Actor 句柄 / Runtime actor handle
         *
         * 由 PhysicsSystem 创建和释放，持有 PxRigidDynamic 或 PxRigidStatic。
         * 不参与序列化。
         *
         * Runtime handle owned by PhysicsSystem (created via Physics::PhysicsObject,
         * released via PhysicsSystem::ShutDown + on_destroy<RigidbodyComponent> signal).
         * Holds either PxRigidDynamic or PxRigidStatic. Not serialized.
         */
        PxRigidActor* Actor = nullptr;
    };

    /**
     * @brief 碰撞体形状枚举 / Collider shape enumeration
     *
     * 支持的碰撞体几何类型：Box（盒子）、Sphere（球体）、Capsule（胶囊体）。
     * Supported collider geometry types: Box, Sphere, Capsule.
     */
    enum class ColliderShape : unsigned char
    {
        Box,       ///< 盒子碰撞体 / Box collider
        Sphere,    ///< 球体碰撞体 / Sphere collider
        Capsule    ///< 胶囊体碰撞体 / Capsule collider
    };

    /**
     * @brief 碰撞体组件 / Collider component
     *
     * 碰撞体几何和材质参数。RigidbodyComponent 需要此组件才能参与碰撞。
     * 每个实体一个 ColliderComponent。
     *
     * Collider geometry and material parameters. Required for a RigidbodyComponent
     * to actually participate in collision. One ColliderComponent per entity.
     */
    struct ColliderComponent
    {
        ColliderComponent() = default;
        ColliderComponent(const ColliderComponent&) = default;

        ColliderShape Shape = ColliderShape::Box;  ///< 碰撞体形状 / Collider shape

        XMFLOAT3 HalfExtents = { 0.5f, 0.5f, 0.5f }; ///< 半尺寸（仅 Box） / Half extents (Box only)
        float Radius = 0.5f;                          ///< 半径（Sphere / Capsule） / Radius (Sphere / Capsule)
        float HalfHeight = 0.5f;                      ///< 半高（仅 Capsule，不含端盖） / Half height (Capsule only, excluding caps)

        float StaticFriction = 0.5f;    ///< 静摩擦系数 / Static friction coefficient
        float DynamicFriction = 0.5f;   ///< 动摩擦系数 / Dynamic friction coefficient
        float Restitution = 0.1f;       ///< 弹性系数 / Restitution (bounciness)
    };

    /**
     * @brief 标签组件 / Tag component
     *
     * 存储实体的可读名称，用于调试、编辑器和按名称查找实体。
     * Stores the entity's human-readable name, used for debugging, editor,
     * and finding entities by name.
     */
    struct TagComponent
    {
        std::string Name = "Unnamed Entity";  ///< 实体名称 / Entity name
    };

    /**
     * @brief 网格组件 / Mesh component
     *
     * 3D 网格渲染数据。包含模型路径（用于序列化 + DX11 懒加载）和
     * 运行时 shared_ptr<Model>。
     *
     * 在 DX12 模式下，完整的 Model 管线尚未连接，MeshRendererSystem
     * 使用 DX12Box 占位符配合 TintColor 进行渲染。
     * IsVisible=false 时跳过渲染。
     *
     * Mesh component for rendering. Holds a model path (for serialization +
     * lazy loading on DX11) and a runtime shared_ptr<Model>. In DX12 mode where
     * the full Model pipeline isn't wired yet, MeshRendererSystem renders a
     * DX12Box placeholder using TintColor. IsVisible=false skips rendering.
     */
    struct MeshComponent
    {
        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;

        std::string ModelPath;                                    ///< 序列化 + 懒加载源 / Serialization + lazy load source
        std::shared_ptr<Model> ModelPtr;                          ///< DX11 运行时句柄，懒加载，不序列化 / DX11 runtime handle, lazy loaded, not serialized
        XMFLOAT4 TintColor = { 1.0f, 1.0f, 1.0f, 1.0f };        ///< DX12 占位色 / 调试色 / DX12 placeholder color / debug color
        bool IsVisible = true;                                    ///< 是否可见 / Is visible
    };
}

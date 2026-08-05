/**
 * @file PhysicsSystem.h
 * @brief 物理系统 / Physics system
 *
 * 由 PhysX 驱动的物理系统。惰性创建 PxRigidDynamic actor 给
 * 同时拥有 RigidbodyComponent + ColliderComponent + TransformComponent
 * 的实体，通过绑定的 PhysicsScene 步进模拟，并将结果位姿回写
 * 到 TransformComponent（非运动学刚体）。
 *
 * 使用方法：Application 在 scene->AddSystem<PhysicsSystem>() 之前
 * 调用 scene->SetPhysicsScene(&physicsScene)。然后 scene->Update(dt)
 * 驱动物理模拟。
 *
 * PhysX-driven physics system. Lazily creates PxRigidDynamic actors for
 * entities that have RigidbodyComponent + ColliderComponent + TransformComponent,
 * steps the simulation via the bound PhysicsScene, and writes the resulting
 * pose back to TransformComponent for non-kinematic bodies.
 *
 * Usage: Application calls scene->SetPhysicsScene(&physicsScene) before
 * scene->AddSystem<PhysicsSystem>(). Then scene->Update(dt) drives physics.
 */
#pragma once
#include "System.h"
#include "../../Physics/PhysicsScene/PhysicsScene.h"
#include "../Components/Components.h"
#include "../Entity/Entity.h"
#include "../../entt-master/include/entt.hpp"

namespace YingLong
{
    /**
     * @brief 物理系统 / Physics system
     *
     * 管理场景中所有刚体的物理模拟。主要职责：
     *   1. 惰性创建 PhysX actor（PxRigidDynamic / PxRigidStatic）
     *   2. 步进物理模拟
     *   3. 将模拟结果回写到 TransformComponent
     *   4. 提供 ImGui 编辑器面板用于调试
     *   5. 管理 actor 生命周期（创建/销毁/资源释放）
     *
     * Manages physics simulation for all rigid bodies in the scene.
     * Main responsibilities:
     *   1. Lazily create PhysX actors (PxRigidDynamic / PxRigidStatic)
     *   2. Step the physics simulation
     *   3. Write simulation results back to TransformComponent
     *   4. Provide ImGui editor panel for debugging
     *   5. Manage actor lifecycle (create/destroy/resource release)
     */
    class PhysicsSystem : public System
    {
    public:
        /**
         * @brief 默认构造函数 / Default constructor
         */
        PhysicsSystem() = default;

        /**
         * @brief 析构函数 / Destructor
         *
         * 调用 ShutDown() 释放所有物理资源。
         * Calls ShutDown() to release all physics resources.
         */
        ~PhysicsSystem() override;

        void Initialize() override {}

        /**
         * @brief 场景级更新 / Scene-level update
         *
         * 执行完整的物理更新流程：
         *   1. 惰性创建新实体的 actor
         *   2. 步进物理模拟
         *   3. 将结果回写到 TransformComponent
         *
         * Executes the full physics update flow:
         *   1. Lazily create actors for new entities
         *   2. Step the physics simulation
         *   3. Write results back to TransformComponent
         *
         * @param scene 场景引用 / Scene reference
         * @param DeltaTime 增量时间 / Delta time
         */
        void UpdateScene(Scene& scene, float DeltaTime) override;

        /**
         * @brief 系统关闭 / System shutdown
         *
         * 遍历注册表，释放所有由本系统创建的 actor。
         * 这是主要清理路径，在 Scene::Unload() 中 Registry.clear()
         * 触发 on_destroy 之前执行。
         *
         * Walks the registry and releases all actors created by this system.
         * This is the primary cleanup path, executed in Scene::Unload() before
         * Registry.clear() fires on_destroy.
         */
        void ShutDown() override;

        /**
         * @brief ImGui 编辑器面板 / ImGui editor panel
         *
         * 列出物理实体，允许实时编辑 Mass/UseGravity/IsKinematic
         * （立即应用到 PxActor）以及碰撞体材质参数
         * （摩擦/弹性通过 PxMaterial 实时应用）。
         * 碰撞体几何更改需要重建实体。
         *
         * Lists physics entities, allows live editing of Mass/UseGravity/IsKinematic
         * (applied to PxActor immediately) and collider material params
         * (friction/restitution applied live via PxMaterial).
         * Collider geometry changes require entity rebuild.
         *
         * @param scene 场景引用 / Scene reference
         */
        void RenderImGuiEditor(Scene& scene);

        // === 力 / 冲量 / 速度 API ===
        // === Force / Impulse / Velocity API ===

        void ApplyForce(Scene& scene, entt::entity e, const XMFLOAT3& force);
        void ApplyImpulse(Scene& scene, entt::entity e, const XMFLOAT3& impulse);
        void ApplyTorque(Scene& scene, entt::entity e, const XMFLOAT3& torque);
        void ApplyAngularImpulse(Scene& scene, entt::entity e, const XMFLOAT3& impulse);
        void SetLinearVelocity(Scene& scene, entt::entity e, const XMFLOAT3& vel);
        void SetAngularVelocity(Scene& scene, entt::entity e, const XMFLOAT3& vel);

        // === 碰撞事件 API ===
        // === Collision Event API ===

        struct CollisionEvent
        {
            entt::entity EntityA;
            entt::entity EntityB;
            XMFLOAT3 ContactPoint;
            XMFLOAT3 ContactNormal;
            float ContactDistance;
            bool IsTrigger;
        };

        std::vector<CollisionEvent> GetAndClearCollisionEvents();

        /**
         * @brief 遗留单实体更新（System ABI 兼容性）
         *        Legacy single-entity update (System ABI compatibility)
         *
         * 空实现。物理由 UpdateScene 驱动。
         * No-op. Physics is driven by UpdateScene.
         *
         * @param entity 实体 / Entity
         * @param DeltaTime 增量时间 / Delta time
         */
        void Update(Entity& entity, float DeltaTime) override {}

    private:
        /**
         * @brief 为实体创建 PxRigidActor / Create a PxRigidActor for the entity
         *
         * 读取 TransformComponent（初始位姿）、RigidbodyComponent（质量/标志）、
         * ColliderComponent（几何 + 材质）。创建 PxMaterial + PxShape +
         * PxRigidDynamic/PxRigidStatic，并将 actor 添加到物理场景。
         * 写入 rb.Actor。
         *
         * Reads from TransformComponent (initial pose), RigidbodyComponent (mass/flags),
         * ColliderComponent (geometry + material). Creates PxMaterial + PxShape +
         * PxRigidDynamic/PxRigidStatic, and adds the actor to the physics scene.
         * Writes rb.Actor.
         *
         * @param scene 场景引用 / Scene reference
         * @param e 实体句柄 / Entity handle
         * @param reg 注册表引用 / Registry reference
         */
        void CreateActor(Scene& scene, entt::entity e, entt::registry& reg);

        /**
         * @brief 将 PxRigidDynamic 全局位姿回写到 TransformComponent
         *        Copy PxRigidDynamic global pose back to TransformComponent
         *
         * @param actor PhysX 刚体动态对象 / PhysX rigid dynamic actor
         * @param tr 变换组件引用 / Transform component reference
         */
        void WriteBackToTransform(const PxRigidDynamic& actor, TransformComponent& tr) const;

        /**
         * @brief 同步 PxRigidDynamic 的速度到 RigidbodyComponent
         *        Sync PxRigidDynamic velocities to RigidbodyComponent
         *
         * @param actor PhysX 刚体动态对象 / PhysX rigid dynamic actor
         * @param rb 刚体组件引用 / Rigidbody component reference
         */
        void SyncVelocities(const PxRigidDynamic& actor, RigidbodyComponent& rb) const;

        /**
         * @brief 从 PxActor 查找对应的 entt::entity
         *        Find the entt::entity corresponding to a PxActor
         *
         * @param reg 注册表引用 / Registry reference
         * @param actor PhysX Actor 指针 / PhysX Actor pointer
         * @return entt::entity 对应的实体，找不到返回 entt::null
         */
        entt::entity FindEntityByActor(entt::registry& reg, PxActor* actor) const;

        /**
         * @brief entt on_destroy<RigidbodyComponent> 回调
         *        entt on_destroy<RigidbodyComponent> sink
         *
         * 在组件（和可能的实体）消失之前释放 actor。每个注册表连接一次。
         * Releases the actor before the component (and possibly entity) goes away.
         * Connected once per registry.
         *
         * @param r 注册表 / Registry
         * @param e 实体 / Entity
         */
        static void OnRigidbodyDestroyed(entt::registry& r, entt::entity e);

        /**
         * @brief 场景指针 / Scene pointer
         *
         * 在首次 UpdateScene 时捕获，使 ShutDown 可以遍历注册表并
         * 释放 actor，即使 on_destroy 从未连接（例如场景在首次
         * Update 调用前就被销毁）。
         *
         * Captured on first UpdateScene so ShutDown can walk the registry and
         * release actors even if on_destroy was never connected (e.g. scene
         * destroyed before any Update call).
         */
        Scene* m_scene = nullptr;

        /**
         * @brief on_destroy 钩子是否已连接 / Whether on_destroy hook is connected
         */
        bool m_destroyHooked = false;
    };
}

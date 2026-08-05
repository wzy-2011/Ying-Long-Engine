/**
 * @file PhysicsSystem.cpp
 * @brief 物理系统实现 / Physics system implementation
 *
 * 实现 PhysicsSystem 的所有功能，包括 PhysX actor 创建、
 * 物理模拟步进、位姿回写、ImGui 编辑器面板等。
 *
 * Implements all PhysicsSystem functionality, including PhysX actor creation,
 * physics simulation stepping, pose write-back, ImGui editor panel, etc.
 */
#include "PhysicsSystem.h"
#include "../Scene/Scene.h"
#include "../../Physics/Physics.h"
#include "../../Debug/DX12Log.h"
#include "ImGui/imgui.h"

#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

namespace YingLong
{
    namespace
{
    /**
     * @brief 将度为单位的欧拉角转换为 PhysX 四元数
     *        Convert an Euler triple in DEGREES to a PhysX quaternion.
     *
     * TransformComponent::Rotation 使用度（与 Mesh::GetTransformXM 一致），
     * 但 XMQuaternionRotationRollPitchYaw 期望弧度，因此在此转换。
     *
     * TransformComponent::Rotation uses degrees (consistent with Mesh::GetTransformXM),
     * but XMQuaternionRotationRollPitchYaw expects radians, so we convert here.
     *
     * @param eulerDegrees 欧拉角（度） / Euler angles (degrees)
     * @return PxQuat PhysX 四元数 / PhysX quaternion
     */
    PxQuat EulerToPxQuat(const XMFLOAT3& eulerDegrees) noexcept
    {
        constexpr float deg2rad = XM_PI / 180.0f;
        XMVECTOR q = XMQuaternionRotationRollPitchYaw(
            eulerDegrees.x * deg2rad,
            eulerDegrees.y * deg2rad,
            eulerDegrees.z * deg2rad);
        XMFLOAT4 qf;
        XMStoreFloat4(&qf, q);
        return PxQuat(qf.x, qf.y, qf.z, qf.w);
    }

    /**
     * @brief 将 PhysX 四元数转换为度为单位的欧拉角
     *        Convert a PhysX quaternion to Euler angles in DEGREES
     *
     * 与 TransformComponent 的单位一致。
     * Matches TransformComponent's unit.
     *
     * @param q PhysX 四元数 / PhysX quaternion
     * @param out 输出欧拉角（度） / Output Euler angles (degrees)
     */
    void PxQuatToEuler(const PxQuat& q, XMFLOAT3& out) noexcept
    {
        constexpr float rad2deg = 180.0f / XM_PI;

        // roll (x-axis rotation)
        float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
        float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        out.x = std::atan2(sinr_cosp, cosr_cosp) * rad2deg;

        // pitch (y-axis rotation)
        float sinp = 2.0f * (q.w * q.y - q.z * q.x);
        if (std::fabs(sinp) >= 1.0f)
            out.y = std::copysign(XM_PIDIV2, sinp) * rad2deg;
        else
            out.y = std::asin(sinp) * rad2deg;

        // yaw (z-axis rotation)
        float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        out.z = std::atan2(siny_cosp, cosy_cosp) * rad2deg;
    }
}

    PhysicsSystem::~PhysicsSystem()
    {
        // 析构时确保所有物理资源被释放
        // Ensure all physics resources are released on destruction
        ShutDown();
    }

    void PhysicsSystem::ShutDown()
    {
        // 显式清理：遍历注册表并释放我们创建的每个 actor。
        // 这在 Scene::Unload() 中 Registry.clear() 触发 on_destroy 之前运行，
        // 因此这是主要清理路径。on_destroy 是实体级销毁的次要安全网。
        // Explicit cleanup: walk the registry and release every actor we created.
        // This runs in Scene::Unload() BEFORE Registry.clear() fires on_destroy,
        // so it's the primary cleanup path. on_destroy is a secondary safety net
        // for entity-level destruction while the scene is still alive.
        if (!m_scene)
            return;

        auto& reg = m_scene->GetRegistry();
        auto view = reg.view<RigidbodyComponent>();
        for (auto e : view)
        {
            auto& rb = reg.get<RigidbodyComponent>(e);
            if (rb.Actor)
            {
                // 从场景中移除 actor
                // Remove actor from scene
                if (auto* pxScene = rb.Actor->getScene())
                {
                    pxScene->removeActor(*rb.Actor);
                }
                // 释放 actor（级联释放 shape 和 material）
                // Release actor (cascades to release shapes and materials)
                rb.Actor->release();
                rb.Actor = nullptr;
            }
        }
        m_scene = nullptr;
    }

    void PhysicsSystem::UpdateScene(Scene& scene, float DeltaTime)
    {
        m_scene = &scene;
        auto& reg = scene.GetRegistry();
        PhysicsScene* ps = scene.GetPhysicsScene();

        if (!ps || !ps->IsValid())
        {
            // 未绑定物理场景；无事可做
            // No physics scene bound; nothing to do.
            return;
        }

        // 恰好连接一次注册表的 on_destroy<RigidbodyComponent> 信号。
        // entt 信号是每注册表的；如果注册表被替换（Scene::Clear
        // 隐式创建一个新的？实际上不是，Clear 只是清除实体），
        // 当 m_destroyHooked 重置时我们会重新连接。为安全起见，我们通过
        // 注册表地址检查；如果注册表指针改变，重新连接。
        // Hook the registry's on_destroy<RigidbodyComponent> signal exactly once.
        // entt signals are per-registry; if the registry is replaced (Scene::Clear
        // creates a new one implicitly? Actually no, Clear just clears entities),
        // we re-hook when m_destroyHooked is reset. For safety we check by registry
        // address; if the registry pointer changes, re-hook.
        if (!m_destroyHooked)
        {
            reg.on_destroy<RigidbodyComponent>().connect<&PhysicsSystem::OnRigidbodyDestroyed>();
            m_destroyHooked = true;
        }

        // 1. 惰性创建同时拥有三个组件但尚无 actor 的实体的 actor
        // 1. Lazy-create actors for entities that have all three components but no actor yet.
        auto view = reg.view<TransformComponent, RigidbodyComponent, ColliderComponent>();
        for (auto e : view)
        {
            auto& rb = reg.get<RigidbodyComponent>(e);
            if (rb.Actor == nullptr)
            {
                CreateActor(scene, e, reg);
            }
        }

        // 2. 步进模拟
        // 2. Step the simulation.
        ps->Step(DeltaTime);

        // 3. 将结果位姿和速度回写到非运动学动态刚体。
        //    静态刚体（Mass==0）和运动学刚体不由物理驱动。
        // 3. Write back the resulting pose and velocities for non-kinematic dynamic bodies.
        //    Static bodies (Mass==0) and kinematic bodies are not driven by physics.
        for (auto e : view)
        {
            auto& rb = reg.get<RigidbodyComponent>(e);
            auto& tr = reg.get<TransformComponent>(e);
            if (rb.Actor && rb.Mass > 0.0f && !rb.IsKinematic)
            {
                WriteBackToTransform(static_cast<PxRigidDynamic&>(*rb.Actor), tr);
                SyncVelocities(static_cast<PxRigidDynamic&>(*rb.Actor), rb);
            }
        }
    }

    void PhysicsSystem::CreateActor(Scene& scene, entt::entity e, entt::registry& reg)
    {
        auto& rb = reg.get<RigidbodyComponent>(e);
        auto& col = reg.get<ColliderComponent>(e);
        auto& tr = reg.get<TransformComponent>(e);

        if (!Physics::PhysicsObject)
        {
            DX12LogError("[PhysicsSystem] PhysicsObject is null, skipping actor creation\n");
            return;
        }

        // 创建材质
        // Create material
        PxMaterial* material = Physics::PhysicsObject->createMaterial(
            col.StaticFriction, col.DynamicFriction, col.Restitution);
        if (!material)
        {
            DX12LogError("[PhysicsSystem] Failed to create PxMaterial\n");
            return;
        }

        // 根据碰撞体类型创建 shape
        // Create shape based on collider type
        PxShape* shape = nullptr;
        switch (col.Shape)
        {
        case ColliderShape::Box:
        {
            PxBoxGeometry boxGeom(
                std::fabs(col.HalfExtents.x),
                std::fabs(col.HalfExtents.y),
                std::fabs(col.HalfExtents.z));
            shape = Physics::PhysicsObject->createShape(boxGeom, *material);
            break;
        }
        case ColliderShape::Sphere:
        {
            PxSphereGeometry sphereGeom(std::fabs(col.Radius));
            shape = Physics::PhysicsObject->createShape(sphereGeom, *material);
            break;
        }
        case ColliderShape::Capsule:
        {
            // PxCapsuleGeometry 使用沿 X 轴的半高
            // PxCapsuleGeometry uses half-height along the X axis.
            PxCapsuleGeometry capGeom(std::fabs(col.Radius), std::fabs(col.HalfHeight));
            shape = Physics::PhysicsObject->createShape(capGeom, *material);
            break;
        }
        }

        if (!shape)
        {
            DX12LogError("[PhysicsSystem] Failed to create PxShape\n");
            material->release();
            return;
        }

        // 如果是触发器，设置 TRIGGER_SHAPE 标志并清除 SIMULATION_SHAPE
        // If this is a trigger, set TRIGGER_SHAPE flag and clear SIMULATION_SHAPE
        if (col.IsTrigger)
        {
            shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
            shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
        }

        // createShape 增加材质的引用计数；释放我们的初始引用，
        // 这样当 shape（以及 actor）被释放时材质也会被释放。
        // 如果不这样做，PxPhysics::release() 会检测到泄漏的
        // PxMaterial 并在关闭时触发断言。
        // createShape increments the material's refcount; release our initial
        // reference so the material is freed when the shape (and thus the actor)
        // is released. Without this, PxPhysics::release() detects a leaked
        // PxMaterial and triggers an assertion on shutdown.
        material->release();

        // 从 TransformComponent 获取初始位姿
        // Initial pose from TransformComponent
        PxTransform pose(PxVec3(tr.Position.x, tr.Position.y, tr.Position.z),
                         EulerToPxQuat(tr.Rotation));

        // 根据 Mass 分支：0 = 静态（PxRigidStatic），>0 = 动态（PxRigidDynamic）。
        // 静态刚体不可移动 — 非常适合地面、墙壁、触发器。
        // 动态刚体参与模拟并使其位姿被回写。
        // Branch on Mass: 0 = static (PxRigidStatic), >0 = dynamic (PxRigidDynamic).
        // Static bodies are immovable — perfect for ground, walls, triggers.
        // Dynamic bodies participate in simulation and get their pose written back.
        PxRigidActor* actor = nullptr;
        if (rb.Mass > 0.0f)
        {
            PxRigidDynamic* dynamic = Physics::PhysicsObject->createRigidDynamic(pose);
            if (!dynamic)
            {
                DX12LogError("[PhysicsSystem] Failed to create PxRigidDynamic\n");
                shape->release();
                material->release();
                return;
            }
            dynamic->attachShape(*shape);
            shape->release();

            // 设置质量和惯性张量
            // Set mass and inertia tensor
            PxRigidBodyExt::updateMassAndInertia(*dynamic, rb.Mass);
            // 设置阻尼
            // Set damping
            dynamic->setLinearDamping(rb.LinearDamping);
            dynamic->setAngularDamping(rb.AngularDamping);
            // 设置重力标志
            // Set gravity flag
            if (!rb.UseGravity)
            {
                dynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
            }
            // 设置运动学标志
            // Set kinematic flag
            if (rb.IsKinematic)
            {
                dynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
            }
            actor = dynamic;
        }
        else
        {
            PxRigidStatic* staticActor = Physics::PhysicsObject->createRigidStatic(pose);
            if (!staticActor)
            {
                DX12LogError("[PhysicsSystem] Failed to create PxRigidStatic\n");
                shape->release();
                material->release();
                return;
            }
            staticActor->attachShape(*shape);
            shape->release();
            // UseGravity / IsKinematic 标志对静态刚体无意义
            // UseGravity / IsKinematic flags are meaningless for static bodies.
            actor = staticActor;
        }

        // 添加到物理场景
        // Add to scene
        scene.GetPhysicsScene()->AddActor(*actor);

        // 将所有权移交给组件
        // Hand off ownership to the component
        rb.Actor = actor;

        // 注意：材质现在由 shape 引用，而 shape 由 actor 引用。
        // 释放 actor（在 ShutDown / OnRigidbodyDestroyed 中）会级联释放。
        // Note: material is now referenced by the shape which is referenced by the
        // actor. Releasing the actor (in ShutDown / OnRigidbodyDestroyed) cascades.
    }

    void PhysicsSystem::WriteBackToTransform(const PxRigidDynamic& actor, TransformComponent& tr) const
    {
        // 获取全局位姿并同步到 TransformComponent
        // Get global pose and sync to TransformComponent
        PxTransform pose = actor.getGlobalPose();
        tr.Position = XMFLOAT3(pose.p.x, pose.p.y, pose.p.z);
        PxQuatToEuler(pose.q, tr.Rotation);
        // 缩放不由物理驱动
        // Scale is not driven by physics.
    }

    void PhysicsSystem::SyncVelocities(const PxRigidDynamic& actor, RigidbodyComponent& rb) const
    {
        PxVec3 linVel = actor.getLinearVelocity();
        PxVec3 angVel = actor.getAngularVelocity();
        rb.LinearVelocity = XMFLOAT3(linVel.x, linVel.y, linVel.z);
        rb.AngularVelocity = XMFLOAT3(angVel.x, angVel.y, angVel.z);
    }

    entt::entity PhysicsSystem::FindEntityByActor(entt::registry& reg, PxActor* actor) const
    {
        if (!actor)
            return entt::null;
        auto view = reg.view<RigidbodyComponent>();
        for (auto e : view)
        {
            auto& rb = reg.get<RigidbodyComponent>(e);
            if (rb.Actor == actor)
                return e;
        }
        return entt::null;
    }

    // === 力 / 冲量 / 速度 API ===

    void PhysicsSystem::ApplyForce(Scene& scene, entt::entity e, const XMFLOAT3& force)
    {
        auto& reg = scene.GetRegistry();
        if (!reg.all_of<RigidbodyComponent>(e)) return;
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor && rb.Mass > 0.0f && !rb.IsKinematic)
        {
            PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
            dyn->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eFORCE);
        }
    }

    void PhysicsSystem::ApplyImpulse(Scene& scene, entt::entity e, const XMFLOAT3& impulse)
    {
        auto& reg = scene.GetRegistry();
        if (!reg.all_of<RigidbodyComponent>(e)) return;
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor && rb.Mass > 0.0f && !rb.IsKinematic)
        {
            PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
            dyn->addForce(PxVec3(impulse.x, impulse.y, impulse.z), PxForceMode::eIMPULSE);
        }
    }

    void PhysicsSystem::ApplyTorque(Scene& scene, entt::entity e, const XMFLOAT3& torque)
    {
        auto& reg = scene.GetRegistry();
        if (!reg.all_of<RigidbodyComponent>(e)) return;
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor && rb.Mass > 0.0f && !rb.IsKinematic)
        {
            PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
            dyn->addTorque(PxVec3(torque.x, torque.y, torque.z), PxForceMode::eFORCE);
        }
    }

    void PhysicsSystem::ApplyAngularImpulse(Scene& scene, entt::entity e, const XMFLOAT3& impulse)
    {
        auto& reg = scene.GetRegistry();
        if (!reg.all_of<RigidbodyComponent>(e)) return;
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor && rb.Mass > 0.0f && !rb.IsKinematic)
        {
            PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
            dyn->addTorque(PxVec3(impulse.x, impulse.y, impulse.z), PxForceMode::eIMPULSE);
        }
    }

    void PhysicsSystem::SetLinearVelocity(Scene& scene, entt::entity e, const XMFLOAT3& vel)
    {
        auto& reg = scene.GetRegistry();
        if (!reg.all_of<RigidbodyComponent>(e)) return;
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor && rb.Mass > 0.0f)
        {
            PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
            dyn->setLinearVelocity(PxVec3(vel.x, vel.y, vel.z));
        }
    }

    void PhysicsSystem::SetAngularVelocity(Scene& scene, entt::entity e, const XMFLOAT3& vel)
    {
        auto& reg = scene.GetRegistry();
        if (!reg.all_of<RigidbodyComponent>(e)) return;
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor && rb.Mass > 0.0f)
        {
            PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
            dyn->setAngularVelocity(PxVec3(vel.x, vel.y, vel.z));
        }
    }

    // === 碰撞事件 API ===

    std::vector<PhysicsSystem::CollisionEvent> PhysicsSystem::GetAndClearCollisionEvents()
    {
        std::vector<CollisionEvent> resolved;
        if (!m_scene)
            return resolved;

        PhysicsScene* ps = m_scene->GetPhysicsScene();
        if (!ps || !ps->IsValid())
            return resolved;

        auto rawEvents = ps->GetCollisionCallback()->GetAndClearEvents();
        auto& reg = m_scene->GetRegistry();

        for (const auto& raw : rawEvents)
        {
            CollisionEvent evt;
            evt.EntityA = FindEntityByActor(reg, raw.ActorA);
            evt.EntityB = FindEntityByActor(reg, raw.ActorB);
            evt.ContactPoint = raw.ContactPoint;
            evt.ContactNormal = raw.ContactNormal;
            evt.ContactDistance = raw.ContactDistance;
            evt.IsTrigger = raw.IsTrigger;
            resolved.push_back(evt);
        }
        return resolved;
    }

    void PhysicsSystem::OnRigidbodyDestroyed(entt::registry& r, entt::entity e)
    {
        auto* rb = r.try_get<RigidbodyComponent>(e);
        if (!rb || !rb->Actor)
            return;

        PxRigidActor* actor = rb->Actor;
        // 如果仍附加在场景中，先从场景移除
        // Remove from scene first if still attached
        if (auto* scene = actor->getScene())
        {
            scene->removeActor(*actor);
        }
        // 释放 actor；附加的 shape 和它们的材质由 PhysX 释放
        // （材质是共享的，引用计数）
        // Release the actor; attached shapes and their materials are released
        // by PhysX (materials are shared, refcounted).
        actor->release();
        rb->Actor = nullptr;
    }

    void PhysicsSystem::RenderImGuiEditor(Scene& scene)
    {
        if (!ImGui::Begin("物理编辑器"))
        {
            ImGui::End();
            return;
        }

        auto& reg = scene.GetRegistry();

        // 实体列表面板
        // Entity list panel
        if (ImGui::CollapsingHeader("实体", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto view = reg.view<TransformComponent, RigidbodyComponent>();
            for (auto e : view)
            {
                auto& tr = reg.get<TransformComponent>(e);
                auto& rb = reg.get<RigidbodyComponent>(e);
                auto* tag = reg.try_get<TagComponent>(e);

                // 构建标签：名称 + 实体 ID（确保唯一）
                // Build label: name + entity ID (ensures uniqueness)
                char label[128];
                if (tag)
                {
                    snprintf(label, sizeof(label), "%s##%u", tag->Name.c_str(), (unsigned)e);
                }
                else
                {
                    snprintf(label, sizeof(label), "实体 %u##%u", (unsigned)e, (unsigned)e);
                }

                if (ImGui::TreeNode(label))
                {
                    // Transform 面板
                    // Transform panel
                    if (ImGui::TreeNode("变换"))
                    {
                        ImGui::DragFloat3("位置", &tr.Position.x, 0.1f);
                        ImGui::DragFloat3("旋转", &tr.Rotation.x, 0.05f);
                        ImGui::DragFloat3("缩放", &tr.Scale.x, 0.1f);
                        ImGui::TreePop();
                    }

                    // Rigidbody 面板
                    // Rigidbody panel
                    if (ImGui::TreeNode("刚体"))
                    {
                        bool massChanged = ImGui::DragFloat("质量", &rb.Mass, 0.1f, 0.0f, 1000.0f);
                        bool gravityChanged = ImGui::Checkbox("使用重力", &rb.UseGravity);
                        bool kinematicChanged = ImGui::Checkbox("是否为运动学", &rb.IsKinematic);
                        bool dampingChanged = ImGui::DragFloat("线性阻尼", &rb.LinearDamping, 0.01f, 0.0f, 5.0f);
                        dampingChanged |= ImGui::DragFloat("角阻尼", &rb.AngularDamping, 0.01f, 0.0f, 5.0f);

                        // 速度显示（只读，由物理引擎每帧同步）
                        // Velocity display (read-only, synced from physics engine each frame)
                        ImGui::Separator();
                        ImGui::Text("线速度: (%.2f, %.2f, %.2f)", rb.LinearVelocity.x, rb.LinearVelocity.y, rb.LinearVelocity.z);
                        ImGui::Text("角速度: (%.2f, %.2f, %.2f)", rb.AngularVelocity.x, rb.AngularVelocity.y, rb.AngularVelocity.z);
                        float linSpeed = sqrtf(rb.LinearVelocity.x * rb.LinearVelocity.x + rb.LinearVelocity.y * rb.LinearVelocity.y + rb.LinearVelocity.z * rb.LinearVelocity.z);
                        float angSpeed = sqrtf(rb.AngularVelocity.x * rb.AngularVelocity.x + rb.AngularVelocity.y * rb.AngularVelocity.y + rb.AngularVelocity.z * rb.AngularVelocity.z);
                        ImGui::Text("线速度大小: %.2f  角速度大小: %.2f", linSpeed, angSpeed);

                        // 力 / 冲量施加
                        // Force / Impulse application
                        if (rb.Mass > 0.0f && !rb.IsKinematic && rb.Actor)
                        {
                            ImGui::Separator();
                            static float forceInput[3] = { 0, 0, 0 };
                            static float impulseInput[3] = { 0, 0, 0 };
                            ImGui::DragFloat3("力##Force", forceInput, 1.0f);
                            ImGui::SameLine();
                            if (ImGui::Button("施加力##ApplyForce"))
                            {
                                ApplyForce(scene, e, XMFLOAT3(forceInput[0], forceInput[1], forceInput[2]));
                            }
                            ImGui::DragFloat3("冲量##Impulse", impulseInput, 1.0f);
                            ImGui::SameLine();
                            if (ImGui::Button("施加冲量##ApplyImpulse"))
                            {
                                ApplyImpulse(scene, e, XMFLOAT3(impulseInput[0], impulseInput[1], impulseInput[2]));
                            }
                            if (ImGui::Button("向上弹跳 (冲量)##BounceUp"))
                            {
                                ApplyImpulse(scene, e, XMFLOAT3(0.0f, 10.0f, 0.0f));
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("停止运动##Stop"))
                            {
                                SetLinearVelocity(scene, e, XMFLOAT3(0, 0, 0));
                                SetAngularVelocity(scene, e, XMFLOAT3(0, 0, 0));
                            }
                        }

                        // 实时应用到 PxActor
                        // Apply live to PxActor
                        if (rb.Actor)
                        {
                            if (gravityChanged)
                            {
                                rb.Actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rb.UseGravity);
                            }
                            if (rb.Mass > 0.0f)
                            {
                                PxRigidDynamic* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
                                if (massChanged)
                                {
                                    dyn->setMass(rb.Mass);
                                }
                                if (kinematicChanged)
                                {
                                    dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rb.IsKinematic);
                                }
                                if (dampingChanged)
                                {
                                    dyn->setLinearDamping(rb.LinearDamping);
                                    dyn->setAngularDamping(rb.AngularDamping);
                                }
                            }
                        }

                        // 状态显示
                        // Status display
                        ImGui::Text("状态: %s", rb.Actor ? (rb.Mass > 0.0f ? "动态" : "静态") : "无Actor");
                        if (rb.Actor)
                        {
                            PxTransform pose = rb.Actor->getGlobalPose();
                            ImGui::Text("姿态: (%.2f, %.2f, %.2f)", pose.p.x, pose.p.y, pose.p.z);
                        }
                        ImGui::TreePop();
                    }

                    // Collider 面板
                    // Collider panel
                    if (auto* col = reg.try_get<ColliderComponent>(e))
                    {
                        if (ImGui::TreeNode("碰撞体"))
                        {
                            const char* shapes[] = { "盒体", "球体", "胶囊体" };
                            int shapeIdx = static_cast<int>(col->Shape);
                            ImGui::Combo("形状", &shapeIdx, shapes, 3);
                            if (shapeIdx != static_cast<int>(col->Shape))
                            {
                                ImGui::TextDisabled("  (几何体更改需重建实体)");
                            }

                            // 触发器开关（可实时切换）
                            // Trigger toggle (can be toggled at runtime)
                            bool triggerChanged = ImGui::Checkbox("触发器", &col->IsTrigger);
                            if (triggerChanged && rb.Actor)
                            {
                                PxShape* shapesArr[1];
                                PxU32 count = rb.Actor->getShapes(shapesArr, 1);
                                if (count > 0 && shapesArr[0])
                                {
                                    shapesArr[0]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !col->IsTrigger);
                                    shapesArr[0]->setFlag(PxShapeFlag::eTRIGGER_SHAPE, col->IsTrigger);
                                }
                            }

                            // 根据形状显示不同的参数
                            // Show different parameters based on shape
                            if (col->Shape == ColliderShape::Box)
                            {
                                ImGui::DragFloat3("半边长", &col->HalfExtents.x, 0.05f, 0.01f);
                            }
                            else if (col->Shape == ColliderShape::Sphere)
                            {
                                ImGui::DragFloat("半径", &col->Radius, 0.05f, 0.01f);
                            }
                            else if (col->Shape == ColliderShape::Capsule)
                            {
                                ImGui::DragFloat("半径", &col->Radius, 0.05f, 0.01f);
                                ImGui::DragFloat("半高", &col->HalfHeight, 0.05f, 0.01f);
                            }

                            // 材质参数
                            // Material parameters
                            bool fricChanged = ImGui::DragFloat("静摩擦", &col->StaticFriction, 0.05f, 0.0f, 5.0f);
                            fricChanged |= ImGui::DragFloat("动摩擦", &col->DynamicFriction, 0.05f, 0.0f, 5.0f);
                            bool restChanged = ImGui::DragFloat("弹性", &col->Restitution, 0.05f, 0.0f, 1.0f);

                            // 通过 PxMaterial 实时应用摩擦/弹性
                            // Apply friction/restitution live via PxMaterial
                            if (rb.Actor && (fricChanged || restChanged))
                            {
                                PxShape* shapes[1];
                                PxU32 count = rb.Actor->getShapes(shapes, 1);
                                if (count > 0 && shapes[0])
                                {
                                    PxMaterial* mats[1];
                                    PxU32 matCount = shapes[0]->getMaterials(mats, 1);
                                    if (matCount > 0 && mats[0])
                                    {
                                        mats[0]->setStaticFriction(col->StaticFriction);
                                        mats[0]->setDynamicFriction(col->DynamicFriction);
                                        mats[0]->setRestitution(col->Restitution);
                                    }
                                }
                            }

                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }

        // 创建新物理实体面板
        // Create new physics entity panel
        if (ImGui::CollapsingHeader("创建实体"))
        {
            static char name[64] = "PhysicsBox";
            ImGui::InputText("名称", name, sizeof(name));

            static float posX = 0.0f, posY = 5.0f, posZ = 0.0f;
            ImGui::DragFloat3("初始位置", &posX, 0.1f);

            // 添加动态物理盒按钮
            // Add dynamic physics box button
            if (ImGui::Button("添加物理盒体"))
            {
                auto e = scene.CreateEntity(name);
                scene.AddComponent<TransformComponent>(e, XMFLOAT3{ posX, posY, posZ });
                scene.AddComponent<RigidbodyComponent>(e);
                scene.AddComponent<ColliderComponent>(e);
                auto& mesh = scene.AddComponent<MeshComponent>(e);
                mesh.TintColor = XMFLOAT4{ 0.5f, 0.5f, 1.0f, 1.0f };  // 蓝色 / Blue
            }
            ImGui::SameLine();
            // 添加静态盒子按钮
            // Add static box button
            if (ImGui::Button("添加静态盒体"))
            {
                auto e = scene.CreateEntity(name);
                scene.AddComponent<TransformComponent>(e, XMFLOAT3{ posX, posY, posZ });
                auto& srb = scene.AddComponent<RigidbodyComponent>(e);
                srb.Mass = 0.0f;  // PxRigidStatic
                scene.AddComponent<ColliderComponent>(e);
                auto& mesh = scene.AddComponent<MeshComponent>(e);
                mesh.TintColor = XMFLOAT4{ 0.6f, 0.6f, 0.6f, 1.0f };  // 灰色 / Gray
            }
        }

        // 射线检测调试面板
        // Raycast debug panel
        if (ImGui::CollapsingHeader("射线检测"))
        {
            static float origin[3] = { 0.0f, 0.0f, -20.0f };
            static float dir[3] = { 0.0f, 0.0f, 1.0f };
            static float maxDist = 100.0f;
            static RaycastHit lastHit;

            // 从相机同步起点/方向（如果请求）
            // Sync origin/direction from camera if requested
            if (Camera* cam = scene.GetMainCamera())
            {
                XMFLOAT3 camPos = cam->GetPosition();
                if (ImGui::Button("使用相机位置"))
                {
                    origin[0] = camPos.x;
                    origin[1] = camPos.y;
                    origin[2] = camPos.z;
                }
                ImGui::SameLine();
                if (ImGui::Button("使用相机朝向"))
                {
                    XMVECTOR fwd = cam->GetForwardVector();
                    XMFLOAT3 fwdF;
                    XMStoreFloat3(&fwdF, fwd);
                    dir[0] = fwdF.x;
                    dir[1] = fwdF.y;
                    dir[2] = fwdF.z;
                }
            }

            ImGui::DragFloat3("起点", origin, 0.1f);
            ImGui::DragFloat3("方向", dir, 0.05f);
            ImGui::DragFloat("最大距离", &maxDist, 1.0f, 0.1f, 10000.0f);

            // 发射射线按钮
            // Cast ray button
            if (ImGui::Button("发射射线"))
            {
                PhysicsScene* ps = scene.GetPhysicsScene();
                if (ps && ps->IsValid())
                {
                    lastHit = ps->Raycast(
                        XMFLOAT3(origin[0], origin[1], origin[2]),
                        XMFLOAT3(dir[0], dir[1], dir[2]),
                        maxDist);
                }
            }

            ImGui::Separator();
            // 显示射线检测结果
            // Display raycast result
            if (lastHit.Hit)
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "命中");
                ImGui::Text("位置: (%.2f, %.2f, %.2f)",
                            lastHit.Position.x, lastHit.Position.y, lastHit.Position.z);
                ImGui::Text("法线:   (%.2f, %.2f, %.2f)",
                            lastHit.Normal.x, lastHit.Normal.y, lastHit.Normal.z);
                ImGui::Text("距离:  %.2f", lastHit.Distance);

                // 查找拥有此 actor 的实体
                // Find the entity that owns this actor
                if (lastHit.Actor)
                {
                    auto view = reg.view<RigidbodyComponent>();
                    const char* entityName = "<unknown>";
                    for (auto e : view)
                    {
                        auto& rb = reg.get<RigidbodyComponent>(e);
                        if (rb.Actor == lastHit.Actor)
                        {
                            if (auto* tag = reg.try_get<TagComponent>(e))
                            {
                                entityName = tag->Name.c_str();
                            }
                            else
                            {
                                entityName = "<unnamed>";
                            }
                            break;
                        }
                    }
                    ImGui::Text("实体:    %s", entityName);
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "未命中 (或尚未发射)");
            }
        }

        // 碰撞事件日志面板
        // Collision event log panel
        if (ImGui::CollapsingHeader("碰撞事件"))
        {
            // 每帧获取并显示碰撞事件
            // Get and display collision events each frame
            auto events = GetAndClearCollisionEvents();

            ImGui::Text("本帧事件数: %d", (int)events.size());
            ImGui::Separator();

            // 保留最近 20 条事件用于显示
            // Keep last 20 events for display
            static std::vector<CollisionEvent> eventHistory;
            for (const auto& evt : events)
            {
                eventHistory.push_back(evt);
            }
            if (eventHistory.size() > 20)
            {
                eventHistory.erase(eventHistory.begin(), eventHistory.begin() + (eventHistory.size() - 20));
            }

            // 显示事件历史
            // Display event history
            for (int i = (int)eventHistory.size() - 1; i >= 0; i--)
            {
                const auto& evt = eventHistory[i];
                auto* tagA = reg.try_get<TagComponent>(evt.EntityA);
                auto* tagB = reg.try_get<TagComponent>(evt.EntityB);
                const char* nameA = tagA ? tagA->Name.c_str() : "<无名>";
                const char* nameB = tagB ? tagB->Name.c_str() : "<无名>";

                if (evt.IsTrigger)
                {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[触发器] %s <-> %s", nameA, nameB);
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[碰撞] %s <-> %s", nameA, nameB);
                    ImGui::Text("  接触点: (%.2f, %.2f, %.2f)", evt.ContactPoint.x, evt.ContactPoint.y, evt.ContactPoint.z);
                    ImGui::Text("  法线: (%.2f, %.2f, %.2f)  距离: %.3f", evt.ContactNormal.x, evt.ContactNormal.y, evt.ContactNormal.z, evt.ContactDistance);
                }
            }

            if (ImGui::Button("清空事件历史"))
            {
                eventHistory.clear();
            }
        }

        ImGui::End();
    }
}

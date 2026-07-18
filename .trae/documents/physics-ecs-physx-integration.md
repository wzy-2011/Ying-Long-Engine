# Dracovis Engine 物理系统 ECS+PhysX 完整集成

## Context

当前物理系统是空壳状态，存在以下结构性问题：

1. **PhysicsScene API 缺失**：只有 `InistializePhysicsScene()` 和 `GetPhysicsScene()`，没有 `Simulate`/`AddActor`/`FetchResults` 包装。[Application.cpp:294-295](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\Application\Application.cpp#L294-L295) 直接调用裸 `PxScene*` API，绕过抽象层。
2. **PhysicsCapsule API 错误**：[PhysicsCapsule.cpp:53-80](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\Physics\PhysicsGeometry\PhysicsCapsule.cpp#L53-L80) 只创建 `PxMaterial` 和 `PxShape`，**从未创建 `PxRigidDynamic` actor**，shape 没附加到任何 actor，也没 add 到 scene——shape 漂浮无效，simulate 永远空跑。
3. **PhysicsSystem 是手写积分**：[PhysicsSystem.h](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\ECS\System\PhysicsSystem.h) 用 `XMFLOAT3 velocity/acceleration` 自己积分，**完全不调用 PhysX**，与 `PhysicsScene` 脱节。只重写 `Update(Entity&, float)`，没重写 `Scene::Update` 实际调用的 `UpdateScene(Scene&, float)`。
4. **DX12 路径不调用物理**：`Application::DoFrameDX12` 完全跳过 simulate，DX12 模式下物理停滞。
5. **RigidbodyComponent 不持有 PxRigidDynamic 句柄**，无法做物理-渲染同步。
6. **Scene 不持有 PhysicsScene 引用**，PhysicsSystem 无法获取物理场景。

用户已确认采用「完整 ECS+PhysX 集成 + 组件驱动同步 + DX12 也走 PhysicsScene」方案，并要求完善 DX11 模式下的物理适配。

## 目标

- PhysicsScene 提供完整 API 包装（Step/AddActor/RemoveActor/IsValid）
- ECS 组件设计：RigidbodyComponent 持有 PxRigidDynamic 句柄，新增 ColliderComponent
- PhysicsSystem 重写为 PhysX 驱动：通过 Scene::Update → UpdateScene 自动创建 actor、步进物理、回写 Transform
- Application 通过 Scene + PhysicsSystem 使用物理，移除裸 PxScene 调用
- DX11 与 DX12 双路径统一调用 PhysicsScene::Step
- PhysicsCapsule 标记 deprecated 但保留（不破坏现有 DX11 后备）

## 实施步骤

### 阶段 1：PhysicsScene API 补全

**文件**：[CodeFile/Physics/PhysicsScene/PhysicsScene.h](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\Physics\PhysicsScene\PhysicsScene.h) / [.cpp](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\Physics\PhysicsScene\PhysicsScene.cpp)

新增公共 API（保留 `InistializePhysicsScene` 原拼写，本次不改名）：

```cpp
// PhysicsScene.h 新增
bool Step(float dt);                       // simulate(dt) + fetchResults(true)，做 null 检查
void Simulate(float dt);                   // 裸 simulate（无 null 检查，调用方需保证）
bool FetchResults(bool block = true);
void AddActor(physx::PxActor& actor);
void RemoveActor(physx::PxActor& actor);
bool IsValid() const noexcept;             // PhysicsSceneObject != nullptr
```

`Step` 内部：若 `!IsValid()` 返回 false；否则 `Simulate(dt)` + `FetchResults(true)`。`AddActor`/`RemoveActor` 直接转发给 `PhysicsSceneObject`，调用方负责生命周期。

**验证**：编译通过；将 Application.cpp L294-295 改为 `PhysicsSceneObject.Step(16.6f)` 后行为不变（空场景模拟）。

---

### 阶段 2：ECS 组件设计 + 序列化适配

**文件**：
- [CodeFile/ECS/Components/Components.h](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\ECS\Components\Components.h)
- [CodeFile/ECS/Scene/Scene.cpp](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\ECS\Scene\Scene.cpp)（Save/Load）

**重构 RigidbodyComponent**：
```cpp
struct RigidbodyComponent {
    float Mass = 1.0f;
    bool UseGravity = true;
    bool IsKinematic = false;
    physx::PxRigidDynamic* Actor = nullptr;  // 运行时句柄，由 PhysicsSystem 创建，不序列化
};
```
移除手写 `Velocity`/`Acceleration`（由 PxRigidDynamic 内部管理）。

**新增 ColliderComponent**：
```cpp
enum class ColliderShape { Box, Sphere, Capsule };
struct ColliderComponent {
    ColliderShape Shape = ColliderShape::Box;
    DirectX::XMFLOAT3 HalfExtents = { 0.5f, 0.5f, 0.5f };  // Box
    float Radius = 0.5f;       // Sphere / Capsule
    float HalfHeight = 0.5f;   // Capsule
    float StaticFriction = 0.5f;
    float DynamicFriction = 0.5f;
    float Restitution = 0.3f;
};
```

**序列化适配**（Scene.cpp 的 Save/Load）：
- RigidbodyComponent：新增 `IsKinematic` 字段；`Actor` 不序列化
- 新增 ColliderComponent 段：序列化所有字段
- 旧场景文件无 ColliderComponent 段时使用默认值（向后兼容）

**验证**：编译通过；序列化往返不丢字段；现有无物理实体不受影响。

---

### 阶段 3：Scene 持有 PhysicsScene + PhysicsSystem 重写（核心）

**文件**：
- [CodeFile/ECS/Scene/Scene.h](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\ECS\Scene\Scene.h) / [.cpp](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\ECS\Scene\Scene.cpp)
- [CodeFile/ECS/System/PhysicsSystem.h](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\ECS\System\PhysicsSystem.h)（新增 .cpp）

#### Scene 改动
```cpp
// Scene.h 新增
void SetPhysicsScene(PhysicsScene* ps) noexcept { PhysicsScenePtr = ps; }
PhysicsScene* GetPhysicsScene() const noexcept { return PhysicsScenePtr; }
private:
    PhysicsScene* PhysicsScenePtr = nullptr;
```
需 `#include "../../Physics/PhysicsScene/PhysicsScene.h"` 或前向声明。

#### PhysicsSystem 重写为 PhysX 驱动

```cpp
// PhysicsSystem.h
class PhysicsSystem : public System
{
public:
    ~PhysicsSystem() override;
    void Initialize() override {}
    void UpdateScene(Scene& scene, float DeltaTime) override;
    void ShutDown() override;
private:
    void CreateActor(Scene& scene, entt::entity e, entt::registry& reg);
    void WriteBackToTransform(const physx::PxRigidDynamic& actor, TransformComponent& tr);
    static void OnRigidbodyDestroyed(entt::registry& r, entt::entity e);
    bool m_hooked = false;  // on_destroy 信号只连一次
};
```

#### 关键实现要点

**`UpdateScene` 流程**：
1. 首次调用时连接 `reg.on_destroy<RigidbodyComponent>()` 信号到 `OnRigidbodyDestroyed`
2. 遍历 `view<TransformComponent, RigidbodyComponent, ColliderComponent>`：
   - 若 `rb.Actor == nullptr` → 调用 `CreateActor` 创建 PxMaterial + PxShape + PxRigidDynamic，setGlobalPose 用 TransformComponent 初始 pose，AddActor 到 PhysicsScene，回填 `rb.Actor`
3. 调用 `scene.GetPhysicsScene()->Step(DeltaTime)`（单点步进，所有 actor 一起）
4. 遍历同 view：若 `rb.Actor && !rb.IsKinematic` → 把 PxRigidDynamic 的 globalPose 写回 TransformComponent（Position + Rotation）

**`CreateActor`**：
- `Physics::PhysicsObject->createMaterial(staticFriction, dynamicFriction, restitution)`
- 按 `ColliderShape` 创建 `PxBoxGeometry`/`PxSphereGeometry`/`PxCapsuleGeometry`，调用 `Physics::PhysicsObject->createShape(geom, *material)`
- `Physics::PhysicsObject->createRigidDynamic(PxTransform(initialPos))`
- `actor->attachShape(*shape)` 后 `shape->release()`（actor 持有引用）
- `PxRigidBodyExt::updateMassAndInertia(*actor, rb.Mass)`
- 若 `rb.UseGravity` 否，`actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY)`
- 若 `rb.IsKinematic`，`actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true)`
- `scene.GetPhysicsScene()->AddActor(*actor)`
- `rb.Actor = actor`

**`OnRigidbodyDestroyed`**（entt 销毁钩子）：
```cpp
auto* rb = r.try_get<RigidbodyComponent>(e);
if (rb && rb->Actor) {
    if (auto* s = rb->Actor->getScene()) s->removeActor(*rb->Actor);
    rb->Actor->release();   // 自动释放 attached shapes
    rb->Actor = nullptr;
}
```

**`ShutDown`**：遍历当前 registry 释放所有 actor（应对 Scene::Unload 时 Clear 触发 on_destroy 信号 + 双重保险）。

**`WriteBackToTransform`**：从 `PxRigidDynamic::getGlobalPose()` 的 `PxVec3 p` 和 `PxQuat q` 写回 `TransformComponent.Position/Rotation`（Rotation 用欧拉角，可简化为 `XMQuaternionToAxisAngle` 或仅存 quat 字段；本次保留欧拉，简单转换）。

**验证**：手动构造一个挂三组件的 entity，Step 后 Position.y 随重力下降；DestroyEntity 后无 PhysX 泄漏告警；ShutDown 释放干净。

---

### 阶段 4：Application 接线 + DX11/DX12 双路径统一 + PhysicsCapsule deprecated

**文件**：
- [CodeFile/Application/Application.cpp](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\Application\Application.cpp)
- [CodeFile/Physics/PhysicsGeometry/PhysicsCapsule.h](file:///e:\Projects\Dracovis-Engine\Dracovis-Engine\Dracovis Engine\CodeFile\Physics\PhysicsGeometry\PhysicsCapsule.h)

#### Application 改动

1. **`InitializeScene`**（L157）末尾、`Activate()` 之前插入：
```cpp
CurrentScene->SetPhysicsScene(&PhysicsSceneObject);
CurrentScene->AddSystem<PhysicsSystem>();
```
需在 Application.cpp 顶部 include `../ECS/System/PhysicsSystem.h`。

2. **`DoFrame`**：**删除 L294-295** 硬编码 `simulate/fetchResults`。物理改由 L244 已存在的 `CurrentScene->Update(16.5f)` 经 PhysicsSystem::UpdateScene 自动触发。

3. **`DoFrameDX12`**（L488）开头、渲染前插入：
```cpp
if (CurrentScene) CurrentScene->Update(0.016f);
```
使 DX12 路径也走 PhysicsSystem→Step。

4. **构造函数初始化顺序**保持：`Physics::Create`（L27）→ `PhysicsSceneObject.InistializePhysicsScene`（L65）→ `InitializeScene`（L84）内 `SetPhysicsScene` → `AddSystem<PhysicsSystem>`。线程内 `InitializeScene` 已在 `PhysicsInitializingThread.join()` 之后（L81→L84），顺序安全。

5. **`capsule` 处理**：保留 `capsule.InitializeCapsuleObject()`（L66）和 `capsule.Shutdown()`（L154），添加注释 `// DEPRECATED: 保留为 legacy，新代码应使用 ECS RigidbodyComponent+ColliderComponent`。

6. **L294-295 替换**：删除裸 PxScene 调用，改为依赖 Scene::Update。

#### PhysicsCapsule.h 标记 deprecated
在文件顶部加注释：
```cpp
// DEPRECATED: 此类仅创建 PxShape 但未创建 PxRigidDynamic actor，shape 不会进入物理模拟。
// 新代码应使用 ECS 的 RigidbodyComponent + ColliderComponent，由 PhysicsSystem 自动创建 actor。
// 保留此类是为了 DX11 后备兼容，不再推荐使用。
```

#### 新增示例 entity 验证物理

在 `InitializeScene` 末尾添加示例 entity 用于验证：
```cpp
auto e = CurrentScene->CreateEntity("PhysicsTestBox");
CurrentScene->AddComponent<TransformComponent>(e, XMFLOAT3{0.0f, 5.0f, 0.0f});
CurrentScene->AddComponent<RigidbodyComponent>(e);  // 默认 Mass=1, UseGravity=true
CurrentScene->AddComponent<ColliderComponent>(e);   // 默认 Box 0.5
```
启动后该 entity 应在 (0, 5, 0) 出现并自由落体下落（DX11/DX12 均生效）。可通过 ImGui 控制台观察 entity 的 TransformComponent.Position.y 变化。

**验证**：
- DX11 模式：entity 受重力下落（通过 ImGui 监控或扩展控制窗口）
- DX12 模式：同样下落
- 关闭窗口无 PhysX 泄漏告警
- 切换 DX11/DX12 时物理状态连续不重置

---

## 关键设计决策

1. **物理在主线程步进**：不引入互斥锁，PhysX 自身线程安全，应用层场景切换单线程。
2. **懒初始化 actor**：PhysicsSystem::UpdateScene 遍历时若发现 `rb.Actor == nullptr` 才创建，避免分两阶段初始化的复杂度。
3. **entt on_destroy 信号**：自动处理 entity 销毁时的 actor 释放，无需 Scene 配合。
4. **WriteBack 仅非 kinematic**：kinematic body 由用户代码 setKinematicTarget 驱动，PhysicsSystem 不写回；dynamic body 由物理驱动，每帧写回 Transform。
5. **保留 PhysicsCapsule legacy**：不删代码，避免破坏现有 DX11 后备路径。
6. **不在本次实现 Raycast**：PhysicsScene 预留 `Raycast()` 接口注释，留待下个任务。
7. **不改造 DX12Box 渲染对象**：DX12 渲染对象保持现状，物理 entity 通过 ImGui 控制台观察验证；将 entity 与 DX12 渲染对象绑定是下个增强任务。

## 风险与对策

| 风险 | 对策 |
|---|---|
| PxShape/PxMaterial 在 PxPhysics 之前释放 | actor release 自动释放其 attached shapes；PhysicsContext（PxPhysics）在 Application 成员最后析构 |
| on_destroy 在 Scene::Clear 时触发但 PhysicsScene 可能已失效 | `OnRigidbodyDestroyed` 用 `actor->getScene()` 判空，已失效则跳过 removeActor |
| 双 registry（Scene vs Entity::static） | PhysicsSystem 只用 `Scene::GetRegistry()`，Entity 类 legacy 不动 |
| DX12 切换时物理 entity 状态延续 | 同一 CurrentScene 跨 DX11/DX12，物理状态连续，无重置 |
| PhysX 在 release 模式下的 NDEBUG 问题（已知预存在） | 不在本次范围；用户使用 Debug 配置验证 |

## 验证清单

- [ ] 阶段 1：编译通过；Application L294-295 改为 `Step(16.6f)` 后行为不变
- [ ] 阶段 2：编译通过；Scene::Save/Load 往返不丢字段；无物理实体不受影响
- [ ] 阶段 3：编译通过；构造测试 entity 后 Step 受重力下落；DestroyEntity 无 PhysX 泄漏告警
- [ ] 阶段 4：DX11 模式 entity 下落可见；DX12 模式同样下落；切换模式物理状态连续
- [ ] 全部完成后：Debug 编译通过；启动 → 切换 DX11/DX12 → 关闭窗口全流程无异常；无 PhysX 内存泄漏告警

## 不在本次范围

- 不实现 Raycast（PxRaycast 集成）
- 不改造 DX12Box 渲染对象为 entity 绑定（增强项，下个任务）
- 不重构 PhysicsCapsule（标记 deprecated 保留）
- 不重命名 `InistializePhysicsScene`（拼写错误，避免破坏调用点）
- 不解决 PX 崩溃问题（用户已确认不影响正常使用）
- 不解决 Release 模式 PhysX NDEBUG 配置问题（预存在）

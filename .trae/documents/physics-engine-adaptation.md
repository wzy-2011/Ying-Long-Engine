# 物理系统完善与引擎适配实施计划

## 概述

延续上一会话已完成的 ECS+PhysX 基础集成，本轮工作聚焦于：
1. 修复物理系统关键 bug（静态体、生命周期清理、dt 单位）
2. 新增 MeshComponent + MeshRendererSystem，让 ECS 实体可在 DX11/DX12 双路径可视化
3. 替换 deprecated PhysicsCapsule 为 ECS 实体
4. 新增 ImGui 物理编辑器（运行时检视/编辑组件 + 实时应用到 PxActor）
5. 新增 PhysicsScene 射线检测 API + 调试面板

PX 崩溃问题用户确认不影响正常使用，本轮不处理。

## 现状分析

### 已完成（上一会话）
- PhysicsScene API：Step/Simulate/FetchResults/AddActor/RemoveActor/IsValid
- ECS 组件：TransformComponent / RigidbodyComponent（含 PxRigidDynamic*）/ ColliderComponent（Box/Sphere/Capsule）
- Scene 持有 PhysicsScene* 指针，AddSystem 模板可注册 PhysicsSystem
- PhysicsSystem::UpdateScene 懒创建 PxRigidDynamic、Step、回写 TransformComponent
- Application 接线：PhysicsTestBox 实体、SetPhysicsScene、AddSystem<PhysicsSystem>
- DX11/DX12 双路径统一通过 Scene::Update 驱动物理
- 序列化：RigidbodyComponent/ColliderComponent 完整 Save/Load

### 关键缺陷
| # | 缺陷 | 影响 |
|---|------|------|
| 1 | `RigidbodyComponent::Actor` 是 `PxRigidDynamic*`，无法表达静态体 | Mass=0 路径用 kinematic hack 绕过，无法做地面/墙体 |
| 2 | `PhysicsSystem::ShutDown` 空实现，依赖 on_destroy 信号（仅 Update 调用后才连接） | 场景从未 Update 即销毁时泄漏 PxActor |
| 3 | DX11 路径 `CurrentScene->Update(16.5f)` 传 16.5 秒 | PhysX 默认 maxTimestep=0.0166s，参数无效或爆炸 |
| 4 | ECS 实体无 MeshComponent，Scene::Render 只渲染 Models | PhysicsTestBox 不可见 |
| 5 | DoFrameDX12 只渲染硬编码 pDX12DemoScene | DX12 模式下 ECS 实体不可见 |
| 6 | 无地面静态体 | PhysicsTestBox 永远下落 |
| 7 | 无 ImGui 物理编辑器 | 无法运行时调试 |
| 8 | PhysicsScene 无 Raycast API | 无法做鼠标拾取等交互 |
| 9 | Application 仍持 deprecated PhysicsCapsule 成员 | 死代码 + 销毁路径混乱 |

## 实施变更

### 阶段 A：物理核心 Bug 修复

#### A1. 支持静态体（PxRigidStatic）

**文件**：[Components.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/Components/Components.h)

变更 `RigidbodyComponent::Actor` 类型：
```cpp
struct RigidbodyComponent {
    float Mass = 1.0f;            // 0 = PxRigidStatic; >0 = PxRigidDynamic
    bool UseGravity = true;
    bool IsKinematic = false;
    PxRigidActor* Actor = nullptr;  // 由 PxRigidDynamic 或 PxRigidStatic 赋值
};
```

**文件**：[PhysicsSystem.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.cpp)

重写 `CreateActor` 中 Mass 分支：
- `Mass > 0`：创建 `PxRigidDynamic`，调 `updateMassAndInertia`、设 gravity flag、kinematic flag
- `Mass == 0`：创建 `PxRigidStatic`，仅 attachShape + AddActor，无 mass/gravity/kinematic 设置

重写 `UpdateScene` 回写条件：
```cpp
if (rb.Actor && rb.Mass > 0.0f && !rb.IsKinematic) {
    WriteBackToTransform(static_cast<PxRigidDynamic&>(*rb.Actor), tr);
}
```

`OnRigidbodyDestroyed` 改用 `PxRigidActor*`（`release()` / `getScene()` / `removeActor()` 均为基类 API，无需改）。

#### A2. 修复 PhysicsSystem::ShutDown 显式清理

**文件**：[PhysicsSystem.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.h) + [PhysicsSystem.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.cpp)

新增 `Scene* m_scene = nullptr;` 成员，在 `UpdateScene` 入口记录 `m_scene = &scene;`。

`ShutDown` 实现显式遍历释放：
```cpp
void PhysicsSystem::ShutDown() {
    if (!m_scene) return;
    auto& reg = m_scene->GetRegistry();
    auto view = reg.view<RigidbodyComponent>();
    for (auto e : view) {
        auto& rb = reg.get<RigidbodyComponent>(e);
        if (rb.Actor) {
            if (auto* pxScene = rb.Actor->getScene()) pxScene->removeActor(*rb.Actor);
            rb.Actor->release();
            rb.Actor = nullptr;
        }
    }
    m_scene = nullptr;
}
```

#### A3. 修复 DX11 dt 单位

**文件**：[Application.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.cpp)

`DoFrame()` 中 `CurrentScene->Update(16.5f)` 改为 `CurrentScene->Update(0.016f)`（与 DX12 路径一致，假设 60fps；后续可换为真实帧计时）。

### 阶段 B：MeshComponent + MeshRendererSystem

#### B1. 新增 MeshComponent

**文件**：[Components.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/Components/Components.h)

```cpp
struct MeshComponent {
    MeshComponent() = default;
    MeshComponent(const MeshComponent&) = default;

    std::string ModelPath;                                    // 序列化 + 懒加载源
    std::shared_ptr<Model> ModelPtr;                          // DX11 运行时句柄，懒加载，不序列化
    XMFLOAT4 TintColor = { 1.0f, 1.0f, 1.0f, 1.0f };        // DX12 占位色
    bool Visible = true;
};
```

需要 `#include "../Graphics/Drawable/Model.h"` 与 `#include <memory>`。

#### B2. 新建 MeshRendererSystem

**新文件**：`CodeFile/ECS/System/MeshRendererSystem.h` + `.cpp`

```cpp
class MeshRendererSystem : public System {
public:
    MeshRendererSystem() = default;
    ~MeshRendererSystem() override;

    void Initialize() override {}
    void UpdateScene(Scene& scene, float DeltaTime) override;  // 清理失效缓存
    void ShutDown() override;

    // DX11 渲染：遍历 MeshComponent 实体，懒加载 Model，同步 Transform，Draw
    void RenderDX11(Scene& scene, Graphics& gfx);
    // DX12 渲染：遍历 MeshComponent 实体，懒创建 DX12Box（缓存），同步 Transform，Draw
    void RenderDX12(Scene& scene, DX12Core& core, ID3D12GraphicsCommandList* cmdList,
                    const float* viewMatrix, const float* projMatrix, float dt);

    void Update(Entity& entity, float DeltaTime) override {}  // no-op

private:
    // DX12 box 缓存：entity → DX12Box
    std::unordered_map<entt::entity, std::unique_ptr<DX12Box>> m_dx12Boxes;

    void PruneInvalid(Scene& scene);  // 移除已销毁实体的缓存
};
```

- `RenderDX11`：`auto view = reg.view<TransformComponent, MeshComponent>();` 遍历，`if (!mesh.ModelPtr && !mesh.ModelPath.empty()) mesh.ModelPtr = std::make_shared<Model>(gfx, mesh.ModelPath);`，同步 `ModelPtr->Position/Rotation/Scale = tr.*`，调用 `mesh.ModelPtr->Draw(gfx)`
- `RenderDX12`：遍历同视图，`m_dx12Boxes` 中懒创建 `DX12Box(core)`，`SetPosition/SetRotation/SetScale/SetColor/SetViewMatrix/SetProjectionMatrix`，`box->Update(dt); box->Draw(cmdList)`
- `PruneInvalid`：遍历 `m_dx12Boxes`，`if (!reg.valid(e)) m_dx12Boxes.erase(e);`
- `ShutDown`：`m_dx12Boxes.clear();`

#### B3. 扩展 Scene::Render

**文件**：[Scene.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/Scene/Scene.cpp)

在 `Scene::Render` 末尾（Models 渲染后）追加：
```cpp
if (auto* meshRenderer = GetSystem<MeshRendererSystem>()) {
    meshRenderer->RenderDX11(*this, graphics);
}
```

Scene.cpp 顶部 `#include "../System/MeshRendererSystem.h"`。

#### B4. 扩展 DoFrameDX12 渲染 ECS 实体

**文件**：[Application.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.cpp)

`DoFrameDX12()` 中，在 `pDX12DemoScene->Render(commandList)` 之后追加：
```cpp
if (CurrentScene) {
    auto* meshRenderer = CurrentScene->GetSystem<MeshRendererSystem>();
    if (meshRenderer) {
        // Camera::GetMatrix() / GetProjection() 返回 XMMATRIX，
        // 需通过 XMStoreFloat4x4 转 float[16] 供 DX12Primitive::SetViewMatrix 使用
        XMMATRIX viewMat = MainWindow.camera.GetMatrix();
        XMMATRIX projMat = MainWindow.camera.GetProjection();
        XMFLOAT4X4 viewF, projF;
        XMStoreFloat4x4(&viewF, viewMat);
        XMStoreFloat4x4(&projF, projMat);
        meshRenderer->RenderDX12(*CurrentScene, *renderer->GetCore(),
                                  commandList,
                                  reinterpret_cast<const float*>(&viewF),
                                  reinterpret_cast<const float*>(&projF),
                                  0.016f);
    }
}
```

> Camera.h 已确认 API：`GetMatrix()` / `GetProjection()` 返回 `XMMATRIX`，`GetForwardVector()` 返回 `XMVECTOR`，`GetPosition()` 返回 `XMFLOAT3`。射线检测起点用 `GetPosition()`，方向用 `GetForwardVector()`。

#### B5. MeshComponent 序列化

**文件**：[Scene.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/Scene/Scene.cpp)

`Save`：在 Collider 段之后追加 Mesh 段（ModelPath / TintColor / Visible）。
`Load`：解析 Mesh 段，构造 MeshComponent。

### 阶段 C：静态地面 + PhysicsCapsule 替换

#### C1. 新增地面静态体

**文件**：[Application.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.cpp)

`InitializeScene()` 中 PhysicsTestBox 之后追加：
```cpp
auto ground = CurrentScene->CreateEntity("Ground");
CurrentScene->AddComponent<TransformComponent>(ground, XMFLOAT3{ 0.0f, -0.5f, 0.0f });
auto& groundRb = CurrentScene->AddComponent<RigidbodyComponent>(ground);
groundRb.Mass = 0.0f;  // → PxRigidStatic
auto& groundCol = CurrentScene->AddComponent<ColliderComponent>(ground);
groundCol.Shape = ColliderShape::Box;
groundCol.HalfExtents = XMFLOAT3{ 50.0f, 0.5f, 50.0f };
auto& groundMesh = CurrentScene->AddComponent<MeshComponent>(ground);
groundMesh.TintColor = XMFLOAT4{ 0.4f, 0.4f, 0.4f, 1.0f };
```

#### C2. PhysicsTestBox 加 MeshComponent

同文件，PhysicsTestBox 创建处追加：
```cpp
auto& boxMesh = CurrentScene->AddComponent<MeshComponent>(physicsBox);
boxMesh.TintColor = XMFLOAT4{ 0.8f, 0.3f, 0.2f, 1.0f };
```

#### C3. 新增 Capsule 测试实体（替代 PhysicsCapsule）

```cpp
auto physicsCapsule = CurrentScene->CreateEntity("PhysicsTestCapsule");
CurrentScene->AddComponent<TransformComponent>(physicsCapsule, XMFLOAT3{ 2.0f, 5.0f, 0.0f });
CurrentScene->AddComponent<RigidbodyComponent>(physicsCapsule);
auto& capCol = CurrentScene->AddComponent<ColliderComponent>(physicsCapsule);
capCol.Shape = ColliderShape::Capsule;
capCol.Radius = 1.0f;
capCol.HalfHeight = 2.0f;
auto& capMesh = CurrentScene->AddComponent<MeshComponent>(physicsCapsule);
capMesh.TintColor = XMFLOAT4{ 0.2f, 0.8f, 0.4f, 1.0f };
```

#### C4. 移除 Application::capsule 成员

**文件**：[Application.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.h)

- 删除 `#include "../Physics/PhysicsGeometry/PhysicsCapsule.h"`
- 删除成员 `PhysicsCapsule capsule;`

**文件**：[Application.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.cpp)

- 删除构造函数初始化列表中的 `capsule(5.0f, 5.0f)`
- 删除 `PhysicsInitializingThread` 中的 `this->capsule.InitializeCapsuleObject();`
- 删除析构函数中的 `capsule.Shutdown();`

> PhysicsCapsule.h 文件本身保留（标注 deprecated 供参考），仅从 Application 移除使用。

### 阶段 D：ImGui 物理编辑器

#### D1. PhysicsSystem 编辑器面板

**文件**：[PhysicsSystem.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.h) + [.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.cpp)

新增方法：
```cpp
void RenderImGuiEditor(Scene& scene);
```

实现要点：
- `ImGui::Begin("Physics Editor")`
- **实体列表**：`reg.view<TransformComponent, RigidbodyComponent>()` 遍历，TreeNode 显示 TagComponent 名称
  - Position / Rotation（DragFloat3）— 直接改 TransformComponent
  - Mass / UseGravity / IsKinematic（DragFloat / Checkbox）— 改后实时应用到 PxActor：
    - `actor->setMass(rb.Mass)`（仅 dynamic）
    - `actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rb.UseGravity)`
    - `static_cast<PxRigidDynamic*>(actor)->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rb.IsKinematic)`
  - Collider 子节点：Shape（Combo Box/Sphere/Capsule）、HalfExtents/Radius/HalfHeight、Friction、Restitution
    - 注：运行时改变 collider 几何需重建 shape（高开销），本轮仅显示，标注 "需重建实体生效"
  - 显示运行时状态：当前 pose、是否 in scene
- **新建实体按钮**：点击后在场景原点创建一个默认 box 实体（Transform+Rigidbody+Collider+Mesh）

#### D2. 编辑器调用点

**文件**：[Application.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.cpp)

`DoFrame()` 与 `DoFrameDX12()` 中追加：
```cpp
if (CurrentScene) {
    if (auto* ps = CurrentScene->GetSystem<PhysicsSystem>()) {
        ps->RenderImGuiEditor(*CurrentScene);
    }
}
```

### 阶段 E：射线检测 API

#### E1. PhysicsScene::Raycast

**文件**：[PhysicsScene.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Physics/PhysicsScene/PhysicsScene.h) + [.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Physics/PhysicsScene/PhysicsScene.cpp)

新增结构与方法：
```cpp
struct RaycastHit {
    bool Hit = false;
    XMFLOAT3 Position = { 0, 0, 0 };
    XMFLOAT3 Normal = { 0, 0, 0 };
    float Distance = 0.0f;
    PxRigidActor* Actor = nullptr;
};

RaycastHit Raycast(const XMFLOAT3& origin, const XMFLOAT3& unitDir, float maxDist);
```

实现：`m_scene->raycast(PxVec3(...), PxVec3(...), maxDist, hitInfo, PxHitFlag::eDEFAULT)`，填充 RaycastHit。

#### E2. ImGui 射线调试面板

在 PhysicsSystem::RenderImGuiEditor 中追加 "Raycast" 折叠头：
- 起点（默认相机位置）、方向（默认相机前向）、最大距离
- "Cast Ray" 按钮 → 调 PhysicsScene::Raycast → 显示命中信息
- 命中点可视化（ImGui 文本显示坐标 + 命中 actor 的 entity tag）

### 阶段 F：项目文件 + 编译验证

#### F1. 更新 vcxproj

**文件**：[Dracovis Engine.vcxproj](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/Dracovis%20Engine.vcxproj) + [.filters](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/Dracovis%20Engine.vcxproj.filters)

追加：
```xml
<ClInclude Include="CodeFile\ECS\System\MeshRendererSystem.h" />
<ClCompile Include="CodeFile\ECS\System\MeshRendererSystem.cpp" />
```

#### F2. 编译验证

```
MSBuild "Dracovis Engine.vcxproj" /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

修复任何编译错误（预期会出现 Camera 矩阵 API 名字、yaml-cpp convert 等小问题）。

## 验证清单

- [ ] Debug x64 编译通过
- [ ] DX11 模式：PhysicsTestBox（橙色方块）从 y=5 自由下落，落到地面（灰色大方块）上停止
- [ ] DX11 模式：PhysicsTestCapsule（绿色胶囊）同样下落并滚动
- [ ] DX12 模式：同样可见下落（DX12Box 占位渲染）
- [ ] F5 切换 DX11/DX12：物理状态连续（同一 PhysicsScene 持续模拟）
- [ ] ImGui "Physics Editor" 面板可列出实体、实时改 Mass/Gravity/Kinematic 并生效
- [ ] ImGui "Raycast" 面板可从相机位置投射射线，命中后显示坐标与实体名
- [ ] 关闭窗口无 PhysX 泄漏 abort（已知 PX 崩溃问题不在此列）

## 假设与决策

1. **DX12 渲染 ECS 实体用 DX12Box 占位** — 真正的 DX12 Mesh 类需要完整的顶点/索引/材质管线，本轮不实现。DX12Box 按 TransformComponent 位置 + ColliderComponent 尺寸（如有）渲染占位立方体。
2. **ImGui 编辑器修改 collider 几何不实时生效** — PxShape 几何不可变，需重建实体。本轮仅显示，标注说明。
3. **dt 使用固定 0.016f** — 不引入帧计时器，保持简单。后续可换为 chrono steady_clock。
4. **PhysicsCapsule.h 文件保留** — 仅从 Application 移除使用，类定义保留供参考。
5. **Camera 矩阵 API** — 已确认 Camera.h 提供 `GetMatrix()` / `GetProjection()` 返回 XMMATRIX，通过 `XMStoreFloat4x4` 转 float[16] 给 DX12Primitive::SetViewMatrix/SetProjectionMatrix。
6. **MeshComponent 序列化仅存 ModelPath + TintColor + Visible** — ModelPtr 是运行时懒加载，不序列化。
7. **静态体回写跳过** — PxRigidStatic 不可移动，UpdateScene 中 `Mass > 0 && !IsKinematic` 才回写。

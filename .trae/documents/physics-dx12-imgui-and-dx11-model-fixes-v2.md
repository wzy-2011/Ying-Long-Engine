# 物理/ImGui/模型三问题修复实施计划 v2

## 概述

本计划解决用户最新反馈的三个问题：

1. **DX11 模型不可见**：之前相机同步修复未解决，根因是相机配置文件累积漂移（Y 轴 1033°）
2. **DX12 ImGui 鬼闪严重**：ImGuiDX12 单缓冲区在 `FRAME_COUNT=2` 双缓冲下被多帧共享
3. **DX12 物理不稳定**：新创建的正方体下落后到处乱飞，根因是 `Restitution=0.3` + 无阻尼

---

## Phase 1 探索结论

### 问题 1：DX11 模型不可见

**根因链**：
- `Application.cpp:71` 加载 `SceneData/Camera/MainCamera.camera`
- 配置文件内容为 `Position: [16.4, 10.1, -14.8]`, `Rotation: [27, 1033, 11.4]`
- Y 轴旋转 1033°（≈2.87 圈），相机视角完全偏离模型
- `Application.cpp:164` `CurrentScene->SetMainCamera(std::make_unique<Camera>(MainWindow.camera))` 把漂移相机复制进场景
- 虽然 `Application.cpp:281` 同步 `*sceneCam = MainWindow.camera`，但 `MainWindow.camera` 本身已经是漂移状态

**Camera 旋转单位**：`Camera.cpp:7-19` 中 `(Rotation.x / 360.0f) * XM_2PI` 表示 Rotation 以度为单位，1033° 会被转换成有效角度，但累积漂移说明相机控制逻辑没有规范化

**Camera::Reset()**：`Camera.cpp:214-223` 已存在 Reset 方法，但未被调用

### 问题 2：DX12 ImGui 鬼闪

**根因确认**：
- `ImGuiDX12.h:52-53` 使用单个 `pVertexBuffer`/`pIndexBuffer`（`ComPtr<ID3D12Resource>`）
- `FRAME_COUNT=2`（`DX12Core.h:24`），DX12 双缓冲渲染
- `ImGuiDX12.cpp:286-317` 中 `EndFrame` 每帧 Map + memcpy 覆盖同一缓冲区
- 当 CPU 在帧 N+1 覆盖缓冲区时，帧 N 的 GPU 可能仍在读取 → 数据损坏 → 鬼闪

**已确认排除的原因**（来自上一会话）：
- BlendState 已正确配置 `SrcBlendAlpha=ONE, DestBlendAlpha=INV_SRC_ALPHA`
- AlphaMode 已设置为 `DXGI_ALPHA_MODE_IGNORE`
- ViewportsEnable 和 DockingEnable 已禁用

**DX12Core API**：
- `DX12Core::GetCurrentBackBufferIndex()` 返回当前帧索引（0 或 1）
- `DX12Core::GetFrameCount()` 返回 `FRAME_COUNT=2`

### 问题 3：DX12 物理不稳定

**根因**：
- `Components.h:91` `float Restitution = 0.3f` — 默认弹性 30%，弹跳明显
- `RigidbodyComponent`（`Components.h:48-61`）**没有** `LinearDamping`/`AngularDamping` 字段
- `PhysicsSystem.cpp:196-218` `CreateActor` 中 `PxRigidDynamic` 创建后**未设置阻尼**
- 结果：物体落地后弹跳 + 角动量不衰减 → 到处乱飞

**PhysX API**：
- `PxRigidDynamic::setLinearDamping(float)` — 线性阻尼
- `PxRigidDynamic::setAngularDamping(float)` — 角阻尼
- 默认值都是 0（无阻尼）

---

## Phase 3 实施方案

### 修改 1：DX11 相机修复

**文件**：`Dracovis Engine/SceneData/Camera/MainCamera.camera`

**修改**：重置配置文件内容为合理值
```yaml
Position: [0.0, 5.0, -15.0]
Rotation: [0.0, 0.0, 0.0]
```

**理由**：
- 相机位于 `(0, 5, -15)`，看向 +Z 方向（`Camera.cpp:10` 中 `XMVectorSet(0,0,1,0)` 为默认 focus 方向）
- 模型在场景原点附近，距离 15 单位合适
- 旋转归零，消除累积漂移

**文件**：`Dracovis Engine/CodeFile/Graphics/Camera/Camera.cpp`

**修改**：在 `ControlCameraRotation` 末尾添加旋转规范化，防止未来累积漂移

需要先读取 `ControlCameraRotation` 完整实现（`Camera.cpp:155-165`）确认修改位置。规范化逻辑：将每个轴的旋转角度限制在 `[0, 360)` 范围内。

### 修改 2：ImGuiDX12 重构为每帧缓冲区

**文件**：`Dracovis Engine/CodeFile/Graphics/DX12/ImGuiDX12.h`

**修改**：
1. 添加 `#include <array>` 和 `#include "../../Graphics/DX12/DX12Core.h"`（前向声明已存在）
2. 删除单缓冲区成员：
   ```cpp
   Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer;
   Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer;
   UINT VertexBufferSize;
   UINT IndexBufferSize;
   std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingVertexBuffers;
   std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingIndexBuffers;
   UINT frameCount = 2;
   ```
3. 添加每帧资源结构：
   ```cpp
   struct ImGuiFrameResource
   {
       Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
       Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
       UINT VertexBufferSize = 0;
       UINT IndexBufferSize = 0;
   };
   std::array<ImGuiFrameResource, FRAME_COUNT> FrameResources;
   UINT CurrentFrameIndex = 0;
   ```
4. 添加方法 `UINT GetCurrentFrameIndex() const noexcept { return CurrentFrameIndex; }`
5. 添加 `DX12Core* pCore = nullptr;` 用于获取当前帧索引

**文件**：`Dracovis Engine/CodeFile/Graphics/DX12/ImGuiDX12.cpp`

**修改 1**：`Initialize` 函数（行 34-89）
- 在 `CreateResources(core)` 调用前保存 `pCore = &core;`

**修改 2**：`Shutdown` 函数（行 91-119）
- 删除 `pVertexBuffer.Reset()` / `pIndexBuffer.Reset()` / `pendingVertexBuffers.clear()` / `pendingIndexBuffers.clear()`
- 添加循环重置每帧资源：
  ```cpp
  for (auto& res : FrameResources)
  {
      res.VertexBuffer.Reset();
      res.IndexBuffer.Reset();
      res.VertexBufferSize = 0;
      res.IndexBufferSize = 0;
  }
  pCore = nullptr;
  ```

**修改 3**：`EndFrame` 函数（行 143-404）
- 在函数开始处同步当前帧索引：
  ```cpp
  if (pCore)
      CurrentFrameIndex = pCore->GetCurrentBackBufferIndex();
  auto& frameRes = FrameResources[CurrentFrameIndex];
  ```
- 替换所有 `pVertexBuffer` → `frameRes.VertexBuffer`
- 替换所有 `pIndexBuffer` → `frameRes.IndexBuffer`
- 替换所有 `VertexBufferSize` → `frameRes.VertexBufferSize`
- 替换所有 `IndexBufferSize` → `frameRes.IndexBufferSize`
- 删除延迟释放队列逻辑（行 400-403），因为每帧独立缓冲区不再需要

**修改 4**：缓冲区重建逻辑（行 192-278）
- VB 重建：使用 `frameRes.VertexBuffer` 替代 `pVertexBuffer`
- IB 重建：使用 `frameRes.IndexBuffer` 替代 `pIndexBuffer`
- 移除 `pendingVertexBuffers.push_back(std::move(...))` 和 `pendingIndexBuffers.push_back(std::move(...))`
- 因为每帧独立，旧缓冲区可以直接 Reset 而不会 GPU use-after-free（GPU 已经在该帧的 fence 同步下完成）

**修改 5**：缓冲区上传和绑定（行 286-359）
- 使用 `frameRes.VertexBuffer` 和 `frameRes.IndexBuffer` 的 Map/Unmap
- VBV/IBV 使用 `frameRes.VertexBuffer->GetGPUVirtualAddress()`

### 修改 3：物理参数修复

**文件**：`Dracovis Engine/CodeFile/ECS/Components/Components.h`

**修改 1**：`RigidbodyComponent`（行 48-61）添加阻尼字段
```cpp
struct RigidbodyComponent
{
    // ... existing fields ...
    float Mass = 1.0f;
    bool UseGravity = true;
    bool IsKinematic = false;

    // Damping applies only to PxRigidDynamic (Mass > 0). Higher values cause
    // velocity to decay faster. PhysX default is 0 (no damping).
    float LinearDamping = 0.1f;
    float AngularDamping = 0.1f;

    PxRigidActor* Actor = nullptr;
};
```

**修改 2**：`ColliderComponent`（行 72-92）降低默认 Restitution
```cpp
float Restitution = 0.1f;  // was 0.3f — 0.3 caused visible bouncing
```

**文件**：`Dracovis Engine/CodeFile/ECS/System/PhysicsSystem.cpp`

**修改 1**：`CreateActor`（行 196-218）在 `updateMassAndInertia` 后设置阻尼
```cpp
PxRigidBodyExt::updateMassAndInertia(*dynamic, rb.Mass);
dynamic->setLinearDamping(rb.LinearDamping);
dynamic->setAngularDamping(rb.AngularDamping);
if (!rb.UseGravity)
    dynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
if (rb.IsKinematic)
    dynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
```

**修改 2**：`RenderImGuiEditor`（行 312-348）在 Rigidbody 树节点中添加阻尼编辑 UI

在 `kinematicChanged` 之后添加：
```cpp
bool dampingChanged = ImGui::DragFloat("Linear Damping", &rb.LinearDamping, 0.01f, 0.0f, 5.0f);
dampingChanged |= ImGui::DragFloat("Angular Damping", &rb.AngularDamping, 0.01f, 0.0f, 5.0f);
```

在 `if (rb.Mass > 0.0f)` 块中添加：
```cpp
if (dampingChanged)
{
    dyn->setLinearDamping(rb.LinearDamping);
    dyn->setAngularDamping(rb.AngularDamping);
}
```

---

## 假设与决策

1. **相机重置位置选择 `(0, 5, -15)`**：基于 `Camera.cpp` 默认 focus 方向 `(0,0,1)`，相机在 -Z 方向看向 +Z，模型在原点附近。高度 5 提供俯视角度便于观察。

2. **ImGuiDX12 使用 `std::array<ImGuiFrameResource, FRAME_COUNT>`**：因为 `FRAME_COUNT` 是编译时常量（`DX12Core.h:24` `constexpr UINT FRAME_COUNT = 2`），`std::array` 是最合适的选择。

3. **每帧缓冲区不再需要延迟释放队列**：因为每帧的缓冲区只在该帧使用，下一帧该缓冲区会被新的 Map/Unmap 覆盖。DX12Core 的 `MoveToNextFrame`（`DX12Core.cpp:639-650`）已经通过 Fence 确保该帧的 GPU 工作完成，所以覆盖是安全的。

4. **阻尼默认值 0.1**：PhysX 推荐值范围 0.05-0.5，0.1 提供轻微阻尼而不影响物理真实感。

5. **Restitution 默认值改为 0.1**：0.3 导致明显弹跳，0.1 接近真实世界硬表面材料（如木材 0.1-0.2）。

6. **相机规范化**：在 `ControlCameraRotation` 末尾添加，将每个轴的角度限制在 `[0, 360)` 范围内，防止未来再次累积漂移。

---

## 验证步骤

### 编译验证
1. 使用单线程编译（`/FS` 标志，避免 PDB 死锁）
2. 确认无编译错误和警告

### 运行时验证
1. **DX11 模式**：
   - 启动应用，进入 DX11 模式
   - 确认模型可见（Nanosuit 和 Cerberus）
   - 确认相机控制正常（WASD+RMB 移动，WASD+LMB 旋转）
   - 检查 `MainCamera.camera` 文件不再累积漂移

2. **DX12 模式**：
   - 启动应用，进入 DX12 模式
   - 确认 ImGui 窗口不再鬼闪
   - 创建新的正方体（通过 Physics Editor）
   - 确认正方体下落后稳定着陆，不乱飞
   - 在 Physics Editor 中调整阻尼参数，确认实时生效

### 回归测试
1. 确认 DX11 模式下其他功能（光源、背景色、ImGui）正常
2. 确认 DX12 模式下其他功能（Demo Scene、ECS 渲染）正常
3. 确认物理系统在 DX11/DX12 双模式下都稳定

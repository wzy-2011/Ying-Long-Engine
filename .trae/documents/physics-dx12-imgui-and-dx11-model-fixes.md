# 物理系统、DX12 ImGui 鬼闪、DX11 模型可见性修复计划

## Context（背景）

用户最新反馈三个问题：
1. **DX11 模型不可见**："不行，模型依然没有出现"（之前的相机修复未解决问题）
2. **DX12 ImGui 鬼闪严重**："建议直接重构DX12的ImGui部分"
3. **DX12 物理不稳定**："创建新的正方体后会先下落，碰到地面后到处乱飞一会儿后才落地"

经过详细代码审查，已确认每个问题的根因如下。

---

## 问题 1：DX11 模型不可见

### 根因分析

通过代码审查排除了以下假设：
- **模型加载失败**：资源文件存在（`Resources/Cube/Cube.obj`、`Resources/Cerberus/Cerberus.fbx`），且 ModelImporter 在文件不存在时会触发 `assert(false)` + 空指针解引用导致崩溃。应用未崩溃，说明模型加载成功。
- **渲染管线错误**：`Scene::Render` → `model->Draw(graphics)` → `mesh->Draw(gfx)` → `TransformConstantBuffer::Bind` 使用 `graphics.GetCamera().GetMatrix()` 获取相机矩阵，路径正确。
- **相机同步错误**：已在上次修复中正确同步 `*sceneCam = MainWindow.camera`。

**真正根因**：相机配置文件 `SceneData/Camera/MainCamera.camera` 保存了一个有问题的相机状态：
```
Position: [16.3974667, 10.0630665, -14.819562]
Rotation: [27, 1033, 11.3559999]
```
- Y 轴旋转 1033°（≈2.87 圈）说明相机在多次会话中累积漂移
- 虽然数学上等价于 313°，但用户可能无法通过当前控制方案（WASD+LMB 旋转）将相机转回有效视角

**次要问题**：`Graphics::UpdateSceneGraphicsResolution` 更新 `CameraObject.Aspect`，但随后 `Scene::Render` 调用 `graphics.SetCamera(*MainCamera)` 覆盖了 CameraObject，导致 Aspect 始终为 800/600（MainCamera 的默认值），渲染画面可能拉伸但不会导致完全不可见。

### 修复方案

**A. 重置相机到已知良好位置**

修改 `Application::InitializeScene`（[Application.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Application/Application.cpp#L158) 第 158 行附近）：
- 在 `SetMainCamera` 之前，重置 `MainWindow.camera` 到合理位置（如 `(0, 5, -15)` 看向原点）
- 不依赖可能已损坏的保存文件

**B. 修复相机 Aspect 同步**

修改 `Graphics::UpdateSceneGraphicsResolution`（[Graphics.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Graphics/Graphics.cpp#L114) 第 114 行）：
- 当前只更新 `this->CameraObject.SetResolution(...)`
- 需要同时更新或返回新的 Aspect，以便 `MainWindow.camera` 也能同步

**C. 验证渲染管线**

添加 DX12Log 调试日志确认：
- Scene::Render 被调用
- Models 数量
- 每个 Model 的 MeshList 大小

---

## 问题 2：DX12 ImGui 鬼闪（需重构）

### 根因分析（已确认）

`ImGuiDX12` 使用**单个**顶点/索引缓冲区，在 `FRAME_COUNT=2` 双缓冲下被多帧共享：

```cpp
// ImGuiDX12.h 中的成员
Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer;
Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer;
UINT VertexBufferSize = 0;
UINT IndexBufferSize = 0;
```

**问题流程**：
1. 帧 N：CPU 通过 `Map`/`memcpy`/`Unmap` 写入顶点/索引数据
2. 帧 N：GPU 从命令队列执行绘制（异步）
3. 帧 N+1：CPU 再次 `Map` 并覆盖同一缓冲区
4. 如果帧 N 的 GPU 工作尚未完成 → **GPU 读取到半新半旧的数据 → 鬼闪/损坏**

`DX12Core::MoveToNextFrame` 虽然使用 Fence 等待，但 ImGui 缓冲区没有按帧分离。

### 修复方案：重构 ImGuiDX12 使用每帧缓冲区

修改 [ImGuiDX12.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Graphics/DX12/ImGuiDX12.h) 和 [ImGuiDX12.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/Graphics/DX12/ImGuiDX12.cpp)：

```cpp
// ImGuiDX12.h - 替换单缓冲为每帧缓冲数组
struct FrameResources
{
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
    UINT VertexBufferSize = 0;
    UINT IndexBufferSize = 0;
};
std::array<FrameResources, FRAME_COUNT> m_frameResources;
UINT m_currentFrameIndex = 0;
```

`EndFrame` 修改要点：
1. 使用 `m_currentFrameIndex` 索引当前帧的资源
2. 仅在当前帧缓冲区不够大时重建（不影响其他帧）
3. 绘制完成后，`m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAME_COUNT`

`BeginFrame` 修改要点：
- 从 `DX12Core::GetCurrentBackBufferIndex()` 获取当前帧索引，确保与渲染目标同步

移除不再需要的成员：
- `pVertexBuffer`、`pIndexBuffer`、`VertexBufferSize`、`IndexBufferSize`
- `pendingVertexBuffers`、`pendingIndexBuffers`（延迟释放队列不再需要，因为每帧有独立缓冲）

---

## 问题 3：DX12 物理不稳定

### 根因分析（已确认）

查看 [PhysicsSystem.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.cpp#L132) 的 `CreateActor`：

```cpp
// 材质参数（来自 ColliderComponent 默认值）
StaticFriction = 0.5f;
DynamicFriction = 0.5f;
Restitution = 0.3f;  // ← 弹性 30%，导致弹跳

// 创建 PxRigidDynamic 后：
PxRigidBodyExt::updateMassAndInertia(*dynamic, rb.Mass);
// ← 没有设置 LinearDamping / AngularDamping（默认为 0）
// ← 没有启用 CCD（连续碰撞检测）
```

**问题**：
- Box 从 y=5 自由落体，落地速度 ≈ √(2×9.81×4.5) ≈ 9.4 m/s
- Restitution=0.3 → 反弹速度 ≈ 2.8 m/s（明显弹跳）
- 无 AngularDamping → 碰撞产生的角动量持续旋转（乱飞）
- 无 LinearDamping → 弹跳能量衰减慢

### 修复方案

**A. 降低默认弹性，增加阻尼**

修改 [Components.h](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/Components/Components.h#L72) 中的 `ColliderComponent` 默认值：

```cpp
float Restitution = 0.1f;  // 从 0.3 降低（几乎不弹）
```

修改 `RigidbodyComponent` 增加阻尼字段：

```cpp
struct RigidbodyComponent
{
    // ... 现有字段 ...
    float LinearDamping = 0.1f;   // 新增：线性阻尼
    float AngularDamping = 0.1f;  // 新增：角阻尼
};
```

**B. 在 CreateActor 中应用阻尼**

修改 [PhysicsSystem.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.cpp#L196) 第 196 行附近，创建 `PxRigidDynamic` 后：

```cpp
PxRigidBodyExt::updateMassAndInertia(*dynamic, rb.Mass);
dynamic->setLinearDamping(rb.LinearDamping);    // 新增
dynamic->setAngularDamping(rb.AngularDamping);  // 新增
```

**C. ImGui 编辑器支持实时修改阻尼**

修改 [PhysicsSystem.cpp](file:///e:/Projects/Dracovis-Engine/Dracovis-Engine/Dracovis%20Engine/CodeFile/ECS/System/PhysicsSystem.cpp#L313) 第 313 行附近的 ImGui 编辑器，在 Rigidbody 树节点中增加：

```cpp
ImGui::DragFloat("Linear Damping", &rb.LinearDamping, 0.01f, 0.0f, 1.0f);
ImGui::DragFloat("Angular Damping", &rb.AngularDamping, 0.01f, 0.0f, 1.0f);
// 应用到 PxActor
if (rb.Actor && rb.Mass > 0.0f)
{
    auto* dyn = static_cast<PxRigidDynamic*>(rb.Actor);
    dyn->setLinearDamping(rb.LinearDamping);
    dyn->setAngularDamping(rb.AngularDamping);
}
```

---

## 实施步骤

1. **DX11 相机修复**（问题 1）
   - 修改 `Application::InitializeScene`：在创建 Scene 前，将 `MainWindow.camera` 重置到 `(0, 5, -15)` 位置、`(0, 0, 0)` 旋转
   - 修改 `Graphics::UpdateSceneGraphicsResolution`：同时更新 `CameraObject.Aspect`（现有）并确保 Aspect 不被 `SetCamera` 覆盖（改为在 `SetCamera` 中保留 Aspect，或在同步 sceneCam 时更新 Aspect）

2. **ImGuiDX12 重构**（问题 2）
   - 修改 `ImGuiDX12.h`：替换单缓冲区成员为 `std::array<FrameResources, FRAME_COUNT>`，添加 `m_currentFrameIndex`
   - 修改 `ImGuiDX12.cpp` 的 `EndFrame`：
     - 使用当前帧索引的缓冲区
     - 按需重建（仅在大小不足时）
     - 绘制完成后递增帧索引
   - 修改 `ImGuiDX12.cpp` 的 `BeginFrame`：从 `DX12Core::GetCurrentBackBufferIndex()` 同步帧索引
   - 移除延迟释放队列（`pendingVertexBuffers`/`pendingIndexBuffers`）

3. **物理参数修复**（问题 3）
   - 修改 `Components.h`：降低 `Restitution` 默认值到 0.1；在 `RigidbodyComponent` 增加 `LinearDamping`/`AngularDamping`
   - 修改 `PhysicsSystem.cpp` 的 `CreateActor`：创建动态体后设置阻尼
   - 修改 `PhysicsSystem.cpp` 的 `RenderImGuiEditor`：增加阻尼编辑 UI

4. **编译验证**
   - 使用 `/FS` 单线程编译（项目约定）
   - 确认 0 error

5. **运行时验证**
   - 启动应用（默认 DX12 模式）
   - 验证 ImGui 窗口不再鬼闪
   - 验证物理 Box 落地后稳定（不乱飞）
   - 按 F5 切换到 DX11 模式
   - 验证 Nanosuit/Cerberus 模型可见
   - 验证相机可控

---

## 关键文件清单

| 文件 | 修改内容 |
|------|----------|
| `Application/Application.cpp` | InitializeScene 中重置相机 |
| `Graphics/Graphics.cpp` | UpdateSceneGraphicsResolution 修复 Aspect 同步 |
| `Graphics/DX12/ImGuiDX12.h` | 重构为每帧缓冲区 |
| `Graphics/DX12/ImGuiDX12.cpp` | EndFrame/BeginFrame 使用每帧缓冲区 |
| `ECS/Components/Components.h` | RigidbodyComponent 增加阻尼字段；Restitution 默认值降低 |
| `ECS/System/PhysicsSystem.cpp` | CreateActor 设置阻尼；ImGui 编辑器增加阻尼 UI |

---

## 验证方法

1. **编译**：`MSBuild /m:1 /p:Configuration=Release`（单线程，避免 PDB 死锁）
2. **运行**：启动应用，观察以下行为：
   - DX12 模式下 ImGui 窗口稳定无鬼闪
   - DX12 模式下物理 Box 落地后稳定（轻微弹跳后停止）
   - F5 切换到 DX11 后，Nanosuit/Cerberus 模型可见
   - DX11 相机可控（WASD+RMB 移动，WASD+LMB 旋转）
3. **日志检查**：确认 `debug_errors.txt` 为空，DX12Log 输出无错误

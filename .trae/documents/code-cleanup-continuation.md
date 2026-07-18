# Dracovis Engine 代码清理与重构（续）

## 摘要

延续上一会话已批准的 4 阶段重构计划。阶段 1（删除 BindableDX12/ 死代码）已完成。本计划覆盖阶段 2 剩余部分、阶段 3、阶段 4 的完整执行步骤。

## 当前状态分析

经过对实际代码的核查，确认以下事实：

### 阶段 2 状态
- 已完成：`DX12Log.h` 重写（合并 5 函数为 `DX12LogImpl` + `_DEBUG` gating + 文件日志保留 3 秒刷新）
- 已完成：`ImGuiDX12.cpp` 每帧日志移除
- **未完成**：9 个文件仍直接使用 `OutputDebugStringA`，绕过统一的 `DX12Log` 通道（无颜色、无文件落盘、Release 模式下无法消除）：
  - `CodeFile/Graphics/DX12/DX12Core.cpp`（约 60 处）
  - `CodeFile/Graphics/DX12/DX12Renderer.cpp`（约 50 处）
  - `CodeFile/Graphics/DX12/DX12Fence.cpp`（1 处）
  - `CodeFile/Graphics/DX12/DX12ShaderCompiler.h`（1 处）
  - `CodeFile/Graphics/DX12/DX12PipelineState.cpp`（1 处）
  - `CodeFile/Graphics/DX12/DX12RootSignature.cpp`（2 处）
  - `CodeFile/Application/Application.cpp`（约 30 处，集中在 EnableDX12Mode / InitializeDX12DemoScene）
  - `CodeFile/Application/Window/Window.cpp`
  - `CodeFile/Debug/DX12Log.h`（**保留**，这是 OutputDebugStringA 的统一出口）

### 阶段 3 状态
- `DX12Primitives.h`/`.cpp` 中 `DX12Triangle` 和 `DX12Box` 存在约 200 行重复代码：
  - 完全相同：`CreateTransformBuffer`、`Update`、`UpdateTransformBuffer`、`Draw`、`SetPosition`、`SetRotation`、`SetScale`、`SetViewMatrix`、`SetProjectionMatrix`
  - 几乎相同：`CreatePlaceholderCBVs`（仅 material Albedo 来源不同：固定灰色 vs `Color[]`）
  - 完全不同：几何创建（triangle vs box vertices/indices）

### 阶段 4 状态
- 关键发现：`Physics::InitializePhysics` 第 79-83 行创建 `Physics::PhysicsCpuDispatcher`，但 `PhysicsScene::InistializePhysicsScene` 第 13 行**自己创建了** `pCpuDispatcher` 并赋值给场景。`Physics::PhysicsCpuDispatcher` 从未被任何 Scene 使用 → 在 PhysX Debug 模式下触发 abort()。
- `Application::~Application()` 已显式调用 `capsule.Shutdown()`，但析构顺序本身已正确（声明顺序：`PhysicsContext` → `PhysicsSceneObject` → `capsule`，反向析构时 capsule 先于 PhysicsContext 销毁）。Stage 4 仅需移除死代码。

## 已确认的假设与决策

1. **OutputDebugStringA 替换策略**：仅替换用户模块中的调用，保留 `DX12Log.h::DX12LogImpl` 中的调用（它是统一出口）。`std::to_string` 拼接的日志改用 `std::string` 构造后单次调用 `DX12LogXxx`。
2. **DX12Primitive 基类设计**：使用 **非虚 + CRTP 风格 protected helper** 还是 **虚函数基类**？决策：**采用普通虚函数基类**，因为 Update/Draw 已在每帧调用，虚函数开销可忽略；几何创建用纯虚 `CreateGeometry()` 由派生类实现。这样能最大化代码消除（约 200 行 → 约 50 行）。
3. **PhysicsCpuDispatcher 处理**：直接删除创建代码 + 静态成员声明 + 析构中的释放块。**不**给 `PhysicsScene` 改成使用 `Physics::PhysicsCpuDispatcher`，因为现有 PhysicsScene 实现工作正常且更内聚。
4. **DX12Triangle 的 SetColor**：阶段 3 时给 `DX12Triangle` 也加 `SetColor`（与 DX12Box 接口对齐，便于基类暴露统一 API），让 Triangle 的 SetColor 更新 material Albedo。这是**新增的小功能**，但属于消除差异、简化基类接口的必要部分。
5. **不重命名 `InistializePhysicsScene`**（拼写错误 Inistialize→Initialize）：超出本次清理范围，避免破坏外部调用点。
6. **DX12Box::Color 数组**：基类 `DX12Primitive` 不持有 `Color`，由派生类自己持有；`CreatePlaceholderCBVs` 改为纯虚，由派生类实现各自的 material 初始化。这样保持基类简洁。

## 计划变更

### 阶段 2 — 统一日志 API（替换 OutputDebugStringA）

**目标**：所有 DX12 模块和 Application 通过 `DX12Log/DX12LogSuccess/DX12LogWarning/DX12LogError` 输出，确保 Release 模式零开销、统一颜色与文件落盘。

**文件与改动**：

#### `CodeFile/Graphics/DX12/DX12Core.cpp`
- 已 `#include "../../Debug/DX12Log.h"`（确认存在，第 3 行）
- 替换规则：
  - `"Step N: Creating X..."` → `DX12Log("[DX12Core] Step N: Creating X...\n")`
  - `"X created successfully"` → `DX12LogSuccess("[DX12Core] X created successfully\n")`
  - `"Warning: ..."` → `DX12LogWarning("[DX12Core] ...\n")`
  - `"FAILED Step N (...): "` + `e.what()` + `"\n"` → 用 `std::string` 拼接为单条消息后 `DX12LogError(msg.c_str())`
  - `"Failed to close command list"`、`"Present failed"` → `DX12LogError`
- **不删除**已有的 `DX12Log("[DX12Core] === Starting ...")` 等头部日志，仅替换 `OutputDebugStringA` 调用
- **保留** `#if defined(_DEBUG)` 包裹的 debug-layer 块结构

#### `CodeFile/Graphics/DX12/DX12Renderer.cpp`
- 已 `#include "../../Debug/DX12Log.h"`（第 2 行）
- 同 DX12Core.cpp 的替换规则
- `"Window dimensions: X x Y"` 多次 `OutputDebugStringA` 拼接 → 改为 `std::string` 一次拼接后单次 `DX12Log`

#### `CodeFile/Graphics/DX12/DX12Fence.cpp`
- 替换第 59 行 `OutputDebugStringA("[DX12Fence::Wait] Timeout waiting for GPU fence!\n")` → `DX12LogError("[DX12Fence::Wait] Timeout waiting for GPU fence!\n")`
- 添加 `#include "../../Debug/DX12Log.h"`

#### `CodeFile/Graphics/DX12/DX12ShaderCompiler.h`
- 替换第 59 行 `OutputDebugStringA(errorMsg.c_str())` → `DX12LogError(errorMsg.c_str())`
- 添加 `#include "../../Debug/DX12Log.h"`

#### `CodeFile/Graphics/DX12/DX12PipelineState.cpp`
- 替换第 204 行 `OutputDebugStringA("[DX12PipelineState] CreatePSO failed\n")` → `DX12LogError("[DX12PipelineState] CreatePSO failed\n")`
- 添加 `#include "../../Debug/DX12Log.h"`

#### `CodeFile/Graphics/DX12/DX12RootSignature.cpp`
- 替换第 127 行和第 365 行 → `DX12LogError`
- 添加 `#include "../../Debug/DX12Log.h"`

#### `CodeFile/Application/Application.cpp`
- 已 `#include "../Graphics/DX12/DX12.h"`（间接，但 DX12Log.h 不通过 DX12.h 引入）
- 添加 `#include "../Debug/DX12Log.h"`
- `EnableDX12Mode`：约 25 处 `OutputDebugStringA` → `DX12Log`/`DX12LogSuccess`/`DX12LogError`；用 `std::string` 拼接 `e.what()` 错误消息
- `InitializeDX12DemoScene`：约 15 处同样处理

#### `CodeFile/Application/Window/Window.cpp`
- 检查后替换（保留必要的 WM_ 调试输出，但统一到 DX12Log）

#### `CodeFile/Debug/DX12Log.h`
- **保持不变**（这是 OutputDebugStringA 的统一出口）

**验证**：编译通过；Debug 模式运行时控制台输出与之前等价（带颜色 + 落盘）；Release 模式下 `DX12Log*` 为空操作，无任何控制台 I/O。

---

### 阶段 3 — 提取 DX12Primitive 基类

**目标**：消除 `DX12Triangle` 与 `DX12Box` 之间约 200 行重复代码。

**设计**：在 `DX12Primitives.h` 中新增 `class DX12Primitive`，包含所有公共成员与方法；`DX12Triangle` 和 `DX12Box` 仅实现 `CreateGeometry()` 与 `InitMaterial(DX12MaterialCB&)` 两个虚方法。

#### `CodeFile/Graphics/DX12/DX12Primitives.h` 改动
1. 新增基类 `DX12Primitive`：
   ```cpp
   class DX12Primitive
   {
   public:
       DX12Primitive(DX12Core& core);
       virtual ~DX12Primitive() = default;
       void Update(float deltaTime);
       void Draw(ID3D12GraphicsCommandList* commandList);
       void SetPosition(float x, float y, float z);
       void SetRotation(float pitch, float yaw, float roll);
       void SetScale(float scale);
       void SetViewMatrix(const float* viewMatrix);
       void SetProjectionMatrix(const float* projMatrix);
       float* GetRotationSpeed() { return RotationSpeed; }
   protected:
       virtual void CreateGeometry() = 0;
       virtual void InitMaterial(DX12MaterialCB& materialData) = 0;
       void CreateTransformBuffer();
       void CreatePlaceholderCBVs();
       void UpdateTransformBuffer();
       DX12Core& Core;
       std::unique_ptr<VertexBufferDX12<DX12Vertex>> pVertexBuffer;
       std::unique_ptr<IndexBufferDX12<UINT>> pIndexBuffer;
       std::unique_ptr<ConstantBufferDX12<DX12Transform>> pTransformBuffer;
       std::unique_ptr<ConstantBufferDX12<DX12PointLightCB>> pPointLightBuffer;
       std::unique_ptr<ConstantBufferDX12<DX12SpotLightCB>> pSpotLightBuffer;
       std::unique_ptr<ConstantBufferDX12<DX12MaterialCB>> pMaterialBuffer;
       UINT IndexCount = 0;
       float Position[3] = {0,0,0};
       float Rotation[3] = {0,0,0};
       float RotationSpeed[3] = {0,0,0};
       float Scale = 1.0f;
       float ViewMatrix[16] = {};
       float ProjectionMatrix[16] = {};
   };
   ```
   基类构造函数初始化 identity matrices 并调用 `CreateGeometry()` + `CreateTransformBuffer()` + `CreatePlaceholderCBVs()`。

2. `DX12Triangle` 简化为：
   ```cpp
   class DX12Triangle : public DX12Primitive
   {
   public:
       using DX12Primitive::DX12Primitive;
   protected:
       void CreateGeometry() override;
       void InitMaterial(DX12MaterialCB& materialData) override;
   };
   ```
   - 新增 `SetColor(r,g,b,a)`：仅 Triangle 需要时再加。**决策**：不加，Triangle 不需要颜色（保持灰色默认）。

3. `DX12Box` 简化为：
   ```cpp
   class DX12Box : public DX12Primitive
   {
   public:
       using DX12Primitive::DX12Primitive;
       void SetColor(float r, float g, float b, float a);
   protected:
       void CreateGeometry() override;
       void InitMaterial(DX12MaterialCB& materialData) override;
   private:
       float Color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
   };
   ```
   `SetColor` 实现：更新 `Color[]`，重建 `DX12MaterialCB` 并 `pMaterialBuffer->Update(...)`。

#### `CodeFile/Graphics/DX12/DX12Primitives.cpp` 改动
- 实现 `DX12Primitive::Update`、`Draw`、`SetPosition` 等 9 个公共方法（从原 DX12Triangle/Box 抽取）
- 实现 `DX12Primitive::CreateTransformBuffer`、`CreatePlaceholderCBVs`、`UpdateTransformBuffer`（公共逻辑）
- `DX12Triangle::CreateGeometry`：原 `CreateTriangleGeometry` 内容
- `DX12Box::CreateGeometry`：原 `CreateBoxGeometry` 内容
- `DX12Triangle::InitMaterial`：写灰色的 Albedo
- `DX12Box::InitMaterial`：用 `Color[]` 写 Albedo
- 删除原 `DX12Triangle::~DX12Triangle()` 和 `DX12Box::~DX12Box()` 的 `= default` 显式声明（基类有虚析构即可）

**验证**：编译通过；启动 DX12 demo scene，3 个 box 仍正常旋转 + 颜色正确；不改变运行时行为。

---

### 阶段 4 — 移除 PhysicsCpuDispatcher 死代码

**目标**：消除 PhysX Debug 模式下因 `Physics::PhysicsCpuDispatcher` 创建但未使用导致的 abort()。

#### `CodeFile/Physics/Physics.h` 改动
- 删除第 33 行 `static PxDefaultCpuDispatcher* PhysicsCpuDispatcher;`

#### `CodeFile/Physics/Physics.cpp` 改动
- 删除第 9 行 `PxDefaultCpuDispatcher* Physics::PhysicsCpuDispatcher = nullptr;`
- 删除第 19-23 行析构中的释放块（if PhysicsCpuDispatcher → release）
- 删除第 79-83 行 `InitializePhysics` 中的创建代码：
  ```cpp
  // 删除：
  this->PhysicsCpuDispatcher = PxDefaultCpuDispatcherCreate(...);
  if (!this->PhysicsCpuDispatcher) { throw ...; }
  ```

**验证**：Debug 模式下关闭窗口时不再 abort()；PhysX 模拟仍正常（因为 PhysicsScene 自己创建 dispatcher）。

---

## 实施顺序

1. **阶段 2 先行**（日志统一）：可独立编译验证，不影响其他模块
2. **阶段 3**（DX12Primitive 基类）：在阶段 2 完成后进行
3. **阶段 4**（Physics 死代码移除）：可独立进行，与阶段 2/3 解耦

每个阶段独立验证：编译通过 + 运行行为不变。

## 验证清单

- [ ] 阶段 2：Debug 编译通过；运行 DX12 demo，控制台日志带颜色、格式与之前等价；`Data/debug_all.txt` 与 `Data/debug_errors.txt` 3 秒后刷新
- [ ] 阶段 2：Release 编译通过；运行时无控制台 I/O（DX12Log 为空操作）
- [ ] 阶段 3：Debug 编译通过；运行 DX12 demo，3 个 box 旋转 + 颜色正确；DX12Triangle 仍正常
- [ ] 阶段 3：行数减少（DX12Primitives.cpp 约 436 行 → 约 250 行）
- [ ] 阶段 4：Debug 编译通过；关闭窗口时无 abort()；PhysX 模拟（capsule 落地等）正常
- [ ] 全部完成后整体回归测试：启动应用 → F5 切换 DX12 → 关闭窗口，全流程无异常

## 不在本次范围内

- 不重构 ECS/Scene 模块（DX11 路径，工作正常）
- 不修改 Bindable/ 目录（DX11 Bindable 抽象）
- 不重命名 `InistializePhysicsScene`（避免外部调用点失效）
- 不删除 DX11 路径（保持双后端可切换）
- 不优化 ImGui 渲染路径（已在前一会话优化完毕）

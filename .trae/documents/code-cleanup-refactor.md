# Dracovis Engine DX12 代码清理与重构计划

## Context

经过多轮 DX12 迁移和 Bug 修复（ODR 违规、ImGui 集成、蓝色闪屏、描述符堆问题等），代码积累了不少屎山代码：
- **BindableDX12/ 目录是死代码**：6 个 .cpp + 6 个 .h 文件已被 `Graphics/DX12/` 目录取代，但仍被编译。其中 `ConstantBufferDX12.cpp` 还有显式模板实例化（XMMATRIX、XMFLOAT4X4 等），是潜在的 ODR 违规炸弹。
- **日志系统性能极差**：`DX12Log.h` 的 5 个函数每次调用都做 OutputDebugStringA + SetConsoleColor + cout + mutex + 字符串拼接 + 文件 I/O 检查，且没有 _DEBUG gating。`ImGuiDX12.cpp` 每帧输出 5-10 条日志（Frame 计数、DisplaySize、DrawData summary、draw call 详情、Frame done），是严重的性能杀手。
- **DX12Primitives.cpp 代码重复**：`DX12Triangle` 和 `DX12Box` 几乎完全相同，Draw/UpdateTransformBuffer/CreatePlaceholderCBVs/CreateTransformBuffer 都重复实现。
- **Application 析构顺序问题**：`~Application` 中 `capsule.Shutdown()` 在 `PhysicsContext` 释放之前调用，但 `PhysicsSceneObject` 和 `capsule` 作为成员变量的析构顺序与声明顺序不匹配，可能导致 abort() 崩溃。

目标：清理屎山代码、提高运行效率、消除潜在的 ODR 风险、修复析构顺序。

## 实施方案（分 4 阶段，每阶段编译验证）

### 阶段 1：删除 BindableDX12/ 死代码

**目标**：完全移除 `Graphics/BindableDX12/` 目录，消除 ODR 违规风险。

**操作步骤**：
1. 删除 `Dracovis Engine\CodeFile\Graphics\BindableDX12\` 目录下所有文件：
   - `BindableDX12.h`, `BindableDX12.cpp`
   - `IndexBufferDX12.h`, `IndexBufferDX12.cpp`
   - `VertexBufferDX12.h`, `VertexBufferDX12.cpp`
   - `ConstantBufferDX12.h`, `ConstantBufferDX12.cpp`
   - `TextureDX12.h`, `TextureDX12.cpp`
   - `SamplerDX12.h`, `SamplerDX12.cpp`

2. 编辑 `Dracovis Engine\Dracovis Engine.vcxproj`，移除所有引用 `BindableDX12\` 路径的 `<ClInclude>` 和 `<ClCompile>` 节点（约第 185-190 行和第 261-266 行）。

3. 检查 `DX12/BindableDX12.h` 是否需要将 `BindableDX12/BindableDX12.h` 中的 `BindableDX12Helper` 类合并进来。根据 grep 结果，`BindableDX12Helper` 只在 `BindableDX12/` 目录的 .cpp 中使用，删除目录后没有引用，所以不需要合并。

4. 编译验证：`MSBuild Dracovis Engine.sln /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal`

**风险**：低。`DX12/` 目录的同名文件已完全取代 `BindableDX12/` 目录的功能，grep 确认没有代码 include `BindableDX12/` 目录的文件（所有 `#include "BindableDX12.h"` 都解析到 `DX12/BindableDX12.h`）。

---

### 阶段 2：日志系统精简

**目标**：消除每帧日志开销，保留初始化/错误/警告日志。

#### 2.1 重构 `DX12Log.h`

**关键文件**：`Dracovis Engine\CodeFile\Debug\DX12Log.h`

**修改**：
1. **合并 5 个日志函数为 1 个核心函数**，对外保留 5 个 API 名字但内部委托：
   ```cpp
   enum class LogSeverity { Info, Success, Warning, Error, Header };
   inline void DX12LogImpl(LogSeverity sev, const char* msg) { ... }
   ```
   消除 5 个函数中重复的 OutputDebugStringA + SetConsoleColor + cout + AppendToLogFile 代码。

2. **添加 _DEBUG gating**：
   - 用 `#ifdef _DEBUG` 包裹所有日志函数体
   - Release 模式下日志函数变成空内联函数，零开销
   - 文件日志功能也只在 _DEBUG 下启用

3. **优化 AppendToLogFile**：
   - 移除每次调用都执行的 `FlushLogFilesIfNeeded`，改为外部定时调用（或在 AtExit 钩子中调用）
   - 或者保持现状但 `FlushLogFilesIfNeeded` 内部的时间检查保持轻量（已经是轻量的，只是 steady_clock 比较）

4. **消除冗余 SetConsoleColor 调用**：当前每个日志函数调用 2 次 SetConsoleColor（一次设置颜色、一次重置），合并为 1 次（在输出后立即重置，或在下次调用时设置）。

#### 2.2 精简 `ImGuiDX12.cpp` 每帧日志

**关键文件**：`Dracovis Engine\CodeFile\Graphics\DX12\ImGuiDX12.cpp`

**修改**：
1. **BeginFrame**（第 138-141 行）：移除 `s_frameCounter` 和 `DX12LogHeader("[ImGuiDX12] === Frame X ===")` 和 `DX12Log("DisplaySize = ...")`。这些每帧日志是无意义的开销。

2. **EndFrame**（第 192-197 行）：移除 `DX12Log("[ImGuiDX12] DrawData: CmdLists=... Vtx=... Idx=...")` 每帧日志。

3. **EndFrame**（第 412-424 行）：移除每个 draw call 的详细日志（`loggedDrawCalls < 3` 块）。这是为调试蓝色闪屏添加的临时代码，问题已解决。

4. **EndFrame**（第 435 行）：移除 `DX12LogSuccess("[ImGuiDX12] Frame done: X draw calls")` 每帧日志。

5. **保留**：
   - VB/IB 重建警告（`DX12LogWarning`，只在 buffer 不足时触发，不是每帧）
   - Missing resources 错误（`DX12LogError`，只在初始化失败时触发）
   - CreateResources 中的初始化日志（一次性，不影响运行时性能）

#### 2.3 统一 `DX12Core.cpp` 和 `DX12Renderer.cpp` 的日志

**修改**：
1. `DX12Core.cpp`：将 `OutputDebugStringA("[DX12Core] Step X: ...")` 替换为 `DX12Log`/`DX12LogSuccess`/`DX12LogError`，统一日志 API。
2. `DX12Renderer.cpp`：同上，将 `OutputDebugStringA` 替换为 `DX12Log` 系列。

**验证**：编译通过，运行时控制台输出干净，无每帧日志刷屏。

---

### 阶段 3：DX12Primitives 代码重复消除

**目标**：提取基类 `DX12Primitive`，消除 `DX12Triangle` 和 `DX12Box` 之间的代码重复。

**关键文件**：
- `Dracovis Engine\CodeFile\Graphics\DX12\DX12Primitives.h`
- `Dracovis Engine\CodeFile\Graphics\DX12\DX12Primitives.cpp`

**修改**：

1. **新增基类 `DX12Primitive`**（在 DX12Primitives.h 中）：
   ```cpp
   class DX12Primitive
   {
   public:
       DX12Primitive(DX12Core& core);
       virtual ~DX12Primitive() = default;
       
       void Update(float deltaTime);           // 公共实现
       void Draw(ID3D12GraphicsCommandList*);  // 公共实现
       
       // Setters (公共实现)
       void SetPosition(float x, float y, float z);
       void SetRotation(float pitch, float yaw, float roll);
       void SetScale(float scale);
       void SetViewMatrix(const float* viewMatrix);
       void SetProjectionMatrix(const float* projMatrix);
       float* GetRotationSpeed() { return RotationSpeed; }
       
   protected:
       // 子类只需实现此方法提供几何数据
       virtual void CreateGeometry(std::vector<DX12Vertex>& vertices, 
                                    std::vector<UINT>& indices) = 0;
       // 子类可覆盖以自定义 material（如 Box 的 Color 作为 Albedo）
       virtual DX12MaterialCB CreateMaterialData() const;
       
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
       float ViewMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
       float ProjectionMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
       
   private:
       void UpdateTransformBuffer();        // 公共实现
       void CreateTransformBuffer();        // 公共实现
       void CreatePlaceholderCBVs();        // 公共实现
   };
   ```

2. **简化 `DX12Triangle` 和 `DX12Box`**：
   ```cpp
   class DX12Triangle : public DX12Primitive {
   public:
       DX12Triangle(DX12Core& core) : DX12Primitive(core) { Init(); }
   protected:
       void CreateGeometry(std::vector<DX12Vertex>& v, std::vector<UINT>& i) override;
   };
   
   class DX12Box : public DX12Primitive {
   public:
       DX12Box(DX12Core& core) : DX12Primitive(core) { Init(); }
       void SetColor(float r, float g, float b, float a);
   protected:
       void CreateGeometry(std::vector<DX12Vertex>& v, std::vector<UINT>& i) override;
       DX12MaterialCB CreateMaterialData() const override;
   private:
       float Color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
   };
   ```

3. **DX12Primitives.cpp**：删除 `DX12Triangle::Draw`、`DX12Box::Draw`、`UpdateTransformBuffer`、`CreateTransformBuffer`、`CreatePlaceholderCBVs` 的重复实现，只保留 `CreateGeometry` 的 override。

**减少代码量**：约 200 行重复代码 → 约 80 行基类实现 + 各子类只保留 geometry 创建（~20 行/类）。

---

### 阶段 4：Application 析构顺序修复

**目标**：确保 PhysX 资源在 PhysicsContext 释放前正确清理。

**关键文件**：
- `Dracovis Engine\CodeFile\Application\Application.h`
- `Dracovis Engine\CodeFile\Application\Application.cpp`

**问题分析**：
- `Application.h` 成员声明顺序：`PhysicsContext`（第 48 行）→ `PhysicsSceneObject`（第 52 行）→ `capsule`（第 53 行）
- C++ 析构顺序与声明顺序**相反**：`capsule` 先析构 → `PhysicsSceneObject` 析构 → `PhysicsContext` 析构
- `capsule` 析构调用 `Shutdown()` 释放 PxShape/PxMaterial，此时 `PhysicsSceneObject`（持有 PxScene）还未析构，`PhysicsContext`（持有 PxPhysics）也还活着 → 顺序正确
- 但 `PhysicsSceneObject` 析构时释放 PxScene，此时 `PhysicsContext` 还活着 → 正确
- `PhysicsContext` 析构时释放 PxPhysics/PxFoundation → 正确

**实际检查**：声明顺序看起来是正确的。但 `~Application` 中显式调用 `capsule.Shutdown()` 在 `MainWindow.ShutdownDX12()` 之后，这是不必要的（DX12 资源与 PhysX 无关）。

**修改**：
1. **`Application.h`**：重新排列成员声明顺序，确保析构顺序正确：
   ```cpp
   // 析构顺序（从后往前）：capsule → PhysicsSceneObject → PhysicsContext
   // 即 PhysicsContext 必须最后析构（先声明）
   std::unique_ptr<Physics> PhysicsContext;  // 先声明，最后析构
   PhysicsScene PhysicsSceneObject;           // 中间析构
   PhysicsCapsule capsule;                    // 后声明，先析构
   ```

2. **`~Application`**（第 138-154 行）：
   - 移除显式 `capsule.Shutdown()` 调用（析构会自动调用）
   - 或者保留但确保在 `PhysicsContext` 释放之前
   - 添加 `PhysicsSceneObject` 的显式清理（如果有 Shutdown 方法）

3. **检查线程 join**：
   - `Application` 构造函数中 `InitializationThread.detach()`（第 87 行）→ 改为在构造函数末尾 join
   - 但当前代码已经在 `while (!IsInitializationFinished)` 循环中等待 → 可以保留 detach，但需要确保 `IsInitializationFinished` 是 `std::atomic<bool>`

4. **abort() 问题分析**：
   - 用户反馈点击"跳过"后物理引擎模块出现内存泄漏 → 这是 CRT Debug Heap 检测到的泄漏
   - 点击"终止"则正常结束 → 说明是 Debug 模式的 _CrtCheckMemory 触发
   - 根因：`Physics::PhysicsCpuDispatcher` 在 `Physics::InitializePhysics` 中创建（第 79 行），但 `PhysicsScene` 也创建了自己的 `pCpuDispatcher`（第 13 行）→ `Physics::PhysicsCpuDispatcher` 从未被使用且未被释放
   - **修复**：移除 `Physics::InitializePhysics` 中第 79-83 行的 `PhysicsCpuDispatcher` 创建（它未被任何场景使用）

**验证**：编译通过，关闭窗口时不再弹出 abort() 对话框。

---

## 验证方法

每阶段完成后：
1. `MSBuild "Dracovis Engine.sln" /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal`
2. 运行 `Build\Debug - windows\x86_64\Dracovis Engine.exe`（沙箱外）
3. 检查：
   - 控制台输出干净（无每帧日志刷屏）
   - 3D 场景正常渲染
   - ImGui 窗口正常显示
   - 切换 ImGui 活动窗口无蓝色闪屏
   - 关闭窗口无 abort() 崩溃
   - `Data/debug_all.txt` 和 `Data/debug_errors.txt` 正确保存

全部完成后：
4. 切换到 Release 模式编译验证（日志应为空操作）
5. 确认运行时性能提升（FPS 应明显提高，特别是 ImGui 窗口切换时）

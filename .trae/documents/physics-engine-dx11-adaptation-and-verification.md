# 物理系统 DX11 适配与运行时验证计划

## Summary

上一会话完成了物理系统对引擎集成的核心代码（Phase A–E：PhysicsSystem 修复、MeshComponent + MeshRendererSystem、ImGui 编辑器、Raycast API、Application 接入），并在 Phase F 阶段修复了 `Visible`→`IsVisible` 命名冲突和 Scene.cpp 缩进问题，但**未重新编译验证**。

本计划完成剩余工作：
1. **编译验证**：确认上次修复解决了所有编译错误
2. **DX11 占位渲染补全**：发现的真实缺口——当前 `MeshRendererSystem::RenderDX11` 在 `MeshComponent::ModelPath` 为空时不渲染任何东西，所有测试实体（PhysicsTestBox / PhysicsTestCapsule / Ground）均未设 `ModelPath`，导致 **DX11 模式下物理实体完全不可见**。这是用户明确要求的"DX11 模式适配"未完成的部分。
3. **运行时验证**：在 DX11/DX12 双模式下确认物理实体可见、可交互、可序列化。

PX 崩溃问题不在本次范围（用户确认不影响正常使用）。

## Current State Analysis

### 已完成（Phase A–E，代码已写入但未重新编译）

| 模块 | 文件 | 状态 |
|---|---|---|
| 静态体支持（Mass=0 → PxRigidStatic） | `PhysicsSystem.cpp` | ✅ |
| PhysicsSystem::ShutDown 显式释放所有 actor | `PhysicsSystem.cpp` | ✅ |
| MeshComponent + MeshRendererSystem | `Components.h`、`MeshRendererSystem.h/.cpp` | ✅ |
| DX11 渲染路径（仅 Model::Draw，无占位） | `MeshRendererSystem.cpp::RenderDX11` | ⚠️ 缺口 |
| DX12 渲染路径（DX12Box 占位） | `MeshRendererSystem.cpp::RenderDX12` | ✅ |
| ImGui 物理编辑器 | `PhysicsSystem.cpp::RenderImGuiEditor` | ✅ |
| Raycast API | `PhysicsScene.h/.cpp` | ✅ |
| PhysicsCapsule 废弃清理 | `Application.h/.cpp` | ✅ |
| 测试实体（Box/Capsule/Ground） | `Application.cpp::InitializeScene` | ✅ |
| 序列化 MeshComponent | `Scene.cpp` | ✅ |
| yaml-cpp XMFLOAT4 convert | `yaml-cpp/.../yaml.h` | ✅ |
| 项目文件注册 | `Dracovis Engine.vcxproj/.filters` | ✅ |

### 待办

1. **编译验证（Debug x64）** —— 上次会话末修复了 `Visible`→`IsVisible`，需重新编译确认所有错误已消除。
2. **DX11 占位渲染补全** —— 见下方 Proposed Changes。
3. **运行时验证** —— 见下方 Verification。

### 关键发现：DX11 模式不可见问题

`MeshRendererSystem::RenderDX11` 当前逻辑（`MeshRendererSystem.cpp` L45–L80）：

```cpp
if (!mesh.ModelPtr && !mesh.ModelPath.empty()) {
    mesh.ModelPtr = std::make_shared<Model>(gfx, mesh.ModelPath);  // 懒加载
}
if (mesh.ModelPtr) {
    // 同步 Transform → Model 并 Draw
}
```

但 `Application::InitializeScene` 创建的所有测试实体**均未设置 `ModelPath`**：

```cpp
auto physicsBox = CurrentScene->CreateEntity("PhysicsTestBox");
// ... Transform/Rigidbody/Collider
auto& boxMesh = CurrentScene->AddComponent<MeshComponent>(physicsBox);
boxMesh.TintColor = XMFLOAT4{ 0.8f, 0.3f, 0.2f, 1.0f };  // 只设了颜色，没有 ModelPath
```

因此 `ModelPath.empty()` 为 true，懒加载被跳过，`ModelPtr` 为 nullptr，`if (mesh.ModelPtr)` 不成立 → **不渲染任何东西**。

DX12 路径有 `DX12Box` 占位，所以 DX12 模式下实体可见；DX11 路径缺少对称的占位实现，这是用户要求的"DX11 模式适配"未完成的部分。

## Proposed Changes

### 阶段 1：编译验证（无代码改动，仅验证）

执行 Debug x64 编译，确认 `Visible`→`IsVisible` 重命名和 Scene.cpp 缩进修复解决了所有编译错误。如有新错误，逐个修复。

```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ^
  "Dracovis Engine.vcxproj" /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

### 阶段 2：新增 SolidBoxDrawable 类（DX11 占位渲染）

参考现有 `SolidSphereDrawable`（`CodeFile/Graphics/Drawable/SolidSphere.h/.cpp`）的实现模式，新增 `SolidBoxDrawable`，用于 DX11 模式下物理实体的占位渲染。

**为什么选 SolidSphereDrawable 作为模板**：
- 已使用 `SolidColorConstantBuffer` 绑定 TintColor，与 `MeshComponent::TintColor` 语义一致
- 已使用 `SolidVertexShader.hlsl` / `SolidPixelShader.hlsl`，无需新建 shader
- 已通过 `DrawableBase<>` 提供 `Draw(Graphics&)` 入口，复用 Bindable 体系
- 构造函数 `SolidSphereDrawable(Graphics&, float radius, XMFLOAT3 color)` 简洁，Box 可对应 `(Graphics&, XMFLOAT3 halfExtents, XMFLOAT3 color)`

**新增文件**：

#### `CodeFile/Graphics/Drawable/SolidBoxDrawable.h`（新建）

```cpp
#pragma once
#include "DrawableBase.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/SolidColorConstantBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/IndexBuffer.h"
#include "../Geometry/Cube.h"
#include <DirectXMath.h>

namespace DE
{
    // DX11 solid-color box placeholder for physics-driven entities that have no
    // MeshComponent::ModelPath. Mirrors DX12Box's role in the DX12 path. Color
    // comes from MeshComponent::TintColor; half-extents come from ColliderComponent
    // (or TransformComponent.Scale as fallback).
    class SolidBoxDrawable : public DrawableBase<SolidBoxDrawable>
    {
    public:
        SolidBoxDrawable(Graphics& gfx, DirectX::XMFLOAT3 halfExtents, DirectX::XMFLOAT3 color);

        void SetPosition(DirectX::XMFLOAT3 Position) noexcept;
        void SetColor(DirectX::XMFLOAT3 Color) noexcept;
        void SetRotation(DirectX::XMFLOAT3 Rotation) noexcept;  // Euler radians

        void Update(float dt, float aspect) noexcept override;
        DirectX::XMMATRIX GetTransformXM() const noexcept override;
        const DirectX::XMFLOAT3& GetColor() const noexcept;

    private:
        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };  // Euler radians
        DirectX::XMFLOAT3 HalfExtents = { 0.5f, 0.5f, 0.5f };
        DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };

        friend class SolidColorConstantBuffer;
    };
}
```

#### `CodeFile/Graphics/Drawable/SolidBoxDrawable.cpp`（新建）

参考 `SolidSphere.cpp` 实现：
- 使用 `Cube::Make<Vertex>()` 生成几何（顶点含 Position + Color）
- 用 `XMMatrixScaling(halfExtents.x, halfExtents.y, halfExtents.z)` 缩放单位立方体到目标尺寸
- 顶点的 `Color` 字段填 `color`
- `GetTransformXM` 组合 `Rotation → Scaling → Translation`：

```cpp
DirectX::XMMATRIX SolidBoxDrawable::GetTransformXM() const noexcept
{
    namespace dx = DirectX;
    return dx::XMMatrixScaling(HalfExtents.x, HalfExtents.y, HalfExtents.z)
         * dx::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z)
         * dx::XMMatrixTranslation(Position.x, Position.y, Position.z);
}
```

注意：`HalfExtents` 在构造时确定（基于 ColliderComponent）。若 ImGui 编辑器允许实时改 HalfExtents，需重建 drawable（参考现有 DX12 路径：每次实体首次出现才懒创建，destroyed 后从缓存移除；HalfExtents 变更不会触发重建，仅显示旧尺寸——可接受，作为占位）。

#### 项目文件注册

在 `Dracovis Engine.vcxproj` 的 `<ItemGroup>` 中添加：
```xml
<ClInclude Include="CodeFile\Graphics\Drawable\SolidBoxDrawable.h" />
<ClCompile Include="CodeFile\Graphics\Drawable\SolidBoxDrawable.cpp" />
```

在 `Dracovis Engine.vcxproj.filters` 中添加到 `CodeFile\Graphics\Drawable` 过滤器。

### 阶段 3：重构 MeshRendererSystem::RenderDX11 使用占位渲染

修改 `CodeFile/ECS/System/MeshRendererSystem.h`：
- 新增 `#include "../../Graphics/Drawable/SolidSphere.h"` 和 `"../../Graphics/Drawable/SolidBoxDrawable.h"`
- 新增成员：`std::unordered_map<entt::entity, std::unique_ptr<SolidBoxDrawable>> m_dx11Boxes;`
- 新增成员：`std::unordered_map<entt::entity, std::unique_ptr<SolidSphereDrawable>> m_dx11Spheres;`
- 新增 private 方法：`void PruneInvalidDX11(Scene& scene);`（与现有 `PruneInvalid` 类似但作用于 DX11 缓存）

修改 `CodeFile/ECS/System/MeshRendererSystem.cpp::RenderDX11`：

```cpp
void MeshRendererSystem::RenderDX11(Scene& scene, Graphics& gfx)
{
    auto& reg = scene.GetRegistry();
    auto view = reg.view<TransformComponent, MeshComponent>();
    for (auto e : view)
    {
        auto& mesh = reg.get<MeshComponent>(e);
        auto& tr = reg.get<TransformComponent>(e);
        if (!mesh.IsVisible) continue;

        // 路径 A: 有 ModelPath → 用 Model::Draw（原逻辑）
        if (!mesh.ModelPath.empty())
        {
            if (!mesh.ModelPtr) {
                try { mesh.ModelPtr = std::make_shared<Model>(gfx, mesh.ModelPath); }
                catch (...) { mesh.ModelPath.clear(); }
            }
            if (mesh.ModelPtr) {
                mesh.ModelPtr->Position = tr.Position;
                mesh.ModelPtr->Rotation = tr.Rotation;
                mesh.ModelPtr->Scale    = tr.Scale;
                mesh.ModelPtr->Draw(gfx);
            }
            continue;  // 用了 Model 就不走占位路径
        }

        // 路径 B: 无 ModelPath → 用占位 drawable（按 Collider 形状）
        auto* col = reg.try_get<ColliderComponent>(e);
        XMFLOAT3 color = { mesh.TintColor.x, mesh.TintColor.y, mesh.TintColor.z };

        if (col && col->Shape == ColliderShape::Sphere)
        {
            // 用 SolidSphereDrawable
            auto it = m_dx11Spheres.find(e);
            if (it == m_dx11Spheres.end())
            {
                auto s = std::make_unique<SolidSphereDrawable>(gfx, std::fabs(col->Radius), color);
                it = m_dx11Spheres.emplace(e, std::move(s)).first;
            }
            it->second->SetPosition(tr.Position);
            it->second->SetColor(color);
            // 球无需旋转
            it->second->Update(0.0f, 1.0f);
            it->second->Draw(gfx);
        }
        else
        {
            // Box / Capsule（Capsule 用 Box 近似，仅占位）→ SolidBoxDrawable
            XMFLOAT3 halfExtents = { 0.5f, 0.5f, 0.5f };
            if (col)
            {
                if (col->Shape == ColliderShape::Box)
                    halfExtents = { std::fabs(col->HalfExtents.x),
                                    std::fabs(col->HalfExtents.y),
                                    std::fabs(col->HalfExtents.z) };
                else if (col->Shape == ColliderShape::Capsule)
                    halfExtents = { std::fabs(col->Radius),
                                    std::fabs(col->HalfHeight) + std::fabs(col->Radius),
                                    std::fabs(col->Radius) };
            }
            else
            {
                // 无 Collider，用 Transform.Scale 作为半尺寸
                halfExtents = { std::fabs(tr.Scale.x) * 0.5f,
                                std::fabs(tr.Scale.y) * 0.5f,
                                std::fabs(tr.Scale.z) * 0.5f };
            }

            auto it = m_dx11Boxes.find(e);
            if (it == m_dx11Boxes.end())
            {
                auto b = std::make_unique<SolidBoxDrawable>(gfx, halfExtents, color);
                it = m_dx11Boxes.emplace(e, std::move(b)).first;
            }
            it->second->SetPosition(tr.Position);
            it->second->SetRotation(tr.Rotation);
            it->second->SetColor(color);
            it->second->Update(0.0f, 1.0f);
            it->second->Draw(gfx);
        }
    }
}
```

`ShutDown` 中追加清理：
```cpp
void MeshRendererSystem::ShutDown()
{
    m_dx12Boxes.clear();
    m_dx11Boxes.clear();
    m_dx11Spheres.clear();
}
```

`PruneInvalid` 中追加 DX11 缓存清理（或在原方法中扩展）。

### 阶段 4：运行时验证

执行 `..\Build\Debug - windows\x86_64\Dracovis Engine.exe`，按以下清单验证：

| # | 验证项 | 模式 | 预期 |
|---|---|---|---|
| 1 | PhysicsTestBox 下落 | DX11 | 橙色方块从 y=5 自由落体到 Ground 上停止 |
| 2 | PhysicsTestCapsule 下落 | DX11 | 绿色占位盒（Capsule 用 Box 近似）从 y=5 落到地面 |
| 3 | Ground 可见 | DX11 | y=-0.5 处有灰色大平板 |
| 4 | PhysicsTestBox 下落 | DX12 | 橙色 DX12Box 从 y=5 自由落体 |
| 5 | F5 切换 DX11→DX12→DX11 | 切换 | 物理状态连续，无崩溃 |
| 6 | ImGui "Physics Editor" 面板 | 两者 | 列出 PhysicsTestBox/Capsule/Ground，可实时改 Mass/Gravity/Kinematic 并生效 |
| 7 | ImGui "Create Entity" → "Add Physics Box" | 两者 | 新建实体从指定位置下落，DX11/DX12 均可见 |
| 8 | ImGui "Raycast" → "Use Camera Position"+"Use Camera Forward"+"Cast Ray" | 两者 | 命中后显示坐标与实体名 |
| 9 | 关闭窗口 | 两者 | 无 PhysX 泄漏 abort（已知 PX Debug 崩溃不在范围内，但应能正常退出） |
| 10 | DX12Log 日志 | 两者 | `Data/debug_all.txt` 和 `Data/debug_errors.txt` 刷新且无新增 error |

### 阶段 5（可选）：Capsule 占位改进

如果用户希望 DX11 下 Capsule 形状更准确（而非 Box 近似），可新增 `SolidCapsuleDrawable` 使用 `Geometry/Capsule.h` 的几何。本计划暂不实现，作为后续增强项。

## Assumptions & Decisions

### 决策

1. **PX 崩溃不在范围**：用户明确说"不用纠结了，不影响正常使用"。本计划不试图修复 PhysX Debug 模式 abort。

2. **DX11 占位用 SolidBoxDrawable 而非复用 BoxDrawable**：现有 `BoxDrawable` 构造函数需要 RNG 和分布参数，用于自动随机化运动，不适合作为静态占位。新建 `SolidBoxDrawable` 与 `SolidSphereDrawable` 模式一致，可复用 `SolidColorConstantBuffer` 和 `Solid*Shader.hlsl`。

3. **Capsule 用 Box 近似**：Capsule 几何渲染复杂度较高，本阶段先用 Box 占位（半尺寸按 `Radius`/`HalfHeight` 推导）。用户若需精确可视化可后续扩展。

4. **SolidBoxDrawable 的 HalfExtents 在构造时固定**：与 DX12 路径的 `DX12Box` 缓存策略一致——实体首次出现时按 Collider 推导尺寸创建，之后只更新 Position/Rotation/Color。若用户通过 ImGui 改 HalfExtents，占位尺寸不实时同步（仅物理碰撞同步），可接受作为占位行为。

5. **SolidBoxDrawable 不参与光照**：使用 `SolidPixelShader.hlsl`（纯色），不受 PointLight/SpotLight 影响。这与 SolidSphereDrawable 一致，作为调试占位足够。

### 假设

- 现有 `SolidVertexShader.hlsl` / `SolidPixelShader.hlsl` 已存在且可用（SolidSphereDrawable 已使用）
- `Cube::Make<Vertex>()`（`Geometry/Cube.h`）的接口与 `Sphere::Make<Vertex>()` 一致
- `DrawableBase<T>::Draw(Graphics&)` 通过 AddStaticBind/AddBind 累积的 bindables 自动绑定并绘制

## Verification Steps

1. **编译验证**：执行 MSBuild 命令，确认 0 error。如有错误，逐个修复。
2. **新增 SolidBoxDrawable**：实现 .h/.cpp，注册到 .vcxproj/.filters，再次编译。
3. **重构 RenderDX11**：实现占位渲染路径，再次编译。
4. **运行时验证**：按上表 10 项逐一确认。
5. **日志检查**：确认 `Data/debug_all.txt` 和 `Data/debug_errors.txt` 中无新增 error。

## Files Touched

**新建**：
- `Dracovis Engine/CodeFile/Graphics/Drawable/SolidBoxDrawable.h`
- `Dracovis Engine/CodeFile/Graphics/Drawable/SolidBoxDrawable.cpp`

**修改**：
- `Dracovis Engine/CodeFile/ECS/System/MeshRendererSystem.h`（新增 DX11 缓存成员、include）
- `Dracovis Engine/CodeFile/ECS/System/MeshRendererSystem.cpp`（重写 RenderDX11、扩展 ShutDown/PruneInvalid）
- `Dracovis Engine/Dracovis Engine.vcxproj`（注册 SolidBoxDrawable.h/.cpp）
- `Dracovis Engine/Dracovis Engine.vcxproj.filters`（同上）

**只读验证**（不改）：
- 所有其他 PhysicsSystem / Application / Scene / Components / PhysicsScene 文件已就绪

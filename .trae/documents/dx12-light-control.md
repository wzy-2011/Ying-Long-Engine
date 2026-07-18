# DX12 灯光控制功能实现计划

## Context

当前 DX12 模式下只有 PointLight 数据传播（`DX12PointLightCB`），且：
- `DX12SpotLightCB` 结构已定义但从未更新，`pSpotLightBuffer` 始终保持 0 灯
- `SpotLightTwo` 在 DX12 模式下完全无效
- DX12 没有任何灯光控制 UI
- 用户需要：DX12 下灯光的**位置移动**、**动态创建**，同时支持 **PointLight** 和 **SpotLight**

## 关键技术点

### SpotLight 数据转换（基于 Lighting.hlsli 分析）
PBR shader 的 `CalculateSpotLightAttenuation` 用 `dot(-L, Direction)` 结果（余弦值）与 `OuterConeAngle`/`InnerConeAngle` 比较。因此传给 shader 的角度必须是**余弦值**，而非弧度。

- `SpotLight::Data.Rotation`（角度，0-360）→ `Direction`（单位向量）：复用 DX11 LightManager 的逻辑，默认方向 (1,0,0)，经 `XMMatrixRotationRollPitchYaw` 旋转
- `InnerConeAngle`/`OuterConeAngle`（弧度）→ `cos(弧度)`

### 灯光数据结构
在 `Application.h` 中定义独立的纯数据 struct，不依赖 DX11 的 PointLight/SpotLight 类（避免 DX11 mesh 构造开销）：

```cpp
struct DX12PointLightState {
    DirectX::XMFLOAT3 Position = {0,5,0};
    DirectX::XMFLOAT3 Color = {1,1,1};
    float Intensity = 1000.0f;
    bool Enabled = true;
};
struct DX12SpotLightState {
    DirectX::XMFLOAT3 Position = {0,5,0};
    DirectX::XMFLOAT3 Color = {1,1,1};
    float Intensity = 10000.0f;
    DirectX::XMFLOAT3 Rotation = {0,0,0};       // 角度 0-360
    float OuterConeAngle = XM_PI / 4;           // 弧度 (45°)
    float InnerConeAngle = XM_PI / 8;           // 弧度 (22.5°)
    bool Enabled = true;
};
```

## 实施步骤

### 1. Application.h — 添加灯光容器
- 添加 `DX12PointLightState`/`DX12SpotLightState` struct 定义
- 添加成员：`std::vector<DX12PointLightState> DX12PointLights;` 和 `std::vector<DX12SpotLightState> DX12SpotLights;`
- 添加 `#include <DirectXMath.h>` 和 `#include <vector>`（如未包含）

### 2. Application.cpp — InitializeDX12DemoScene 初始化默认灯光
在 `InitializeDX12DemoScene()` 末尾添加：
- 清空 `DX12PointLights`/`DX12SpotLights`
- 从 `PointLightOne`/`PointLightTwo` 拷贝数据到 `DX12PointLights`（2 个默认点光）
- 从 `SpotLightTwo` 拷贝数据到 `DX12SpotLights`（1 个默认聚光）
- 添加 1 个额外默认 SpotLight（角度向下）方便观察

### 3. DX12Primitives.h/cpp — 添加 SetSpotLightData
- `DX12Primitive` 添加 `void SetSpotLightData(const DX12SpotLightCB& data);` 方法声明
- 实现：`if (pSpotLightBuffer) pSpotLightBuffer->Update(data);`（模式同 `SetPointLightData`）

### 4. DX12DemoScene.h/cpp — 添加 SetSpotLightData
- 声明 `void SetSpotLightData(const DX12SpotLightCB& data);`
- 实现：遍历 Triangles/Boxes 调用 `SetSpotLightData`（模式同 `SetPointLightData`）

### 5. MeshRendererSystem.h/cpp — 添加 SetDX12SpotLightData
- 声明 `void SetDX12SpotLightData(const DX12SpotLightCB& data);`
- 前向声明 `struct DX12SpotLightCB;`
- 实现：遍历 `m_dx12Boxes` 调用 `SetSpotLightData`（模式同 `SetDX12PointLightData`）

### 6. Application.cpp — DoFrameDX12 构建 SpotLight CB 并传播
在现有 `pointLightData` 构建逻辑之后，添加 `spotLightData` 构建：
```cpp
DX12SpotLightCB spotLightData = {};
spotLightData.SpotLightCount = 0;
for (auto& sl : DX12SpotLights) {
    if (!sl.Enabled || spotLightData.SpotLightCount >= 50) break;
    auto& slot = spotLightData.Lights[spotLightData.SpotLightCount];
    slot.Position[0..2] = sl.Position;
    slot.Intensity = sl.Intensity;
    slot.Color[0..2] = sl.Color;
    slot.InnerConeAngle = cos(sl.InnerConeAngle);  // 弧度→余弦值
    // Direction: Rotation(角度) → 单位向量
    float pitchRad = sl.Rotation.x / 360.0f * XM_2PI;
    float yawRad   = sl.Rotation.y / 360.0f * XM_2PI;
    float rollRad  = sl.Rotation.z / 360.0f * XM_2PI;
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(pitchRad, yawRad, rollRad);
    XMVECTOR dir = XMVector3Normalize(XMVector3Transform(XMVectorSet(1,0,0,0), rotMat));
    slot.Direction[0..2] = <从 dir 提取>;
    slot.OuterConeAngle = cos(sl.OuterConeAngle);
    spotLightData.SpotLightCount++;
}
pDX12DemoScene->SetSpotLightData(spotLightData);
meshRenderer->SetDX12SpotLightData(spotLightData);
```

### 7. Application.cpp — DoFrameDX12 添加 ImGui 灯光控制 UI
在 "DX12 Mode" 窗口之后添加独立的 "DX12 Lights" 窗口：
```
[DX12 Lights]
  Point Lights: 2
  [Add Point Light] [Remove Last]
  > PointLight 0  [x] Enabled
      Position: [drag float3]
      Color:    [drag float3]
      Intensity:[drag float]
  > PointLight 1  ...
  Spot Lights: 1
  [Add Spot Light] [Remove Last]
  > SpotLight 0   [x] Enabled
      Position: [drag float3]
      Color:    [drag float3]
      Rotation: [drag float3] (角度)
      Intensity:[drag float]
      Outer Angle: [slider 1-179°]
      Inner Angle: [slider 0-179°]
```
- "Add" 按钮 push_back 默认状态
- "Remove Last" 弹出最后一个（保留至少 0 个）
- 每个 TreeNode 显示 Enabled 复选框 + 参数编辑
- Outer Angle slider 用角度显示/编辑，内部转弧度存储

## 涉及文件
1. `Dracovis Engine\CodeFile\Application\Application.h` — 添加 struct + 成员
2. `Dracovis Engine\CodeFile\Application\Application.cpp` — 初始化 + DoFrameDX12 + UI
3. `Dracovis Engine\CodeFile\Graphics\DX12\DX12Primitives.h` — SetSpotLightData 声明
4. `Dracovis Engine\CodeFile\Graphics\DX12\DX12Primitives.cpp` — SetSpotLightData 实现
5. `Dracovis Engine\CodeFile\Graphics\DX12\DX12DemoScene.h` — SetSpotLightData 声明
6. `Dracovis Engine\CodeFile\Graphics\DX12\DX12DemoScene.cpp` — SetSpotLightData 实现
7. `Dracovis Engine\CodeFile\ECS\System\MeshRendererSystem.h` — SetDX12SpotLightData 声明
8. `Dracovis Engine\CodeFile\ECS\System\MeshRendererSystem.cpp` — SetDX12SpotLightData 实现

## 验证
1. 编译通过（Debug|x64）
2. 运行引擎（默认 DX12 模式）
3. 看到 "DX12 Lights" 窗口，包含 2 点光 + 1-2 聚光
4. 拖动 PointLight 位置 → 3D 场景光照变化
5. 拖动 SpotLight 位置和 Rotation → 聚光锥角照射方向变化
6. 调整 Outer/Inner Angle → 锥角大小变化
7. 点击 "Add Point Light" → 新灯光生效
8. 点击 "Remove Last" → 灯光减少
9. 切换 Enabled → 灯光开关

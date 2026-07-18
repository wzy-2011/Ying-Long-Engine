# ImGui 旋转速度控制窗口

## Context

用户要求适配 ImGui 到 DX12 渲染管线，并创建一个控制窗口来控制 DemoScene 中图案的旋转速度。当前 ImGuiDX12 已集成且正常工作，DX12 帧循环中已有基础 ImGui 窗口（"DX12 Mode" 信息 + 相机控制）。但旋转速度在 DX12Triangle/DX12Box 中硬编码（0.5 rad/s），无法外部控制。

## 修改文件

| 文件 | 修改内容 |
|------|---------|
| `DX12Primitives.h` | DX12Triangle/DX12Box 添加 `RotationSpeed[3]` 成员和 getter/setter |
| `DX12Primitives.cpp` | Update() 使用 RotationSpeed 替代硬编码值 |
| `DX12DemoScene.h` | 添加 `SpawnControlWindow()` 方法声明 |
| `DX12DemoScene.cpp` | 实现 `SpawnControlWindow()`，用 ImGui::SliderFloat 控制旋转速度 |
| `Application.cpp` | DoFrameDX12() 中调用 SpawnControlWindow |

所有文件路径基于 `Dracovis Engine\CodeFile\Graphics\DX12\` 或 `Dracovis Engine\CodeFile\Application\`。

---

## 步骤

### 1. DX12Triangle 添加旋转速度成员

**DX12Primitives.h** — DX12Triangle 类：
```cpp
// Transform data
float Position[3];
float Rotation[3];
float RotationSpeed[3];  // 新增：rad/s
float Scale;
```

构造函数初始化列表中：`RotationSpeed{0.0f, 0.0f, 0.5f}`（保持现有行为）

添加 getter：
```cpp
float* GetRotationSpeed() { return RotationSpeed; }
```

### 2. DX12Box 添加旋转速度成员

**DX12Primitives.h** — DX12Box 类：
```cpp
float RotationSpeed[3];  // 新增
```

构造函数初始化：`RotationSpeed{0.3f, 0.5f, 0.0f}`（保持现有行为）

添加 getter：
```cpp
float* GetRotationSpeed() { return RotationSpeed; }
```

### 3. Update() 使用 RotationSpeed

**DX12Primitives.cpp**：

DX12Triangle::Update():
```cpp
void DX12Triangle::Update(float deltaTime)
{
    Rotation[0] += deltaTime * RotationSpeed[0];
    Rotation[1] += deltaTime * RotationSpeed[1];
    Rotation[2] += deltaTime * RotationSpeed[2];
    UpdateTransformBuffer();
}
```

DX12Box::Update() 同理。

### 4. DX12DemoScene 添加 SpawnControlWindow

**DX12DemoScene.h** — 添加方法：
```cpp
void SpawnControlWindow();
```

**DX12DemoScene.cpp** — 实现：
```cpp
void DX12DemoScene::SpawnControlWindow()
{
    ImGui::Begin("Rotation Control");

    for (size_t i = 0; i < Triangles.size(); i++)
    {
        if (ImGui::TreeNode((void*)(intptr_t)i, "Triangle %d", (int)i))
        {
            float* speed = Triangles[i]->GetRotationSpeed();
            ImGui::SliderFloat("Pitch Speed", &speed[0], -2.0f, 2.0f);
            ImGui::SliderFloat("Yaw Speed", &speed[1], -2.0f, 2.0f);
            ImGui::SliderFloat("Roll Speed", &speed[2], -2.0f, 2.0f);
            ImGui::TreePop();
        }
    }

    for (size_t i = 0; i < Boxes.size(); i++)
    {
        if (ImGui::TreeNode((void*)(intptr_t)(i + 100), "Box %d", (int)i))
        {
            float* speed = Boxes[i]->GetRotationSpeed();
            ImGui::SliderFloat("Pitch Speed", &speed[0], -2.0f, 2.0f);
            ImGui::SliderFloat("Yaw Speed", &speed[1], -2.0f, 2.0f);
            ImGui::SliderFloat("Roll Speed", &speed[2], -2.0f, 2.0f);
            ImGui::TreePop();
        }
    }

    ImGui::End();
}
```

### 5. Application::DoFrameDX12 调用控制窗口

**Application.cpp** DoFrameDX12() 中，在 `pDX12DemoScene->Update()` 之前添加：
```cpp
pDX12DemoScene->SpawnControlWindow();
```

## 验证

1. 编译零错误
2. 运行程序，ImGui 应显示 "Rotation Control" 窗口
3. 展开 Triangle/Box 节点，拖动 Slider 可实时改变旋转速度
4. 速度为 0 时物体停止旋转，负值时反转旋转方向

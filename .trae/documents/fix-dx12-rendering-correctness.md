# DX12 渲染正确性修复计划

## 摘要

DX12 渲染管线不崩溃但画面不正确。核心问题：法线反向、无相机矩阵传递、每帧 GetBuffer 泄漏、描述符表可能无效。

## 问题清单

| # | 严重度 | 问题 | 影响 |
|---|--------|------|------|
| 1 | 严重 | Box 6面法线全部反向 | 光照完全错误 |
| 2 | 严重 | View/Projection 矩阵从未传递给 DX12 物体 | 无透视投影，相机不动 |
| 3 | 中等 | 每帧 GetBuffer() 资源泄漏 | 引用计数持续增加 |
| 4 | 中等 | 描述符表 root param 4-5 绑定无效描述符 | 调试层报错或渲染异常 |
| 5 | 低 | F5 切换无边沿检测 | 按住高频切换 |

## 修复方案

### 修复 1：修正 Box/Triangle 法线方向

**文件**: `DX12Primitives.cpp`

Box 法线翻转（第 210-238 行）：
- Front (z=+0.5): `{0,0,-1}` → `{0,0,1}`
- Back (z=-0.5): `{0,0,1}` → `{0,0,-1}`
- Top (y=+0.5): `{0,-1,0}` → `{0,1,0}`
- Bottom (y=-0.5): `{0,1,0}` → `{0,-1,0}`
- Right (x=+0.5): `{-1,0,0}` → `{1,0,0}`
- Left (x=-0.5): `{1,0,0}` → `{-1,0,0}`

Triangle 法线翻转（第 38-40 行）：
- `{0,0,-1}` → `{0,0,1}`

### 修复 2：传递相机矩阵给 DX12 物体

**文件**: `DX12Primitives.h`, `DX12Primitives.cpp`, `DX12DemoScene.cpp`

1. DX12Triangle/DX12Box 添加方法：
```cpp
void SetViewMatrix(const float* viewMatrix);
void SetProjectionMatrix(const float* projMatrix);
```
实现：memcpy 到成员 ViewMatrix/ProjectionMatrix，然后调用 UpdateTransformBuffer()。

2. DX12DemoScene::Update() 中传递矩阵给所有 Box/Triangle：
```cpp
for (auto& box : Boxes) {
    if (box) {
        box->SetViewMatrix(ViewMatrix);
        box->SetProjectionMatrix(ProjectionMatrix);
    }
}
```

### 修复 3：缓存交换链后缓冲

**文件**: `DX12Renderer.h`, `DX12Renderer.cpp`

方案：使用 `RenderTargetDX12` 数组（FRAME_COUNT=2 个），在 Initialize 时一次性缓存两个后缓冲，BeginFrame 时按 backBufferIndex 选择对应的 RT。

1. DX12Renderer.h 添加：
```cpp
static constexpr int FRAME_COUNT = 2;
std::unique_ptr<RenderTargetDX12> pRenderTargets[FRAME_COUNT];
```
替换原来的单个 `pRenderTarget`。

2. Initialize() 中为两个后缓冲各调用一次 InitializeFromSwapChain。

3. BeginFrame() 中按 `backBufferIndex` 选择 `pRenderTargets[backBufferIndex]`，不再每帧调用 InitializeFromSwapChain。

4. ExecuteResize() 中重新缓存两个后缓冲。

5. 所有使用 `pRenderTarget` 的地方改为 `pRenderTargets[currentIndex]`。

### 修复 4：描述符表绑定安全处理

**文件**: `DX12Primitives.cpp`

当前 placeholder shader（PS）不使用纹理采样器，但 root signature 仍声明了描述符表。有两种方案：

**方案A（推荐）**：在 DX12Core 初始化时，在 CBVSRVUAV 堆和 Sampler 堆的起始位置创建 null SRV 和默认 Sampler 描述符，这样即使绑定空的描述符表也不会报错。

**方案B**：在 Draw() 中只在有有效纹理时才绑定描述符表。

选择方案A，因为更安全且为后续纹理支持做准备。

1. 在 DX12Core::Initialize() 创建描述符堆后，在 CBVSRVUAV 堆索引 0 创建一个 null SRV（占位），在 Sampler 堆索引 0 创建一个默认 Sampler 描述符。

### 修复 5：F5 按键边沿检测

**文件**: `Application.cpp`

将 `KeyIsPressed` 改为上升沿检测：

```cpp
static bool f5WasPressed = false;
bool f5IsPressed = MainWindow.keyboard.KeyIsPressed(VK_F5);
if (f5IsPressed && !f5WasPressed)
{
    EnableDX12Mode(!bUseDX12);
}
f5WasPressed = f5IsPressed;
```

## 验证步骤

1. 编译通过
2. 启动程序，确认 DX12 模式激活
3. 视觉验证：3 个旋转 Box 正确渲染（有透视效果、法线正确、颜色不同）
4. ImGui 面板正常显示
5. 拖拽窗口不会崩溃（resize 正常）
6. F5 切换 DX11/DX12 正常工作
7. D3D12 调试层无严重错误输出

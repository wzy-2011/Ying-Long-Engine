# DX12 渲染管线修复计划

## Context
项目的 DX12 渲染管线已部分实现但存在多个严重 Bug，当前默认使用 DX11 模式（`bUseDX12 = false`）。需要修复所有 Bug 并将 DX12 设为默认工作模式。

## 修复步骤（按依赖顺序）

### 1. 修复资源状态追踪（Bug 3）
- **文件**: `RenderTargetDX12.h/.cpp`, `DepthStencilDX12.h/.cpp`, `DX12Renderer.cpp`
- **问题**: `TransitionTo()` 要求调用方传入 `oldState`，但硬编码的旧状态可能不正确
- **修复**:
  - 在 `RenderTargetDX12` 和 `DepthStencilDX12` 中添加 `CurrentState` 成员变量
  - `TransitionTo` 只接收 `newState`，内部使用 `CurrentState` 作为旧状态
  - 在 `Initialize`/`InitializeFromSwapChain` 中设置初始状态
  - `DX12Renderer::BeginFrame/EndFrame` 中移除 `oldState` 参数

### 2. 修复 DSV 描述符堆溢出（Bug 4）
- **文件**: `DX12Core.cpp`, `DX12Core.h`
- **问题**: DSV 堆只有 1 个描述符，但 DX12Core 和 DepthStencilDX12 都试图分配 DSV；Resize 时会溢出
- **修复**:
  - DSV 堆大小从 1 增至 4
  - 移除 `DX12Core::CreateDepthStencilView()` 中的资源创建和 DSV 分配（不再维护 `pDepthStencil`）
  - 移除 `DX12Core::Initialize()` 中 Step 8 的调用
  - 修改 `DX12Core::Resize()` 不再重建内部深度模板
  - `GetDSVHandle()` 改为返回 DepthStencilDX12 管理的句柄（或标记为废弃）

### 3. 修复 CreateCommittedResource 返回悬空指针（Bug 1）
- **文件**: `DX12Core.h`, `DX12Core.cpp`
- **问题**: 返回 `resource.Get()` 但 ComPtr 在函数退出时释放引用
- **修复**: 返回类型改为 `Microsoft::WRL::ComPtr<ID3D12Resource>`，直接返回 ComPtr

### 4. 修复 AddSampler 错误类型（Bug 2）
- **文件**: `DX12RootSignature.h`, `DX12RootSignature.cpp`
- **问题**: `AddSampler()` 使用 `static_cast<D3D12_ROOT_PARAMETER_TYPE>(3)` 创建无效的根参数
- **修复**: 删除 `AddSampler()` 方法，已有 `AddDescriptorTableSampler()` 和 `AddStaticSampler()` 替代

### 5. 修复 Build() 创建两次根签名（Bug 5）
- **文件**: `DX12RootSignature.h`, `DX12RootSignature.cpp`
- **问题**: `Build()` 先创建本地 rootSignature（未使用），再 new DX12RootSignature（又创建一次）
- **修复**: 为 `DX12RootSignature` 添加接受已创建根签名的构造函数，`Build()` 传递已创建的根签名

### 6. 修复 UploadBuffer::Allocate 返回值歧义（Bug 6）
- **文件**: `DX12UploadBuffer.cpp`
- **问题**: 满时返回 0，但 0 也是有效偏移
- **修复**: 满时抛出 `std::runtime_error`

### 7. 修复 PSO 着色器编译失败时崩溃（Bug 7）
- **文件**: `DX12PipelineState.h`, `DX12PipelineState.cpp`
- **问题**: 回退到 `Initialize()` 默认路径时，没有着色器字节码，`CreatePSO()` 抛异常
- **修复**: 添加 `SetPlaceholderShaders()` 方法，使用 D3DCompile 编译最小着色器作为回退

### 8. 修复缺失的常量缓冲区绑定 + 着色器布局不匹配（Bug 8 + Bug 9）
- **文件**: `DX12Primitives.h`, `DX12Primitives.cpp`
- **问题 A（Bug 9）**: `DX12Transform` 有 Model+View+Projection 三个矩阵，但 `PBRVertexShader` 的 cbuffer 只需 Model+MVP 两个矩阵
- **问题 B（Bug 8）**: DX12Box/DX12Triangle 只绑定根参数 3（Transform），但 PBR 像素着色器还需要 b0(PointLight)、b1(SpotLight)、b2(Material)、纹理表(t0-t3)、采样器表(s0)
- **修复**:
  - 修改 `DX12Transform` 为 `ModelMatrix[16] + ModelViewProjMatrix[16]`
  - 添加占位 CBV 结构体（PointLight、SpotLight、Material），匹配着色器 cbuffer 布局
  - 在 DX12Triangle/DX12Box 构造函数中创建占位 CBV
  - 在 Draw 中绑定所有 6 个根参数
  - 修改 `UpdateTransformBuffer` 计算 ModelViewProj 矩阵（需转置）
  - 创建默认白色纹理 SRV 和线性采样器用于纹理表和采样器表

### 9. 将 DX12 设为默认模式
- **文件**: `Application.cpp`
- **修复**: `bUseDX12(false)` 改为 `bUseDX12(true)`

## 修改文件汇总

| 文件 | 修改内容 |
|------|----------|
| `DX12Core.h` | CreateCommittedResource 返回 ComPtr；移除 pDepthStencil 相关 |
| `DX12Core.cpp` | CreateCommittedResource 返回值；DSV 堆增至 4；移除内部深度模板创建 |
| `DX12RootSignature.h` | 删除 AddSampler；添加新构造函数 |
| `DX12RootSignature.cpp` | 删除 AddSampler；修改 Build；实现新构造函数 |
| `RenderTargetDX12.h` | 添加 CurrentState；修改 TransitionTo 签名 |
| `RenderTargetDX12.cpp` | TransitionTo 使用内部状态；设置初始状态 |
| `DepthStencilDX12.h` | 添加 CurrentState；修改 TransitionTo 签名 |
| `DepthStencilDX12.cpp` | TransitionTo 使用内部状态；设置初始状态 |
| `DX12Renderer.cpp` | 移除 TransitionTo 的 oldState 参数 |
| `DX12UploadBuffer.cpp` | Allocate 满时抛出异常 |
| `DX12PipelineState.h` | 添加 SetPlaceholderShaders |
| `DX12PipelineState.cpp` | 实现占位着色器回退 |
| `DX12Primitives.h` | 修改 DX12Transform；添加占位 CBV 结构体和成员 |
| `DX12Primitives.cpp` | 修改 UpdateTransformBuffer；Draw 绑定所有根参数 |
| `Application.cpp` | bUseDX12 改为 true |

## 验证方法

1. 编译项目确保无编译错误
2. 运行程序，应直接进入 DX12 模式
3. 应看到 3 个旋转的 Box（DemoScene），带有正确的颜色和变换
4. ImGui 面板正常显示和交互
5. 按 F5 可切换到 DX11 模式再切回 DX12
6. 调整窗口大小时不崩溃（Resize 测试）
7. D3D12 调试层无错误输出

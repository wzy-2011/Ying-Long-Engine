# 修复 BindableDX12 C3668 编译错误计划

## 问题概述

BindableDX12/ 目录下的子类（SamplerDX12、IndexBufferDX12、ConstantBufferDX12、TextureDX12、VertexBufferDX12）override 了基类中不存在的4个虚方法，导致 C3668 编译错误。

## 当前状态分析

两个 BindableDX12.h 文件定义了同一个类 `DE::BindableDX12`，但内容略有不同：

| 文件 | Bind() 签名 | 额外内容 |
|------|-------------|----------|
| `BindableDX12/BindableDX12.h` | `ID3D12GraphicsCommandList*` | BindableDX12Helper 类 |
| `DX12/BindableDX12.h` | `::ID3D12GraphicsCommandList*` | 独立 inline 辅助函数 |

BindableDX12/ 子类 override 但基类缺少的虚方法：
1. `GetGPUHandle()` → SamplerDX12, TextureDX12 使用
2. `GetCPUHandle()` → SamplerDX12, TextureDX12 使用
3. `GetType()` → SamplerDX12, TextureDX12 使用
4. `GetResource()` → IndexBufferDX12, ConstantBufferDX12, TextureDX12, VertexBufferDX12 使用

DX12/ 子类不需要这些方法（它们有自己的实现方式，不走描述符表路径）。

## 修改计划

### 步骤1：修改 BindableDX12/BindableDX12.h

在 `BindableDX12` 类的 public 区域（`GetRootParameterIndex()` 之后），添加4个带默认实现的虚方法：

```cpp
virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return {}; }
virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return {}; }
virtual D3D12_DESCRIPTOR_RANGE_TYPE GetType() const noexcept { return D3D12_DESCRIPTOR_RANGE_TYPE_SRV; }
virtual ID3D12Resource* GetResource() const noexcept { return nullptr; }
```

### 步骤2：修改 DX12/BindableDX12.h

同样在 `BindableDX12` 类的 public 区域添加相同的4个虚方法（使用 `::` 前缀保持风格一致）：

```cpp
virtual ::D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return {}; }
virtual ::D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return {}; }
virtual ::D3D12_DESCRIPTOR_RANGE_TYPE GetType() const noexcept { return ::D3D12_DESCRIPTOR_RANGE_TYPE_SRV; }
virtual ::ID3D12Resource* GetResource() const noexcept { return nullptr; }
```

### 步骤3：编译验证

运行构建确认 C3668 错误已消除。如有其他编译错误，逐一修复。

### 步骤4：运行程序验证 DX12 渲染

启动程序，按 F5 切换到 DX12 模式，确认渲染输出正确。

## 设计决策

- 这4个虚方法使用**默认实现**（非纯虚），因为不是所有子类都需要实现全部方法（如 SamplerDX12 不需要 GetResource，IndexBufferDX12 不需要 GetGPUHandle）
- `GetType()` 默认返回 `SRV` 是因为大多数资源是着色器资源视图
- `GetGPUHandle()` 和 `GetCPUHandle()` 默认返回空句柄 `{}`（零初始化）
- `GetResource()` 默认返回 `nullptr`
- 两个头文件保持内容一致但风格各自保持（BindableDX12/ 不用 `::` 前缀，DX12/ 用 `::` 前缀）

## 风险与注意事项

- 两个头文件定义同一个类是 ODR 违规的隐患，当前通过 include 路径隔离规避，但长期应统一为一个头文件
- 添加虚方法会改变 vtable 布局，但由于两个头文件同步修改，不会引起 ABI 不匹配

# DX12 三大问题修复计划

## Context

用户确认 DX12 渲染已可见，但存在三个问题：
1. 命令行缺少调试信息（所有日志走 OutputDebugStringA，控制台看不到）
2. 窗口拉伸时内存泄漏（DSV 描述符泄漏 + ResizeBuffers 顺序错误）
3. 关闭窗口时抛异常（Shutdown 空指针 + 双重销毁窗口）

## 修改文件清单

| 文件 | 修改内容 |
|------|---------|
| `Debug/DX12Log.h` | 新建：双通道日志工具（同时输出到调试器和控制台） |
| `DX12Core.cpp` | Shutdown 空指针防护、Resize HRESULT 检查、关键路径添加日志 |
| `DX12Core.h` | 添加 bInitialized 成员 |
| `DX12Renderer.cpp` | 修复 ExecuteResize 顺序、关键路径添加日志 |
| `Window.cpp` | Destroy 双重销毁防护、pDX12Renderer 改为 make_unique |
| `Window.h` | pDX12Renderer 改为 unique_ptr |
| `DepthStencilDX12.cpp` | DSV 描述符使用固定索引避免泄漏 |
| `DX12Fence.h` | Wait 添加超时参数 |
| `DX12Fence.cpp` | Wait 实现超时和空指针保护 |

所有文件路径基于 `Dracovis Engine\CodeFile\Graphics\DX12\` 或 `Dracovis Engine\CodeFile\Application\`。

---

## 修复步骤

### 第一阶段：修复关闭窗口异常（最严重）

#### 1.1 DX12Core::Shutdown() 空指针防护

**文件**: `DX12Core.h` — 添加 `bool bInitialized = false;` 成员

**文件**: `DX12Core.cpp`:
- `Initialize()` 末尾设 `bInitialized = true;`
- `Shutdown()` 改为：
  ```cpp
  void DX12Core::Shutdown()
  {
      if (!bInitialized) return;
      bInitialized = false;
      if (Fence && pCommandQueue) { try { WaitForGPU(); } catch (...) {} }
      // 后续 ComPtr::Reset() 对 nullptr 安全，不需额外检查
      pCommandList.Reset();
      for (UINT i = 0; i < FRAME_COUNT; ++i) { pCommandAllocators[i].Reset(); pRenderTargets[i].Reset(); }
      pSwapChain.Reset(); pCommandQueue.Reset(); pDevice.Reset();
      RTVHeap.reset(); DSVHeap.reset(); CBVSRVUAVHeap.reset(); SamplerHeap.reset();
      Fence.reset(); RootSignature.reset(); UploadBuffer.reset(); PipelineState.reset();
  }
  ```

#### 1.2 Window::Destroy() 双重销毁防护

**文件**: `Window.cpp` 第104-109行：
```cpp
void Window::Destroy()
{
    if (hWnd == nullptr) return;
    DestroyWindow(hWnd);
    hWnd = nullptr;
    this->IsDragging = false;
}
```

#### 1.3 pDX12Renderer 改为 unique_ptr

**文件**: `Window.h` 第130行：
```cpp
std::unique_ptr<DX12Renderer> pDX12Renderer;  // 替换 class DX12Renderer*
```

**文件**: `Window.cpp`：
- `InitializeDX12()`: `new DX12Renderer()` → `std::make_unique<DX12Renderer>()`
- `ShutdownDX12()`: `delete pDX12Renderer; pDX12Renderer = nullptr;` → `pDX12Renderer.reset();`
- `GetDX12Renderer()`: `return pDX12Renderer;` → `return pDX12Renderer.get();`

#### 1.4 DX12Fence::Wait() 添加超时

**文件**: `DX12Fence.h` 第19行：`void Wait(UINT64 value, DWORD timeoutMs = 5000);`

**文件**: `DX12Fence.cpp` 第48-55行：
```cpp
void DX12Fence::Wait(UINT64 value, DWORD timeoutMs)
{
    if (!pFence) return;
    if (pFence->GetCompletedValue() < value)
    {
        pFence->SetEventOnCompletion(value, FenceEvent);
        DWORD result = WaitForSingleObject(FenceEvent, timeoutMs);
        if (result == WAIT_TIMEOUT)
            OutputDebugStringA("[DX12Fence::Wait] Timeout!\n");
    }
}
```

### 第二阶段：修复窗口拉伸内存泄漏

#### 2.1 修复 ExecuteResize 顺序

**文件**: `DX12Renderer.cpp` 第342-391行，重写 `ExecuteResize()`：

正确顺序：先释放所有资源 → 再 ResizeBuffers → 再重建资源

```cpp
void DX12Renderer::ExecuteResize()
{
    if (!bNeedsResize) return;
    int width = PendingWidth, height = PendingHeight;
    if (width <= 0 || height <= 0) { bNeedsResize = false; return; }

    WaitForGPU();
    int oldWidth = Width, oldHeight = Height;

    try {
        // 1. 先释放所有交换链缓冲区引用
        for (int i = 0; i < FRAME_COUNT; i++) pRenderTargets[i]->Shutdown();
        pDepthStencil->Shutdown();

        // 2. 再 Resize（内部调用 ResizeBuffers）
        pCore->Resize(width, height);
        Width = width; Height = height;

        // 3. 重建资源
        for (int i = 0; i < FRAME_COUNT; i++)
            pRenderTargets[i]->InitializeFromSwapChain(*pCore, i);
        pDepthStencil->Initialize(*pCore, width, height);

        bNeedsResize = false;
    } catch (const std::exception& e) {
        Width = oldWidth; Height = oldHeight;
        bNeedsResize = false;
        // 日志输出
    }
}
```

#### 2.2 DSV 描述符泄漏修复

**文件**: `DepthStencilDX12.cpp` `CreateDSV()` 第138行：

将 `DSVHeapIndex = core.GetDSVHeap()->Allocate();` 改为使用固定索引 0：
```cpp
DSVHeapIndex = 0;  // 固定使用索引0，避免 resize 时描述符泄漏
DSVHandle = core.GetDSVHeap()->GetCPUHandle(DSVHeapIndex);
```

同时在 `DX12Core::CreateDescriptorHeaps()` 中，创建 DSV 堆后预先 Allocate 一个槽位占住索引0。

#### 2.3 ResizeBuffers HRESULT 检查

**文件**: `DX12Core.cpp` 第660行，检查 `ResizeBuffers` 返回值：
```cpp
HRESULT hr = pSwapChain->ResizeBuffers(FRAME_COUNT, newWidth, newHeight,
    DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
if (FAILED(hr))
    throw std::runtime_error("ResizeBuffers failed: hr=" + std::to_string(hr));
```

### 第三阶段：添加控制台调试日志

#### 3.1 创建 DX12Log.h

新建 `Dracovis Engine/CodeFile/Debug/DX12Log.h`：
```cpp
#pragma once
#include <Windows.h>
#include <iostream>
#include <string>

namespace DE
{
    inline void DX12Log(const char* msg)
    {
        OutputDebugStringA(msg);
        std::cout << msg;
    }
    inline void DX12LogError(const char* msg)
    {
        OutputDebugStringA(msg);
        std::cerr << msg;
    }
}
```

#### 3.2 在关键路径替换日志

仅替换关键路径（非全量替换，控制改动范围）：
- `DX12Core::Initialize()` 每个步骤的成功/失败 → `DX12Log`
- `DX12Core::Shutdown()` 入口/退出 → `DX12Log`
- `DX12Core::Resize()` 结果 → `DX12Log`
- `DX12Renderer::Initialize()` 每个步骤 → `DX12Log`
- `DX12Renderer::Shutdown()` → `DX12Log`
- `DX12Renderer::ExecuteResize()` 失败 → `DX12LogError`
- `Window::InitializeDX12()` 成功/失败 → `DX12Log`

---

## 验证步骤

1. 编译通过（零错误）
2. 运行程序 → 控制台应显示 DX12 初始化步骤信息
3. 拉伸窗口多次 → 不崩溃、不抛异常
4. 关闭窗口 → 不抛异常、正常退出
5. 确认 DX12 渲染仍正常显示

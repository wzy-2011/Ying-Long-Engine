/**
 * @file DX12.h
 * @brief DX12 模块统一入口头文件 / DX12 module unified entry header
 *
 * 包含所有 DX12 相关组件的头文件，外部模块只需包含此文件
 * 即可使用全部 DX12 功能。
 *
 * Includes all DX12-related component headers. External modules only need
 * to include this file to use all DX12 functionality.
 *
 * 模块组成 / Module composition:
 *   - Core:        DX12Core, DX12Fence, DX12DescriptorHeap, DX12UploadBuffer, DX12RootSignature
 *   - Render:      RenderTargetDX12, DepthStencilDX12, DX12PipelineState, DX12Drawable, DX12Renderer, ImGuiDX12
 *   - Bindable:    BindableDX12, VertexBufferDX12, IndexBufferDX12, ConstantBufferDX12, TextureDX12, SamplerDX12
 *   - Primitives:  DX12Primitives, DX12DemoScene
 */
#pragma once

// DX12 Core Components / DX12 核心组件
#include "DX12Core.h"
#include "DX12Fence.h"
#include "DX12DescriptorHeap.h"
#include "DX12UploadBuffer.h"
#include "DX12RootSignature.h"

// DX12 Render Components / DX12 渲染组件
#include "RenderTargetDX12.h"
#include "DepthStencilDX12.h"
#include "DX12PipelineState.h"
#include "DX12Drawable.h"
#include "DX12Renderer.h"
#include "ImGuiDX12.h"

// DX12 Bindable Components / DX12 可绑定组件
#include "BindableDX12.h"
#include "VertexBufferDX12.h"
#include "IndexBufferDX12.h"
#include "ConstantBufferDX12.h"
#include "TextureDX12.h"
#include "SamplerDX12.h"

// DX12 Demo Primitives and Scene / DX12 演示图元和场景
#include "DX12Primitives.h"
#include "DX12DemoScene.h"
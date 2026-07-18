/**
 * @file Bindable.h
 * @brief DX11 可绑定对象基类头文件 / DX11 Bindable object base class header file
 *
 * 定义了所有 DX11 可绑定资源的抽象基类，提供统一的绑定接口
 * 和设备/设备上下文访问方法。
 * Defines the abstract base class for all DX11 bindable resources,
 * providing a unified binding interface and device/device context access methods.
 *
 * @note 已弃用：新代码应使用 Graphics/BindableDX12/ 目录中的 BindableDX12
 *       DEPRECATED: New code should use BindableDX12 in Graphics/BindableDX12/
 */

#pragma once

// =============================================================================
// DEPRECATED: This file contains the old DX11 Bindable implementation.
// New code should use BindableDX12 in the Graphics/BindableDX12/ directory.
// =============================================================================

#pragma message("WARNING: Bindable.h is deprecated. Use BindableDX12 instead.")

#include <vector>
#include <iostream>
#include <wrl/client.h>
#include <d3d11.h>

using namespace Microsoft;

namespace YingLong
{
	class Graphics;

	/**
	 * @brief DX11 可绑定对象基类 / DX11 bindable object base class
	 *
	 * 所有可绑定到渲染管线的 DX11 资源的抽象基类。
	 * Abstract base class for all DX11 resources that can be bound to the rendering pipeline.
	 *
	 * 子类需要实现 Bind() 方法以将资源绑定到相应的管线阶段。
	 * Subclasses must implement the Bind() method to bind the resource
	 * to the appropriate pipeline stage.
	 *
	 * @deprecated 请使用 BindableDX12 替代 / Use BindableDX12 instead
	 */
	class Bindable [[deprecated("Use BindableDX12 instead")]]
	{
	public:
		/**
		 * @brief 将资源绑定到渲染管线 / Bind the resource to the rendering pipeline
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 纯虚函数，由子类实现具体的绑定逻辑。
		 * Pure virtual function, subclasses implement specific binding logic.
		 */
		virtual void Bind(class Graphics& graphics) noexcept = 0;

		/**
		 * @brief 析构函数 / Destructor
		 */
		virtual ~Bindable() = default;

		/**
		 * @brief 获取设备上下文 / Get the device context
		 * @param graphics 图形设备指针 / Graphics device pointer
		 * @return ID3D11DeviceContext 的 ComPtr / ComPtr to ID3D11DeviceContext
		 *
		 * 从 Graphics 对象中获取 D3D11 设备上下文。
		 * Retrieves the D3D11 device context from the Graphics object.
		 */
		static WRL::ComPtr<ID3D11DeviceContext> GetDevicContext(class Graphics* graphics) noexcept;

		/**
		 * @brief 获取设备 / Get the device
		 * @param graphics 图形设备指针 / Graphics device pointer
		 * @return ID3D11Device 的 ComPtr / ComPtr to ID3D11Device
		 *
		 * 从 Graphics 对象中获取 D3D11 设备。
		 * Retrieves the D3D11 device from the Graphics object.
		 */
		static WRL::ComPtr<ID3D11Device> GetDevice(class Graphics* graphics) noexcept;
	};
}

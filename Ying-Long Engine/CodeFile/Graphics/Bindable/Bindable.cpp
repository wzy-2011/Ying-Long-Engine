/**
 * @file Bindable.cpp
 * @brief DX11 可绑定对象基类实现文件 / DX11 Bindable object base class implementation file
 *
 * 实现 Bindable 基类的静态辅助方法，用于获取 D3D11 设备和设备上下文。
 * Implements static helper methods of the Bindable base class for
 * retrieving D3D11 device and device context.
 *
 * @note 已弃用：新代码应使用 Graphics/BindableDX12/ 目录中的 BindableDX12
 *       DEPRECATED: New code should use BindableDX12 in Graphics/BindableDX12/
 */

#include "Bindable.h"
#include "../Graphics.h"

namespace YingLong
{
	WRL::ComPtr<ID3D11DeviceContext> Bindable::GetDevicContext(Graphics* graphics) noexcept
	{
		// 直接返回 Graphics 对象中存储的设备上下文指针
		// Directly return the device context pointer stored in the Graphics object
		return graphics->pDeviceContext;
	}

	WRL::ComPtr<ID3D11Device> Bindable::GetDevice(Graphics* graphics) noexcept
	{
		// 直接返回 Graphics 对象中存储的设备指针
		// Directly return the device pointer stored in the Graphics object
		return graphics->pDevice;
	}
}

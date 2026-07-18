/**
 * @file Topology.cpp
 * @brief DX11 图元拓扑实现文件 / DX11 primitive topology implementation file
 *
 * 实现 Topology 类的构造和绑定功能。
 * Implements Topology class construction and binding functionality.
 */

#include "Topology.h"

namespace YingLong
{
	Topology::Topology(Graphics& graphics, D3D11_PRIMITIVE_TOPOLOGY type) : type(type)
	{

	}

	void Topology::Bind(Graphics& graphics) noexcept
	{
		// 设置输入装配阶段的图元拓扑类型
		// Set primitive topology type for the input assembler stage
		GetDevicContext(&graphics)->IASetPrimitiveTopology(type);
	}
}

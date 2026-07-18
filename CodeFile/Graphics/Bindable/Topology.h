/**
 * @file Topology.h
 * @brief DX11 图元拓扑头文件 / DX11 primitive topology header file
 *
 * 封装 D3D11 图元拓扑类型，用于指定输入装配阶段如何解释顶点数据。
 * Encapsulates D3D11 primitive topology type, used to specify how the
 * input assembler stage interprets vertex data.
 */

#pragma once
#include "Bindable.h"

namespace YingLong
{
	/**
	 * @brief DX11 图元拓扑类 / DX11 primitive topology class
	 *
	 * 管理 D3D11 图元拓扑状态，控制输入装配阶段将顶点数据
	 * 组装成何种图元（如点、线、三角形等）。
	 * Manages D3D11 primitive topology state, controlling what type of
	 * primitive (points, lines, triangles, etc.) the input assembler
	 * stage assembles from vertex data.
	 */
	class Topology : public Bindable
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param type 图元拓扑类型 / Primitive topology type
		 *
		 * 创建指定图元拓扑类型的 Topology 对象。
		 * Creates a Topology object with the specified primitive topology type.
		 */
		Topology(Graphics& graphics, D3D11_PRIMITIVE_TOPOLOGY type);

		/**
		 * @brief 绑定图元拓扑 / Bind primitive topology
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将图元拓扑设置到输入装配阶段。
		 * Sets the primitive topology to the input assembler stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

	protected:
		D3D11_PRIMITIVE_TOPOLOGY type;   ///< 图元拓扑类型 / Primitive topology type
	};
}

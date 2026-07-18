/** @file SolidCone.h
 *  @brief 纯色线框锥体可绘制对象 - Solid-color wireframe cone drawable object
 *
 *  包含纯色线框锥体可绘制对象类定义，用于聚光灯可视化。
 *  锥体以线框模式渲染，与点光源的实体球体形成视觉区分。
 *  Contains the solid-color wireframe cone drawable object class definition, used for
 *  spot light visualization. Cones render in wireframe to visually distinguish from
 *  point light solid spheres.
 */
#pragma once
#include "DrawableBase.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/SolidColorConstantBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Geometry/Cone.h"
#include "../Bindable/VertexBuffer.h"
#include <DirectXMath.h>

namespace YingLong
{
	/** @brief 纯色锥体可绘制对象类
	 *  Solid-color cone drawable object class
	 *
	 *  使用纯色着色器的锥体可绘制对象，支持位置和颜色设置，
	 *  以及锥体高度和底面半径（开口大小）的动态调整。
	 *
	 *  Cone drawable object using solid color shader, supporting
	 *  position and color settings, as well as dynamic adjustment
	 *  of cone height and base radius (opening size).
	 */
	class SolidConeDrawable : public DrawableBase<SolidConeDrawable>
	{
	public:
		/** @brief 拷贝构造函数
		 *  Copy constructor
		 *
		 *  @param 要拷贝的锥体对象 / The cone object to copy from
		 */
		SolidConeDrawable(const SolidConeDrawable&);

		/** @brief 构造函数
		 *  Constructor
		 *
		 *  使用指定的高度、半径和颜色创建锥体。
		 *  Creates a cone with specified height, radius and color.
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 *  @param height 锥体高度 / Cone height
		 *  @param radius 底面半径 / Base radius
		 *  @param color 锥体颜色 / Cone color
		 */
		SolidConeDrawable(Graphics& graphics, float height, float radius, XMFLOAT3 color);

		/** @brief 设置位置
		 *  Set position
		 *
		 *  @param Position 新位置 / New position
		 */
		void SetPosition(XMFLOAT3 Position) noexcept;

		/** @brief 设置旋转（弧度）
		 *  Set rotation (radians)
		 *
		 *  @param Rotation 旋转角（弧度，pitch/yaw/roll）/ Rotation angles in radians (pitch/yaw/roll)
		 */
		void SetRotation(XMFLOAT3 Rotation) noexcept;

		/** @brief 设置颜色
		 *  Set color
		 *
		 *  @param Color 新颜色 / New color
		 */
		void SetColor(XMFLOAT3 Color) noexcept;

		/** @brief 更新锥体角度 / Update cone angle
		 *
		 *  重新生成锥体几何体以匹配新的锥角。
		 *  Regenerates cone geometry to match new cone angle.
		 *
		 *  @param height 新高度 / New height
		 *  @param radius 新底面半径 / New base radius
		 */
		void UpdateAngle(float height, float radius);

		/** @brief 更新锥体状态
		 *  Update cone state
		 *
		 *  @param dt 时间增量（秒） / Time delta in seconds
		 *  @param aspect 宽高比 / Aspect ratio
		 */
		void Update(float dt, float aspect) noexcept override;

		/** @brief 获取变换矩阵
		 *  Get transformation matrix
		 *
		 *  返回锥体的世界变换矩阵。
		 *  Returns the world transformation matrix of the cone.
		 *
		 *  @return DirectX 变换矩阵 / DirectX transformation matrix
		 */
		XMMATRIX GetTransformXM() const noexcept override;

		/** @brief 获取颜色
		 *  Get color
		 *
		 *  @return 颜色的常量引用 / Const reference to color
		 */
		const XMFLOAT3& GetColor() const noexcept;

	private:
		/** @brief 重新生成顶点和索引缓冲区 / Regenerate vertex and index buffers
		 *
		 *  根据当前锥体参数重新生成几何体数据并更新 GPU 缓冲区。
		 *  Regenerates geometry data based on current cone parameters and updates GPU buffers.
		 */
		void RegenerateGeometry();

		XMFLOAT3 Position = { 1.0f, 1.0f, 1.0f };       ///< 位置 / Position
		XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };       ///< 旋转（弧度）/ Rotation (radians)
		XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };          ///< 颜色 / Color
		float ConeHeight = 1.0f;                         ///< 锥体高度 / Cone height
		float ConeRadius = 0.5f;                         ///< 底面半径 / Base radius

		friend class SolidColorConstantBuffer; ///< 纯色常量缓冲区友元类 / Solid color constant buffer friend class
	};
}
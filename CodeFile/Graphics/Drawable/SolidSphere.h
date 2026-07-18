/** @file SolidSphere.h
 *  @brief 纯色球体可绘制对象 - Solid-color sphere drawable object
 *
 *  包含纯色球体可绘制对象类定义。
 *  Contains the solid-color sphere drawable object class definition.
 */
#pragma once
#include "DrawableBase.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/SolidColorConstantBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Bindable/Texture.h"
#include "../Geometry/Sphere.h"
#include "../Bindable/VertexBuffer.h"
#include <DirectXMath.h>

namespace YingLong
{
	/** @brief 纯色球体可绘制对象类
	 *  Solid-color sphere drawable object class
	 *
	 *  使用纯色着色器的球体可绘制对象，支持位置和颜色设置。
	 *  Sphere drawable object using solid color shader, supporting
	 *  position and color settings.
	 */
	class SolidSphereDrawable : public DrawableBase<SolidSphereDrawable>
	{
	public:
		/** @brief 拷贝构造函数
		 *  Copy constructor
		 *
		 *  @param 要拷贝的球体对象 / The sphere object to copy from
		 */
		SolidSphereDrawable(const SolidSphereDrawable&);

		/** @brief 构造函数
		 *  Constructor
		 *
		 *  使用指定的半径和颜色创建球体。
		 *  Creates a sphere with specified radius and color.
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 *  @param radius 球体半径 / Sphere radius
		 *  @param color 球体颜色 / Sphere color
		 */
		SolidSphereDrawable(Graphics& graphics, float radius, XMFLOAT3 color);
		
		/** @brief 设置位置
		 *  Set position
		 *
		 *  @param Position 新位置 / New position
		 */
		void SetPosition(XMFLOAT3 Position) noexcept;

		/** @brief 设置颜色
		 *  Set color
		 *
		 *  @param Color 新颜色 / New color
		 */
		void SetColor(XMFLOAT3 Color) noexcept;

		/** @brief 更新球体状态
		 *  Update sphere state
		 *
		 *  @param dt 时间增量（秒） / Time delta in seconds
		 *  @param aspect 宽高比 / Aspect ratio
		 */
		void Update(float dt, float aspect) noexcept override;

		/** @brief 获取变换矩阵
		 *  Get transformation matrix
		 *
		 *  返回球体的世界变换矩阵。
		 *  Returns the world transformation matrix of the sphere.
		 *
		 *  @return DirectX 变换矩阵 / DirectX transformation matrix
		 */
		XMMATRIX GetTransformXM() const noexcept override;

		/** @brief 获取颜色
		 *  Get color
		 *
		 *  @return 颜色的常量引用 / Const reference to color
		 */
		const XMFLOAT3& GetColor()const noexcept;

	private:
		XMFLOAT3 Position = { 1.0f, 1.0f, 1.0f }; ///< 位置 / Position
		XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };    ///< 颜色 / Color

		friend class SolidColorConstantBuffer; ///< 纯色常量缓冲区友元类 / Solid color constant buffer friend class
	};
}

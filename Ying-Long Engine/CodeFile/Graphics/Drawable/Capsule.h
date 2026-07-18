/** @file Capsule.h
 *  @brief 胶囊体可绘制对象 - Capsule drawable object
 *
 *  包含胶囊体可绘制对象类定义，支持 PBR 着色和 ImGui 控制窗口。
 *  Contains the capsule drawable object class definition with PBR shading
 *  and ImGui control window support.
 */
#pragma once
#include "DrawableBase.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/SolidColorConstantBuffer.h"
#include "../../Graphics/Geometry/Capsule.h"

namespace YingLong
{
	/** @brief 胶囊体可绘制对象类
	 *  Capsule drawable object class
	 *
	 *  一个带有 PBR 材质的胶囊体可绘制对象，支持位置、旋转、颜色等属性，
	 *  并提供 ImGui 控制面板用于调试。
	 *
	 *  A capsule drawable object with PBR material, supporting position,
	 *  rotation, color and other properties, and provides an ImGui control
	 *  panel for debugging.
	 */
	class CapsuleDrawable : public DrawableBase<CapsuleDrawable>
	{
	public:
		/** @brief 默认构造函数
		 *  Default constructor
		 */
		CapsuleDrawable() = default;

		/** @brief 构造函数
		 *  Constructor
		 *
		 *  使用指定的半径、半高和颜色创建胶囊体。
		 *  Creates a capsule with specified radius, half height, and color.
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 *  @param radius 胶囊体半径 / Capsule radius
		 *  @param HalfHeight 胶囊体半高 / Capsule half height
		 *  @param Color 胶囊体颜色 / Capsule color
		 */
		CapsuleDrawable(Graphics& graphics, float radius, float HalfHeight, XMFLOAT3 Color);

		/** @brief 拷贝构造函数
		 *  Copy constructor
		 *
		 *  @param other 要拷贝的胶囊体对象 / The capsule object to copy from
		 */
		CapsuleDrawable(const CapsuleDrawable& other) noexcept;

		/** @brief 更新胶囊体状态
		 *  Update capsule state
		 *
		 *  @param dt 时间增量（秒） / Time delta in seconds
		 *  @param aspect 宽高比 / Aspect ratio
		 */
		void Update(float dt, float aspect) noexcept override;

		/** @brief 创建 ImGui 控制窗口
		 *  Spawn ImGui control window
		 *
		 *  创建用于调整胶囊体参数的 ImGui 窗口。
		 *  Creates an ImGui window for adjusting capsule parameters.
		 *
		 *  @param ImGuiWindowName ImGui 窗口名称 / ImGui window name
		 */
		void SpawnControlWindow(const char* ImGuiWindowName) noexcept;

		/** @brief 获取变换矩阵
		 *  Get transformation matrix
		 *
		 *  返回胶囊体的世界变换矩阵。
		 *  Returns the world transformation matrix of the capsule.
		 *
		 *  @return DirectX 变换矩阵 / DirectX transformation matrix
		 */
		XMMATRIX GetTransformXM() const noexcept override;

	private:
		XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };   ///< 位置 / Position
		XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };   ///< 旋转（欧拉角，度） / Rotation (Euler angles, degrees)
		XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };      ///< 颜色 / Color

		float radius;     ///< 半径 / Radius
		float HalfHeight; ///< 半高 / Half height

		friend class SolidColorConstantBuffer; ///< 纯色常量缓冲区友元类 / Solid color constant buffer friend class
	};
}

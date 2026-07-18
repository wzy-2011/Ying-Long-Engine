/** @file Box.h
 *  @brief 立方体可绘制对象 - Box drawable object
 *
 *  包含使用 PBR 着色的立方体可绘制对象类定义，支持随机旋转和轨道运动。
 *  Contains the box drawable object class definition with PBR shading,
 *  supporting random rotation and orbital motion.
 */
#pragma once
#include <random>
#include "DrawableBase.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Bindable/Texture.h"
#include "../Geometry/Cube.h"
#include "../Light/PointLight.h"

namespace YingLong
{
	/** @brief 立方体可绘制对象类
	 *  Box drawable object class
	 *
	 *  一个带有 PBR 材质的立方体可绘制对象，使用随机数生成器初始化
	 *  随机位置、旋转和角速度。主要用于演示和测试场景。
	 *
	 *  A cube drawable object with PBR material, initialized with random
	 *  position, rotation, and angular velocity using a random number generator.
	 *  Primarily used for demo and test scenes.
	 */
	class BoxDrawable : public DrawableBase<BoxDrawable>
	{
	public:
		/** @brief 构造函数
		 *  Constructor
		 *
		 *  使用随机数生成器创建立方体，初始化静态绑定（首次创建时）
		 *  和实例级绑定。
		 *
		 *  Creates a box using random number generators, initializing static
		 *  bindings (on first creation) and instance-level bindings.
		 *
		 *  @param gfx 图形设备对象引用 / Graphics device object reference
		 *  @param rng 随机数生成器引用 / Random number generator reference
		 *  @param adist 角度分布（用于初始角度） / Angle distribution (for initial angles)
		 *  @param ddist 角速度分布（用于旋转速度） / Angular velocity distribution (for rotation speed)
		 *  @param odist 轨道分布 / Orbit distribution
		 *  @param rdist 半径分布（用于轨道半径） / Radius distribution (for orbit radius)
		 */
		BoxDrawable(Graphics& gfx, std::mt19937& rng,
			std::uniform_real_distribution<float>& adist,
			std::uniform_real_distribution<float>& ddist,
			std::uniform_real_distribution<float>& odist,
			std::uniform_real_distribution<float>& rdist);

		/** @brief 更新立方体状态
		 *  Update box state
		 *
		 *  根据时间增量更新旋转角度和保存宽高比。
		 *  Updates rotation angles based on time delta and saves aspect ratio.
		 *
		 *  @param dt 时间增量（秒） / Time delta in seconds
		 *  @param aspect 宽高比 / Aspect ratio
		 */
		void Update(float dt, float aspect) noexcept;

		/** @brief 获取变换矩阵
		 *  Get transformation matrix
		 *
		 *  返回立方体的世界变换矩阵，包含轨道平移、旋转和位置平移。
		 *  Returns the world transformation matrix of the box, including
		 *  orbital translation, rotation, and position translation.
		 *
		 *  @return DirectX 变换矩阵 / DirectX transformation matrix
		 */
		XMMATRIX GetTransformXM() const noexcept;

	private:
		float r;                ///< 轨道半径 / Orbit radius
		float roll = 0.0f;      ///< 滚转角（绕X轴） / Roll angle (around X axis)
		float pitch = 0.0f;     ///< 俯仰角（绕Y轴） / Pitch angle (around Y axis)
		float yaw = 0.0f;       ///< 偏航角（绕Z轴） / Yaw angle (around Z axis)
		float theta;            ///< 球坐标theta角 / Spherical coordinate theta angle
		float phi;              ///< 球坐标phi角 / Spherical coordinate phi angle
		float chi;              ///< 附加旋转角 / Additional rotation angle

		float droll;            ///< 滚转角速度 / Roll angular velocity
		float dpitch;           ///< 俯仰角速度 / Pitch angular velocity
		float dyaw;             ///< 偏航角速度 / Yaw angular velocity
		float dtheta;           ///< theta角速度 / Theta angular velocity
		float dphi;             ///< phi角速度 / Phi angular velocity
		float dchi;             ///< chi角速度 / Chi angular velocity

		float Aspect;           ///< 宽高比 / Aspect ratio
	};
}

/**
 * @file PhysicsCapsule.h
 * @brief 物理胶囊体（已弃用） / Physics capsule (deprecated)
 *
 * 旧版胶囊体碰撞形状封装。仅创建 PxShape 但不创建 PxRigidDynamic actor，
 * shape 不会进入物理模拟。
 *
 * Legacy capsule collision shape wrapper. Only creates PxShape but does NOT
 * create a PxRigidDynamic actor, so the shape does not participate in
 * physics simulation.
 *
 * @deprecated 已弃用。新代码应使用 ECS 的 RigidbodyComponent + ColliderComponent，
 *             由 PhysicsSystem 自动创建 actor 并加入 PhysicsScene。
 *             保留此类是为了 DX11 后备兼容，不再推荐使用。
 *
 *             Deprecated. New code should use ECS RigidbodyComponent +
 *             ColliderComponent. PhysicsSystem automatically creates the actor
 *             and adds it to PhysicsScene. Kept for DX11 fallback compatibility.
 */
#pragma once
#include <DirectXMath.h>
#include "../Physics.h"
#include "../PhysicsScene/PhysicsScene.h"

namespace YingLong
{
	/**
	 * @brief 物理胶囊体类（已弃用） / Physics capsule class (deprecated)
	 *
	 * 封装 PhysX 胶囊体碰撞形状和材质。
	 * 仅创建 PxShape，不创建刚体，不参与物理模拟。
	 *
	 * Wraps PhysX capsule collision shape and material.
	 * Only creates PxShape, no rigid body, does not participate in simulation.
	 *
	 * @deprecated 使用 ECS ColliderComponent 替代 / Use ECS ColliderComponent instead
	 */
	class PhysicsCapsule
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		PhysicsCapsule();

		/**
		 * @brief 构造函数（半径 + 半高） / Constructor (radius + half height)
		 * @param radius 胶囊体半径 / Capsule radius
		 * @param HalfHeight 胶囊体半高 / Capsule half height
		 */
		PhysicsCapsule(float radius, float HalfHeight);

		/**
		 * @brief 构造函数（几何 + 材质） / Constructor (geometry + material)
		 * @param radius 半径 / Radius
		 * @param HalfHeight 半高 / Half height
		 * @param staticFriction 静摩擦系数 / Static friction coefficient
		 * @param dynamicFriction 动摩擦系数 / Dynamic friction coefficient
		 * @param restitution  restitution / Restitution (bounciness)
		 * @param capsuleDensity 密度 / Density
		 */
		PhysicsCapsule(float radius, float HalfHeight, float staticFriction,
			float dynamicFriction, float restitution, float capsuleDensity);

		/**
		 * @brief 完整构造函数 / Full constructor
		 * @param radius 半径 / Radius
		 * @param HalfHeight 半高 / Half height
		 * @param staticFriction 静摩擦 / Static friction
		 * @param dynamicFriction 动摩擦 / Dynamic friction
		 * @param restitution  restitution / Restitution
		 * @param capsuleDensity 密度 / Density
		 * @param position 位置 / Position
		 * @param rotation 旋转 / Rotation
		 * @param scale 缩放 / Scale
		 */
		PhysicsCapsule(float radius, float HalfHeight, float staticFriction,
			float dynamicFriction, float restitution, float capsuleDensity,
			PxVec3 position, PxVec3 rotation, PxVec3 scale);

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 * @param other 源对象 / Source object
		 */
		PhysicsCapsule(const PhysicsCapsule& other) noexcept;

		/**
		 * @brief 初始化胶囊体对象 / Initialize capsule object
		 *
		 * 创建 PxMaterial 和 PxShape。
		 * Creates PxMaterial and PxShape.
		 */
		void InitializeCapsuleObject();

		/**
		 * @brief 关闭并释放资源 / Shutdown and release resources
		 */
		void Shutdown();

		/**
		 * @brief 析构函数 / Destructor
		 */
		~PhysicsCapsule();

	private:
		float radius;              ///< 胶囊体半径 / Capsule radius
		float HalfHeight;          ///< 胶囊体半高 / Capsule half height
		float staticFriction;      ///< 静摩擦系数 / Static friction coefficient
		float dynamicFriction;     ///< 动摩擦系数 / Dynamic friction coefficient
		float restitution;         ///<  restitution / Restitution (bounciness)
		float capsuleDensity;      ///< 密度 / Density
		PxVec3 position = { 0.0f, 0.0f, 0.0f };  ///< 位置 / Position

		PxMaterial* material = nullptr;           ///< 物理材质 / Physics material
		PxTransform relativePose = { };            ///< 相对位姿 / Relative pose
		PxShape* capsule = nullptr;                ///< 胶囊体形状 / Capsule shape
	};
}

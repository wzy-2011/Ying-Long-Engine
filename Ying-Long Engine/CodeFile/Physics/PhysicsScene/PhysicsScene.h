/**
 * @file PhysicsScene.h
 * @brief 物理场景 / Physics scene
 *
 * 封装 PhysX PxScene，提供物理模拟步进、Actor 管理和射线检测等功能。
 *
 * Wraps PhysX PxScene, providing physics simulation stepping, actor management,
 * and raycasting functionality.
 */
#pragma once
#include <vector>
#include <DirectXMath.h>
#include "../Physics.h"

using namespace DirectX;

namespace YingLong
{
	/**
	 * @brief 射线检测结果 / Raycast hit result
	 *
	 * PhysicsScene::Raycast 的返回结果。如果 Hit 为 false，
	 * 其他字段保持默认（零/空）值。
	 *
	 * Return value of PhysicsScene::Raycast. If Hit is false,
	 * other fields remain at their default (zero/null) values.
	 */
	struct RaycastHit
	{
		bool Hit = false;                                  ///< 是否命中 / Whether hit
		XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };         ///< 命中点位置 / Hit position
		XMFLOAT3 Normal = { 0.0f, 0.0f, 0.0f };           ///< 命中点法线 / Hit normal
		float Distance = 0.0f;                              ///< 命中距离 / Hit distance
		PxRigidActor* Actor = nullptr;                     ///< 命中的刚体 Actor / Hit rigid actor
	};

	/**
	 * @brief 原始碰撞事件 / Raw collision event
	 *
	 * 由 PhysicsCollisionCallback 产生，存储 PxActor 指针。
	 * PhysicsSystem 负责将 PxActor* 解析为 entt::entity。
	 *
	 * Produced by PhysicsCollisionCallback, stores PxActor pointers.
	 * PhysicsSystem resolves PxActor* to entt::entity.
	 */
	struct RawCollisionEvent
	{
		PxActor* ActorA = nullptr;                         ///< 碰撞方 A / Collision party A
		PxActor* ActorB = nullptr;                         ///< 碰撞方 B / Collision party B
		XMFLOAT3 ContactPoint = { 0, 0, 0 };              ///< 接触点 / Contact point
		XMFLOAT3 ContactNormal = { 0, 0, 0 };             ///< 接触法线 / Contact normal
		float ContactDistance = 0.0f;                      ///< 接触距离 / Contact distance
		bool IsTrigger = false;                            ///< 是否为触发器事件 / Whether this is a trigger event
	};

	/**
	 * @brief 碰撞回调类 / Collision callback class
	 *
	 * 实现 PxSimulationEventCallback，收集每帧的碰撞和触发器事件。
	 * PhysicsSystem 在帧末查询并清空事件队列。
	 *
	 * Implements PxSimulationEventCallback, collecting per-frame contact
	 * and trigger events. PhysicsSystem drains the event queue at end of frame.
	 */
	class PhysicsCollisionCallback : public PxSimulationEventCallback
	{
	public:
		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
		void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
		void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {}
		void onWake(PxActor** actors, PxU32 count) override {}
		void onSleep(PxActor** actors, PxU32 count) override {}
		void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}

		/// 取出并清空事件队列 / Drain and clear the event queue
		std::vector<RawCollisionEvent> GetAndClearEvents();

	private:
		std::vector<RawCollisionEvent> m_Events;
	};

	/**
	 * @brief 物理场景类 / Physics scene class
	 *
	 * 封装 PhysX PxScene，提供：
	 *   - 物理模拟步进（Step/Simulate/FetchResults）
	 *   - Actor 添加/移除
	 *   - 射线检测（Raycast）
	 *
	 * Wraps PhysX PxScene, providing:
	 *   - Physics simulation stepping (Step/Simulate/FetchResults)
	 *   - Actor add/remove
	 *   - Raycasting
	 */
	class PhysicsScene
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 */
		PhysicsScene();
		PhysicsScene(const PhysicsScene& other) = default;

		/**
		 * @brief 初始化物理场景 / Initialize physics scene
		 *
		 * 创建 PxScene 和 CPU 调度器。
		 * Creates PxScene and CPU dispatcher.
		 */
		void InistializePhysicsScene();

		/**
		 * @brief 获取底层 PxScene 指针 / Get underlying PxScene pointer
		 * @return PxScene* 物理场景指针 / Physics scene pointer
		 */
		PxScene* GetPhysicsScene() noexcept;

		// === Physics simulation API ===
		// === 物理模拟 API ===

		/**
		 * @brief 步进模拟 / Step simulation
		 *
		 * 将模拟向前推进 dt 秒。内部执行 simulate + fetchResults(true)。
		 * 如果场景未初始化则返回 false。
		 *
		 * Advances the simulation by dt seconds. Internally performs
		 * simulate + fetchResults(true). Returns false if the scene
		 * is not initialized.
		 *
		 * @param dt 增量时间（秒） / Delta time in seconds
		 * @return bool 是否成功 / Whether successful
		 */
		bool Step(float dt);

		/**
		 * @brief 原始 simulate 调用 / Raw simulate call
		 *
		 * 调用者必须确保场景有效。
		 * Caller must ensure the scene is valid.
		 *
		 * @param dt 增量时间（秒） / Delta time in seconds
		 */
		void Simulate(float dt);

		/**
		 * @brief 阻塞等待模拟完成 / Block until simulation completes
		 * @param block 是否阻塞 / Whether to block
		 * @return bool 是否成功 / Whether successful
		 */
		bool FetchResults(bool block = true);

		/**
		 * @brief 添加 Actor 到场景 / Add actor to scene
		 *
		 * 添加 PxRigidDynamic 或 PxRigidStatic 到物理场景。
		 * 调用者保留所有权；移除后需要释放 actor。
		 *
		 * Adds a PxRigidDynamic or PxRigidStatic to the physics scene.
		 * Caller retains ownership; release the actor after removing it.
		 *
		 * @param actor 要添加的 Actor / Actor to add
		 */
		void AddActor(PxActor& actor);

		/**
		 * @brief 从场景移除 Actor / Remove actor from scene
		 * @param actor 要移除的 Actor / Actor to remove
		 */
		void RemoveActor(PxActor& actor);

		/**
		 * @brief 检查底层 PxScene 是否已创建 / Check if underlying PxScene has been created
		 * @return bool 是否已创建 / Whether created
		 */
		bool IsValid() const noexcept;

		// === Query API ===
		// === 查询 API ===

		/**
		 * @brief 射线检测 / Raycast
		 *
		 * 从 origin 出发，沿 unitDir 方向（内部会归一化）发射射线，
		 * 最大距离 maxDist 米。返回命中信息。如果场景无效或未命中，
		 * RaycastHit::Hit 为 false。
		 *
		 * Casts a ray from `origin` in `unitDir` direction (normalized internally)
		 * up to `maxDist` meters. Returns hit info. If the scene is invalid
		 * or nothing was hit, RaycastHit::Hit is false.
		 *
		 * @param origin 射线起点 / Ray origin
		 * @param unitDir 射线方向（会被归一化） / Ray direction (will be normalized)
		 * @param maxDist 最大距离 / Maximum distance
		 * @return RaycastHit 命中结果 / Hit result
		 */
		RaycastHit Raycast(const XMFLOAT3& origin, const XMFLOAT3& unitDir, float maxDist);

		/**
		 * @brief 获取碰撞回调对象 / Get collision callback object
		 *
		 * PhysicsSystem 通过此方法访问碰撞事件队列。
		 * PhysicsSystem accesses the collision event queue through this method.
		 *
		 * @return PhysicsCollisionCallback* 碰撞回调指针 / Collision callback pointer
		 */
		PhysicsCollisionCallback* GetCollisionCallback() noexcept { return &m_CollisionCallback; }

		/**
		 * @brief 析构函数 / Destructor
		 */
		~PhysicsScene();

	private:
		PxScene* PhysicsSceneObject = nullptr;         ///< PhysX 场景对象 / PhysX scene object
		PxDefaultCpuDispatcher* pCpuDispatcher = nullptr;  ///< CPU 调度器 / CPU dispatcher
		PhysicsCollisionCallback m_CollisionCallback;       ///< 碰撞回调 / Collision callback
	};
}

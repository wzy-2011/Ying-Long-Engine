/**
 * @file System.h
 * @brief ECS 系统基类 / ECS system base class
 *
 * System 是所有 ECS 系统的抽象基类。系统处理具有特定组件组合的实体，
 * 实现游戏逻辑、物理、渲染等功能。
 *
 * 提供两种更新接口：
 *   - Update(Entity&, float): 单实体更新（遗留接口）
 *   - UpdateScene(Scene&, float): 场景级批量更新（推荐）
 *
 * System is the abstract base class for all ECS systems. Systems process
 * entities with specific component combinations, implementing game logic,
 * physics, rendering, etc.
 *
 * Provides two update interfaces:
 *   - Update(Entity&, float): single-entity update (legacy interface)
 *   - UpdateScene(Scene&, float): scene-level batch update (recommended)
 */
#pragma once
#include <chrono>

namespace YingLong
{
	class Scene;
	class Entity;

	/**
	 * @brief ECS 系统基类 / ECS system base class
	 *
	 * 所有自定义系统都应继承此类并重写相应的虚函数。
	 * 推荐重写 UpdateScene 进行场景级批量处理以获得最佳性能。
	 *
	 * All custom systems should inherit from this class and override
	 * the appropriate virtual functions.
	 * It's recommended to override UpdateScene for scene-level batch
	 * processing for best performance.
	 */
	class System
	{
	public:
		/**
		 * @brief 虚析构函数 / Virtual destructor
		 */
		virtual ~System() = default;

		/**
		 * @brief 单实体更新 / Single-entity update
		 *
		 * 遗留接口，对单个实体进行更新。
		 * 新系统应优先使用 UpdateScene 进行批量处理。
		 *
		 * Legacy interface, updates a single entity.
		 * New systems should prefer UpdateScene for batch processing.
		 *
		 * @param entity 实体 / Entity
		 * @param DeltaTime 增量时间 / Delta time
		 */
		virtual void Update(Entity& entity, float DeltaTime) = 0;

		/**
		 * @brief 场景级更新 / Scene-level update
		 *
		 * 对整个场景进行更新，可遍历 registry 中所有相关实体。
		 * 默认实现为空，子类按需重写。
		 *
		 * Updates the entire scene, can iterate all relevant entities in registry.
		 * Default implementation is empty; subclasses override as needed.
		 *
		 * @param scene 场景引用 / Scene reference
		 * @param DeltaTime 增量时间 / Delta time
		 */
		virtual void UpdateScene(Scene& scene, float DeltaTime);

		/**
		 * @brief 系统初始化 / System initialization
		 *
		 * 在系统被添加到场景时调用。默认实现为空。
		 * Called when the system is added to a scene. Default implementation is empty.
		 */
		virtual void Initialize();

		/**
		 * @brief 系统关闭 / System shutdown
		 *
		 * 在系统从场景移除或场景卸载时调用，用于清理资源。
		 * 默认实现为空。
		 * Called when the system is removed from the scene or the scene
		 * is unloaded, used for resource cleanup. Default implementation is empty.
		 */
		virtual void ShutDown();

	private:

	};
}

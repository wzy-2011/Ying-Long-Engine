/**
 * @file Entity.h
 * @brief ECS 实体管理类 / ECS entity management class
 *
 * Entity 类封装了 entt::registry，提供实体创建、销毁和组件操作的接口。
 * 所有实体共享同一个静态注册表（RegistryObject），实现全局实体管理。
 *
 * Entity class wraps entt::registry, providing interfaces for entity creation,
 * destruction, and component operations. All entities share the same static
 * registry (RegistryObject) for global entity management.
 */
#pragma once
#include <iostream>
#include <DirectXMath.h>
#include <../../entt-master/include/entt.hpp>
#include "../Components/Components.h"

using namespace DirectX;

namespace YingLong
{
	/**
	 * @brief ECS 实体管理器 / ECS entity manager
	 *
	 * 封装 entt::registry，提供实体和组件的增删查改操作。
	 * 采用静态注册表模式，所有 Entity 实例共享同一个注册表。
	 *
	 * Wraps entt::registry to provide CRUD operations for entities and components.
	 * Uses static registry pattern — all Entity instances share the same registry.
	 */
	class Entity
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 */
		Entity();

		/**
		 * @brief 析构函数 / Destructor
		 */
		~Entity();

		/**
		 * @brief 创建一个新实体 / Create a new entity
		 *
		 * 自动为新实体添加 TagComponent 用于名称标识。
		 * Automatically adds TagComponent to the new entity for name identification.
		 *
		 * @param tag 实体标签名称 / Entity tag name
		 * @return entt::entity 创建的实体句柄 / Created entity handle
		 */
		entt::entity CreateEntity(const std::string& tag = "Unnamed Entity");

		/**
		 * @brief 更新实体逻辑 / Update entity logic
		 * @param TimeStep 时间步长 / Time step
		 * @param index 实体索引（预留） / Entity index (reserved)
		 */
		void Update(float TimeStep, int index = 0);

		/**
		 * @brief 检查实体是否有效 / Check if entity is valid
		 * @param entity 实体句柄 / Entity handle
		 * @return bool 实体是否存在且有效 / Whether the entity exists and is valid
		 */
		bool IsEntityValid(entt::entity entity) noexcept;

		/**
		 * @brief 销毁实体 / Destroy an entity
		 *
		 * 从注册表中移除实体及其所有组件。
		 * Removes the entity and all its components from the registry.
		 *
		 * @param entity 实体句柄 / Entity handle
		 */
		void DestroyEntity(entt::entity entity);

		/**
		 * @brief 向实体添加组件 / Add a component to an entity
		 *
		 * 如果组件已存在，先移除再添加（相当于重置）。
		 * 如果实体无效，输出错误信息但仍尝试操作。
		 *
		 * If the component already exists, remove it first then add (equivalent to reset).
		 * If the entity is invalid, prints error but still attempts the operation.
		 *
		 * @tparam T 组件类型 / Component type
		 * @tparam Args 构造参数类型列表 / Constructor argument type list
		 * @param entity 实体句柄 / Entity handle
		 * @param args 组件构造参数 / Component constructor arguments
		 * @return T& 新添加组件的引用 / Reference to the newly added component
		 */
		template<typename T, typename... Args>
		T& AddComponent(entt::entity entity, Args&&... args)
		{
			if (!IsEntityValid(entity))
			{
				std::cout << "Cannot add component to invalid entity!";
			}

			if (this->HasComponent<T>(entity))
			{
				this->RemoveComponent<T>(entity);
				std::cout << "The component is exist.The engine remove it and add it again!";
			}

			return this->RegistryObject.emplace<T>(entity, std::forward<Args>(args)...);
		}

		/**
		 * @brief 从实体移除组件 / Remove a component from an entity
		 *
		 * 如果实体无效或组件不存在，输出错误信息。
		 * Prints error if entity is invalid or component doesn't exist.
		 *
		 * @tparam T 组件类型 / Component type
		 * @param entity 实体句柄 / Entity handle
		 */
		template<typename T>
		void RemoveComponent(entt::entity entity)
		{
			if (!this->IsEntityValid(entity) || !this->HasComponent<T>(entity))
			{
				std::cout << "Cannot remove component from invalid entity or component not exists!";
			}

			this->RegistryObject.remove<T>(entity);
		}

		/**
		 * @brief 检查实体是否拥有指定组件 / Check if entity has the specified component
		 * @tparam T 组件类型 / Component type
		 * @param entity 实体句柄 / Entity handle
		 * @return bool 实体有效且拥有该组件时返回 true / True if entity is valid and has the component
		 */
		template<typename T>
		bool HasComponent(entt::entity entity)
		{
			return this->IsEntityValid(entity) && this->RegistryObject.all_of<T>(entity);
		}

		/**
		 * @brief 获取注册表引用 / Get registry reference
		 *
		 * 用于系统级遍历和批量操作，允许直接访问 entt::registry。
		 * Used for system-level iteration and batch operations, allowing direct
		 * access to entt::registry.
		 *
		 * @return entt::registry& 注册表引用 / Registry reference
		 */
		entt::registry& GetRegistry() noexcept;

	private:
		/**
		 * @brief 静态 entt 注册表 / Static entt registry
		 *
		 * 所有 Entity 实例共享同一个注册表，实现全局实体管理。
		 * All Entity instances share the same registry for global entity management.
		 */
		static entt::registry RegistryObject;
	};
}

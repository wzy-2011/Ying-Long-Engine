/**
 * @file Scene.h
 * @brief ECS 场景类 / ECS scene class
 *
 * Scene 是 ECS 架构的顶层容器，管理：
 *   - entt::registry：所有实体和组件
 *   - System 列表：所有逻辑系统
 *   - 主相机：场景观察视角
 *   - 光源列表：点光源和聚光灯
 *   - 模型列表：预加载的模型资源
 *   - 物理场景指针：绑定 PhysX 物理场景
 *
 * 场景生命周期：Unloaded → Loading → Active ↔ Paused → Unloaded
 *
 * Scene is the top-level container of the ECS architecture, managing:
 *   - entt::registry: all entities and components
 *   - System list: all logic systems
 *   - Main camera: scene observation viewpoint
 *   - Light list: point lights and spot lights
 *   - Model list: preloaded model resources
 *   - Physics scene pointer: bound PhysX physics scene
 *
 * Scene lifecycle: Unloaded → Loading → Active ↔ Paused → Unloaded
 */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../entt-master/include/entt.hpp"
#include "../Entity/Entity.h"
#include "../../Graphics/Camera/Camera.h"
#include "../../Graphics/Light/LightManager.h"
#include "../System/System.h"
#include "../../Physics/PhysicsScene/PhysicsScene.h"
#include "../../Graphics/Drawable/Model.h"

namespace YingLong
{
	/**
	 * @brief ECS 场景类 / ECS scene class
	 *
	 * 场景是实体、组件、系统和资源的集合。每个场景拥有独立的
	 * entt::registry，可以加载/保存/激活/暂停。
	 *
	 * A scene is a collection of entities, components, systems, and resources.
	 * Each scene has its own entt::registry and can be loaded/saved/activated/paused.
	 */
	class Scene
	{
	public:
		/**
		 * @brief 场景状态枚举 / Scene state enumeration
		 */
		enum class State { Loading, Active, Paused, Unloaded };

		/**
		 * @brief 默认构造函数 / Default constructor
		 *
		 * 创建名为 "Untitled Scene" 的空场景。
		 * Creates an empty scene named "Untitled Scene".
		 */
		Scene();

		/**
		 * @brief 命名构造函数 / Named constructor
		 * @param name 场景名称 / Scene name
		 */
		Scene(const std::string& name);

		/**
		 * @brief 析构函数 / Destructor
		 *
		 * 自动调用 Unload() 释放所有资源。
		 * Automatically calls Unload() to release all resources.
		 */
		~Scene();

		// --- 实体管理 / Entity management ---

		/**
		 * @brief 创建实体 / Create an entity
		 * @param name 实体名称 / Entity name
		 * @return entt::entity 实体句柄 / Entity handle
		 */
		entt::entity CreateEntity(const std::string& name = "Entity");

		/**
		 * @brief 按名称查找实体 / Find entity by name
		 * @param name 实体名称 / Entity name
		 * @return entt::entity 找到的实体，未找到返回 entt::null / Found entity, or entt::null if not found
		 */
		entt::entity FindEntityByName(const std::string& name);

		/**
		 * @brief 获取所有实体列表 / Get all entities list
		 * @return std::vector<entt::entity> 所有拥有 TagComponent 的实体 / All entities with TagComponent
		 */
		std::vector<entt::entity> GetAllEntities();

		/**
		 * @brief 销毁实体 / Destroy an entity
		 * @param entity 实体句柄 / Entity handle
		 */
		void DestroyEntity(entt::entity entity);

		/**
		 * @brief 清空场景 / Clear the scene
		 *
		 * 销毁所有实体、光源和模型，但保留系统。
		 * Destroys all entities, lights, and models, but keeps systems.
		 */
		void Clear();

		/**
		 * @brief 检查实体是否存在 / Check if entity exists
		 * @param entity 实体句柄 / Entity handle
		 * @return bool 实体是否有效 / Whether the entity is valid
		 */
		bool HasEntity(entt::entity entity) const;

		// --- 组件操作（通过场景的 registry） / Component operations (via scene registry) ---

		/**
		 * @brief 向实体添加组件 / Add a component to an entity
		 * @tparam T 组件类型 / Component type
		 * @tparam Args 构造参数类型列表 / Constructor argument type list
		 * @param entity 实体句柄 / Entity handle
		 * @param args 构造参数 / Constructor arguments
		 * @return T& 组件引用 / Component reference
		 */
		template<typename T, typename... Args>
		T& AddComponent(entt::entity entity, Args&&... args)
		{
			return Registry.emplace<T>(entity, std::forward<Args>(args)...);
		}

		/**
		 * @brief 获取实体的组件指针 / Get component pointer of an entity
		 * @tparam T 组件类型 / Component type
		 * @param entity 实体句柄 / Entity handle
		 * @return T* 组件指针，不存在返回 nullptr / Component pointer, or nullptr if not present
		 */
		template<typename T>
		T* GetComponent(entt::entity entity)
		{
			return Registry.try_get<T>(entity);
		}

		/**
		 * @brief 检查实体是否拥有组件 / Check if entity has a component
		 * @tparam T 组件类型 / Component type
		 * @param entity 实体句柄 / Entity handle
		 * @return bool 是否拥有该组件 / Whether the entity has the component
		 */
		template<typename T>
		bool HasComponent(entt::entity entity) const
		{
			return Registry.all_of<T>(entity);
		}

		/**
		 * @brief 移除实体的组件 / Remove a component from an entity
		 * @tparam T 组件类型 / Component type
		 * @param entity 实体句柄 / Entity handle
		 */
		template<typename T>
		void RemoveComponent(entt::entity entity)
		{
			Registry.remove<T>(entity);
		}

		// --- 系统管理 / System management ---

		/**
		 * @brief 添加系统 / Add a system
		 *
		 * 创建系统实例并调用 Initialize()，添加到系统列表末尾。
		 * Creates a system instance, calls Initialize(), and appends to the system list.
		 *
		 * @tparam T 系统类型 / System type
		 * @tparam Args 构造参数类型列表 / Constructor argument type list
		 * @param args 构造参数 / Constructor arguments
		 * @return T& 系统引用 / System reference
		 */
		template<typename T, typename... Args>
		T& AddSystem(Args&&... args)
		{
			auto system = std::make_unique<T>(std::forward<Args>(args)...);
			system->Initialize();
			auto& ref = *system;
			Systems.push_back(std::move(system));
			return ref;
		}

		/**
		 * @brief 获取指定类型的系统指针 / Get system pointer of specified type
		 *
		 * 通过 dynamic_cast 遍历系统列表查找匹配类型。
		 * Walks the system list and finds a matching type via dynamic_cast.
		 *
		 * @tparam T 系统类型 / System type
		 * @return T* 系统指针，未找到返回 nullptr / System pointer, or nullptr if not found
		 */
		template<typename T>
		T* GetSystem()
		{
			for (auto& sys : Systems)
			{
				if (auto* p = dynamic_cast<T*>(sys.get()))
				{
					return p;
				}
			}
			return nullptr;
		}

		/**
		 * @brief 移除指定类型的系统 / Remove system of specified type
		 *
		 * 找到匹配类型的系统后调用 ShutDown() 然后从列表中移除。
		 * Calls ShutDown() on the matching system then removes it from the list.
		 *
		 * @tparam T 系统类型 / System type
		 */
		template<typename T>
		void RemoveSystem()
		{
			auto it = std::remove_if(Systems.begin(), Systems.end(),
				[](const std::unique_ptr<System>& sys) {
					return dynamic_cast<T*>(sys.get()) != nullptr;
				});
			if (it != Systems.end())
			{
				(*it)->ShutDown();
				Systems.erase(it);
			}
		}

		// --- 场景生命周期 / Scene lifecycle ---

		/**
		 * @brief 从文件加载场景 / Load scene from file
		 * @param path YAML 文件路径 / YAML file path
		 */
		void Load(const std::string& path);

		/**
		 * @brief 保存场景到文件 / Save scene to file
		 * @param path YAML 文件路径 / YAML file path
		 */
		void Save(const std::string& path) const;

		/**
		 * @brief 激活场景 / Activate the scene
		 *
		 * 从 Unloaded 或 Paused 状态切换到 Active，并初始化所有系统。
		 * Switches from Unloaded or Paused state to Active and initializes all systems.
		 */
		void Activate();

		/**
		 * @brief 暂停场景 / Pause the scene
		 *
		 * 从 Active 状态切换到 Paused，Update 和 Render 将不再执行。
		 * Switches from Active state to Paused; Update and Render will no longer execute.
		 */
		void Pause();

		/**
		 * @brief 卸载场景 / Unload the scene
		 *
		 * 关闭所有系统，清空所有实体、光源、模型，状态设为 Unloaded。
		 * Shuts down all systems, clears all entities, lights, models, and sets state to Unloaded.
		 */
		void Unload();

		/**
		 * @brief 更新场景 / Update the scene
		 *
		 * 按添加顺序调用所有系统的 UpdateScene()。
		 * Calls UpdateScene() on all systems in insertion order.
		 *
		 * @param deltaTime 增量时间 / Delta time
		 */
		void Update(float deltaTime);

		/**
		 * @brief 渲染场景 / Render the scene
		 *
		 * 设置相机、提交光源、更新光照、渲染模型和 ECS 实体。
		 * Sets camera, submits lights, updates lighting, renders models and ECS entities.
		 *
		 * @param graphics 图形设备 / Graphics device
		 */
		void Render(class Graphics& graphics);

		// --- 相机管理 / Camera management ---

		/**
		 * @brief 设置主相机 / Set main camera
		 * @param camera 相机智能指针 / Camera unique pointer
		 */
		void SetMainCamera(std::unique_ptr<Camera> camera);

		/**
		 * @brief 获取主相机指针 / Get main camera pointer
		 * @return Camera* 主相机指针 / Main camera pointer
		 */
		Camera* GetMainCamera() const;

		// --- 光源管理 / Light management ---

		/**
		 * @brief 添加点光源 / Add a point light
		 * @param light 点光源 / Point light
		 */
		void AddPointLight(const PointLight& light);

		/**
		 * @brief 添加聚光灯 / Add a spot light
		 * @param light 聚光灯 / Spot light
		 */
		void AddSpotLight(const SpotLight& light);

		/**
		 * @brief 清空所有光源 / Clear all lights
		 */
		void ClearLights();

		// --- 模型管理 / Model management ---

		/**
		 * @brief 添加模型 / Add a model
		 * @param model 模型智能指针 / Model unique pointer
		 */
		void AddModel(std::unique_ptr<Model> model);

		/**
		 * @brief 获取所有模型 / Get all models
		 * @return const std::vector<std::unique_ptr<Model>>& 模型列表 / Model list
		 */
		const std::vector<std::unique_ptr<Model>>& GetModels() const;

		/**
		 * @brief 设置物理场景指针 / Set physics scene pointer
		 *
		 * PhysicsScene 实例由 Application 持有并通过此接口绑定到场景。
		 * PhysicsSystem 每次 Update 时从此指针读取。
		 *
		 * The PhysicsScene instance is owned by Application and bound to the scene
		 * via this interface. PhysicsSystem reads from this pointer each Update.
		 *
		 * @param ps 物理场景指针 / Physics scene pointer
		 */
		void SetPhysicsScene(PhysicsScene* ps) noexcept { PhysicsScenePtr = ps; }

		/**
		 * @brief 获取物理场景指针 / Get physics scene pointer
		 * @return PhysicsScene* 物理场景指针 / Physics scene pointer
		 */
		PhysicsScene* GetPhysicsScene() const noexcept { return PhysicsScenePtr; }

		// --- 状态查询 / State queries ---

		/**
		 * @brief 获取当前场景状态 / Get current scene state
		 * @return State 场景状态 / Scene state
		 */
		State GetState() const { return CurrentState; }

		/**
		 * @brief 获取场景名称 / Get scene name
		 * @return const std::string& 场景名称 / Scene name
		 */
		const std::string& GetName() const { return Name; }

		/**
		 * @brief 获取注册表引用 / Get registry reference
		 * @return entt::registry& 注册表引用 / Registry reference
		 */
		entt::registry& GetRegistry() { return Registry; }

		/**
		 * @brief 获取 const 注册表引用 / Get const registry reference
		 * @return const entt::registry& 注册表引用 / Registry reference
		 */
		const entt::registry& GetRegistry() const { return Registry; }

	private:
		std::string Name;                                      ///< 场景名称 / Scene name
		entt::registry Registry;                               ///< ECS 注册表 / ECS registry
		std::vector<std::unique_ptr<System>> Systems;          ///< 系统列表 / System list
		std::unique_ptr<Camera> MainCamera;                    ///< 主相机 / Main camera
		std::vector<PointLight> PointLights;                   ///< 点光源列表 / Point light list
		std::vector<SpotLight> SpotLights;                     ///< 聚光灯列表 / Spot light list
		std::vector<std::unique_ptr<Model>> Models;            ///< 模型列表 / Model list
		State CurrentState = State::Unloaded;                  ///< 当前状态 / Current state
		float DeltaTime = 0.0f;                                ///< 当前帧增量时间 / Current frame delta time
		float TimeScale = 1.0f;                                ///< 时间缩放 / Time scale

		std::unordered_map<std::string, entt::entity> EntityNameMap;  ///< 名称→实体映射 / Name→entity map

		/**
		 * @brief 物理场景非拥有指针 / Non-owning physics scene pointer
		 *
		 * 由 Application 持有 PhysicsScene 实例并赋值。
		 * Application owns the PhysicsScene instance and assigns it here.
		 */
		PhysicsScene* PhysicsScenePtr = nullptr;
	};
}

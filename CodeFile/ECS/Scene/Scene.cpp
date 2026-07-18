/**
 * @file Scene.cpp
 * @brief ECS 场景类实现 / ECS scene class implementation
 *
 * 实现 Scene 类的所有非模板成员函数，包括实体管理、
 * 场景加载/保存、生命周期管理、渲染等功能。
 *
 * Implements all non-template member functions of the Scene class,
 * including entity management, scene load/save, lifecycle management,
 * rendering, etc.
 */
#include "Scene.h"
#include "../../Graphics/Graphics.h"
#include "../System/MeshRendererSystem.h"
#include "../../../yaml-cpp/include/yaml-cpp/yaml.h"
#include <fstream>
#include <algorithm>

namespace YingLong
{
	Scene::Scene() : Name("Untitled Scene")
	{
	}

	Scene::Scene(const std::string& name) : Name(name)
	{
	}

	Scene::~Scene()
	{
		// 析构时自动卸载场景，确保资源正确释放
		// Automatically unload scene on destruction to ensure proper resource release
		Unload();
	}

	// --- 实体管理 / Entity management ---

	entt::entity Scene::CreateEntity(const std::string& name)
	{
		// 在注册表中创建新实体
		// Create new entity in registry
		entt::entity entity = Registry.create();

		if (!name.empty())
		{
			// 添加 TagComponent 并注册到名称映射表
			// Add TagComponent and register to name map
			AddComponent<TagComponent>(entity, name);
			EntityNameMap[name] = entity;
		}

		return entity;
	}

	entt::entity Scene::FindEntityByName(const std::string& name)
	{
		// 在名称映射表中查找，并验证实体仍然有效
		// Look up in name map and verify entity is still valid
		auto it = EntityNameMap.find(name);
		if (it != EntityNameMap.end() && Registry.valid(it->second))
		{
			return it->second;
		}
		return entt::null;
	}

	std::vector<entt::entity> Scene::GetAllEntities()
	{
		// 收集所有拥有 TagComponent 的实体（即有名实体）
		// Collect all entities with TagComponent (i.e. named entities)
		std::vector<entt::entity> entities;
		auto view = Registry.view<TagComponent>();
		for (auto entity : view) {
			entities.push_back(entity);
		}
		return entities;
	}

	void Scene::DestroyEntity(entt::entity entity)
	{
		if (Registry.valid(entity))
		{
			// 如果实体有名称，先从名称映射表中移除
			// If entity has a name, remove from name map first
			if (auto* tag = Registry.try_get<TagComponent>(entity))
			{
				EntityNameMap.erase(tag->Name);
			}
			// 销毁实体（同时移除所有组件）
			// Destroy entity (removes all components too)
			Registry.destroy(entity);
		}
	}

	void Scene::Clear()
	{
		// 清空注册表、名称映射、光源和模型
		// Clear registry, name map, lights, and models
		Registry.clear();
		EntityNameMap.clear();
		PointLights.clear();
		SpotLights.clear();
		Models.clear();
	}

	bool Scene::HasEntity(entt::entity entity) const
	{
		return Registry.valid(entity);
	}

	// --- 场景生命周期 / Scene lifecycle ---

	void Scene::Load(const std::string& path)
	{
		CurrentState = State::Loading;

		try
		{
			// 打开并读取 YAML 文件
			// Open and read YAML file
			std::ifstream file(path);
			if (!file.is_open())
			{
				std::cerr << "Failed to open scene file: " << path << std::endl;
				CurrentState = State::Unloaded;
				return;
			}

			std::stringstream ss;
			ss << file.rdbuf();
			auto data = YAML::Load(ss.str());

			// 加载场景名称
			// Load scene name
			if (data["Name"])
			{
				Name = data["Name"].as<std::string>();
			}

			// 加载实体列表
			// Load entity list
			if (data["Entities"])
			{
				for (const auto& entityData : data["Entities"])
				{
					// 创建实体并设置名称
					// Create entity and set name
					std::string name = entityData["Name"].as<std::string>("Entity");
					entt::entity entity = CreateEntity(name);

					// 加载 TransformComponent
					// Load TransformComponent
					if (entityData["Transform"])
					{
						auto& transform = AddComponent<TransformComponent>(entity);
						auto& t = entityData["Transform"];
						if (t["Position"])
						{
							transform.Position = t["Position"].as<XMFLOAT3>();
						}
						if (t["Rotation"])
						{
							transform.Rotation = t["Rotation"].as<XMFLOAT3>();
						}
						if (t["Scale"])
						{
							transform.Scale = t["Scale"].as<XMFLOAT3>();
						}
					}

					// 加载 RigidbodyComponent
					// Load RigidbodyComponent
					if (entityData["Rigidbody"])
					{
						auto& rb = AddComponent<RigidbodyComponent>(entity);
						auto& r = entityData["Rigidbody"];
						rb.Mass = r["Mass"].as<float>(1.0f);
						rb.UseGravity = r["UseGravity"].as<bool>(true);
						rb.IsKinematic = r["IsKinematic"].as<bool>(false);
					}

					// 加载 ColliderComponent
					// Load ColliderComponent
					if (entityData["Collider"])
					{
						auto& col = AddComponent<ColliderComponent>(entity);
						auto& c = entityData["Collider"];
						auto shapeStr = c["Shape"].as<std::string>("Box");
						if (shapeStr == "Sphere") col.Shape = ColliderShape::Sphere;
						else if (shapeStr == "Capsule") col.Shape = ColliderShape::Capsule;
						else col.Shape = ColliderShape::Box;

						col.HalfExtents = c["HalfExtents"].as<XMFLOAT3>(XMFLOAT3{ 0.5f, 0.5f, 0.5f });
						col.Radius = c["Radius"].as<float>(0.5f);
						col.HalfHeight = c["HalfHeight"].as<float>(0.5f);
						col.StaticFriction = c["StaticFriction"].as<float>(0.5f);
						col.DynamicFriction = c["DynamicFriction"].as<float>(0.5f);
						col.Restitution = c["Restitution"].as<float>(0.3f);
					}

					// 加载 MeshComponent
					// Load MeshComponent
					if (entityData["Mesh"])
					{
						auto& mesh = AddComponent<MeshComponent>(entity);
						auto& m = entityData["Mesh"];
						mesh.ModelPath = m["ModelPath"].as<std::string>("");
						mesh.TintColor = m["TintColor"].as<XMFLOAT4>(XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f });
						mesh.IsVisible = m["IsVisible"].as<bool>(true);
					}
				}
			}

			// 加载相机（目前仅读取位置，未完全实现）
			// Load camera (currently only reads position, not fully implemented)
			if (data["Camera"])
			{
				YAML::Node cameraData = data["Camera"];
				if (MainCamera)
				{
					if (cameraData["Position"])
					{
						// 需要修改 Camera 类以支持直接设置 Position
						// Need to modify Camera class to support direct Position setting
					}
				}
			}

			// 加载成功，切换到激活状态
			// Load succeeded, switch to active state
			CurrentState = State::Active;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error loading scene: " << e.what() << std::endl;
			CurrentState = State::Unloaded;
		}
	}

	void Scene::Save(const std::string& path) const
	{
		// 使用 YAML::Emitter 构建输出
		// Use YAML::Emitter to build output
		YAML::Emitter out;
		out << YAML::BeginMap;

		// 保存场景名称
		// Save scene name
		out << YAML::Key << "Name" << YAML::Value << Name;

		// 保存实体列表
		// Save entity list
		out << YAML::Key << "Entities" << YAML::BeginSeq;
		auto view = Registry.view<TagComponent>();
		for (auto entity : view) {
			out << YAML::BeginMap;

			// 保存实体名称
			// Save entity name
			if (auto* tag = Registry.try_get<TagComponent>(entity))
			{
				out << YAML::Key << "Name" << YAML::Value << tag->Name;
			}
			else
			{
				out << YAML::Key << "Name" << YAML::Value << "Entity";
			}

			// 保存 TransformComponent
			// Save TransformComponent
			if (auto* transform = Registry.try_get<TransformComponent>(entity))
			{
				out << YAML::Key << "Transform" << YAML::BeginMap;
				out << YAML::Key << "Position" << YAML::Value << transform->Position;
				out << YAML::Key << "Rotation" << YAML::Value << transform->Rotation;
				out << YAML::Key << "Scale" << YAML::Value << transform->Scale;
				out << YAML::EndMap;
			}

			// 保存 RigidbodyComponent
			// Save RigidbodyComponent
			if (auto* rb = Registry.try_get<RigidbodyComponent>(entity))
			{
				out << YAML::Key << "Rigidbody" << YAML::BeginMap;
				out << YAML::Key << "Mass" << YAML::Value << rb->Mass;
				out << YAML::Key << "UseGravity" << YAML::Value << rb->UseGravity;
				out << YAML::Key << "IsKinematic" << YAML::Value << rb->IsKinematic;
				out << YAML::EndMap;
			}

			// 保存 ColliderComponent
			// Save ColliderComponent
			if (auto* col = Registry.try_get<ColliderComponent>(entity))
			{
				out << YAML::Key << "Collider" << YAML::BeginMap;
				std::string shapeStr = "Box";
				if (col->Shape == ColliderShape::Sphere) shapeStr = "Sphere";
				else if (col->Shape == ColliderShape::Capsule) shapeStr = "Capsule";
				out << YAML::Key << "Shape" << YAML::Value << shapeStr;
				out << YAML::Key << "HalfExtents" << YAML::Value << col->HalfExtents;
				out << YAML::Key << "Radius" << YAML::Value << col->Radius;
				out << YAML::Key << "HalfHeight" << YAML::Value << col->HalfHeight;
				out << YAML::Key << "StaticFriction" << YAML::Value << col->StaticFriction;
				out << YAML::Key << "DynamicFriction" << YAML::Value << col->DynamicFriction;
				out << YAML::Key << "Restitution" << YAML::Value << col->Restitution;
				out << YAML::EndMap;
			}

			// 保存 MeshComponent
			// Save MeshComponent
			if (auto* mesh = Registry.try_get<MeshComponent>(entity))
			{
				out << YAML::Key << "Mesh" << YAML::BeginMap;
				out << YAML::Key << "ModelPath" << YAML::Value << mesh->ModelPath;
				out << YAML::Key << "TintColor" << YAML::Value << mesh->TintColor;
				out << YAML::Key << "IsVisible" << YAML::Value << mesh->IsVisible;
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		// 保存相机位置
		// Save camera position
		if (MainCamera)
		{
			out << YAML::Key << "Camera" << YAML::BeginMap;
			out << YAML::Key << "Position" << YAML::Value << MainCamera->GetPosition();
			out << YAML::EndMap;
		}

		out << YAML::EndMap;

		// 写入文件
		// Write to file
		std::ofstream file(path);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open scene file for writing: " + path);
		}
		file << out.c_str();
	}

	void Scene::Activate()
	{
		// 只能从 Unloaded 或 Paused 状态激活
		// Can only activate from Unloaded or Paused state
		if (CurrentState == State::Unloaded || CurrentState == State::Paused)
		{
			CurrentState = State::Active;
			// 初始化所有系统
			// Initialize all systems
			for (auto& system : Systems)
			{
				system->Initialize();
			}
		}
	}

	void Scene::Pause()
	{
		// 只能从 Active 状态暂停
		// Can only pause from Active state
		if (CurrentState == State::Active)
		{
			CurrentState = State::Paused;
		}
	}

	void Scene::Unload()
	{
		// 关闭所有系统
		// Shut down all systems
		for (auto& system : Systems)
		{
			system->ShutDown();
		}
		Systems.clear();
		// 清空所有实体和资源
		// Clear all entities and resources
		Clear();
		CurrentState = State::Unloaded;
	}

	void Scene::Update(float deltaTime)
	{
		if (CurrentState != State::Active)
			return;

		// 应用时间缩放
		// Apply time scale
		DeltaTime = deltaTime * TimeScale;

		// 按添加顺序更新所有系统
		// Update all systems in insertion order
		for (auto& system : Systems)
		{
			system->UpdateScene(*this, DeltaTime);
		}
	}

	void Scene::Render(Graphics& graphics)
	{
		if (CurrentState != State::Active)
			return;

		// 设置主相机到图形设备
		// Set main camera to graphics device
		if (MainCamera)
		{
			graphics.SetCamera(*MainCamera);
		}

		// 提交所有光源到 LightManager
		// Submit all lights to LightManager
		for (const auto& light : PointLights)
		{
			LightManager::SubmitPointLight(light);
		}
		for (const auto& light : SpotLights)
		{
			LightManager::SubmitSpotLight(light);
		}

		// 更新光照状态
		// Update lighting state
		LightManager::Update(graphics);

		// 渲染所有独立模型
		// Render all standalone models
		for (const auto& model : Models)
		{
			model->Draw(graphics);
		}

		// 通过 MeshRendererSystem 渲染 ECS 实体
		// Render ECS entities via MeshRendererSystem
		if (auto* meshRenderer = GetSystem<MeshRendererSystem>())
		{
			meshRenderer->RenderDX11(*this, graphics);
		}
	}

	// --- 相机管理 / Camera management ---

	void Scene::SetMainCamera(std::unique_ptr<Camera> camera)
	{
		MainCamera = std::move(camera);
	}

	Camera* Scene::GetMainCamera() const
	{
		return MainCamera.get();
	}

	// --- 光源管理 / Light management ---

	void Scene::AddPointLight(const PointLight& light)
	{
		PointLights.push_back(light);
	}

	void Scene::AddSpotLight(const SpotLight& light)
	{
		SpotLights.push_back(light);
	}

	void Scene::ClearLights()
	{
		PointLights.clear();
		SpotLights.clear();
	}

	// --- 模型管理 / Model management ---

	void Scene::AddModel(std::unique_ptr<Model> model)
	{
		Models.push_back(std::move(model));
	}

	const std::vector<std::unique_ptr<Model>>& Scene::GetModels() const
	{
		return Models;
	}
}

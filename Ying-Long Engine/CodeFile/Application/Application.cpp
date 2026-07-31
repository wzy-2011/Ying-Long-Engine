/**
 * @file Application.cpp
 * @brief Ying-Long Engine 应用程序实现 / Application implementation of Ying-Long Engine
 *
 * 包含 Application 类的所有实现：构造函数中的多线程资源加载、
 * 主循环、DX11/DX12 帧渲染、场景初始化、模式切换等。
 *
 * Contains all implementations of the Application class: multi-threaded
 * resource loading in the constructor, main loop, DX11/DX12 frame rendering,
 * scene initialization, mode switching, etc.
 */

#include "Application.h"
#include "../Graphics/DX12/DX12.h"
#include "../Graphics/DX12/DX12Primitives.h"
#include "../Debug/DX12Log.h"
#include "../ECS/System/PhysicsSystem.h"
#include "../ECS/System/MeshRendererSystem.h"
#include <comdef.h>

namespace YingLong
{
	// 静态单例实例定义 / Static singleton instance definition
	std::unique_ptr<Application> Application::Instance;

	/**
	 * @brief 构造函数：初始化窗口、资源加载、进入 DX12 模式（可选）
	 *        Constructor: initialize windows, load resources, enter DX12 mode (optional)
	 *
	 * 执行流程 / Execution flow:
	 *   1. 创建启动窗口（SplashWindow）和主窗口（MainWindow）
	 *      Create splash window and main window
	 *   2. 显示启动窗口，同时在后台启动初始化线程
	 *      Show splash window while starting initialization thread in background
	 *   3. 初始化线程内并行加载：几何体、模型、物理、场景导入
	 *      Parallel loading inside init thread: geometry, models, physics, scene import
	 *   4. 主线程处理消息循环，等待加载完成
	 *      Main thread processes message loop, waiting for load completion
	 *   5. 加载完成后初始化 ECS 场景，递增构建编号
	 *      After loading, initialize ECS scene and increment build number
	 *   6. 销毁启动窗口，可选初始化 DX12，显示主窗口
	 *      Destroy splash window, optionally init DX12, show main window
	 *
	 * @param windowTitle 主窗口标题 / Main window title
	 */
	Application::Application(std::wstring windowTitle)
	:
	SplashWindow(1000, 800, L"Ying-Long Engine Loading", true),
	MainWindow(1750, 900, windowTitle.c_str()),
	bUseDX12(true),
	pDX12DemoScene(nullptr)
	{
		// 标记初始化是否完成，用于主线程等待
		// Flag indicating whether initialization is finished, used by main thread to wait
		static bool IsInitializationFinished = false;

		// 显示启动（加载）窗口
		// Show the splash (loading) window
		// Show the splash (loading) window
		this->SplashWindow.Display();

		// 初始化三个 DX11 光源对象（2 个点光源 + 1 个聚光灯）- 仅在 DX11 模式下
		// Initialize three DX11 light objects (2 point lights + 1 spot light) - only in DX11 mode
		if (!bUseDX12)
		{
			PointLightOne = std::make_unique<PointLight>(MainWindow.graphics());
			PointLightTwo = std::make_unique<PointLight>(MainWindow.graphics());
			SpotLightTwo = std::make_unique<SpotLight>(MainWindow.graphics());
		}

		// 启动后台初始化线程，避免阻塞主线程的消息循环
		// Start background initialization thread to avoid blocking the main thread's message loop
		std::thread InitializationThread([this]()
		{
			try
			{
				// 创建 PhysX 物理上下文（PxPhysics 工厂对象）
				// Create PhysX physics context (PxPhysics factory object)
				this->PhysicsContext = Physics::Create();

			// 初始化光源管理器（仅 DX11 模式）
			// Initialize the light manager (DX11 mode only)
			if (!bUseDX12)
			{
				LightManager::Initialize(this->MainWindow.graphics());
			}

			// 几何体加载线程：生成 10 个随机旋转盒子 + 1 个胶囊体（仅 DX11 模式）
			// Geometry loading thread: generate 10 random-rotated boxes + 1 capsule (DX11 mode only)
			std::thread GeometryLoadingThread([this]()
				{
					if (bUseDX12)
						return;

					// 随机数生成器，用于随机盒子的位置和旋转
					// Random number generator for random box positions and rotations
					std::mt19937 rng(std::random_device{ }());
					std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);
					std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);
					std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);
					std::uniform_real_distribution<float> rdist(6.0f, 20.0f);
					for (auto i = 0; i < 10; i++)
					{
						boxes.push_back(std::make_unique<BoxDrawable>(
							this->MainWindow.graphics(),
							rng,
							adist,
							ddist,
							odist,
							rdist));
					}

					// 创建一个演示用胶囊体
					// Create a demo capsule
					this->aCapsule.push_back(std::make_unique<CapsuleDrawable>(
						this->MainWindow.graphics(), 1.0f, 2.0f, XMFLOAT3{ 1.0f, 0.5, 0.0f }));
				});

			// 模型 1 加载线程：加载 Cube 模型（Nanosuit 占位用）（仅 DX11 模式）
			// Model 1 loading thread: load Cube model (placeholder for Nanosuit) (DX11 mode only)
			std::thread Model1LoadingThread([this]()
				{
					if (bUseDX12)
						return;

					std::lock_guard<std::mutex> guard(this->Locker);
					this->Nanosuit = Model(this->MainWindow.graphics(), "Resources/Cube/Cube.obj");
				});
			// 模型 2 加载线程：加载 Cerberus FBX 模型（仅 DX11 模式）
			// Model 2 loading thread: load Cerberus FBX model (DX11 mode only)
			std::thread Model2LoadingThread([this]()
				{
					if (bUseDX12)
						return;

					std::lock_guard<std::mutex> guard(this->Locker);
					this->Cerberus = Model(this->MainWindow.graphics(), "Resources/Cerberus/Cerberus.fbx");
				});
			// 物理场景初始化线程
			// Physics scene initialization thread
			std::thread PhysicsInitializingThread([this]()
				{
					this->PhysicsSceneObject.InistializePhysicsScene();
				});
			// 场景数据导入线程：从磁盘加载相机、光源、模型、背景色等配置
			// Scene data import thread: load camera, lights, model, background color configs from disk
			std::thread ImportingThread([this]()
				{
					std::lock_guard<std::mutex> guard(this->Locker);
					this->MainWindow.camera.Import("SceneData/Camera/MainCamera.camera");
					if (!bUseDX12)
					{
						this->PointLightOne->Import("SceneData/Light/PointLight/One.pl");
						this->PointLightTwo->Import("SceneData/Light/PointLight/Two.pl");
					}
					this->Nanosuit.Import("SceneData/Model/Nanosuit.model");
					this->MainWindow.graphics().ImportBackgroundColor("SceneData/BackgroundColor/MainSceneBackgroundColor.col");
				});
			// 等待所有加载线程完成（join 的顺序不影响完成顺序，只是回收线程资源）
			// Wait for all loading threads to complete (join order doesn't affect
			// completion order, it just reclaims thread resources)
			Model2LoadingThread.join();
			Model1LoadingThread.join();
			GeometryLoadingThread.join();
			ImportingThread.join();
			PhysicsInitializingThread.join();

			// 初始化 ECS 场景
			// Initialize the ECS scene
			std::cerr << "[InitializationThread] Calling InitializeScene..." << std::endl;
			this->InitializeScene();
			std::cerr << "[InitializationThread] InitializeScene completed" << std::endl;

			// 标记初始化完成，主线程退出等待循环
			// Mark initialization as finished, main thread exits the wait loop
			std::cerr << "[InitializationThread] Setting IsInitializationFinished = true" << std::endl;
			IsInitializationFinished = true;
			}
			catch (const std::exception& e)
			{
				std::cerr << "[InitializationThread] Exception: " << e.what() << std::endl;
				IsInitializationFinished = true;
			}
			catch (...)
			{
				std::cerr << "[InitializationThread] Unknown exception" << std::endl;
				IsInitializationFinished = true;
			}
		});
		// 分离初始化线程，由它自己管理生命周期
		// Detach the initialization thread, letting it manage its own lifecycle
		InitializationThread.detach();

		// 主线程消息循环：在等待资源加载的同时保持窗口响应
		// Main thread message loop: keep window responsive while waiting for resources to load
		while (!IsInitializationFinished)
		{
			MSG message = { 0 };

			// 从消息队列中取出并分发所有消息
			// Peek and dispatch all messages from the queue
			while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE))
			{
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
		}

		// 读取并递增构建编号
		// Read and increment build number
		this->Counter();

		// 销毁启动窗口
		// Destroy the splash window
		this->SplashWindow.Destroy();

		std::cerr << "[Application] After splash window destroyed, bUseDX12=" << (bUseDX12 ? "true" : "false") << std::endl;

		// 如果启用了 DX12 模式，初始化 DX12 渲染器和演示场景
		// If DX12 mode is enabled, initialize DX12 renderer and demo scene
		if (bUseDX12)
		{
			std::cerr << "[Application] Calling InitializeDX12..." << std::endl;
			MainWindow.InitializeDX12();
			if (MainWindow.IsDX12Enabled())
			{
				InitializeDX12DemoScene();
			}
			else
			{
				// 初始化失败则退回 DX11 模式
				// Fall back to DX11 mode if initialization fails
				bUseDX12 = false;
			}
		}

		// 显示主窗口
		// Show the main window
		this->MainWindow.Display();
	}

	/**
	 * @brief 主消息循环 / Main message loop
	 *
	 * 使用 PeekMessage 实现非阻塞消息循环，每帧空闲时调用 DoFrame()。
	 * 当收到 WM_QUIT 消息时退出循环并返回退出码。
	 *
	 * Uses PeekMessage for non-blocking message loop, calling DoFrame() when idle.
	 * Exits the loop and returns the exit code when WM_QUIT is received.
	 *
	 * @return int 程序退出码 / Program exit code
	 */
	int Application::Go()
	{
		MSG message = { 0 };

		while (true)
		{
			while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
				{
					return (int)message.wParam;
				}
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}

			DoFrame();
		}
	}

	/**
	 * @brief 析构函数 / Destructor
	 *
	 * 手动清理 DX12 资源（因为使用裸指针），然后关闭 DX12 渲染器。
	 * 成员按声明顺序逆序析构，保证 PhysicsContext 在 Scene 和 PhysicsSceneObject 之后销毁。
	 *
	 * Manually cleans up DX12 resources (because raw pointers are used), then
	 * shuts down the DX12 renderer. Members are destructed in reverse declaration
	 * order, ensuring PhysicsContext is destroyed after Scene and PhysicsSceneObject.
	 */
	Application::~Application()
	{
		// 先清理 DX12 演示场景资源
		// Clean up DX12 demo scene resources first
		if (pDX12DemoScene)
		{
			delete pDX12DemoScene;
			pDX12DemoScene = nullptr;
		}

		// 关闭 DX12 渲染器
		// Shutdown DX12 renderer
		MainWindow.ShutdownDX12();

		// Scene 的析构函数会调用 Unload() → 系统 ShutDown() →
		// PhysicsSystem::ShutDown() 释放所有 PxRigidActor 句柄（此时 PhysicsContext 仍存活）。
		// PhysicsSceneObject 的析构函数释放 PxScene + dispatcher。
		// 成员析构顺序保证 PhysicsContext 比 Scene + PhysicsSceneObject 存活更久。
		// Scene's destructor will call Unload() → Systems' ShutDown() →
		// PhysicsSystem::ShutDown() releases all PxRigidActor handles while
		// PhysicsContext (PxPhysics) is still alive. PhysicsSceneObject's dtor
		// then releases PxScene + dispatcher. Member destruction order ensures
		// PhysicsContext outlives Scene + PhysicsSceneObject.
	}

	/**
	 * @brief 初始化 ECS 场景 / Initialize the ECS scene
	 *
	 * 创建主场景，设置主相机，添加光源和模型，注册物理系统和网格渲染系统，
	 * 创建演示用的物理测试实体（盒子、胶囊、地面），最后激活场景。
	 *
	 * Creates the main scene, sets the main camera, adds lights and models,
	 * registers physics and mesh renderer systems, creates demo physics test
	 * entities (box, capsule, ground), and finally activates the scene.
	 */
	void Application::InitializeScene()
	{
		// 创建场景 / Create the scene
		CurrentScene = std::make_unique<Scene>("Main Scene");

		// 添加场景相机 / Add scene camera
		CurrentScene->SetMainCamera(std::make_unique<Camera>(MainWindow.camera));

		// 添加光源到场景 / Add lights to scene
		if (!bUseDX12)
		{
			CurrentScene->AddPointLight(*PointLightOne);
			CurrentScene->AddPointLight(*PointLightTwo);
			CurrentScene->AddSpotLight(*SpotLightTwo);
		}

		// 添加模型到场景 / Add models to scene
		CurrentScene->AddModel(std::make_unique<Model>(Nanosuit));
		CurrentScene->AddModel(std::make_unique<Model>(Cerberus));

		// 绑定物理场景，以便 PhysicsSystem 可以步进和增删 Actor。
		// PhysicsSceneObject 在上方的 PhysicsInitializingThread 中已初始化并 join。
		// Bind the physics scene so PhysicsSystem can step + add/remove actors.
		// PhysicsSceneObject was initialized in PhysicsInitializingThread (joined above).
		CurrentScene->SetPhysicsScene(&PhysicsSceneObject);
		CurrentScene->AddSystem<PhysicsSystem>();
		CurrentScene->AddSystem<MeshRendererSystem>();

		// Demo entity: a box that free-falls under gravity. Verifies that the
		// ECS+PhysX pipeline is wired end-to-end (DX11 and DX12 both step physics
		// via Scene::Update -> PhysicsSystem::UpdateScene).
		auto physicsBox = CurrentScene->CreateEntity("PhysicsTestBox");
		CurrentScene->AddComponent<TransformComponent>(physicsBox, XMFLOAT3{ 0.0f, 5.0f, 0.0f });
		CurrentScene->AddComponent<RigidbodyComponent>(physicsBox);  // Mass=1, UseGravity=true
		CurrentScene->AddComponent<ColliderComponent>(physicsBox);   // Box 0.5x0.5x0.5
		auto& boxMesh = CurrentScene->AddComponent<MeshComponent>(physicsBox);
		boxMesh.TintColor = XMFLOAT4{ 0.8f, 0.3f, 0.2f, 1.0f };  // 橙色，便于识别

		// Capsule 测试实体（替代 deprecated PhysicsCapsule）。
		auto physicsCapsule = CurrentScene->CreateEntity("PhysicsTestCapsule");
		CurrentScene->AddComponent<TransformComponent>(physicsCapsule, XMFLOAT3{ 2.0f, 5.0f, 0.0f });
		CurrentScene->AddComponent<RigidbodyComponent>(physicsCapsule);
		auto& capCol = CurrentScene->AddComponent<ColliderComponent>(physicsCapsule);
		capCol.Shape = ColliderShape::Capsule;
		capCol.Radius = 1.0f;
		capCol.HalfHeight = 2.0f;
		auto& capMesh = CurrentScene->AddComponent<MeshComponent>(physicsCapsule);
		capMesh.TintColor = XMFLOAT4{ 0.2f, 0.8f, 0.4f, 1.0f };  // 绿色

		// 地面静态体（Mass=0 → PxRigidStatic），让 dynamic 实体可以落上去。
		auto ground = CurrentScene->CreateEntity("Ground");
		CurrentScene->AddComponent<TransformComponent>(ground, XMFLOAT3{ 0.0f, -0.5f, 0.0f });
		auto& groundRb = CurrentScene->AddComponent<RigidbodyComponent>(ground);
		groundRb.Mass = 0.0f;
		auto& groundCol = CurrentScene->AddComponent<ColliderComponent>(ground);
		groundCol.Shape = ColliderShape::Box;
		groundCol.HalfExtents = XMFLOAT3{ 50.0f, 0.5f, 50.0f };
		groundCol.Restitution = 0.0f;  // 地面不弹，让物体落定
		auto& groundMesh = CurrentScene->AddComponent<MeshComponent>(ground);
		groundMesh.TintColor = XMFLOAT4{ 0.4f, 0.4f, 0.4f, 1.0f };  // 灰色

		// 激活场景
		CurrentScene->Activate();
	}

	/**
	 * @brief 读取并递增构建编号 / Read and increment build number
	 *
	 * 从 Counter.num 文件读取构建次数，过滤非数字字符，
	 * 将字符串转为整数后加一，再写回文件。
	 * 同时在控制台用绿色输出当前构建编号。
	 *
	 * Reads the build count from Counter.num, filters non-digit characters,
	 * converts the string to an integer, increments it, and writes it back.
	 * Also prints the current build number to the console in green.
	 */
	void Application::Counter() noexcept
	{
		// 读取文件内容到字符串流
		// Read file content into a string stream
		std::ifstream inputFile("Counter.num");
		std::stringstream inputString;
		inputString << inputFile.rdbuf();
		std::string str = inputString.str();
		
		// 过滤出所有数字字符
		// Filter out all digit characters
		std::string Number;
		for (int i = 0; i < str.size(); i++)
		{
			if (str[i] >= '0' && str[i] <= '9')
			{
				Number.push_back(str[i]);
			}
		}

		// 将字符串数字转换为整数（从低位到高位加权求和）
		// Convert string number to integer (weighted sum from low to high)
		this->ReplicationNumber = 0;
		for (int i = Number.size() - 1; i >= 0; i--)
		{
			int t = (int)Number[i] - 48;  // ASCII '0' = 48
			if (i != Number.size() - 1)
			{
				t *= std::pow(10, ((int)Number.size() - i - 1));
			}
			this->ReplicationNumber += t;
		}
		// 构建编号加一
		// Increment build number
		this->ReplicationNumber += 1;

		// 在控制台输出绿色的构建编号
		// Print build number in green to console
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
		std::cout << "Build number: " << this->ReplicationNumber << std::endl;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);

		// 将新的构建编号写回文件
		// Write new build number back to file
		std::stringstream outputString(std::to_string(this->ReplicationNumber));
		std::ofstream outputFile("Counter.num");
		outputFile << outputString.rdbuf();

		outputFile.close();
	}

	/**
	 * @brief 执行一帧（DX11 模式）/ Execute one frame (DX11 mode)
	 *
	 * 每帧执行的完整流程：
	 *   1. F5 热键切换 DX11/DX12 模式（边沿检测）
	 *   2. 更新相机（位置 + 旋转）
	 *   3. 清屏并打开背景色编辑器
	 *   4. 更新并渲染 ECS 场景（或走旧的非 ECS 路径）
	 *   5. 保存场景数据到磁盘（每帧覆盖）
	 *   6. 绘制光源可视化网格、演示几何体
	 *   7. 绘制 ImGui 面板（Simulation、Scene、物理编辑器等）
	 *   8. 结束帧（呈现渲染结果）
	 *
	 * Complete per-frame execution flow:
	 *   1. F5 hotkey to toggle DX11/DX12 mode (edge detection)
	 *   2. Update camera (position + rotation)
	 *   3. Clear screen and open background color editor
	 *   4. Update and render ECS scene (or fall back to old non-ECS path)
	 *   5. Save scene data to disk (overwritten each frame)
	 *   6. Draw light visualizer meshes and demo geometry
	 *   7. Draw ImGui panels (Simulation, Scene, physics editor, etc.)
	 *   8. End frame (present render result)
	 */
	void Application::DoFrame()
		{
			// 热键：按 F5 在 DX11 和 DX12 模式之间切换（边沿检测，防止按住连发）
			// Hotkey: Press F5 to toggle between DX11 and DX12 mode
			// (edge detection to prevent auto-repeat while holding)
			static bool f5WasPressed = false;
			bool f5IsPressed = MainWindow.keyboard.KeyIsPressed(VK_F5);
			if (f5IsPressed && !f5WasPressed)
			{
				EnableDX12Mode(!bUseDX12);
			}
			f5WasPressed = f5IsPressed;

		// 如果已启用 DX12 模式，则走 DX12 渲染路径
		// Use DX12 rendering if enabled
		if (bUseDX12 && MainWindow.IsDX12Enabled())
		{
			DoFrameDX12();
			return;
		}

		// 相机控制必须在 Scene::Render 之前执行，这样场景中的主相机（拷贝）
		// 在绘制前会与 MainWindow.camera 同步。
		// Camera control must happen BEFORE Scene::Render so the scene's
		// MainCamera (a copy) is synced with MainWindow.camera before drawing.
		this->MainWindow.camera.ControlCameraPosition();
		this->MainWindow.camera.ControlCameraRotation();
		if (CurrentScene)
		{
			if (auto* sceneCam = CurrentScene->GetMainCamera())
				*sceneCam = MainWindow.camera;
		}

		// 清屏（用背景色填充后台缓冲区）
		// Clear screen (fill back buffer with background color)
		this->MainWindow.graphics().ClearBuffer(
			this->MainWindow.graphics().color[0],
			this->MainWindow.graphics().color[1],
			this->MainWindow.graphics().color[2]);

		// 启动 ImGui 新帧
		// Start new ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 创建 DockSpace 主窗口
		// Create DockSpace main window
		ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(Viewport->WorkPos);
		ImGui::SetNextWindowSize(Viewport->WorkSize);
		ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		WindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		WindowFlags |= ImGuiWindowFlags_NoBackground;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Ying-Long Engine Editor", NULL, WindowFlags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);
		ImGuiID dockspace_id = ImGui::GetID("Ying-Long Engine Editor Dockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
		ImGui::End();

		this->MainWindow.graphics().ColorEditor();

		// 更新场景（物理 + 所有系统）
		// Update scene (physics + all systems)
		if (CurrentScene)
		{
			// 每帧同步光源数据到 Scene。
			// Scene::AddPointLight/AddSpotLight 是值拷贝，ImGui 修改的是
			// Application 中的原始光源对象，如果不同步，Scene 中的副本永远不更新，
			// 导致移动光源时光照不变。
			// Sync light data to Scene every frame.
			// Scene::AddPointLight/AddSpotLight copies by value; ImGui modifies
			// the original light objects in Application. Without syncing, Scene's
			// copies never update, causing lighting to not change when moving lights.
			CurrentScene->UpdatePointLightData(0, PointLightOne->LightData);
			CurrentScene->UpdatePointLightData(1, PointLightTwo->LightData);
			CurrentScene->UpdateSpotLightData(0, SpotLightTwo->LightData);

			CurrentScene->Update(0.016f);
			CurrentScene->Render(MainWindow.graphics());
		}
		else
		{
			// 旧路径：没有 ECS 场景时直接设置相机和光源（兼容保留）
			// Old path: set camera and lights directly when no ECS scene (kept for compatibility)
			if (!bUseDX12)
			{
				this->MainWindow.graphics().SetCamera(MainWindow.camera);

				// 先收集光源，再更新常量缓冲区（修复帧间延迟问题）
				// Collect lights first, then update CB (fix frame-delay issue)
				this->PointLightOne->Bind();
				this->PointLightTwo->Bind();
				this->SpotLightTwo->Bind();
				LightManager::Update(MainWindow.graphics());
			}
		}

		// 每帧保存场景数据到磁盘（自动持久化，防止意外丢失）
		// Save scene data to disk each frame (auto-persist to prevent data loss)
		if (!bUseDX12)
		{
			this->EngineSaver.SavePointLightSth(
				this->PointLightOne->LightData.Color,
				this->PointLightOne->LightData.Position,
				this->PointLightOne->LightData.Intensity,
				"SceneData/Light/PointLight/One.pl");
			this->EngineSaver.SavePointLightSth(
				this->PointLightTwo->LightData.Color,
				this->PointLightTwo->LightData.Position,
				this->PointLightTwo->LightData.Intensity,
				"SceneData/Light/PointLight/Two.pl");
		}
		this->MainWindow.camera.Save("SceneData/Camera/MainCamera.camera");
		this->Nanosuit.Save("SceneData/Model/Nanosuit.model");
		this->MainWindow.graphics().SaveBackgroundColor("SceneData/BackgroundColor/MainSceneBackgroundColor.col");

		// Nanosuit 和 Cerberus 已经通过 AddModel 由 Scene::Render 绘制。
		// 这里只绘制光源可视化网格（Scene::Render 提交光源数据但不绘制调试网格）。
		// Nanosuit and Cerberus are already drawn by Scene::Render via AddModel.
		// Only draw light visualizers here (Scene::Render submits light data but
		// does not draw the light debug meshes).
		this->PointLightOne->Draw(MainWindow.graphics());
		this->PointLightTwo->Draw(MainWindow.graphics());
		this->SpotLightTwo->Draw(MainWindow.graphics());

		// 演示旋转盒子的更新与绘制（dt 可通过 Simulation 面板调节）
		// Update and draw demo rotating boxes (dt adjustable via Simulation panel)
		static float dt = 10.0f;
		static float Aspect = 800.0f / 600.0f;
		for (auto& b : boxes)
		{
			b->Update(dt / 1000.0f, Aspect);
			b->Draw(MainWindow.graphics());
		}

		// 绘制演示胶囊体 / Draw demo capsule(s)
		for (auto& c : aCapsule)
		{
			c->Draw(MainWindow.graphics());
		}

		// 物理步进已移至 CurrentScene->Update() 中通过 PhysicsSystem::UpdateScene 执行。
		// 之前的直接 simulate/fetchResults 调用已移除，以将物理集中到 ECS 管线中。
		// Physics is stepped inside CurrentScene->Update(16.5f) above via
		// PhysicsSystem::UpdateScene. The previous direct simulate/fetchResults
		// call has been removed to centralize physics in the ECS pipeline.

		// ===== Simulation 面板：控制演示盒子的旋转速度 =====
		// ===== Simulation panel: controls rotation speed of demo boxes =====
		ImGui::Begin("Simulation");
		ImGui::DragFloat("Delta Time", &dt, 0.1f, 0.00001f);
		if (ImGui::Button("Defaut"))
		{
			dt = 10.0f;
		}
		if (ImGui::Button("Stop"))
		{
			dt = 0.0f;
		}
		ImGui::End();

		// ===== Scene 视口面板：显示 3D 场景渲染结果 =====
		// ===== Scene viewport panel: displays 3D scene render result =====
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Scene");

		// 计算鼠标相对于 Scene 面板内的坐标（用于拾取）
		// Calculate mouse coordinates relative to the Scene panel (for picking)
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 mouseScreenPos = ImGui::GetMousePos();
		mouseScreenPos = {
			mouseScreenPos.x - ImGui::GetWindowContentRegionMin().x,
			mouseScreenPos.y - ImGui::GetWindowContentRegionMin().y };
		XMFLOAT2 MousePosition = {
			mouseScreenPos.x - windowPos.x,
			mouseScreenPos.y - windowPos.y };

		// 检测 Scene 面板尺寸变化，动态调整渲染分辨率和宽高比
		// Detect Scene panel size changes, dynamically adjust render resolution and aspect ratio
		ImVec2 CurrentScenePanelSize = ImGui::GetContentRegionAvail();
		static ImVec2 LastScenePanelSize = CurrentScenePanelSize;
		static bool IsFirst = true;
		if ((LastScenePanelSize.x != CurrentScenePanelSize.x) ||
			(LastScenePanelSize.y != CurrentScenePanelSize.y) || IsFirst)
		{
			LastScenePanelSize = CurrentScenePanelSize;
			Aspect = CurrentScenePanelSize.x / CurrentScenePanelSize.y;
			IsFirst = false;

			if (bUseDX12 && MainWindow.GetDX12Renderer())
			{
				MainWindow.GetDX12Renderer()->UpdateSceneSize(
					(int)CurrentScenePanelSize.x, (int)CurrentScenePanelSize.y);
			}
			else
			{
				this->MainWindow.graphics().UpdateSceneGraphicsResolution(
					(int)CurrentScenePanelSize.x, (int)CurrentScenePanelSize.y);
			}
		}
		// 将场景渲染目标作为纹理显示在 ImGui 面板中
		// Display the scene render target as a texture in the ImGui panel
		if (bUseDX12 && MainWindow.GetDX12Renderer())
		{
			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = MainWindow.GetDX12Renderer()->GetSceneSRVHandle();
			if (srvHandle.ptr != 0)
			{
				ImGui::Image((ImTextureID)srvHandle.ptr, ImGui::GetContentRegionAvail());
			}
		}
		else
		{
			ImGui::Image((ImTextureID)this->MainWindow.graphics().SceneRenderTarget->GetRenderTargetResource().Get(),
				ImGui::GetContentRegionAvail());
		}
		
		ImVec2 size = ImGui::GetWindowSize();
		XMFLOAT2 WindowSize = { size.x, size.y };

		// 鼠标左键释放时的射线拾取（目前只是测试框架，暂未实际使用）
		// Ray picking on left mouse release (currently just test scaffolding, not actually used)
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
		{
			Ray ray(MainWindow.camera, WindowSize, MousePosition);

			std::shared_ptr<float> Distance = std::make_shared<float>(0.0f);
			if (ray.Hit(BoundingBox(XMFLOAT3(), XMFLOAT3(1.0f, 1.0f, 1.0f)), Distance.get()))
			{
				//std::cout << "I fucking hit the box! the distance is : " << *Distance << "\n";
			}
		}

		ImGui::End();

		// ===== 各对象的 ImGui 控制面板 / ImGui control panels for various objects =====
		this->aCapsule.back().get()->SpawnControlWindow("aCapsule");

		this->MainWindow.camera.SpawnControlWindow("Camera1");

		this->PointLightOne->SpawnControlWindow("PointLight1");
		this->PointLightTwo->SpawnControlWindow("PointLight2");

		this->SpotLightTwo->SpawnControlWindow("SpotLight1");

		this->Nanosuit.SpawnControlWindow("Nanosuit");
		this->Cerberus.SpawnControlWindow("Cerberus");

		ImGui::PopStyleVar();

		// 物理编辑器面板（实体列表 + 实时编辑 + 射线调试）
		// Physics editor panel (entity list + live editing + raycast debug)
		if (CurrentScene)
		{
			if (auto* ps = CurrentScene->GetSystem<PhysicsSystem>())
			{
				ps->RenderImGuiEditor(*CurrentScene);
			}
		}

		// 结束帧：提交渲染目标，呈现后台缓冲区到屏幕
		// End frame: submit render targets, present back buffer to screen
		this->MainWindow.graphics().EndFrame();
	}

	/**
	 * @brief 启用或禁用 DX12 渲染模式 / Enable or disable DX12 rendering mode
	 *
	 * 在 DX11 和 DX12 之间切换。切换时会：
	 *   - 启用 DX12：初始化 DX12 渲染器 + 创建演示场景
	 *   - 切回 DX11：销毁演示场景 + 关闭 DX12 + 重启 DX11 ImGui 后端
	 *
	 * 所有操作都有异常保护，失败时自动回退到 DX11 模式。
	 *
	 * Switches between DX11 and DX12. When switching:
	 *   - Enable DX12: init DX12 renderer + create demo scene
	 *   - Switch back to DX11: destroy demo scene + shutdown DX12 + restart DX11 ImGui backend
	 *
	 * All operations have exception protection, falling back to DX11 on failure.
	 *
	 * @param enable true 启用 DX12，false 切回 DX11 / true to enable DX12, false to switch back to DX11
	 */
	void Application::EnableDX12Mode(bool enable)
	{
		DX12Log(("[Application::EnableDX12Mode] === DX12 Mode Toggle (requested: "
			+ std::string(enable ? "DX12" : "DX11") + ") ===\n").c_str());

		// 已经是目标模式，直接返回 / Already in requested mode, return immediately
		if (bUseDX12 == enable)
		{
			DX12Log("[Application::EnableDX12Mode] Already in requested mode, skipping\n");
			return;
		}

		if (enable)
		{
			// 启用 DX12：初始化渲染器并创建演示场景
			// Enable DX12: initialize renderer and create demo scene
			DX12Log("[Application::EnableDX12Mode] Initializing DX12...\n");
			try
			{
				MainWindow.InitializeDX12();
				if (MainWindow.IsDX12Enabled())
				{
					DX12LogSuccess("[Application::EnableDX12Mode] DX12 initialized successfully\n");
					bUseDX12 = true;
					InitializeDX12DemoScene();
					DX12LogSuccess("[Application::EnableDX12Mode] Demo scene initialized\n");
				}
				else
				{
					DX12LogWarning("[Application::EnableDX12Mode] DX12 initialization failed, staying in DX11 mode\n");
					bUseDX12 = false;
				}
			}
			catch (const std::exception& e)
			{
				DX12LogError(("[Application::EnableDX12Mode] FAILED: " + std::string(e.what()) + "\n").c_str());
				std::cerr << "Failed to enable DX12 mode: " << e.what() << std::endl;
				bUseDX12 = false;
			}
		}
		else
		{
			// 关闭 DX12：销毁演示场景 + 关闭渲染器 + 重启 DX11 ImGui 后端
			// Shutdown DX12: destroy demo scene + shutdown renderer + restart DX11 ImGui backend
			DX12Log("[Application::EnableDX12Mode] Shutting down DX12...\n");
			try
			{
				// 关闭 DX12 / Shutdown DX12
				if (pDX12DemoScene)
				{
					delete pDX12DemoScene;
					pDX12DemoScene = nullptr;
					DX12Log("[Application::EnableDX12Mode] Demo scene deleted\n");
				}
				MainWindow.ShutdownDX12();
				bUseDX12 = false;
				// 重新注册 DX11 ImGui 渲染后端。ImGuiDX12::Initialize 时通过
				// ImGui_ImplDX11_Shutdown 关闭了 DX11 后端，以防止在 DX12 模式下触发
				// DX11 多视口回调。现在切回 DX11，必须恢复 DX11 后端
				// （字体纹理、VB/IB、多视口交换链），这样 Graphics::ClearBuffer/EndFrame
				// 才能正确渲染 ImGui。
				// Re-register the DX11 ImGui renderer backend. ImGuiDX12::Initialize
				// shut it down via ImGui_ImplDX11_Shutdown to prevent DX11 multi-viewport
				// callbacks from firing in DX12 mode. Now that we're back in DX11, the
				// DX11 backend (font texture, VB/IB, multi-viewport swap chains) must be
				// restored so Graphics::ClearBuffer/EndFrame can render ImGui.
				MainWindow.graphics().ReInitImGui();
				DX12LogSuccess("[Application::EnableDX12Mode] DX11 ImGui backend re-initialized\n");
				DX12LogSuccess("[Application::EnableDX12Mode] DX12 shutdown complete\n");
			}
			catch (const std::exception& e)
			{
				DX12LogError(("[Application::EnableDX12Mode] FAILED during shutdown: " + std::string(e.what()) + "\n").c_str());
				std::cerr << "Failed to shutdown DX12: " << e.what() << std::endl;
			}
		}
	}

	/**
	 * @brief 初始化 DX12 演示场景 / Initialize the DX12 demo scene
	 *
	 * 创建 DX12DemoScene 对象，添加 3 个测试盒子，
	 * 并从 DX11 光源数据拷贝初始化 DX12 光源容器（点光源 + 聚光灯）。
	 * 另外会额外添加一个朝下的聚光灯用于场景观察。
	 *
	 * Creates the DX12DemoScene object, adds 3 test boxes, and initializes
	 * DX12 light containers (point lights + spot lights) by copying from
	 * DX11 light data. Also adds an extra downward-facing spot light
	 * for scene observation.
	 */
	void Application::InitializeDX12DemoScene()
	{
		DX12Log("[Application::InitializeDX12DemoScene] === Initializing Demo Scene ===\n");

		if (!MainWindow.IsDX12Enabled())
		{
			DX12Log("[Application::InitializeDX12DemoScene] DX12 not enabled, skipping\n");
			return;
		}

		DX12Renderer* renderer = MainWindow.GetDX12Renderer();
		if (!renderer)
		{
			DX12LogError("[Application::InitializeDX12DemoScene] FAILED: Renderer is null\n");
			return;
		}

		DX12Core* core = renderer->GetCore();
		if (!core)
		{
			DX12LogError("[Application::InitializeDX12DemoScene] FAILED: Core is null\n");
			return;
		}

		DX12Log("[Application::InitializeDX12DemoScene] Creating DX12DemoScene...\n");
		try
		{
			// Create demo scene
			pDX12DemoScene = new DX12DemoScene(*core);
			pDX12DemoScene->SetCamera(&MainWindow.camera);
			DX12LogSuccess("[Application::InitializeDX12DemoScene] Demo scene created\n");

			// Add some boxes
			DX12Log("[Application::InitializeDX12DemoScene] Creating 3 boxes...\n");
			for (int i = 0; i < 3; i++)
			{
				auto box = std::make_unique<DX12Box>(*core);
				box->SetPosition(static_cast<float>(i - 1) * 2.0f, 0.0f, 0.0f);
				box->SetScale(0.5f);
				box->SetColor(0.5f + i * 0.2f, 0.3f, 0.8f, 1.0f);
				pDX12DemoScene->AddBox(std::move(box));
			}
			DX12LogSuccess("[Application::InitializeDX12DemoScene] Boxes created successfully\n");

			// Initialize DX12 light containers with default values
			// DX11 light objects are not available in DX12 mode, so use defaults
			DX12PointLights.clear();
			DX12SpotLights.clear();

			// Add default point light 1
			{
				DX12PointLightState pl;
				pl.Position = { 5.0f, 5.0f, 5.0f };
				pl.Color = { 1.0f, 0.5f, 0.5f };
				pl.Intensity = 10000.0f;
				pl.Enabled = true;
				DX12PointLights.push_back(pl);
			}

			// Add default point light 2
			{
				DX12PointLightState pl;
				pl.Position = { -5.0f, 5.0f, -5.0f };
				pl.Color = { 0.5f, 0.5f, 1.0f };
				pl.Intensity = 10000.0f;
				pl.Enabled = true;
				DX12PointLights.push_back(pl);
			}

			// Add a downward-facing spot light for scene observation.
			// Rotation {0,0,-90} rotates default dir (1,0,0) to (0,-1,0).
			{
				DX12SpotLightState sl;
				sl.Position = { 0.0f, 10.0f, 0.0f };
				sl.Color = { 0.8f, 0.8f, 1.0f };
				sl.Intensity = 20000.0f;
				sl.Rotation = { 0.0f, 0.0f, -90.0f }; // point -Y (downward)
				sl.OuterConeAngle = XM_PI / 4.0f;
				sl.InnerConeAngle = XM_PI / 6.0f;
				sl.Enabled = true;
				DX12SpotLights.push_back(sl);
			}
			DX12LogSuccess("[Application::InitializeDX12DemoScene] Lights initialized\n");

			// 初始化动态光源缓冲区（支持任意数量光源）
			// Initialize dynamic light buffers (supports arbitrary number of lights)
			DX12Primitive::InitializeLightBuffers(*core, 1000);
			DX12LogSuccess("[Application::InitializeDX12DemoScene] Dynamic light buffers initialized\n");

			DX12LogSuccess("[Application::InitializeDX12DemoScene] === Demo Scene Initialized ===\n");
		}
		catch (const std::exception& e)
		{
			DX12LogError(("[Application::InitializeDX12DemoScene] FAILED: " + std::string(e.what()) + "\n").c_str());
			std::cerr << "Failed to initialize DX12 demo scene: " << e.what() << std::endl;
			if (pDX12DemoScene)
			{
				delete pDX12DemoScene;
				pDX12DemoScene = nullptr;
			}
		}
	}

	/**
	 * @brief 执行一帧（DX12 模式）/ Execute one frame (DX12 mode)
	 *
	 * DX12 渲染路径的完整帧流程：
	 *   1. 更新相机（位置 + 旋转）并同步到 ECS 场景
	 *   2. 更新 ECS 场景（物理步进 + 所有系统）
	 *   3. 构建点光源和聚光灯常量缓冲区
	 *   4. 将光源数据传播给 Demo 场景和 MeshRendererSystem
	 *   5. 调用 DX12Renderer 开始帧 → 开始 ImGui 帧
	 *   6. 创建 DockSpace 主窗口
	 *   7. 绘制各 ImGui 面板（DX12 Mode、DX12 Lights、相机控制等）
	 *   8. 更新并渲染 Demo 场景盒子
	 *   9. 通过 MeshRendererSystem 渲染 ECS 实体
	 *   10. 结束 ImGui 帧 → 结束帧（呈现）
	 *
	 * Complete frame flow for DX12 render path:
	 *   1. Update camera (position + rotation) and sync to ECS scene
	 *   2. Update ECS scene (physics step + all systems)
	 *   3. Build point light and spot light constant buffers
	 *   4. Propagate light data to demo scene and MeshRendererSystem
	 *   5. Call DX12Renderer to begin frame → begin ImGui frame
	 *   6. Create DockSpace main window
	 *   7. Draw various ImGui panels (DX12 Mode, DX12 Lights, camera control, etc.)
	 *   8. Update and render demo scene boxes
	 *   9. Render ECS entities via MeshRendererSystem
	 *   10. End ImGui frame → end frame (present)
	 */
	void Application::DoFrameDX12()
	{
		if (!bUseDX12 || !MainWindow.IsDX12Enabled())
			return;

		DX12Renderer* renderer = MainWindow.GetDX12Renderer();
		if (!renderer)
			return;

		// Camera control (same input scheme as DX11: WASD + RMB to move,
		// WASD + LMB to rotate). Must happen before ECS rendering so the
		// view/projection matrices passed to MeshRendererSystem are current.
		this->MainWindow.camera.ControlCameraPosition();
		this->MainWindow.camera.ControlCameraRotation();
		if (CurrentScene)
		{
			if (auto* sceneCam = CurrentScene->GetMainCamera())
				*sceneCam = MainWindow.camera;
		}

		// Step physics + ECS through the same Scene::Update path used by DX11.
		// PhysicsSystem::UpdateScene will run inside this call, so DX12 mode
		// shares the same physics state as DX11 (no divergence on F5 toggle).
		if (CurrentScene)
		{
			CurrentScene->Update(0.016f);
		}

		std::vector<DX12PointLightData> pointLightList;
		std::vector<DX12SpotLightData> spotLightList;

		for (const auto& pl : DX12PointLights)
		{
			if (!pl.Enabled)
				continue;
			DX12PointLightData data = {};
			data.Position[0] = pl.Position.x;
			data.Position[1] = pl.Position.y;
			data.Position[2] = pl.Position.z;
			data.pad0 = 0.0f;
			data.Color[0] = pl.Color.x;
			data.Color[1] = pl.Color.y;
			data.Color[2] = pl.Color.z;
			data.Intensity = pl.Intensity;
			pointLightList.push_back(data);
		}

		for (const auto& sl : DX12SpotLights)
		{
			if (!sl.Enabled)
				continue;
			DX12SpotLightData data = {};
			data.Position[0] = sl.Position.x;
			data.Position[1] = sl.Position.y;
			data.Position[2] = sl.Position.z;
			data.Intensity = sl.Intensity;
			data.Color[0] = sl.Color.x;
			data.Color[1] = sl.Color.y;
			data.Color[2] = sl.Color.z;
			data.InnerConeAngle = cosf(sl.InnerConeAngle);
			float pitchRad = sl.Rotation.x / 360.0f * XM_2PI;
			float yawRad   = sl.Rotation.y / 360.0f * XM_2PI;
			float rollRad  = sl.Rotation.z / 360.0f * XM_2PI;
			XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(pitchRad, yawRad, rollRad);
			XMVECTOR dir = XMVector3Normalize(XMVector3Transform(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotMat));
			XMFLOAT3 dirF;
			XMStoreFloat3(&dirF, dir);
			data.Direction[0] = dirF.x;
			data.Direction[1] = dirF.y;
			data.Direction[2] = dirF.z;
			data.OuterConeAngle = cosf(sl.OuterConeAngle);
			// Rotation 和 pad 仅在 HLSL SpotLight struct 中用于对齐，不参与实际计算
			// Rotation and pad are only for alignment in HLSL SpotLight struct, not used in calculations
			data.Rotation[0] = data.Rotation[1] = data.Rotation[2] = 0.0f;
			data.pad = 0.0f;
			spotLightList.push_back(data);
		}

		XMFLOAT3 camPos = MainWindow.camera.GetPosition();
		DX12LightCountCB lightCountData = {};
		lightCountData.PointLightCount = static_cast<int>(pointLightList.size());
		lightCountData.SpotLightCount = static_cast<int>(spotLightList.size());
		lightCountData.CameraPosition[0] = camPos.x;
		lightCountData.CameraPosition[1] = camPos.y;
		lightCountData.CameraPosition[2] = camPos.z;

		// 更新全局光源缓冲区
		// Update global light buffers
		DX12Primitive::UpdatePointLightBuffer(pointLightList);
		DX12Primitive::UpdateSpotLightBuffer(spotLightList);

		// 将光源计数和相机位置数据推送到渲染器（延迟渲染 Lighting Pass 使用）
		// Push light count and camera position to renderer (used by deferred Lighting Pass)
		if (MainWindow.GetDX12Renderer())
		{
			MainWindow.GetDX12Renderer()->SetLightCountData(lightCountData);
		}

		// Propagate lighting to both the demo scene primitives and the
		// ECS-driven physics placeholders (MeshRendererSystem DX12Boxes).
		if (pDX12DemoScene)
		{
			try
			{
				pDX12DemoScene->SetLightCountData(lightCountData);

				// 同步灯光可视化（球体表示点光源，锥体线框表示聚光灯）
				// Sync light visualization (spheres for point lights, cone wireframes for spot lights)
				pDX12DemoScene->SyncPointLightVisualization(pointLightList, DX12PointLights);
				pDX12DemoScene->SyncSpotLightVisualization(spotLightList, DX12SpotLights);
			}
			catch (const std::exception& e)
			{
				std::cerr << "[DoFrameDX12] Light sync error: " << e.what() << std::endl;
				// 光照同步失败不影响主渲染循环
				// Light sync failure does not affect the main render loop
			}
			catch (...)
			{
				std::cerr << "[DoFrameDX12] Unknown light sync error" << std::endl;
			}
		}
		if (CurrentScene)
		{
			if (auto* meshRenderer = CurrentScene->GetSystem<MeshRendererSystem>())
			{
				meshRenderer->SetDX12LightCountData(lightCountData);
				meshRenderer->SetDX12PointLightBuffer(pointLightList);
				meshRenderer->SetDX12SpotLightBuffer(spotLightList);
			}
		}

		try
		{
			// Begin frame
			float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
			renderer->BeginFrame(clearColor);

			// Begin ImGui frame
			renderer->BeginImGuiFrame();

			// Create a fullscreen DockSpace host window (mirrors the DX11 path
			// in Graphics::ClearBuffer). NoBackground + PassthruCentralNode lets
			// the 3D scene show through the empty central dock area; all
			// subsequent ImGui windows can be docked into it.
			{
				ImGuiWindowFlags dockFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
				const ImGuiViewport* vp = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(vp->WorkPos);
				ImGui::SetNextWindowSize(vp->WorkSize);
				ImGui::SetNextWindowViewport(vp->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				dockFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				dockFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
				dockFlags |= ImGuiWindowFlags_NoBackground;

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::Begin("Ying-Long Engine Editor", nullptr, dockFlags);
				ImGui::PopStyleVar();
				ImGui::PopStyleVar(2);

				ImGuiID dockspaceId = ImGui::GetID("Ying-Long Editor Dockspace");
				ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f),
					ImGuiDockNodeFlags_PassthruCentralNode);
				ImGui::End();
			}

			// ===== Scene viewport panel: displays 3D scene render result =====
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Scene");

			ImVec2 CurrentScenePanelSize = ImGui::GetContentRegionAvail();
			static ImVec2 LastScenePanelSize = CurrentScenePanelSize;
			static bool IsFirst = true;
			if ((LastScenePanelSize.x != CurrentScenePanelSize.x) ||
				(LastScenePanelSize.y != CurrentScenePanelSize.y) || IsFirst)
			{
				LastScenePanelSize = CurrentScenePanelSize;
				renderer->UpdateSceneSize((int)CurrentScenePanelSize.x, (int)CurrentScenePanelSize.y);
				IsFirst = false;
			}

			renderer->BeginSceneRender(clearColor);

			if (pDX12DemoScene)
			{
				int sceneWidth = renderer->GetSceneWidth();
				int sceneHeight = renderer->GetSceneHeight();
				if (sceneWidth > 0 && sceneHeight > 0)
				{
					MainWindow.camera.SetResolution(XMFLOAT2{ (float)sceneWidth, (float)sceneHeight });
				}
				pDX12DemoScene->Update(0.016f);
				ID3D12GraphicsCommandList* commandList = renderer->GetCore()->GetCommandList();
				if (commandList)
				{
					DX12Primitive::UpdateLightBuffers(commandList);
					pDX12DemoScene->Render(commandList);
				}
			}

			if (CurrentScene && renderer)
			{
				auto* meshRenderer = CurrentScene->GetSystem<MeshRendererSystem>();
				if (meshRenderer)
				{
					XMMATRIX viewMat = MainWindow.camera.GetMatrix();
					XMMATRIX projMat = MainWindow.camera.GetProjection();
					XMFLOAT4X4 viewF, projF;
					XMStoreFloat4x4(&viewF, viewMat);
					XMStoreFloat4x4(&projF, projMat);
					meshRenderer->RenderDX12(*CurrentScene, *renderer->GetCore(),
						renderer->GetCore()->GetCommandList(),
						reinterpret_cast<const float*>(&viewF),
						reinterpret_cast<const float*>(&projF),
						0.016f);
				}
			}

			renderer->EndSceneRender();

			// 延迟渲染：在 Lighting Pass 之后渲染线框锥体（前向通道）
			// Deferred rendering: render wireframe cones after Lighting Pass (forward pass)
			if (renderer->IsDeferredRenderingEnabled() && pDX12DemoScene)
			{
				ID3D12GraphicsCommandList* cmdList = renderer->GetCore()->GetCommandList();
				if (cmdList)
				{
					pDX12DemoScene->RenderWireframeCones(cmdList);
				}
				renderer->FinalizeDeferredSceneRender();
			}

			ImGui::Image((ImTextureID)renderer->GetSceneSRVHandle().ptr, ImGui::GetContentRegionAvail());

			ImGui::End();
			ImGui::PopStyleVar();

			// DX12 Mode UI
		ImGui::Begin("DX12 Mode");
		ImGui::Text("Rendering API: DirectX 12");
		ImGui::Text("Press F5 to switch to DX11 mode");
		ImGui::Text("Resolution: %d x %d", renderer->GetWidth(), renderer->GetHeight());
		ImGui::Text("Point Lights: %d / %d", lightCountData.PointLightCount, (int)DX12PointLights.size());
		ImGui::Text("Spot Lights: %d / %d", lightCountData.SpotLightCount, (int)DX12SpotLights.size());

		// 延迟渲染切换
		// Deferred rendering toggle
		if (ImGui::CollapsingHeader("Rendering"))
		{
			bool bDeferred = renderer->IsDeferredRenderingEnabled();
			if (ImGui::Checkbox("Deferred Rendering", &bDeferred))
			{
				renderer->SetUseDeferredRendering(bDeferred);
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(
					"Deferred Rendering:\n"
					"  ON  = Geometry Pass + Lighting Pass (G-Buffer)\n"
					"  OFF = Traditional Forward Rendering");
			}
		}

		// Demo scene control
		if (ImGui::CollapsingHeader("DX12 Demo Scene"))
		{
			ImGui::Text("This scene demonstrates DX12 rendering.");
			ImGui::Text("3 rotating boxes are rendered.");
		}
		ImGui::End();

			// DX12 Lights control panel.
			// Supports dynamic creation/removal and full per-light editing for
			// both PointLight and SpotLight types.
			ImGui::Begin("DX12 Lights");

			// ---- Point Lights ----
			if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Count: %d", (int)DX12PointLights.size());
				if (ImGui::Button("Add Point Light"))
				{
					DX12PointLights.push_back(DX12PointLightState{});
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(DX12PointLights.empty());
				if (ImGui::Button("Remove Last##Point"))
				{
					DX12PointLights.pop_back();
				}
				ImGui::EndDisabled();
				ImGui::Separator();

				for (size_t i = 0; i < DX12PointLights.size(); i++)
				{
					auto& pl = DX12PointLights[i];
					ImGui::PushID(static_cast<int>(i));
					if (ImGui::TreeNode((void*)(intptr_t)i, "PointLight %d", (int)i))
					{
						ImGui::Checkbox("Enabled", &pl.Enabled);
						float pos[3] = { pl.Position.x, pl.Position.y, pl.Position.z };
						if (ImGui::DragFloat3("Position", pos, 0.1f))
							pl.Position = { pos[0], pos[1], pos[2] };
						float col[3] = { pl.Color.x, pl.Color.y, pl.Color.z };
						if (ImGui::DragFloat3("Color", col, 0.01f, 0.0f, 1.0f))
							pl.Color = { col[0], col[1], col[2] };
						ImGui::DragFloat("Intensity", &pl.Intensity, 10.0f, 0.0f, 100000.0f);
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}

			// ---- Spot Lights ----
			if (ImGui::CollapsingHeader("Spot Lights", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Count: %d", (int)DX12SpotLights.size());
				if (ImGui::Button("Add Spot Light"))
				{
					DX12SpotLights.push_back(DX12SpotLightState{});
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(DX12SpotLights.empty());
				if (ImGui::Button("Remove Last##Spot"))
				{
					DX12SpotLights.pop_back();
				}
				ImGui::EndDisabled();
				ImGui::Separator();

				for (size_t i = 0; i < DX12SpotLights.size(); i++)
				{
					auto& sl = DX12SpotLights[i];
					ImGui::PushID(static_cast<int>(i + 1000));
					if (ImGui::TreeNode((void*)(intptr_t)(i + 1000), "SpotLight %d", (int)i))
					{
						ImGui::Checkbox("Enabled", &sl.Enabled);
						float pos[3] = { sl.Position.x, sl.Position.y, sl.Position.z };
						if (ImGui::DragFloat3("Position", pos, 0.1f))
							sl.Position = { pos[0], pos[1], pos[2] };
						float col[3] = { sl.Color.x, sl.Color.y, sl.Color.z };
						if (ImGui::DragFloat3("Color", col, 0.01f, 0.0f, 1.0f))
							sl.Color = { col[0], col[1], col[2] };
						ImGui::DragFloat("Intensity", &sl.Intensity, 100.0f, 0.0f, 1000000.0f);
						float rot[3] = { sl.Rotation.x, sl.Rotation.y, sl.Rotation.z };
						if (ImGui::DragFloat3("Rotation (deg)", rot, 1.0f, -360.0f, 360.0f))
							sl.Rotation = { rot[0], rot[1], rot[2] };
						// Edit cone angles in degrees for intuitiveness.
						float outerDeg = sl.OuterConeAngle * 180.0f / XM_PI;
						float innerDeg = sl.InnerConeAngle * 180.0f / XM_PI;
						if (ImGui::SliderFloat("Outer Angle", &outerDeg, 1.0f, 179.0f, "%.1f deg"))
						{
							sl.OuterConeAngle = outerDeg * XM_PI / 180.0f;
							if (sl.InnerConeAngle > sl.OuterConeAngle)
								sl.InnerConeAngle = sl.OuterConeAngle;
						}
						if (ImGui::SliderFloat("Inner Angle", &innerDeg, 0.0f, outerDeg, "%.1f deg"))
						{
							sl.InnerConeAngle = innerDeg * XM_PI / 180.0f;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}

			ImGui::End();

			// Camera control
			MainWindow.camera.SpawnControlWindow("DX12 Camera");

			// Demo scene control panel
			if (pDX12DemoScene)
			{
				pDX12DemoScene->SpawnControlWindow();
			}

			// Physics editor panel (DX12 path)
			if (CurrentScene)
			{
				if (auto* ps = CurrentScene->GetSystem<PhysicsSystem>())
				{
					ps->RenderImGuiEditor(*CurrentScene);
				}
			}

			// End ImGui frame
			renderer->EndImGuiFrame();

			// End frame
			renderer->EndFrame();
		}
		catch (const _com_error& e)
		{
			// COM 组件调用错误（如 D3D12 设备移除等）
			// COM component call error (e.g. D3D12 device removal)
			std::wcerr << L"[DoFrameDX12] COM error (0x" << std::hex
				<< e.Error() << L"): " << e.ErrorMessage() << std::endl;
			// 检查是否为设备移除错误 / Check if it's a device removal error
			if (static_cast<HRESULT>(e.Error()) == DXGI_ERROR_DEVICE_REMOVED
				|| static_cast<HRESULT>(e.Error()) == DXGI_ERROR_DEVICE_HUNG
				|| static_cast<HRESULT>(e.Error()) == DXGI_ERROR_DEVICE_RESET)
			{
				std::wcerr << L"[DoFrameDX12] Device removed/hung/reset detected. "
					<< L"Consider restarting the application." << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			// 标准库异常（std::runtime_error 等）
			// Standard library exception (std::runtime_error, etc.)
			std::cerr << "[DoFrameDX12] std::exception: " << e.what() << std::endl;
		}
		catch (...)
		{
			// 未知异常 / Unknown exception
			std::cerr << "[DoFrameDX12] Unknown exception caught in frame render" << std::endl;
		}
	}
}

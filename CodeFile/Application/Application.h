/**
 * @file Application.h
 * @brief Ying-Long Engine 应用程序核心类 / Core application class of Ying-Long Engine
 *
 * Application 类是引擎的顶层管理者，负责：
 *   - 窗口创建与管理（启动窗口 + 主窗口）
 *   - 多线程资源加载（模型、几何体、物理、场景数据）
 *   - DX11/DX12 双渲染模式切换
 *   - ECS 场景管理与系统调度
 *   - 光源管理与 ImGui 编辑器面板
 *
 * The Application class is the top-level manager of the engine, responsible for:
 *   - Window creation and management (splash + main window)
 *   - Multi-threaded resource loading (models, geometry, physics, scene data)
 *   - DX11/DX12 dual rendering mode switching
 *   - ECS scene management and system scheduling
 *   - Light management and ImGui editor panels
 */

#pragma once
#include <iostream>
#include <random>
#include <filesystem>
#include <thread>
#include <vector>
#include "../../yaml-cpp/include/yaml-cpp/yaml.h"
#include <mutex>
#include <DirectXMath.h>
#include "../Graphics/Drawable/Capsule.h"
#include "../Graphics/Drawable/Box.h"
#include "../Application/Window/Window.h"
#include "../Graphics/Light/LightManager.h"
#include "../Graphics/Drawable/Model.h"
#include "../Graphics/Ray/Ray.h"
#include "../Physics/Physics.h"
#include "../Physics/PhysicsScene/PhysicsScene.h"
#include "../ECS/Entity/Entity.h"
#include "../ECS/Scene/Scene.h"
#include "../ECS/Components/Components.h"
#include "../FileController/Saver.h"

namespace YingLong
{
	/**
	 * @brief DX12 模式下的点光源纯数据状态 / Pure-data point light state for DX12 mode
	 *
	 * 与 DX11 的 PointLight 类（会构造 DX11 调试网格）解耦，
	 * 使得在 DX12 模式下可以仅通过数据创建和编辑光源。
	 *
	 * Decoupled from the DX11 PointLight class (which constructs DX11 debug
	 * meshes) so lights can be created/edited purely from data in DX12 mode.
	 */
	struct DX12PointLightState
	{
		DirectX::XMFLOAT3 Position = { 0.0f, 5.0f, 0.0f };  ///< 光源世界坐标 / World-space position of the light
		DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };     ///< 光源颜色（RGB，可以超 1.0f 做 HDR）/ Light color (RGB, can exceed 1.0f for HDR)
		float Intensity = 1000.0f;                          ///< 光强（流明近似）/ Light intensity (approximate lumens)
		bool Enabled = true;                                ///< 是否启用该光源 / Whether this light is enabled
	};

	/**
	 * @brief DX12 模式下的聚光灯纯数据状态 / Pure-data spot light state for DX12 mode
	 *
	 * 旋转角度以度为单位（0-360），与 SpotLight::Data 约定一致。
	 * 锥角以弧度存储（便于直观编辑），在填充 DX12 常量缓冲区时转换为 cos(弧度)，
	 * 因为 PBR 着色器会与点积结果进行比较。
	 *
	 * Rotation is in degrees (0-360), matching SpotLight::Data convention.
	 * Cone angles are stored in radians for intuitive editing; converted to
	 * cos(radians) when filling the DX12 constant buffer because the PBR
	 * shader compares against dot() results.
	 */
	struct DX12SpotLightState
	{
		DirectX::XMFLOAT3 Position = { 0.0f, 5.0f, 0.0f };  ///< 光源世界坐标 / World-space position of the light
		DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };     ///< 光源颜色（RGB）/ Light color (RGB)
		float Intensity = 10000.0f;                         ///< 光强 / Light intensity
		DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };  ///< 旋转角（度），决定照射方向 / Rotation in degrees, determines light direction
		float OuterConeAngle = XM_PIDIV4;                   ///< 外锥角（弧度），45 度 / Outer cone angle in radians (45 degrees)
		float InnerConeAngle = XM_PIDIV4 * 0.5f;            ///< 内锥角（弧度），22.5 度 / Inner cone angle in radians (22.5 degrees)
		bool Enabled = true;                                ///< 是否启用 / Whether this light is enabled
	};

	/**
	 * @brief 引擎应用程序主类 / Main engine application class
	 *
	 * Application 是引擎的顶层协调者，采用单例模式（Instance 静态成员）。
	 * 它管理窗口、渲染设备、物理场景、ECS 场景和所有游戏对象。
	 *
	 * Application is the top-level coordinator of the engine, using the singleton
	 * pattern (static Instance member). It manages windows, rendering devices,
	 * physics scenes, ECS scenes, and all game objects.
	 *
	 * 使用示例 / Usage example:
	 * @code
	 *   YingLong::Application::Instance = std::make_unique<YingLong::Application>(L"My Game");
	 *   return YingLong::Application::Instance->Go();
	 * @endcode
	 */
	class Application
	{
		friend class Graphics;  ///< Graphics 类需要访问内部渲染资源 / Graphics class needs access to internal render resources

	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		Application() = default;

		/**
		 * @brief 带窗口标题的构造函数 / Constructor with window title
		 *
		 * 初始化启动窗口、主窗口、光源，并启动多线程加载所有资源。
		 * 加载完成后显示主窗口，可选初始化 DX12 渲染模式。
		 *
		 * Initializes splash window, main window, lights, and starts multi-threaded
		 * loading of all resources. After loading completes, shows the main window
		 * and optionally initializes DX12 rendering mode.
		 *
		 * @param windowTitle 主窗口标题（宽字符串）/ Main window title (wide string)
		 */
		Application(std::wstring windowTitle);

		/**
		 * @brief 进入主消息循环 / Enter the main message loop
		 *
		 * 运行 Windows 消息泵，每帧调用 DoFrame() 进行更新和渲染，
		 * 直到收到 WM_QUIT 消息。
		 *
		 * Runs the Windows message pump, calling DoFrame() each frame for update
		 * and rendering, until a WM_QUIT message is received.
		 *
		 * @return int 退出码（来自 WM_QUIT 的 wParam）/ Exit code (from WM_QUIT wParam)
		 */
		int Go();

		/**
		 * @brief 析构函数 / Destructor
		 *
		 * 按顺序清理 DX12 资源 → 关闭 DX12 渲染器 → 自动析构成员。
		 * 成员析构顺序保证 PhysicsContext 比 Scene 和 PhysicsSceneObject 存活更久。
		 *
		 * Cleans up DX12 resources first, then shuts down the DX12 renderer,
		 * then auto-destructs members. Member destruction order ensures
		 * PhysicsContext outlives Scene + PhysicsSceneObject.
		 */
		~Application();

		/**
		 * @brief 启用或禁用 DX12 渲染模式 / Enable or disable DX12 rendering mode
		 *
		 * 在 DX11 和 DX12 之间切换。切换时会重新初始化对应的 ImGui 后端。
		 * 可通过 F5 热键触发。
		 *
		 * Switches between DX11 and DX12. Re-initializes the corresponding ImGui
		 * backend when switching. Can be triggered via F5 hotkey.
		 *
		 * @param enable true 启用 DX12，false 切回 DX11 / true to enable DX12, false to switch back to DX11
		 */
		void EnableDX12Mode(bool enable);

		/**
		 * @brief 查询当前是否处于 DX12 模式 / Query whether currently in DX12 mode
		 * @return true DX12 模式 / DX12 mode
		 * @return false DX11 模式 / DX11 mode
		 */
		bool IsDX12Mode() const noexcept { return bUseDX12; }

		static std::unique_ptr<Application> Instance;  ///< 应用程序单例 / Application singleton instance

	private:
		/**
		 * @brief 读取并递增构建编号 / Read and increment build number
		 *
		 * 从 Counter.num 文件读取构建次数，加一后写回，
		 * 并在控制台以绿色输出当前构建编号。
		 *
		 * Reads the build count from Counter.num, increments it, writes it back,
		 * and prints the current build number to the console in green.
		 */
		void Counter() noexcept;

		/**
		 * @brief 执行一帧（DX11 模式）/ Execute one frame (DX11 mode)
		 *
		 * 处理输入、更新相机、更新场景、渲染场景、绘制 ImGui 面板、
		 * 保存场景数据。每帧由 Go() 中的消息循环调用。
		 *
		 * Processes input, updates camera, updates scene, renders scene, draws
		 * ImGui panels, saves scene data. Called each frame by the message loop
		 * in Go().
		 */
		void DoFrame();

		/**
		 * @brief 执行一帧（DX12 模式）/ Execute one frame (DX12 mode)
		 *
		 * DX12 渲染路径：更新相机和物理 → 构建光源常量缓冲区 →
		 * 调用 DX12Renderer 开始帧 → 渲染 Demo 场景和 ECS 实体 →
		 * 绘制 ImGui 面板 → 结束帧。
		 *
		 * DX12 render path: update camera and physics → build light constant
		 * buffers → call DX12Renderer to begin frame → render demo scene and
		 * ECS entities → draw ImGui panels → end frame.
		 */
		void DoFrameDX12();

		/**
		 * @brief 初始化 ECS 场景 / Initialize the ECS scene
		 *
		 * 创建场景、设置主相机、添加光源和模型、注册物理系统和网格渲染系统，
		 * 创建测试实体（物理盒子、物理胶囊、地面），然后激活场景。
		 *
		 * Creates the scene, sets the main camera, adds lights and models,
		 * registers physics and mesh renderer systems, creates test entities
		 * (physics box, physics capsule, ground), then activates the scene.
		 */
		void InitializeScene();

		/**
		 * @brief 初始化 DX12 演示场景 / Initialize the DX12 demo scene
		 *
		 * 创建 DX12DemoScene 对象，添加测试盒子，
		 * 并从 DX11 光源数据初始化 DX12 光源容器。
		 *
		 * Creates the DX12DemoScene object, adds test boxes, and initializes
		 * DX12 light containers from DX11 light data.
		 */
		void InitializeDX12DemoScene();

		int ReplicationNumber;  ///< 构建编号计数器 / Build number counter
		Window SplashWindow;    ///< 启动（加载）窗口 / Splash (loading) window
		Window MainWindow;      ///< 主渲染窗口 / Main render window

		std::unique_ptr<Physics> PhysicsContext;  ///< PhysX 物理上下文（PxPhysics 工厂）/ PhysX physics context (PxPhysics factory)

		std::unique_ptr<PointLight> PointLightOne;  ///< DX11 点光源 1 / DX11 point light 1
		std::unique_ptr<PointLight> PointLightTwo;  ///< DX11 点光源 2 / DX11 point light 2
		std::unique_ptr<SpotLight> SpotLightTwo;    ///< DX11 聚光灯 / DX11 spot light

		PhysicsScene PhysicsSceneObject;  ///< PhysX 物理场景 / PhysX physics scene

		std::vector<std::unique_ptr<class BoxDrawable>> boxes;     ///< 演示用旋转盒子 / Demo rotating boxes
		std::vector<std::unique_ptr<class CapsuleDrawable>> aCapsule;  ///< 演示用胶囊体 / Demo capsule
		Model Nanosuit;    ///< Nanosuit 模型（Demo 用）/ Nanosuit model (for demo)
		Model Cerberus;    ///< Cerberus 模型（Demo 用）/ Cerberus model (for demo)
		Saver EngineSaver; ///< 场景数据持久化工具 / Scene data persistence utility
		std::mutex Locker; ///< 多线程加载时的资源互斥锁 / Mutex for resource loading across threads

		/// 使用 Scene 管理 ECS 实体 / Using Scene to manage entities
		std::unique_ptr<Scene> CurrentScene;

		bool bUseDX12;                        ///< 是否启用 DX12 渲染模式 / Whether DX12 rendering mode is enabled
		class DX12DemoScene* pDX12DemoScene;  ///< DX12 演示场景指针 / DX12 demo scene pointer

		/// DX12 光源容器。在 InitializeDX12DemoScene 中从 PointLightOne/Two/SpotLightTwo
		/// 初始化，之后可通过 ImGui 编辑。
		/// DX12 light containers. Initialized in InitializeDX12DemoScene from
		/// PointLightOne/Two/SpotLightTwo, then editable via ImGui.
		std::vector<DX12PointLightState> DX12PointLights;  ///< DX12 点光源列表 / DX12 point light list
		std::vector<DX12SpotLightState> DX12SpotLights;    ///< DX12 聚光灯列表 / DX12 spot light list

		friend class LightManager;  ///< LightManager 需要访问光源成员 / LightManager needs access to light members
	};
}

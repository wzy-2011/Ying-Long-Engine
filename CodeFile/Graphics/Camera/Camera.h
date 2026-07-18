/**
 * @file Camera.h
 * @brief 相机类 / Camera class
 *
 * 提供第一人称视角相机，支持：
 *   - 位置/旋转控制（平移、旋转）
 *   - 键盘鼠标控制（WASD + 鼠标右键旋转）
 *   - 视图矩阵和投影矩阵生成
 *   - 前向/左/上方向向量获取
 *   - YAML 序列化/反序列化
 *   - ImGui 控制面板
 *
 * Provides first-person perspective camera, supporting:
 *   - Position/rotation control (translate, rotate)
 *   - Keyboard/mouse control (WASD + right mouse button rotation)
 *   - View matrix and projection matrix generation
 *   - Forward/left/up direction vector retrieval
 *   - YAML serialization/deserialization
 *   - ImGui control panel
 */
#pragma once
#include <filesystem>
#include "../../../yaml-cpp/include/yaml-cpp/yaml.h"
#include <fstream>
//#include "../Graphics.h"
#include <DirectXMath.h>
#include "../../ImGui/CodeFile/ImGui/imgui_impl_win32.h"
#include "../../ImGui/CodeFile/ImGui/imgui_impl_dx11.h"
#include "../../ImGui/CodeFile/ImGui/imgui.h"
#include <Windows.h>

namespace YingLong
{
	/**
	 * @brief 相机类 / Camera class
	 *
	 * 第一人称视角相机，使用欧拉角（roll-pitch-yaw）表示旋转。
	 * 提供视图矩阵和透视投影矩阵的计算。
	 *
	 * First-person perspective camera using Euler angles (roll-pitch-yaw) for rotation.
	 * Provides view matrix and perspective projection matrix computation.
	 */
	class Camera
	{
	public:
		/**
		 * @brief 获取视图矩阵 / Get view matrix
		 * @return FXMMATRIX 视图矩阵（行优先） / View matrix (row-major)
		 */
		FXMMATRIX GetMatrix() const noexcept;

		/**
		 * @brief 获取投影矩阵 / Get projection matrix
		 * @return FXMMATRIX 透视投影矩阵 / Perspective projection matrix
		 */
		FXMMATRIX GetProjection() const noexcept;

		/**
		 * @brief 生成 ImGui 控制面板 / Spawn ImGui control window
		 * @param CameraName 面板名称 / Panel name
		 */
		void SpawnControlWindow(const char* CameraName) noexcept;

		/**
		 * @brief 平移相机（XMFLOAT3 版本） / Translate camera (XMFLOAT3 version)
		 * @param translation 平移向量 / Translation vector
		 */
		void Translate(XMFLOAT3 translation) noexcept;

		/**
		 * @brief 平移相机（XMVECTOR 版本） / Translate camera (XMVECTOR version)
		 * @param translation 平移向量 / Translation vector
		 */
		void Translate(XMVECTOR translation) noexcept;

		/**
		 * @brief 旋转相机（XMFLOAT3 版本，度为单位） / Rotate camera (XMFLOAT3 version, in degrees)
		 * @param rotation 旋转向量（度） / Rotation vector (degrees)
		 */
		void Rotate(XMFLOAT3 rotation) noexcept;

		/**
		 * @brief 旋转相机（XMVECTOR 版本，度为单位） / Rotate camera (XMVECTOR version, in degrees)
		 * @param rotation 旋转向量（度） / Rotation vector (degrees)
		 */
		void Rotate(XMVECTOR rotation) noexcept;

		/**
		 * @brief 设置分辨率（影响宽高比） / Set resolution (affects aspect ratio)
		 * @param resolution 分辨率（宽，高） / Resolution (width, height)
		 */
		void SetResolution(XMFLOAT2 resolution);

		/**
		 * @brief 获取前向向量 / Get forward vector
		 * @return XMVECTOR 前向单位向量 / Forward unit vector
		 */
		XMVECTOR GetForwardVector() const noexcept;

		/**
		 * @brief 获取左向向量 / Get left vector
		 * @return XMVECTOR 左向单位向量 / Left unit vector
		 */
		XMVECTOR GetLeftVector() const noexcept;

		/**
		 * @brief 获取上方向量 / Get up vector
		 * @return XMVECTOR 上方向量 / Up vector
		 */
		XMVECTOR GetUp() const noexcept;

		/**
		 * @brief 获取位置 / Get position
		 * @return XMFLOAT3 相机位置 / Camera position
		 */
		XMFLOAT3 GetPosition() const noexcept;

		/**
		 * @brief 获取近裁剪面距离 / Get near clip distance
		 * @return float 近裁剪面 / Near clip plane
		 */
		float GetNearZ() const noexcept;

		/**
		 * @brief 获取远裁剪面距离 / Get far clip distance
		 * @return float 远裁剪面 / Far clip plane
		 */
		float GetFarZ() const noexcept;

		/**
		 * @brief 键盘控制相机位置 / Keyboard control camera position
		 *
		 * 使用 WASD 键控制相机移动。Shift 加速，Ctrl 减速。
		 * Uses WASD keys to control camera movement. Shift to accelerate, Ctrl to decelerate.
		 */
		void ControlCameraPosition() noexcept;

		/**
		 * @brief 鼠标控制相机旋转 / Mouse control camera rotation
		 *
		 * 按住鼠标右键并移动鼠标控制俯仰角和偏航角。
		 * 旋转自动归一化到 [0, 360) 范围防止漂移。
		 * Hold right mouse button and move mouse to control pitch and yaw.
		 * Rotation is automatically normalized to [0, 360) to prevent drift.
		 */
		void ControlCameraRotation() noexcept;

		/**
		 * @brief 序列化相机参数 / Serialize camera parameters
		 * @param filePath 文件路径 / File path
		 */
		void Serialize(const std::string& filePath);

		/**
		 * @brief 反序列化相机参数 / Deserialize camera parameters
		 * @param filePath 文件路径 / File path
		 */
		void Deserialize(const std::string& filePath);

		/**
		 * @brief 保存相机参数（热键触发） / Save camera parameters (hotkey triggered)
		 *
		 * 按住 Ctrl+Shift+S 时保存到指定路径。
		 * Saves to specified path when Ctrl+Shift+S is held.
		 *
		 * @param filePath 文件路径 / File path
		 */
		void Save(const std::string& filePath) noexcept;

		/**
		 * @brief 导入相机参数 / Import camera parameters
		 * @param filePath 文件路径 / File path
		 */
		void Import(const std::string& filePath) noexcept;

		/**
		 * @brief 重置相机到默认状态 / Reset camera to default state
		 */
		void Reset() noexcept;

	private:
		XMFLOAT3 Position = { 0.0f, 0.0f, -20.0f };  ///< 相机位置 / Camera position
		XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };   ///< 旋转角（度）/ Rotation angles (degrees)
		float Aspect = 800.0f / 600.0f;             ///< 宽高比 / Aspect ratio
		float FarZ = 10000.0f;                        ///< 远裁剪面 / Far clip plane
		float NearZ = 0.01f;                          ///< 近裁剪面 / Near clip plane
		POINT LastMousePos = { 0, 0 };               ///< 上一帧鼠标位置 / Last frame mouse position
		bool bFirstMouse = true;                      ///< 是否首次鼠标输入 / Whether this is first mouse input
	};
}

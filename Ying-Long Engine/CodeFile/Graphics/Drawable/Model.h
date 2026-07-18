/** @file Model.h
 *  @brief 模型类 - Model class
 *
 *  包含模型类定义，由多个网格组成，支持序列化和 ImGui 控制。
 *  Contains the model class definition, composed of multiple meshes,
 *  supporting serialization and ImGui control.
 */
#pragma once
#include "Mesh.h"
#include <Windows.h>
#include "../../../yaml-cpp/include/yaml-cpp/yaml.h"

namespace YingLong
{
	/** @brief 模型类
	 *  Model class
	 *
	 *  由多个网格组成的模型对象，管理整体的位置、旋转、缩放变换。
	 *  支持从文件加载模型、YAML 序列化/反序列化以及 ImGui 调试控制。
	 *
	 *  Model object composed of multiple meshes, managing overall position,
	 *  rotation, and scale transformations. Supports loading models from files,
	 *  YAML serialization/deserialization, and ImGui debug control.
	 */
	class Model
	{
	public:
		/** @brief 默认构造函数
		 *  Default constructor
		 */
		Model() noexcept;

		/** @brief 从文件加载构造函数
		 *  Constructor loading from file
		 *
		 *  从指定路径加载模型文件并创建网格。
		 *  Loads a model file from the specified path and creates meshes.
		 *
		 *  @param gfx 图形设备对象引用 / Graphics device object reference
		 *  @param modelFilePath 模型文件路径 / Model file path
		 */
		Model(Graphics& gfx, std::string modelFilePath);

		/** @brief 创建 ImGui 控制窗口
		 *  Spawn ImGui control window
		 *
		 *  创建用于调整模型参数的 ImGui 窗口。
		 *  Creates an ImGui window for adjusting model parameters.
		 *
		 *  @param ImGuiWindowName ImGui 窗口名称 / ImGui window name
		 */
		void SpawnControlWindow(const char* ImGuiWindowName) noexcept;

		/** @brief 绘制模型
		 *  Draw the model
		 *
		 *  遍历所有网格并依次绘制。
		 *  Traverses all meshes and draws them sequentially.
		 *
		 *  @param gfx 图形设备对象引用 / Graphics device object reference
		 */
		void Draw(Graphics& gfx) noexcept; 
		
		/** @brief 序列化模型到文件
		 *  Serialize model to file
		 *
		 *  将模型的变换参数序列化为 YAML 文件。
		 *  Serializes model transformation parameters to a YAML file.
		 *
		 *  @param filePath 输出文件路径 / Output file path
		 */
		void Serialize(const std::string& filePath);

		/** @brief 从文件反序列化模型
		 *  Deserialize model from file
		 *
		 *  从 YAML 文件加载模型的变换参数。
		 *  Loads model transformation parameters from a YAML file.
		 *
		 *  @param filePath 输入文件路径 / Input file path
		 */
		void Deserialize(const std::string& filePath);

		/** @brief 保存模型（快捷键触发）
		 *  Save model (shortcut triggered)
		 *
		 *  当按下 Ctrl+S 时序列化模型到文件。
		 *  Serializes model to file when Ctrl+S is pressed.
		 *
		 *  @param filePath 保存文件路径 / Save file path
		 */
		void Save(const std::string& filePath) noexcept;

		/** @brief 导入模型（文件存在时）
		 *  Import model (when file exists)
		 *
		 *  如果文件存在则从文件反序列化模型。
		 *  Deserializes model from file if the file exists.
		 *
		 *  @param filePath 导入文件路径 / Import file path
		 */
		void Import(const std::string& filePath) noexcept;

		/** @brief 重置模型变换
		 *  Reset model transformation
		 *
		 *  将位置、旋转重置为零，缩放重置为1。
		 *  Resets position and rotation to zero, scale to 1.
		 */
		void Reset() noexcept;

		XMFLOAT3 Position; ///< 位置 / Position
		XMFLOAT3 Rotation; ///< 旋转 / Rotation
		XMFLOAT3 Scale;    ///< 缩放 / Scale

	private:
		std::vector<std::shared_ptr<Mesh>> MeshList; ///< 网格列表 / Mesh list
	};
}

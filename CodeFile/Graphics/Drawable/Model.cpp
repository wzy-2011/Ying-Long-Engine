/** @file Model.cpp
 *  @brief 模型类实现 - Model class implementation
 *
 *  包含 Model 类的成员函数实现。
 *  Contains the member function implementations of the Model class.
 */
#include "Model.h"
#include "../../ImGui/CodeFile/ImGui/imgui.h"

namespace YingLong
{
	/** @brief 默认构造函数
	 *  Default constructor
	 */
	Model::Model() noexcept
	{
		// 初始化变换参数
		// Initialize transformation parameters
		this->Position = { 0.0f, 0.0f, 0.0f };
		this->Rotation = { 0.0f, 0.0f, 0.0f };
		this->Scale = { 1.0f, 1.0f, 1.0f };
	}

	/** @brief 从文件加载构造函数
	 *  Constructor loading from file
	 *
	 *  从指定路径加载模型文件并创建网格。
	 *  Loads a model file from the specified path and creates meshes.
	 *
	 *  @param gfx 图形设备对象引用 / Graphics device object reference
	 *  @param modelFilePath 模型文件路径 / Model file path
	 */
	Model::Model(Graphics& gfx, std::string modelFilePath)
	{
		// 初始化变换参数
		// Initialize transformation parameters
		this->Position = { 0.0f, 0.0f, 0.0f };
		this->Rotation = { 0.f, 0.0f, 0.0f };
		this->Scale = { 1.0f, 1.0f, 1.0f };

		try
		{
			// 加载模型数据 / Load model data
			auto DataList = ModelImporter::LoadModel(modelFilePath);
			// 为每个网格数据创建网格对象 / Create mesh objects for each mesh data
			for (auto& data : DataList)
			{
				auto mesh = std::make_shared<Mesh>(gfx, data);
				this->MeshList.push_back(mesh);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "[Model] Failed to load model '" << modelFilePath
				<< "': " << e.what() << std::endl;
			// 不重新抛出：允许引擎在缺少模型的情况下继续运行
			// Don't rethrow: allow engine to continue without this model
		}
	}

	/** @brief 创建 ImGui 控制窗口
	 *  Spawn ImGui control window
	 *
	 *  创建用于调整模型参数的 ImGui 窗口。
	 *  Creates an ImGui window for adjusting model parameters.
	 *
	 *  @param ImGuiWindowName ImGui 窗口名称 / ImGui window name
	 */
	void Model::SpawnControlWindow(const char* ImGuiWindowName) noexcept
	{
		// 开始 ImGui 窗口
		// Begin ImGui window
		ImGui::Begin(ImGuiWindowName);

		// 位置控制
		// Position control
		ImGui::Text("Position");
		ImGui::DragFloat("PositionX", &this->Position.x, 0.1f);
		ImGui::DragFloat("PositionY", &this->Position.y, 0.1f);
		ImGui::DragFloat("PositionZ", &this->Position.z, 0.1f);

		// 旋转控制
		// Rotation control
		ImGui::Text("Rotation");
		ImGui::DragFloat("RotationX", &this->Rotation.x, 0.1f);
		ImGui::DragFloat("RotationY", &this->Rotation.y, 0.1f);
		ImGui::DragFloat("RotationZ", &this->Rotation.z, 0.1f);

		// 缩放控制
		// Scale control
		ImGui::Text("Scale");
		ImGui::SliderFloat("ScaleX", &this->Scale.x, -10.0f, +10.0f);
		ImGui::SliderFloat("ScaleY", &this->Scale.y, -10.0f, +10.0f);
		ImGui::SliderFloat("ScaleZ", &this->Scale.z, -10.0f, +10.0f);

		// 重置按钮
		// Reset button
		if (ImGui::Button("Defaut")) { this->Reset(); }

		// 结束 ImGui 窗口
		// End ImGui window
		ImGui::End();
	}

	/** @brief 绘制模型
	 *  Draw the model
	 *
	 *  遍历所有网格并依次绘制。
	 *  Traverses all meshes and draws them sequentially.
	 *
	 *  @param gfx 图形设备对象引用 / Graphics device object reference
	 */
	void Model::Draw(Graphics& gfx) noexcept
	{
		// 遍历所有网格，应用变换并绘制
		// Traverse all meshes, apply transform and draw
		for (auto& mesh : MeshList)
		{
			// 将模型的变换传递给每个网格
			// Pass model's transform to each mesh
			mesh->Position = this->Position;
			mesh->Rotation = this->Rotation;
			mesh->Scale = this->Scale;
				
			mesh->Draw(gfx);
		}
	}

	/** @brief 序列化模型到文件
	 *  Serialize model to file
	 *
	 *  将模型的变换参数序列化为 YAML 文件。
	 *  Serializes model transformation parameters to a YAML file.
	 *
	 *  @param filePath 输出文件路径 / Output file path
	 */
	void Model::Serialize(const std::string& filePath)
	{
		// 创建 YAML 输出器
		// Create YAML emitter
		YAML::Emitter out;

		// 开始映射
		// Begin map
		out << YAML::BeginMap;

		// 写入位置、旋转、缩放
		// Write position, rotation, scale
		out << YAML::Key << "Position" << YAML::Value << this->Position;
		out << YAML::Key << "Rotation" << YAML::Value << this->Rotation;
		out << YAML::Key << "Scale" << YAML::Value << this->Scale;

		// 结束映射
		// End map
		out << YAML::EndMap;

		// 打开输出文件
		// Open output file
		std::ofstream FileOutput(filePath);
		if (!FileOutput.is_open())
		{
			throw std::runtime_error("Couldn't output the file!(Model)");
		}

		// 写入文件
		// Write to file
		std::stringstream FileStringStream(out.c_str());
		FileOutput << FileStringStream.rdbuf();

		// 关闭文件
		// Close file
		FileOutput.close();
	}

	/** @brief 从文件反序列化模型
	 *  Deserialize model from file
	 *
	 *  从 YAML 文件加载模型的变换参数。
	 *  Loads model transformation parameters from a YAML file.
	 *
	 *  @param filePath 输入文件路径 / Input file path
	 */
	void Model::Deserialize(const std::string& filePath)
	{
		// 打开输入文件
		// Open input file
		std::ifstream FileInput(filePath);
		std::stringstream FileStringStream;
		FileStringStream << FileInput.rdbuf();
		// 加载 YAML 数据
		// Load YAML data
		auto ModelData = YAML::Load(FileStringStream);

		// 读取位置、旋转、缩放
		// Read position, rotation, scale
		this->Position = ModelData["Position"].as<XMFLOAT3>();
		this->Rotation = ModelData["Rotation"].as<XMFLOAT3>();
		this->Scale = ModelData["Scale"].as<XMFLOAT3>();
	}

	/** @brief 保存模型（快捷键触发）
	 *  Save model (shortcut triggered)
	 *
	 *  当按下 Ctrl+S 时序列化模型到文件。
	 *  Serializes model to file when Ctrl+S is pressed.
	 *
	 *  @param filePath 保存文件路径 / Save file path
	 */
	void Model::Save(const std::string& filePath) noexcept
	{
		// 检测 Ctrl+S 快捷键
		// Detect Ctrl+S shortcut
		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('S')) { this->Serialize(filePath); }
	}

	/** @brief 导入模型（文件存在时）
	 *  Import model (when file exists)
	 *
	 *  如果文件存在则从文件反序列化模型。
	 *  Deserializes model from file if the file exists.
	 *
	 *  @param filePath 导入文件路径 / Import file path
	 */
	void Model::Import(const std::string& filePath) noexcept
	{
		// 如果文件存在则反序列化
		// Deserialize if file exists
		if (std::filesystem::exists(filePath))
		{
			this->Deserialize(filePath);
		}
	}

	/** @brief 重置模型变换
	 *  Reset model transformation
	 *
	 *  将位置、旋转重置为零，缩放重置为1。
	 *  Resets position and rotation to zero, scale to 1.
	 */
	void Model::Reset() noexcept
	{
		// 重置为默认值
		// Reset to default values
		this->Position = { };
		this->Rotation = { };
		this->Scale = { 1.0f, 1.0f, 1.0f };
	}
}

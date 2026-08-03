#include "PointLight.h"
#include "LightManager.h"

namespace YingLong
{
	PointLight::PointLight(const PointLight& other) : mesh(other.mesh)
	{
		this->LightData = other.LightData;
	}

	PointLight::PointLight(Graphics& graphics, float radius) : mesh(graphics, radius, this->LightData.Color)
	{

	}

	void PointLight::SpawnControlWindow(const char* PointLightName) noexcept
	{
		if (ImGui::Begin(PointLightName))
		{
			float position[3] = {
				this->LightData.Position.x,
				this->LightData.Position.y,
				this->LightData.Position.z };
			if (ImGui::DragFloat3("位置", position, 0.1f))
			{
				this->LightData.Position = { position[0], position[1], position[2] };
			}
			float color[3] = {
				this->LightData.Color.x,
				this->LightData.Color.y,
				this->LightData.Color.z };
			if (ImGui::DragFloat3("颜色", color, 0.01f, 0.0f, 1.0f))
			{
				this->LightData.Color = { color[0], color[1], color[2] };
			}
			ImGui::DragFloat("强度", &this->LightData.Intensity, 10.0f, 0.0f, 100000.0f);

			if (ImGui::Button("默认")) { Reset(); }
		}
		ImGui::End();
	}

	void PointLight::Reset() noexcept
	{
		this->LightData = { };
	}

	void PointLight::Serialize(const std::string& filePath) const
	{
		const Data& data = this->LightData;

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Position" << YAML::Value << data.Position;
		out << YAML::Key << "Color" << YAML::Value << data.Color;
		out << YAML::Key << "Intensity" << YAML::Value << data.Intensity;

		out << YAML::EndMap;

		std::ofstream FileOutput(filePath);
		if (!FileOutput.is_open())
		{
			std::cout << "Couldn't output the file!(Point Light)";
		}

		std::stringstream FileStringStream(out.c_str());
		FileOutput << FileStringStream.rdbuf();

		FileOutput.close();
	}

	void PointLight::Deserialize(const std::string& filePath)
	{
		std::ifstream FileInput(filePath);
		std::stringstream FileStringStream;
		FileStringStream << FileInput.rdbuf();
		auto PointLightData = YAML::Load(FileStringStream);

		this->LightData.Color = PointLightData["color"].as<XMFLOAT3>();
		this->LightData.Position = PointLightData["position"].as<XMFLOAT3>();
		this->LightData.Intensity = PointLightData["intensity"].as<float>();
	}

	void PointLight::Save(const std::string filePath) const noexcept
	{
		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('S'))
		{
			this->Serialize(filePath);
		}
	}

	void PointLight::Import(const std::string& filePath) noexcept
	{
		if (std::filesystem::exists(filePath)) { this->Deserialize(filePath); }
	}

	void PointLight::Draw(Graphics& graphics) const noexcept
	{
		mesh.SetPosition(this->LightData.Position);
		mesh.SetColor(this->LightData.Color);
		mesh.Draw(graphics);
	}

	void PointLight::Bind() const noexcept
	{
		LightManager::SubmitPointLight(*this);
	}
}

#include "Saver.h"

namespace YingLong
{
	void Saver::SaveFloat(float something, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "float" << YAML::Value << something;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(float)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Saver::SaveFloat3(XMFLOAT3 something, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "XMFLOAT3" << YAML::Value << something;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(XMFLOAT3)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Saver::SavePointLightSth(XMFLOAT3 color, XMFLOAT3 position, float intensity, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "color" << YAML::Value << color;
			out << YAML::Key << "position" << YAML::Value << position;
			out << YAML::Key << "intensity" << YAML::Value << intensity;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(Point Light)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Saver::SaveSpotLightSth(XMFLOAT3 position, XMFLOAT3 color, XMFLOAT3 rotation,
		float intensity, float InnerConeAngle, float OuterConeAngle, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "color" << YAML::Value << color;
			out << YAML::Key << "position" << YAML::Value << position;
			out << YAML::Key << "rotation" << YAML::Value << rotation;
			out << YAML::Key << "intensity" << YAML::Value << intensity;
			out << YAML::Key << "InnerConeAngle" << YAML::Value << InnerConeAngle;
			out << YAML::Key << "OuterConeAngle" << YAML::Value << OuterConeAngle;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(Spot Light)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Saver::SaveModelSth(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "Position" << YAML::Value << position;
			out << YAML::Key << "Rotation" << YAML::Value << rotation;
			out << YAML::Key << "Scale" << YAML::Value << scale;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(Model)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Saver::SaveCubeSth(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 color, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "position" << YAML::Value << position;
			out << YAML::Key << "rotation" << YAML::Value << rotation;
			out << YAML::Key << "color" << YAML::Value << color;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(Cube)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}

	void Saver::SaveCapsuleSth(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 color,
		float HalfHeight, float radius, const std::string name)
	{
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_SHIFT))
		{
			YAML::Emitter out;

			out << YAML::BeginMap;

			out << YAML::Key << "position" << YAML::Value << position;
			out << YAML::Key << "rotation" << YAML::Value << rotation;
			out << YAML::Key << "color" << YAML::Value << color;

			out << YAML::EndMap;

			std::ofstream FileOutput(name);
			if (!FileOutput.is_open())
			{
				std::cout << "Couldn't output the file!(Cube)\n";
			}

			std::stringstream FileStringStream(out.c_str());
			FileOutput << FileStringStream.rdbuf();

			FileOutput.close();
		}
	}
}

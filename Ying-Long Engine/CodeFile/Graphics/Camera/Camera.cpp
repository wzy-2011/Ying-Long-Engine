#include "Camera.h"

using namespace DirectX;

namespace YingLong
{
	FXMMATRIX Camera::GetMatrix() const noexcept
	{
		auto pos = XMLoadFloat3(&this->Position);

		auto focus = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
			XMMatrixRotationRollPitchYaw(
				(this->Rotation.x / 360.0f) * XM_2PI,
				(this->Rotation.y / 360.0f) * XM_2PI,
				(this->Rotation.z / 360.0f) * XM_2PI));
		focus += pos;
		return XMMatrixLookAtLH(
			pos, focus, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	}

	FXMMATRIX Camera::GetProjection() const noexcept
	{
		return XMMatrixPerspectiveFovLH((60.0f / 360.0f) * XM_2PI,
			this->Aspect,
			this->NearZ, this->FarZ);
	}

	void Camera::SpawnControlWindow(const char* CameraName) noexcept
	{
		ImGui::Begin(CameraName);
		ImGui::Text("位置");
		ImGui::DragFloat("位置X", &this->Position.x, 0.1f);
		ImGui::DragFloat("位置Y", &this->Position.y, 0.1f);
		ImGui::DragFloat("位置Z", &this->Position.z, 0.1f);

		ImGui::Text("旋转");
		ImGui::SliderFloat("旋转X", &this->Rotation.x, -360.0f, 360.0f);
		ImGui::SliderFloat("旋转Y", &this->Rotation.y, -360.0f, 360.0f);
		ImGui::SliderFloat("旋转Z", &this->Rotation.z, -360.0f, 360.0f);

		if (ImGui::Button("默认"))
		{
			Reset();
		}

		ImGui::End();
	}

	void Camera::Translate(XMFLOAT3 translation) noexcept
	{
		this->Position.x += translation.x;
		this->Position.y += translation.y;
		this->Position.z += translation.z;
	}

	void Camera::Translate(XMVECTOR translation) noexcept
	{
		XMFLOAT3 FTranslation;
		XMStoreFloat3(&FTranslation, translation);

		this->Position.x += FTranslation.x;
		this->Position.y += FTranslation.y;
		this->Position.z += FTranslation.z;
	}

	void Camera::Rotate(XMFLOAT3 rotation) noexcept
	{
		this->Rotation.x += rotation.x;
		this->Rotation.y += rotation.y;
		this->Rotation.z += rotation.z;
	}

	void Camera::Rotate(XMVECTOR rotation) noexcept
	{
		XMFLOAT3 FRotation;
		XMStoreFloat3(&FRotation, rotation);

		this->Rotation.x += FRotation.x;
		this->Rotation.y += FRotation.y;
		this->Rotation.z += FRotation.z;
	}

	void Camera::SetResolution(XMFLOAT2 resolution)
	{
		this->Aspect = resolution.x / resolution.y;
	}

	XMVECTOR Camera::GetForwardVector() const noexcept
	{
		XMVECTOR Forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

		Forward = XMVector3TransformCoord(Forward,
			XMMatrixRotationRollPitchYaw(
				(this->Rotation.x / 360.0f) * XM_2PI,
				(this->Rotation.y / 360.0f) * XM_2PI,
				(this->Rotation.z / 360.0f) * XM_2PI));
		return Forward;
	}

	XMVECTOR Camera::GetLeftVector() const noexcept
	{
		XMVECTOR Left = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);

		Left = XMVector3TransformCoord(Left,
			XMMatrixRotationRollPitchYaw(
				(this->Rotation.x / 360.0f) * XM_2PI,
				(this->Rotation.y / 360.0f) * XM_2PI,
				(this->Rotation.z / 360.0f) * XM_2PI));
		return Left;
	}

	XMVECTOR Camera::GetUp() const noexcept
	{
		XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		Up = XMVector3TransformCoord(Up,
			XMMatrixRotationRollPitchYaw(
				(this->Rotation.x / 360.0f) * XM_2PI,
				(this->Rotation.y / 360.0f) * XM_2PI,
				(this->Rotation.z / 360.0f) * XM_2PI));
		return Up;
	}

	XMFLOAT3 Camera::GetPosition() const noexcept
	{
		return this->Position;
	}

	float Camera::GetNearZ() const noexcept
	{
		return this->NearZ;
	}

	float Camera::GetFarZ() const noexcept
	{
		return this->FarZ;
	}

	void Camera::ControlCameraPosition() noexcept
	{
		if (GetAsyncKeyState('W') && GetAsyncKeyState(VK_RBUTTON))
			this->Translate(this->GetForwardVector() * 0.3f);
		if (GetAsyncKeyState('S') && GetAsyncKeyState(VK_RBUTTON))
			this->Translate(this->GetForwardVector() * -0.3f);
		if (GetAsyncKeyState('A') && GetAsyncKeyState(VK_RBUTTON))
			this->Translate(this->GetLeftVector() * 0.3f);
		if (GetAsyncKeyState('D') && GetAsyncKeyState(VK_RBUTTON))
			this->Translate(this->GetLeftVector() * -0.3f);
		if (GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_RBUTTON))
			this->Translate(XMFLOAT3{ 0.0f, 0.3f, 0.0f });
		if (GetAsyncKeyState(VK_SHIFT) &&GetAsyncKeyState(VK_RBUTTON))
			this->Translate(XMFLOAT3{ 0.0f, -0.3f, 0.0f });
	}

	void Camera::ControlCameraRotation() noexcept
	{
		bool bHasInput = false;

		if (GetAsyncKeyState(VK_RBUTTON))
		{
			POINT currentPos;
			GetCursorPos(&currentPos);

			if (bFirstMouse)
			{
				LastMousePos = currentPos;
				bFirstMouse = false;
			}

			float dx = (float)(currentPos.x - LastMousePos.x);
			float dy = (float)(currentPos.y - LastMousePos.y);

			this->Rotation.y += dx * 0.1f;
			this->Rotation.x += dy * 0.1f;

			LastMousePos = currentPos;
			bHasInput = true;
		}
		else if (GetAsyncKeyState(VK_MBUTTON))
		{
			POINT currentPos;
			GetCursorPos(&currentPos);

			if (bFirstMouse)
			{
				LastMousePos = currentPos;
				bFirstMouse = false;
			}

			float dx = (float)(currentPos.x - LastMousePos.x);

			this->Rotation.z += dx * 0.1f;

			LastMousePos = currentPos;
			bHasInput = true;
		}
		else
		{
			bFirstMouse = true;
		}

		if (GetAsyncKeyState(VK_LEFT))
		{
			this->Rotation.y -= 0.5f;
			bHasInput = true;
		}
		if (GetAsyncKeyState(VK_RIGHT))
		{
			this->Rotation.y += 0.5f;
			bHasInput = true;
		}
		if (GetAsyncKeyState(VK_UP))
		{
			this->Rotation.x -= 0.5f;
			bHasInput = true;
		}
		if (GetAsyncKeyState(VK_DOWN))
		{
			this->Rotation.x += 0.5f;
			bHasInput = true;
		}

		if (bHasInput)
		{
			auto wrap = [](float deg) noexcept -> float {
				deg = fmodf(deg, 360.0f);
				if (deg < 0.0f) deg += 360.0f;
				return deg;
			};
			this->Rotation.x = wrap(this->Rotation.x);
			this->Rotation.y = wrap(this->Rotation.y);
			this->Rotation.z = wrap(this->Rotation.z);
		}
	}

	void Camera::Serialize(const std::string& filePath)
	{
		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Position" << YAML::Value << this->Position;
		out << YAML::Key << "Rotation" << YAML::Value << this->Rotation;

		out << YAML::EndMap;

		std::ofstream FileOutput(filePath);
		if (!FileOutput.is_open())
		{
			throw std::runtime_error("Couldn't output the file!(Camera)");
		}

		std::stringstream FileStringStream(out.c_str());
		FileOutput << FileStringStream.rdbuf();

		FileOutput.close();
	}

	void Camera::Deserialize(const std::string& filePath)
	{
		std::ifstream FileInput(filePath);
		std::stringstream FileStringStream;
		FileStringStream << FileInput.rdbuf();
		auto CameraData = YAML::Load(FileStringStream);

		this->Position = CameraData["Position"].as<XMFLOAT3>();
		this->Rotation = CameraData["Rotation"].as<XMFLOAT3>();
	}

	void Camera::Save(const std::string& filePath) noexcept
	{
		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('S')) { this->Serialize(filePath); }
	}

	void Camera::Import(const std::string& filePath) noexcept
	{
		if (std::filesystem::exists(filePath))
		{
			this->Deserialize(filePath);
		}
	}

	void Camera::Reset() noexcept
	{
		this->Position.x = 0.0f;
		this->Position.y = 0.0f;
		this->Position.z = 0.0f;

		this->Rotation.x = 0.0f;
		this->Rotation.y = 0.0f;
		this->Rotation.z = 0.0f;
	}
}

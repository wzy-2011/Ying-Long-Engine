#include "SpotLight.h"
#include "LightManager.h"

namespace YingLong
{
	SpotLight::SpotLight(const SpotLight& other) : mesh(other.mesh)
	{
		this->LightData = other.LightData;
	}

	SpotLight::SpotLight(Graphics& graphics, float coneHeight, float coneRadius)
		: mesh(graphics, coneHeight, coneRadius, this->LightData.Color)
	{

	}

	void SpotLight::SpawnControlWindow(const char* SpotLightName) noexcept
	{
		ImGui::Begin(SpotLightName);
		float position[3] = {
			this->LightData.Position.x,
			this->LightData.Position.y,
			this->LightData.Position.z
		};
		if (ImGui::DragFloat3("位置", position, 0.1f))
		{
			this->LightData.Position = { position[0], position[1], position[2] };
		}
		
		float color[3] = {
			this->LightData.Color.x,
			this->LightData.Color.y,
			this->LightData.Color.z
		};
		if (ImGui::DragFloat3("颜色", color, 0.01f, 0.0f, 1.0f))
		{
			this->LightData.Color = { color[0], color[1], color[2] };
		}

		float rotation[3] = {
			this->LightData.Rotation.x,
			this->LightData.Rotation.y,
			this->LightData.Rotation.z
		};
		if (ImGui::DragFloat3("旋转 (度)", rotation, 1.0f, -360.0f, 360.0f))
		{
			this->LightData.Rotation = { rotation[0], rotation[1], rotation[2] };
		}

		ImGui::DragFloat("强度", &this->LightData.Intensity, 100.0f, 0.0f, 1000000.0f);

		float outerDeg = this->LightData.OuterConeAngle * 180.0f / XM_PI;
		float innerDeg = this->LightData.InnerConeAngle * 180.0f / XM_PI;
		if (ImGui::SliderFloat("外角", &outerDeg, 1.0f, 179.0f, "%.1f 度"))
		{
			this->LightData.OuterConeAngle = outerDeg * XM_PI / 180.0f;
			if (this->LightData.InnerConeAngle > this->LightData.OuterConeAngle)
				this->LightData.InnerConeAngle = this->LightData.OuterConeAngle;
		}
		if (ImGui::SliderFloat("内角", &innerDeg, 0.0f, outerDeg, "%.1f 度"))
		{
			this->LightData.InnerConeAngle = innerDeg * XM_PI / 180.0f;
		}

		if (ImGui::Button("默认")) { this->Reset(); }

		ImGui::End();
	}

	void SpotLight::Draw(Graphics& graphics) const noexcept
	{
		mesh.SetPosition(this->LightData.Position);
		// 将 Rotation（度）转换为弧度后传递给锥体，使锥体指向聚光灯方向
		// Convert Rotation (degrees) to radians and pass to cone so it points in the spotlight direction
		mesh.SetRotation(XMFLOAT3{
			this->LightData.Rotation.x / 360.0f * XM_2PI,
			this->LightData.Rotation.y / 360.0f * XM_2PI,
			this->LightData.Rotation.z / 360.0f * XM_2PI });
		mesh.SetColor(this->LightData.Color);

		// 根据外锥角动态调整锥体底面半径（通过 scale 变换，无需重建顶点缓冲区）
		// Dynamically adjust cone base radius via scale transform (no vertex buffer rebuild needed)
		// 原始锥体几何体: height=3.0, radius=0.5 (构造时传入)
		// 需要的底面半径 = visualLength * tan(OuterConeAngle)
		const float visualLength = 3.0f;
		const float originalRadius = 0.5f;
		float neededRadius = visualLength * tanf(this->LightData.OuterConeAngle);
		float radiusScale = neededRadius / originalRadius;
		mesh.SetScale(XMFLOAT3{ radiusScale, radiusScale, 1.0f });

		mesh.Draw(graphics);
	}

	void SpotLight::Bind() const noexcept
	{
		LightManager::SubmitSpotLight(*this);
	}

	void SpotLight::Reset() noexcept
	{
		this->LightData = { };
	}

	void SpotLight::UpdateConeAngle() noexcept
	{
		// 锥体可视长度固定为 3.0 单位 / Visual cone length is fixed at 3.0 units
		const float visualLength = 3.0f;
		float halfAngle = this->LightData.OuterConeAngle;
		float radius = visualLength * tanf(halfAngle);
		// 更新锥体几何体以匹配当前外锥角
		// Update cone geometry to match current outer cone angle
		mesh.UpdateAngle(visualLength, radius);
	}
}
#include "LightManager.h"
#include "../../Application/Application.h"

namespace YingLong
{
	std::vector<PointLight> LightManager::pPointLightList;
	PixelConstantBuffer<PointLight::ConstantBuffer> LightManager::pPointLightConstantBuffer;

	std::vector<SpotLight> LightManager::pSpotLightList;
	PixelConstantBuffer<SpotLight::ConstantBuffer> LightManager::pSpotLightConstantBuffer;

	void LightManager::Initialize(Graphics& graphics)
	{
		// 绑定到 b3/b4 槽位，避免与 b0(LightCountCB) 和 b1(MaterialCB) 冲突
		// Bind to b3/b4 slots to avoid conflict with b0(LightCountCB) and b1(MaterialCB)
		pPointLightConstantBuffer = PixelConstantBuffer<PointLight::ConstantBuffer>(graphics, 3);
		pSpotLightConstantBuffer = PixelConstantBuffer<SpotLight::ConstantBuffer>(graphics, 4);
	}

	void LightManager::SubmitPointLight(PointLight pointLight)
	{
		pPointLightList.push_back(pointLight);

		if (pPointLightList.size() > 50)
		{
			std::cout << "Point light count has reached the maximum supported limit." << std::endl;
		}
	}

	void LightManager::SubmitSpotLight(SpotLight spotLight)
	{
		pSpotLightList.push_back(spotLight);

		if (pSpotLightList.size() > 50)
		{
			std::cout << "Spot light count has reached the maximum supported limit." << std::endl;
		}
	}
	
	void LightManager::Update(Graphics& graphics)
	{
		auto& pointLigtData = pPointLightConstantBuffer.CBufferData;
		auto& spotLightData = pSpotLightConstantBuffer.CBufferData;

		for (UINT i = 0; i < (UINT)pPointLightList.size(); i++)
		{
			pointLigtData.PointLightList[i] = pPointLightList[i].LightData;
		}
		pointLigtData.PointLightCount = (int)pPointLightList.size();
		pointLigtData.CameraPosition = Application::Instance->MainWindow.camera.GetPosition();

		for (UINT i = 0; i < (UINT)pSpotLightList.size(); i++)
		{
			spotLightData.SpotLightList[i] = pSpotLightList[i].LightData;
			auto& rotation = spotLightData.SpotLightList[i].Rotation;
			XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x / 360.0f * XM_2PI, rotation.y / 360.0f * XM_2PI, rotation.z / 360.0f * XM_2PI);
			XMVECTOR Length = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			XMVECTOR Direction = XMVector3Transform(Length, RotationMatrix);
			Direction = XMVector3Normalize(Direction);
			XMStoreFloat3(&spotLightData.SpotLightList[i].Direction, Direction);

			spotLightData.SpotLightList[i].OuterConeAngle = cosf(spotLightData.SpotLightList[i].OuterConeAngle);
			spotLightData.SpotLightList[i].InnerConeAngle = cosf(spotLightData.SpotLightList[i].InnerConeAngle);
		}
		spotLightData.SpotLightCount = (int)pSpotLightList.size();

		pPointLightList.clear();
		pSpotLightList.clear();

		pPointLightConstantBuffer.Update(graphics, pointLigtData);
		pPointLightConstantBuffer.Bind(graphics);
		pSpotLightConstantBuffer.Update(graphics, spotLightData);
		pSpotLightConstantBuffer.Bind(graphics);
	}

	const std::vector<PointLight>& LightManager::GetPointLightList() noexcept
	{
		return pPointLightList;
	}

	PixelConstantBuffer<PointLight::ConstantBuffer> LightManager::GetPointLightConstantBuffer() noexcept
	{
		return pPointLightConstantBuffer;
	}

	const std::vector<SpotLight>& LightManager::GetSpotLightList() noexcept
	{
		return pSpotLightList;
	}

	PixelConstantBuffer<SpotLight::ConstantBuffer> LightManager::GetSpotLightConstantBuffer() noexcept
	{
		return pSpotLightConstantBuffer;
	}
}

/**
 * @file SpotLight.h
 * @brief 聚光灯类 / Spot light class
 *
 * 表示 3D 场景中的聚光灯。包含位置、方向、颜色、强度、
 * 内外锥角等属性。使用 SolidSphereDrawable 可视化光源位置，
 * 支持 ImGui 控制面板和 YAML 序列化/反序列化。
 *
 * Represents a spot light in the 3D scene. Contains position, direction,
 * color, intensity, inner/outer cone angles and other properties.
 * Uses SolidSphereDrawable to visualize the light position, supports
 * ImGui control panel and YAML serialization/deserialization.
 *
 * @note 这是 DX11 路径的聚光灯类。DX12 路径使用独立的
 *       DX12SpotLightState/DX12SpotLightCB。
 *       This is the DX11 path spot light class. The DX12 path uses
 *       independent DX12SpotLightState/DX12SpotLightCB.
 */
#pragma once
#include <filesystem>
#include "../../../yaml-cpp/include/yaml-cpp/yaml.h"
#include <fstream>
#include <sstream>
#include "../Graphics.h"
#include "../Bindable/ConstantBuffers.h"
#include "../Drawable/SolidCone.h"

namespace YingLong
{
	class LightManager;

	/**
	 * @brief 聚光灯类 / Spot light class
	 *
	 * 聚光灯从一个点向特定方向发射锥形光束。
	 * 内锥角内强度最大，外锥角外强度为 0，中间平滑衰减。
	 *
	 * Spot lights emit a cone of light from a point in a specific direction.
	 * Intensity is maximum inside the inner cone, zero outside the outer cone,
	 * and smoothly attenuates in between.
	 */
	class SpotLight
	{
	public:
		/**
		 * @brief 聚光灯数据结构体 / Spot light data structure
		 *
		 * 布局需与着色器中的 cbuffer 匹配。
		 * Layout must match the cbuffer in the shader.
		 */
		struct Data
		{
			Data()
			{
				this->Position = { 0.0f, 0.0f, 0.0f };
				this->Color = { 1.0f, 1.0f, 1.0f };
				this->Intensity = 10000.0f;
				this->InnerConeAngle = 30.0f / 360.0f * XM_2PI;
				this->Direction = { 1.0f, 0.0f, 0.0f };
				this->OuterConeAngle = 45.0f / 360.0f * XM_2PI;
				this->Rotation = { 0.0f, 0.0f, 0.0f };
				this->pad = 0.0f;
			}

		public:
			XMFLOAT3 Position;          ///< 光源位置 / Light position
			float Intensity;            ///< 光强 / Light intensity
			XMFLOAT3 Color;             ///< 光源颜色 / Light color
			float InnerConeAngle;       ///< 内锥角（弧度） / Inner cone angle (radians)
			XMFLOAT3 Direction;         ///< 光照方向（归一化向量） / Light direction (normalized vector)
			float OuterConeAngle;       ///< 外锥角（弧度） / Outer cone angle (radians)
			XMFLOAT3 Rotation;          ///< 旋转（度，用于UI编辑） / Rotation (degrees, used for UI editing)
			float pad;                  ///< 16字节对齐填充 / 16-byte alignment padding
		};

		/**
		 * @brief 聚光灯常量缓冲区 / Spot light constant buffer
		 *
		 * 包含最多 50 个聚光灯的数据，用于着色器计算光照。
		 * Contains data for up to 50 spot lights, used by the shader
		 * to compute lighting.
		 */
		struct ConstantBuffer
		{
		public:
			Data SpotLightList[50];   ///< 聚光灯数组 / Spot light array
			int SpotLightCount;       ///< 聚光灯数量 / Spot light count
		private:
			float padding[3];         ///< 16字节对齐填充 / 16-byte alignment padding
		};

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 * @param other 源对象 / Source object
		 */
		SpotLight(const SpotLight& other);

		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备 / Graphics device
		 * @param coneHeight 可视化锥体高度 / Visualization cone height
		 * @param coneRadius 可视化锥体底面半径 / Visualization cone base radius
		 */
		SpotLight(Graphics& graphics, float coneHeight = 3.0f, float coneRadius = 0.5f);

		/**
		 * @brief 生成 ImGui 控制面板 / Spawn ImGui control window
		 * @param SpotLightName 面板名称 / Panel name
		 */
		void SpawnControlWindow(const char* SpotLightName) noexcept;

		/**
		 * @brief 绘制光源可视化 / Draw light visualization
		 * @param graphics 图形设备 / Graphics device
		 */
		void Draw(Graphics& graphics) const noexcept;

		/**
		 * @brief 绑定光源数据到渲染管线 / Bind light data to render pipeline
		 */
		void Bind() const noexcept;

		/**
		 * @brief 重置光源到默认状态 / Reset light to default state
		 */
		void Reset() noexcept;

		/**
		 * @brief 更新锥体可视化角度 / Update cone visualization angle
		 *
		 * 根据当前外锥角重新生成锥体几何体。
		 * Regenerates cone geometry based on current outer cone angle.
		 */
		void UpdateConeAngle() noexcept;

		Data LightData;  ///< 光源数据 / Light data

	private:
		mutable SolidConeDrawable mesh;  ///< 可视化锥体网格 / Visualization cone mesh
	};
}

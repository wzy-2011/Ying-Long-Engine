/**
 * @file Saver.h
 * @brief 场景数据保存器 / Scene Data Saver
 *
 * 提供将场景数据保存到 YAML 文件的功能，支持保存浮点数、
 * 三维向量、点光源、聚光灯、模型、立方体和胶囊体等数据。
 *
 * Provides functionality for saving scene data to YAML files, supporting
 * saving of floats, 3D vectors, point lights, spot lights, models,
 * cubes, and capsules.
 */

#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../../yaml-cpp/include/yaml-cpp/yaml.h"

using namespace DirectX;

namespace YingLong
{
    /**
     * @brief 场景数据保存器类 / Scene data saver class
     *
     * 负责将各种场景对象数据保存到 YAML 格式文件中。
     * 支持保存浮点数、三维向量以及多种场景对象（光源、模型、几何体等）。
     *
     * Responsible for saving various scene object data to YAML format files.
     * Supports saving floats, 3D vectors, and various scene objects
     * (lights, models, geometries, etc.).
     */
    class Saver
    {
    public:
        /**
         * @brief 默认构造函数 / Default constructor
         */
        Saver() = default;

        /**
         * @brief 保存浮点数 / Save a float value
         * @param something 要保存的浮点数值 / Float value to save
         * @param name 数据项名称 / Data item name
         */
        void SaveFloat(float something, const std::string name);

        /**
         * @brief 保存三维浮点向量 / Save a 3D float vector
         * @param something 要保存的三维向量 / 3D vector to save
         * @param name 数据项名称 / Data item name
         */
        void SaveFloat3(XMFLOAT3 something, const std::string name);

        /**
         * @brief 保存点光源数据 / Save point light data
         * @param color 点光源颜色 / Point light color
         * @param position 点光源位置 / Point light position
         * @param intensity 点光源强度 / Point light intensity
         * @param name 光源名称 / Light name
         */
        void SavePointLightSth(XMFLOAT3 color, XMFLOAT3 position, float intensity, const std::string name);

        /**
         * @brief 保存聚光灯数据 / Save spot light data
         * @param position 聚光灯位置 / Spot light position
         * @param color 聚光灯颜色 / Spot light color
         * @param rotation 聚光灯旋转角度 / Spot light rotation
         * @param intensity 聚光灯强度 / Spot light intensity
         * @param InnerConeAngle 内锥角（弧度）/ Inner cone angle (radians)
         * @param OuterConeAngle 外锥角（弧度）/ Outer cone angle (radians)
         * @param name 光源名称 / Light name
         */
        void SaveSpotLightSth(XMFLOAT3 position, XMFLOAT3 color, XMFLOAT3 rotation,
            float intensity, float InnerConeAngle, float OuterConeAngle, const std::string name);

        /**
         * @brief 保存模型变换数据 / Save model transform data
         * @param position 模型位置 / Model position
         * @param rotation 模型旋转角度 / Model rotation
         * @param scale 模型缩放 / Model scale
         * @param name 模型名称 / Model name
         */
        void SaveModelSth(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale, const std::string name);

        /**
         * @brief 保存立方体数据 / Save cube data
         * @param position 立方体位置 / Cube position
         * @param rotation 立方体旋转角度 / Cube rotation
         * @param color 立方体颜色 / Cube color
         * @param name 立方体名称 / Cube name
         */
        void SaveCubeSth(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 color, const std::string name);

        /**
         * @brief 保存胶囊体数据 / Save capsule data
         * @param position 胶囊体位置 / Capsule position
         * @param rotation 胶囊体旋转角度 / Capsule rotation
         * @param color 胶囊体颜色 / Capsule color
         * @param HalfHeight 胶囊体半高 / Capsule half height
         * @param radius 胶囊体半径 / Capsule radius
         * @param name 胶囊体名称 / Capsule name
         */
        void SaveCapsuleSth(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 color,
            float HalfHeight, float radius, const std::string name);
    };
}

/**
 * @file System.cpp
 * @brief ECS 系统基类实现 / ECS system base class implementation
 *
 * 实现 System 基类的默认空实现，子类可选择性重写。
 * Implements default empty implementations of the System base class;
 * subclasses can optionally override.
 */
#include "System.h"
#include "../Scene/Scene.h"
#include "../Entity/Entity.h"

namespace YingLong
{
	void System::UpdateScene(Scene& scene, float DeltaTime)
	{
		// 默认空实现，子类按需重写
		// Default empty implementation; subclasses override as needed
	}

	void System::Initialize()
	{
		// 默认空实现，子类按需重写
		// Default empty implementation; subclasses override as needed
	}

	void System::ShutDown()
	{
		// 默认空实现，子类按需重写
		// Default empty implementation; subclasses override as needed
	}
}

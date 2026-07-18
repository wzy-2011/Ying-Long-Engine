/**
 * @file Physics.h
 * @brief PhysX 物理系统全局初始化 / PhysX physics system global initialization
 *
 * 封装 PhysX SDK 的全局初始化和关闭。管理 PxFoundation、PxPhysics、
 * PxPvd（调试器）等全局对象，为 PhysicsScene 和其他物理组件提供基础。
 *
 * Wraps PhysX SDK global initialization and shutdown. Manages global objects
 * like PxFoundation, PxPhysics, PxPvd (debugger), providing the foundation
 * for PhysicsScene and other physics components.
 */
#pragma once
#include <PxPhysicsAPI.h>
#include <PxPhysics.h>
#include <thread>
#include <iostream>

using namespace physx;

namespace YingLong
{
	/**
	 * @brief PhysX 全局物理类 / PhysX global physics class
	 *
	 * 管理 PhysX SDK 的全局生命周期。所有静态成员在 InitializePhysics()
	 * 中创建，在析构函数中释放。通过 Create() 工厂方法创建实例。
	 *
	 * Manages the global lifecycle of the PhysX SDK. All static members are
	 * created in InitializePhysics() and released in the destructor.
	 * Instances are created via the Create() factory method.
	 */
	class Physics
	{
	public:
		/**
		 * @brief 构造函数 / Constructor
		 */
		Physics();
		Physics(const Physics& other) = default;

		/**
		 * @brief 初始化 PhysX 物理系统 / Initialize PhysX physics system
		 *
		 * 创建 PxFoundation、PxPhysics 和 PxPvd（调试器）。
		 * Creates PxFoundation, PxPhysics, and PxPvd (debugger).
		 */
		void InitializePhysics();

		/**
		 * @brief 工厂方法：创建 Physics 实例 / Factory method: create Physics instance
		 *
		 * 创建实例并自动调用 InitializePhysics()。
		 * Creates an instance and automatically calls InitializePhysics().
		 *
		 * @return std::unique_ptr<Physics> 物理系统实例 / Physics system instance
		 */
		static std::unique_ptr<Physics> Create()
		{
			std::unique_ptr<Physics> context = std::make_unique<Physics>();
			context->InitializePhysics();

			return std::move(context);
		}

		/**
		 * @brief 析构函数 / Destructor
		 *
		 * 释放所有 PhysX 全局资源。
		 * Releases all PhysX global resources.
		 */
		~Physics();

		static PxPhysics* PhysicsObject;                    ///< PhysX 顶层物理对象 / PhysX top-level physics object
		static PxDefaultErrorCallback PhysicsErrorCallback;  ///< 默认错误回调 / Default error callback
		static PxDefaultAllocator PhysicsAllocator;          ///< 默认内存分配器 / Default memory allocator
		static PxFoundation* PhysicsFoundation;              ///< PhysX 基础对象 / PhysX foundation object
		static PxPvd* PhysicsDebugger;                       ///< PhysX Visual Debugger 连接 / PhysX Visual Debugger connection
	};
}

/**
 * @file Physics.cpp
 * @brief PhysX 物理系统全局初始化实现 / PhysX physics system global initialization implementation
 */
#include "Physics.h"

namespace YingLong
{
	// 静态成员定义 / Static member definitions
	PxPhysics* Physics::PhysicsObject = nullptr;
	PxDefaultErrorCallback Physics::PhysicsErrorCallback;
	PxDefaultAllocator Physics::PhysicsAllocator;
	PxFoundation* Physics::PhysicsFoundation = nullptr;
	PxPvd* Physics::PhysicsDebugger = nullptr;

	Physics::Physics()
	{

	}

	/**
	 * @brief 析构函数实现 / Destructor implementation
	 *
	 * 按顺序释放 PhysX 资源：PhysicsObject -> PVD -> Foundation。
	 * Releases PhysX resources in order: PhysicsObject -> PVD -> Foundation.
	 */
	Physics::~Physics()
{
	if (this->PhysicsObject)
	{
		// 释放 PxPhysics 对象 / Release PxPhysics object
		this->PhysicsObject->release();
		this->PhysicsObject = nullptr;
	}

	if (this->PhysicsDebugger)
	{
		// 释放 PVD 传输层和调试器 / Release PVD transport and debugger
		PxPvdTransport* transport = this->PhysicsDebugger->getTransport();
		this->PhysicsDebugger->release();
		if (transport)
		{
			transport->release();
		}
		this->PhysicsDebugger = nullptr;
	}

	if (this->PhysicsFoundation)
	{
		// 释放 Foundation / Release Foundation
		this->PhysicsFoundation->release();
		this->PhysicsFoundation = nullptr;
	}
}

	/**
	 * @brief 初始化 PhysX 物理系统 / Initialize PhysX physics system
	 *
	 * 初始化流程：
	 *   1. 创建 PxFoundation（基础对象，分配器 + 错误回调）
	 *   2. 创建 PxPvd（Visual Debugger，通过 socket 连接 127.0.0.1:5425）
	 *   3. 创建 PxPhysics（顶层物理对象）
	 *
	 * Initialization flow:
	 *   1. Create PxFoundation (base object, allocator + error callback)
	 *   2. Create PxPvd (Visual Debugger, connects via socket to 127.0.0.1:5425)
	 *   3. Create PxPhysics (top-level physics object)
	 */
	void Physics::InitializePhysics()
{
	// 1. 创建 Foundation / Create Foundation
	this->PhysicsFoundation = PxCreateFoundation(PX_PHYSICS_VERSION,
		this->PhysicsAllocator, this->PhysicsErrorCallback);
	if (!this->PhysicsFoundation)
	{
		throw std::runtime_error("Failed to initialize PhysicsFoundation!");
	}

	// 2. 创建 PVD 调试器 / Create PVD debugger
	this->PhysicsDebugger = PxCreatePvd(*this->PhysicsFoundation);
	if (!this->PhysicsDebugger)
	{
		std::cout << "Failed to create PhysicsDebugger! Using physics without PVD.\n";
	}
	else
	{
		// 通过 socket 连接到 PVD（默认端口 5425）/ Connect to PVD via socket (default port 5425)
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		if (transport)
		{
			this->PhysicsDebugger->connect(*transport, PxPvdInstrumentationFlag::eALL);
		}
	}

	// 3. 创建 PxPhysics 顶层对象 / Create PxPhysics top-level object
	this->PhysicsObject = PxCreatePhysics(PX_PHYSICS_VERSION,
			*this->PhysicsFoundation, PxTolerancesScale(), true, this->PhysicsDebugger);
		if (!this->PhysicsObject)
		{
			throw std::runtime_error("Failed to initialize PhysicsObject!");
		}
	}
}

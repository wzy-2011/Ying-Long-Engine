#include "PhysicsCapsule.h"

namespace YingLong
{
	PhysicsCapsule::PhysicsCapsule() : radius(0.5f), HalfHeight(0.5f), staticFriction(0.8f),
		dynamicFriction(0.8f), restitution(0.3f), capsuleDensity(500.0f)
	{

	}

	PhysicsCapsule::PhysicsCapsule(float radius, float HalfHeight) : staticFriction(0.8f),
		dynamicFriction(0.8f), restitution(0.3f), capsuleDensity(500.0f)
	{
		this->radius = radius;
		this->HalfHeight = HalfHeight;
	}

	PhysicsCapsule::PhysicsCapsule(float radius, float HalfHeight, float staticFriction,
		float dynamicFriction, float restitution, float capsuleDensity)
	{
		this->radius = radius;
		this->HalfHeight = HalfHeight;
		this->staticFriction = staticFriction;
		this->dynamicFriction = dynamicFriction;
		this->restitution = restitution;
		this->capsuleDensity = capsuleDensity;
	}

	PhysicsCapsule::PhysicsCapsule(float radius, float HalfHeight, float staticFriction,
		float dynamicFriction, float restitution, float capsuleDensity,
		PxVec3 position, PxVec3 rotation, PxVec3 scale)
	{
		this->radius = radius;
		this->HalfHeight = HalfHeight;
		this->staticFriction = staticFriction;
		this->dynamicFriction = dynamicFriction;
		this->restitution = restitution;
		this->capsuleDensity = capsuleDensity;
		this->position = position;
	}

	PhysicsCapsule::PhysicsCapsule(const PhysicsCapsule& other) noexcept
	{
		this->HalfHeight = other.HalfHeight;
		this->radius = other.radius;
		this->staticFriction = other.staticFriction;
		this->dynamicFriction = other.dynamicFriction;
		this->restitution = other.restitution;
		this->position = other.position;
		this->capsuleDensity = other.capsuleDensity;
	}

	void PhysicsCapsule::InitializeCapsuleObject()
	{
		if (!Physics::PhysicsObject)
		{
			std::cout << "PhysicsCapsule: PhysicsObject is null!\n";
			return;
		}

		this->material = Physics::PhysicsObject->createMaterial(
			this->staticFriction, this->dynamicFriction, this->restitution);
		if (!this->material)
		{
			std::cout << "PhysicsCapsule: Failed to create material!\n";
			return;
		}

		this->relativePose = PxTransform(PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
		this->capsule = Physics::PhysicsObject->createShape(PxCapsuleGeometry(this->radius,
			this->HalfHeight), *this->material);
		if (!this->capsule)
		{
			std::cout << "PhysicsCapsule: Failed to create shape!\n";
			this->material->release();
			this->material = nullptr;
			return;
		}
		this->capsule->setLocalPose(this->relativePose);
	}

	void PhysicsCapsule::Shutdown()
	{
		if (this->capsule)
		{
			this->capsule->release();
			this->capsule = nullptr;
		}
		
		if (this->material)
		{
			this->material->release();
			this->material = nullptr;
		}
	}

	PhysicsCapsule::~PhysicsCapsule()
	{
		Shutdown();
	}
}

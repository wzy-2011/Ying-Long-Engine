#include "PhysicsScene.h"

namespace YingLong
{
    PhysicsScene::PhysicsScene()
    {
    }

    void PhysicsScene::InistializePhysicsScene()
    {
        PxSceneDesc sceneDesc(Physics::PhysicsObject->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        pCpuDispatcher = PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());
        sceneDesc.cpuDispatcher = pCpuDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;

        // Use Persistent Contact Manifold for more stable box-box and box-static
        // collisions (reduces jittering/tunnelling at rest).
        sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

        // Suppress micro-bounces: relative velocities below this threshold are
        // treated as non-impacting contacts. Without this, tiny residual velocities
        // accumulate and objects never fully settle.
        sceneDesc.bounceThresholdVelocity = 0.5f;

        if (!sceneDesc.isValid())
        {
            printf("PxSceneDesc is invalid!\n");
        }

        this->PhysicsSceneObject = Physics::PhysicsObject->createScene(sceneDesc);
        if (!PhysicsSceneObject)
        {
            std::cout << "PhysicsScene init failed!\n";
            if (pCpuDispatcher)
            {
                pCpuDispatcher->release();
                pCpuDispatcher = nullptr;
            }
            return;
        }
    }

    PxScene* PhysicsScene::GetPhysicsScene() noexcept
    {
        return this->PhysicsSceneObject;
    }

    bool PhysicsScene::IsValid() const noexcept
    {
        return PhysicsSceneObject != nullptr;
    }

    void PhysicsScene::Simulate(float dt)
    {
        if (PhysicsSceneObject)
        {
            PhysicsSceneObject->simulate(dt);
        }
    }

    bool PhysicsScene::FetchResults(bool block)
    {
        if (!PhysicsSceneObject)
            return false;
        return PhysicsSceneObject->fetchResults(block);
    }

    bool PhysicsScene::Step(float dt)
    {
        if (!IsValid())
            return false;
        Simulate(dt);
        return FetchResults(true);
    }

    void PhysicsScene::AddActor(PxActor& actor)
    {
        if (PhysicsSceneObject)
        {
            PhysicsSceneObject->addActor(actor);
        }
    }

    void PhysicsScene::RemoveActor(PxActor& actor)
    {
        if (PhysicsSceneObject)
        {
            PhysicsSceneObject->removeActor(actor);
        }
    }

    PhysicsScene::~PhysicsScene()
    {
        if (this->PhysicsSceneObject)
        {
            this->PhysicsSceneObject->release();
            this->PhysicsSceneObject = nullptr;
        }
        if (pCpuDispatcher)
        {
            pCpuDispatcher->release();
            pCpuDispatcher = nullptr;
        }
    }

    RaycastHit PhysicsScene::Raycast(const XMFLOAT3& origin, const XMFLOAT3& unitDir, float maxDist)
    {
        RaycastHit result;
        if (!PhysicsSceneObject)
            return result;

        PxVec3 pxOrigin(origin.x, origin.y, origin.z);
        PxVec3 pxDir(unitDir.x, unitDir.y, unitDir.z);
        // Normalize the direction in case the caller didn't.
        pxDir = pxDir.getNormalized();

        PxRaycastBuffer hitInfo;
        bool hit = PhysicsSceneObject->raycast(
            pxOrigin, pxDir, maxDist, hitInfo, PxHitFlag::eDEFAULT);

        if (hit && hitInfo.hasBlock)
        {
            result.Hit = true;
            result.Position = XMFLOAT3(
                hitInfo.block.position.x,
                hitInfo.block.position.y,
                hitInfo.block.position.z);
            result.Normal = XMFLOAT3(
                hitInfo.block.normal.x,
                hitInfo.block.normal.y,
                hitInfo.block.normal.z);
            result.Distance = hitInfo.block.distance;
            result.Actor = hitInfo.block.actor;
        }
        return result;
    }
}

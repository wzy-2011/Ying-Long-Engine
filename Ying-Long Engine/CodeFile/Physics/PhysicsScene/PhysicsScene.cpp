#include "PhysicsScene.h"

namespace YingLong
{
    // Custom filter shader: enables collision + contact/trigger notifications
    // for all shape pairs, so onContact and onTrigger callbacks fire.
    static PxFilterFlags CollisionFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
    {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT
                  | PxPairFlag::eNOTIFY_TOUCH_FOUND
                  | PxPairFlag::eNOTIFY_TOUCH_LOST;
        return PxFilterFlag::eDEFAULT;
    }

    // === PhysicsCollisionCallback ===

    void PhysicsCollisionCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
    {
        for (PxU32 i = 0; i < nbPairs; i++)
        {
            const PxContactPair& pair = pairs[i];
            // Extract contact points using extractContacts
            PxU32 contactCount = pair.contactCount;
            if (contactCount == 0)
                continue;

            PxContactPairPoint* contactPoints = new PxContactPairPoint[contactCount];
            PxU32 numContacts = pair.extractContacts(contactPoints, contactCount);

            if (numContacts > 0)
            {
                const PxContactPairPoint& cp = contactPoints[0];
                RawCollisionEvent evt;
                evt.ActorA = pairHeader.actors[0];
                evt.ActorB = pairHeader.actors[1];
                evt.ContactPoint = XMFLOAT3(cp.position.x, cp.position.y, cp.position.z);
                evt.ContactNormal = XMFLOAT3(cp.normal.x, cp.normal.y, cp.normal.z);
                evt.ContactDistance = cp.separation;
                evt.IsTrigger = false;
                m_Events.push_back(evt);
            }

            delete[] contactPoints;
        }
    }

    void PhysicsCollisionCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
    {
        for (PxU32 i = 0; i < count; i++)
        {
            const PxTriggerPair& tp = pairs[i];
            // Skip pairs where an actor was removed
            if (tp.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
                continue;

            RawCollisionEvent evt;
            evt.ActorA = tp.triggerActor;
            evt.ActorB = tp.otherActor;
            evt.IsTrigger = true;
            m_Events.push_back(evt);
        }
    }

    std::vector<RawCollisionEvent> PhysicsCollisionCallback::GetAndClearEvents()
    {
        std::vector<RawCollisionEvent> result;
        std::swap(result, m_Events);
        return result;
    }

    // === PhysicsScene ===

    PhysicsScene::PhysicsScene()
    {
    }

    void PhysicsScene::InistializePhysicsScene()
    {
        PxSceneDesc sceneDesc(Physics::PhysicsObject->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        pCpuDispatcher = PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());
        sceneDesc.cpuDispatcher = pCpuDispatcher;
        sceneDesc.filterShader = CollisionFilterShader;

        // Use Persistent Contact Manifold for more stable box-box and box-static
        // collisions (reduces jittering/tunnelling at rest).
        sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

        // Enable notification for contact and trigger events
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        sceneDesc.simulationEventCallback = &m_CollisionCallback;

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

/**
 * @file Entity.cpp
 * @brief ECS 实体管理类实现 / ECS entity manager implementation
 *
 * 实现 Entity 类的所有非模板成员函数。
 * Implements all non-template member functions of the Entity class.
 */
#include "Entity.h"

namespace YingLong
{
    /**
     * @brief 静态注册表定义 / Static registry definition
     *
     * 所有 Entity 实例共享的全局 entt 注册表。
     * Global entt registry shared by all Entity instances.
     */
    entt::registry Entity::RegistryObject;

    Entity::Entity()
    {
        // 以下为早期 ECS 原型代码，已注释保留供参考
        // The following is early ECS prototype code, commented out for reference
        //struct MeshComponent
        //{
        //    MeshComponent() = default;
        //    MeshComponent(const MeshComponent& other)
        //    {
        //        
        //    };
        //
        //    bool value = true;
        //};
        //
        //struct TransformComponent
        //{
        //    XMFLOAT4X4A Transform;
        //    
        //    TransformComponent() = default;
        //    TransformComponent(const TransformComponent&) = default;
        //    TransformComponent(const XMFLOAT4X4A& transform)
        //        : Transform(transform)
        //    {
        //
        //    };
        //};
        //
        //entt::entity entity = this->RegistryObject.create();
        //
        //this->RegistryObject.emplace<TransformComponent>(entity);
        //
        //this->RegistryObject.on_construct<TransformComponent>().connect<&OnTransformConstruct>();
        //
        ////TransformComponent& transform = this->RegistryObject.get<TransformComponent>(entity);
        //
        //auto view = this->RegistryObject.view<TransformComponent>();
        //for (auto entity : view)
        //{
        //    TransformComponent& transform = view.get<TransformComponent>(entity);
        //}
        //
        //auto group = this->RegistryObject.group<TransformComponent>(entt::get<MeshComponent>);
        //for (auto entity : group)
        //{
        //    auto& [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
        //}
    }

    entt::entity Entity::CreateEntity(const std::string& tag)
    {
        // 在注册表中创建新实体
        // Create a new entity in the registry
        auto entity = Entity::RegistryObject.create();
        // 自动添加标签组件用于实体命名
        // Automatically add tag component for entity naming
        AddComponent<TagComponent>(entity, tag);
        return entity;
    }

    void Entity::DestroyEntity(entt::entity entity)
    {
        // 验证实体有效后再销毁
        // Validate entity before destroying
        if (IsEntityValid(entity))
        {
            Entity::RegistryObject.destroy(entity);
        }
    }

    entt::registry& Entity::GetRegistry() noexcept
    {
        return this->RegistryObject;
    }

    bool Entity::IsEntityValid(entt::entity entity) noexcept
    {
        return Entity::RegistryObject.valid(entity);
    }

    void Entity::Update(float TimeStep, int index)
    {
        // 遍历所有具有 TransformComponent + SpriteRendererComponent 的实体
        // （当前为空实现，预留 2D 精灵渲染系统接口）
        // Iterate all entities with TransformComponent + SpriteRendererComponent
        // (currently empty implementation, reserved for 2D sprite render system)
        auto view = this->RegistryObject.view<TransformComponent, SpriteRendererComponent>();
        view.each([=](const auto& e, TransformComponent& Transform, SpriteRendererComponent& Sprite)
            {

            });
    }

    Entity::~Entity()
    {

    }
}

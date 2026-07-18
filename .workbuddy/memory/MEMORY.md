# Dracovis Engine 项目长期记忆

## 项目定位
自研 C++ 游戏引擎（缩写 DE），MIT 许可，作者 wzy-2011。Windows 平台，x64，C++20，Premake5 构建（`Premake5.lua` + `Generate Project.bat`）。解决方案 `Dracovis Engine.sln` 含 5 个子工程。

## 命名空间与风格
- 引擎代码统一在 `namespace DE` 下
- 注释风格：中英双语 Doxygen 风格（`@file` / `@brief` / `@param`），文件头有详细说明
- 头文件保护用 `#pragma once`

## 核心架构约定
- **Application** 是单例（`Application::Instance`），顶层协调者，friend 了 Graphics/LightManager
- **ECS** 基于 EnTT：Scene 持有 `entt::registry`，System 基类有 `Update`(遗留单实体) 和 `UpdateScene`(推荐场景级) 双接口
- **渲染双模式**：DX11 已 `[[deprecated]]`，新代码走 DX12（`DX12Renderer`）。F5 切换。DX12 模式下 Mesh 渲染用 DX12Box 占位符 + TintColor（完整 Model 管线尚未接通）
- **物理**：PhysX 5.5。PhysicsSystem 惰性创建 PxRigidActor（Mass=0 静态 / >0 动态），结果回写 TransformComponent。PhysicsScene 由 Application 持有，通过 `Scene::SetPhysicsScene` 绑定
- **序列化**：yaml-cpp，YAML_CPP_STATIC_DEFINE 宏。场景/相机/光照/背景色分别存 `SceneData/` 子目录

## 关键路径
- 引擎源码：`Dracovis Engine/CodeFile/`（Application / ECS / Graphics / Physics / Shader / FileController / Exception / EntryPoint）
- 入口：`CodeFile/EntryPoint/EntryPoint.cpp` → `main()` 创建 Application 单例
- 构建产物：`Build/<cfg> - <system>/x64/`
- 第三方库静态链接：ImGui / yaml-cpp / Time / entt-master 均为本仓库内同构建静态库

## 构建配置
Debug / Release / Distribution 三配置；链接 d3d11/d3d12/d3dcompiler/dxgi/DirectXTK + assimp-vc143-mtd + 14 个 PhysX 库

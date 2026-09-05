# AGENTS.md

本仓库中 AI 代理与贡献者的工作指南。

## 修改架构前

- 先阅读 `docs/architecture.md`。它是跨会话的架构唯一真源，包含分层与依赖规则。
- 阅读相关 `docs/dev-history/implementation/*.md`（已归档的开发记录），了解阶段目标与已记录决策。

## 编码规范

### 命名

- 类型、类、结构体、枚举和方法：`PascalCase`。
  - 示例：`AssetManager`、`MeshAsset`、`LoadModel`、`GetMesh`。
- 成员变量：`m_` 前缀 + `camelCase`。
  - 示例：`m_mesh`、`m_textureCache`、`m_objectIndex`、`m_nextObjectId`。
- 局部变量和函数参数：`camelCase`。
  - 示例：`meshHandle`、`objectIndex`、`textureKey`。
- 常量：`k` 前缀 + `PascalCase`。
  - 示例：`kInvalidId`、`kMaxFramesInFlight`。

### 注释

- 使用**简单、简洁、清晰的英语**。
- 优先写简短注释，解释意图或非显然的决策。
- 不要重复代码本身已经表达的内容。

### 代码组织

- 遵循现有 `Kita::Pbrv` 命名空间与目录结构。
- 遵守 `docs/architecture.md` 中的分层与依赖规则。
- 公共 API 尽量不暴露内部实现细节。

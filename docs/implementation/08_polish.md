# 08. 收尾优化

> **阶段目标**：错误处理健壮，Release 构建可用，可选特性按需补全。

- [ ] 统一错误信息输出：资源加载失败时输出文件路径 + 失败原因
- [ ] Release 构建测试，确认 Validation Layers 已关闭、功能正常
- [ ] 阴影：方向光 Shadow Map（可选）
  - 需要单独的深度 Image + ImageView 存储 Shadow Map
  - 通过 Descriptor 将 Shadow Map 采样器传入 PBR 主 Pass（阶段 05 已预留 Binding 槽位）
- [ ] 后续考虑：
  - **Shader 热重载**：运行时监控 `.spv` 变更，自动重建 Pipeline。注意不要在 GPU 正使用 Pipeline 时直接销毁，应新 Pipeline 创建成功后延迟几帧再释放旧 Pipeline
  - **模型热替换**：运行时切换模型无需重启
  - **MSAA**：Dynamic Rendering 下通过 `VkRenderingAttachmentInfo::resolveMode` + `pResolveImageView` + 多采样临时 Color Image 实现。阶段 03 已预留 `VkSampleCountFlagBits` 参数
  - 截图保存
  - glTF 格式支持

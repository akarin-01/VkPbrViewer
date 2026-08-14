# 07. 收尾优化

> **阶段目标**：错误处理健壮，Release 构建可用，可选特性按需补全。

- [ ] 统一错误信息输出：资源加载失败时输出文件路径 + 失败原因
- [ ] Release 构建测试，确认 Validation Layers 已关闭、功能正常
- [ ] `VK_EXT_debug_utils` 结构化回调：注册 debug messenger，校验信息通过 `Log` 模块输出（收阶段 02 的尾巴）
- [ ] 阴影：方向光 Shadow Map（可选）
  - 需要单独的深度 Image + ImageView 存储 Shadow Map
  - Descriptor 布局增加阴影贴图采样器 Binding 槽位（与阴影一起实现，无需提前预留）
  - 通过 Descriptor 将 Shadow Map 传入 PBR 主 Pass
- [ ] 后续考虑：
  - **Shader 热重载**：运行时监控 `.spv` 变更，自动重建 Pipeline。注意不要在 GPU 正使用 Pipeline 时直接销毁，应新 Pipeline 创建成功后延迟几帧再释放旧 Pipeline
  - **模型热替换**：运行时切换模型无需重启
  - **MSAA**：Dynamic Rendering 下通过 `VkRenderingAttachmentInfo::resolveMode` + `pResolveImageView` + 多采样临时 Color Image 实现。阶段 03 已预留 `VkSampleCountFlagBits` 参数
  - 截图保存
  - glTF 材质 / 纹理读取（可选）：`AssetLoader::LoadGltf` 扩展——factor 映射（baseColor → albedo、metallic/roughness → 标量）、metallicRoughness 纹理拆双通道（R → roughness、G → metallic）、三来源纹理加载（外部 URI / data URI / GLB bufferView，需 `stbi_load_from_memory` 内存解码）；若 glTF 未提供 tangent，按 Mikktspace 计算填充

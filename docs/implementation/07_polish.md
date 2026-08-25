# 07. 收尾优化

> **阶段目标**：错误处理与日志通道统一，常用渲染功能补全（自由相机、MSAA、glTF 材质、阴影）。

- [ ] 【错误处理】统一错误通道：加载失败等错误路径明确 LogError / assert / exception 三选一，异常信息包含文件路径与失败原因
- [ ] 【验证层回调】`VK_EXT_debug_utils`：注册 debug messenger，校验信息通过 `Log` 模块输出（收阶段 02 的尾巴）
- [ ] 【自由相机】相机从 Orbit 改为 FPS 风格自由飞行：按住鼠标右键时激活，WASD 前后左右平移、Q/E 上下平移、鼠标移动旋转视角
- [x] 【MSAA】多采样抗锯齿：Dynamic Rendering 下通过 `VkRenderingAttachmentInfo::resolveMode` + `pResolveImageView` + 多采样临时 Color Image 实现（RenderContext 已提供 `SampleCount()` / `MaxSampleCount()`）
- [ ] 【glTF 材质】加载 glTF 模型时顺便读取材质参数与贴图：factor 映射（baseColor → albedo、metallic/roughness → 标量）；metallicRoughness 贴图直接复用现有 MR 槽位（G = roughness、B = metallic，与 shader 通道一致）；三来源贴图加载（外部 URI / data URI / GLB bufferView，`stbi_load_from_memory` 内存解码）
- [ ] 【阴影】方向光 Shadow Map：
  - 阴影贴图：独立的深度 Image + ImageView 存储 Shadow Map
  - Descriptor 布局增加阴影贴图采样器 Binding 槽位
  - 阴影 Map 经 Descriptor 传入 PBR 主 Pass

- [ ] 后续考虑：
  - **Shader 热重载**：运行时监控 `.spv` 变更，自动重建 Pipeline。不要在 GPU 正使用时销毁，新 Pipeline 创建成功后延迟几帧再释放旧 Pipeline
  - 截图保存

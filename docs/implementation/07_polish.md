# 07. 收尾优化

> **阶段目标**：错误处理与日志通道统一，常用渲染功能补全（自由相机、MSAA、glTF 材质、阴影）。

- [ ] 【错误处理】统一错误通道：明确各错误场景的处置方式（约定见文末专题），异常信息包含文件路径与失败原因
- [x] 【验证层回调】`VK_EXT_debug_utils`：注册 debug messenger，校验信息通过 `Log` 模块输出
- [ ] 【自由相机】相机从 Orbit 改为 FPS 风格自由飞行：按住鼠标右键时激活，WASD 前后左右平移、Q/E 上下平移、鼠标移动旋转视角
- [x] 【MSAA】多采样抗锯齿：Dynamic Rendering 下通过 `VkRenderingAttachmentInfo::resolveMode` + `pResolveImageView` + 多采样临时 Color Image 实现（RenderContext 已提供 `SampleCount()` / `MaxSampleCount()`）
- [ ] 【glTF 材质】加载 glTF 模型时顺便读取材质参数与贴图：factor 映射（baseColor → albedo、metallic/roughness → 标量）；metallicRoughness 贴图直接复用现有 MR 槽位（G = roughness、B = metallic，与 shader 通道一致）；三来源贴图加载（外部 URI / data URI / GLB bufferView，`stbi_load_from_memory` 内存解码）
- [x] 【阴影】方向光 Shadow Map：
  - 阴影贴图：独立的深度 Image + ImageView 存储 Shadow Map
  - Descriptor 布局增加阴影贴图采样器 Binding 槽位
  - 阴影 Map 经 Descriptor 传入 PBR 主 Pass

- [ ] 后续考虑：
  - **Shader 热重载**：运行时监控 `.spv` 变更，自动重建 Pipeline。不要在 GPU 正使用时销毁，新 Pipeline 创建成功后延迟几帧再释放旧 Pipeline
  - 截图保存

### 错误处理约定（后续阶段一律参考）

| 错误类别                                 | 方法                                                                              | 举例                                                               |
| ---------------------------------------- | --------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| 外部输入失败：文件不存在、损坏、解码失败 | 抛异常，消息含路径 + 底层原因；边界（UI 回调）try/catch → `Log::Error` 后继续运行 | `Failed to load texture 'xxx.png': <stbi 原因>`（glTF / HDR 同理） |
| 解析警告、可降级项                       | `Log::Warning` + 跳过 / 自动计算，继续渲染                                        | tinygltf warning；不支持的 primitive mode；缺失切线                |
| 枚举 / 传参非法（代码错误）              | 抛异常，初始化期暴露                                                              | `ToFormat` / `GetTextureChannels` 的 default 分支                  |
| GPU 资源创建 / 命令提交失败（致命）      | 抛异常，消息含资源名 + VkResult；传播到 main 的 catch 兜底终止                    | `CreateImage` / `CreateBuffer` 失败；`OneShotCommand` 提交失败     |
| 内部不变量（句柄 / 指针无效）            | assert 中断（debug），不继续使用                                                  | `GetImage` 返回 null                                               |
| 操作取消（正常流程）                     | `return nullopt`                                                                  | 文件对话框取消                                                     |

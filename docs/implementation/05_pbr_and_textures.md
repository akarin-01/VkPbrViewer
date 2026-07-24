# 05. PBR 着色与纹理

> **阶段目标**：模型以 PBR 材质渲染（方向光），纹理贴图生效。

- [ ] CMake：通过 FetchContent 引入 stb_image
- [ ] 替换临时着色器，实现 Cook-Torrance BRDF（D: GGX, F: Schlick Fresnel, G: Smith GGX）
- [ ] 实现 Tone Mapping（如 ACES Filmic）+ Gamma 校正，HDR → LDR 正确显示
- [ ] 使用 stb_image 加载贴图文件（PNG/JPG/BMP/HDR），创建 Vulkan Image + ImageView + Sampler
- [ ] 支持 BaseColor 贴图、Metallic-Roughness 贴图、Normal 贴图
- [ ] 材质数据结构支持「使用贴图」和「使用标量值」两种模式
- [ ] Descriptor 布局中为阴影贴图预留一个组合图像采样器 Binding 槽位（阶段 08 实现阴影时无需大改 Shader 接口）
- [ ] 注意：IBL（基于图像的光照）不在本阶段，使用阶段 04 定义的灯光即可验证 PBR 着色效果；IBL 将在阶段 06 单独处理

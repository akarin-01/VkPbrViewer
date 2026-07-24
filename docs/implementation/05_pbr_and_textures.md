# 05. PBR 着色与纹理

- [ ] CMake：通过 FetchContent 引入 stb_image
- [ ] CMake：寻找 glslangValidator，添加 Shader 编译规则（构建时 GLSL → SPIR-V）
- [ ] 替换临时着色器，实现 Cook-Torrance BRDF（D: GGX, F: Schlick Fresnel, G: Smith GGX）
- [ ] 实现 Tone Mapping（如 ACES Filmic）+ Gamma 校正，HDR → LDR 正确显示
- [ ] 使用 stb_image 加载贴图文件（PNG/JPG/BMP/HDR），创建 Vulkan Image + ImageView + Sampler
- [ ] 支持 BaseColor 贴图、Metallic-Roughness 贴图、Normal 贴图
- [ ] 材质数据结构支持「使用贴图」和「使用标量值」两种模式
- [ ] 注意：IBL（基于图像的光照）不在本阶段，使用方向光即可验证 PBR 着色效果；IBL 将在阶段 06 单独处理

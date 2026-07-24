# 04. 模型加载与基础渲染

> **阶段目标**：`.obj` 模型正确显示在屏幕上，Orbit 相机可交互。

- [ ] CMake：通过 FetchContent 引入 glm，配置 `target_compile_definitions` 添加 `GLM_FORCE_DEPTH_ZERO_TO_ONE`
- [ ] CMake：通过 FetchContent 引入 tinyobjloader
- [ ] 定义顶点结构体：Position（vec3）、Normal（vec3）、TexCoord（vec2）、Tangent（vec4）
- [ ] 定义材质参数结构体：BaseColor（vec4）、Metallic（float）、Roughness（float）、AO（float）等
- [ ] 定义灯光结构体（方向、颜色、强度），在 Uniform Buffer 中为灯光预留空间——后续阶段 05 的 PBR 着色器直接使用，避免阶段 07 再回头重构 Buffer 布局
- [ ] 使用 tinyobjloader 加载 `.obj` 文件，提取顶点和索引数据
- [ ] 加载后计算法线和 Tangent（如果模型缺少）
- [ ] 将顶点和索引数据上传到 GPU Buffer（Staging Buffer → Device Local Buffer）
- [ ] 创建 Uniform Buffer：MVP 矩阵（Model + View + Projection）+ 灯光数据，每帧更新
- [ ] 写一个临时的 Lambert 着色器（先不搞 PBR），确保模型能正确显示
- [ ] 实现 Orbit 相机：鼠标左键旋转、滚轮缩放、中键平移
- [ ] 相机输入连接到 View 矩阵和 Perspective 投影矩阵

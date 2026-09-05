# 需求文档

> **仅作参考**：本文档为需求基线，不跟踪实现现状。若内容与实际代码或其他文档不一致，一律以实际代码为准，并顺手更新本文档消除偏差。

## 核心功能

- 使用 Vulkan 实时渲染 PBR 材质效果
- 通过 UI 面板调节材质参数（BaseColor、Metallic、Roughness 等）
- 支持内置模型，也能从外部导入 `.obj` 模型文件
- 支持导入纹理贴图（PNG / JPG / BMP / HDR）
- 相机控制：鼠标左键拖拽旋转、滚轮推拉缩放、中键平移
- 天空盒可切换
- 灯光参数可调（颜色、强度、位置）
- 实时显示帧率和帧间隔

## 不做或以后再说

- 不要求骨骼动画
- 不要求多相机
- 阴影后面再做
- 模型暂时只支持 `.obj`，glTF 以后再加

## 技术边界

- C++17 + Vulkan 1.3，启用 `VK_KHR_dynamic_rendering` 减少样板代码
- GLFW 创建窗口，Dear ImGui 做 UI
- CMake 构建，依赖管理统一使用 FetchContent（版本以 GIT_TAG 锁定）
- 着色器编译使用 glslangValidator（构建时 GLSL → SPIR-V）
- **程序必须在项目根目录启动**，因为资源路径用的是相对于根目录的路径
- 启动时如果没有 `assets/` 文件夹就直接报错退出

## 渲染流程

1. 天空盒 Pass
2. 阴影 Pass（后面再做）
3. PBR 主渲染 Pass（Cook-Torrance BRDF）

渲染器拆成几个子模块：RenderContext、PipelineManager、ResourceManager。

## UI 面板

- 帧率和帧间隔
- 模型选择（内置列表 + 导入按钮）
- 材质参数：颜色用拾取器 + 数值用滑块 + 贴图导入
- 灯光参数：颜色、强度、位置
- 天空盒切换

## 质量要求

- Debug 模式开启 Vulkan Validation Layers，出错时能定位问题
- 资源创建失败时输出可读的错误信息（不是只返回错误码）
- 主流独显上默认场景不低于 30 帧

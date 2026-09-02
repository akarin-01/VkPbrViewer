# 实现步骤

- [x] [01. CMake 项目骨架](01_cmake.md)
- [x] [02. 窗口与 Vulkan 初始化](02_window_vulkan.md)
- [x] [03. 渲染框架](03_render_framework.md)
- [x] [04. 模型加载与基础渲染](04_model_and_camera.md)
- [x] [05. PBR、天空盒与 UI](05_pbr_skybox_ui.md)
- [x] [06. IBL 环境光照](06_ibl.md)
- [ ] [07. 收尾优化](07_polish.md) — 进行中（见"当前进度"）
- [x] [08. 架构调整（SceneProxy / 多物体 / 资源分层）](08_restructuring.md)

## 当前进度（2026-09-02 更新）

- 已完成 01~06、08。步骤 06 此前未勾选，实际已实现（EnvironmentBaker、漫反射 + 镜面 IBL 均已落地），本次同步。
- 07 进行中：验证层回调（debug messenger）、MSAA 已完成；统一错误处理通道、自由相机、glTF 材质、阴影未做。
- 下一步候选（07 未做项）：统一错误处理通道 → 阴影 → glTF 材质 / 自由相机。开始新步骤时新建 `09_xxx.md` 并在本页登记。
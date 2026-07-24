# 07. UI 与灯光

- [ ] CMake：通过 FetchContent 引入 Dear ImGui（含 Vulkan + GLFW 后端）
- [ ] 集成 Dear ImGui 的 Vulkan + GLFW 后端，创建 ImGui Context
- [ ] ImGui 每帧渲染：Begin → 绘制面板 → End → 录制 ImGui Draw Data
- [ ] UI 面板：帧率/帧间隔、材质参数滑块/颜色拾取器、模型选择、贴图导入按钮
- [ ] 实现文件选择对话框（模型导入、纹理导入），如使用 ImGuiFileDialog
- [ ] Uniform Buffer 中传递灯光参数（至少一个方向光），UI 面板控制颜色/强度/方向
- [ ] 天空盒在 UI 中切换

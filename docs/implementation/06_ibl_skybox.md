# 06. IBL 与天空盒

- [ ] 用 stb_image 加载 HDR 环境贴图（Cubemap）
- [ ] 渲染天空盒：Cubemap + 全屏三角形，作为场景背景
- [ ] 从 HDR 环境贴图生成 Irradiance Map（漫反射卷积）
- [ ] 生成 Prefiltered Environment Map（不同粗糙度 Mip Level）
- [ ] 生成 BRDF Integration LUT（2D 查找表）
- [ ] 将 IBL 数据接入 PBR 着色管线（替换阶段 05 的方向光为环境光照）

> **提示**：IBL 贴图建议离线预计算（工具生成或一次性启动时计算并缓存），避免每次启动都重新卷积导致延迟。

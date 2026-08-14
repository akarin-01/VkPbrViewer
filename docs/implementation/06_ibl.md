# 06. IBL 环境光照

> **阶段目标**：环境光照补充方向光，模型在环境光下正确显示。

- [ ] 从阶段 05 的天空盒 cubemap 生成 Irradiance Map（漫反射卷积）
- [ ] 生成 Prefiltered Environment Map（GGX 重要性采样，不同粗糙度 Mip Level）
- [ ] 生成 BRDF Integration LUT（2D 查找表）
- [ ] 将 IBL 数据接入 PBR 着色管线：descriptor 增加环境贴图 binding，lit.frag 补充环境光项（IBL diffuse + specular），与方向光共存

> **提示**：预计算建议离线（工具生成或一次性启动时计算并缓存），避免每次启动重新卷积导致延迟。

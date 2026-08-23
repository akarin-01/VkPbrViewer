# 06. IBL 环境光照

> **阶段目标**：用天空盒环境图替代临时 ambient，让模型在环境光下获得正确的漫反射与镜面反射。

### 一、漫反射 IBL（Diffuse）

- [x] 从天空盒 cubemap 生成 Irradiance Map
- [x] 创建 IBL 渲染数据资源与描述符
- [x] `lit.frag` 采样 irradiance，计算环境漫反射项，替换临时 ambient

### 二、镜面反射 IBL（Specular）

- [ ] 从天空盒 cubemap 生成 Prefiltered Env Map（多 Mip）
- [ ] 生成 BRDF Integration LUT
- [ ] `lit.frag` 采样 prefiltered + BRDF LUT，计算环境镜面反射项
- [ ] 与方向光、漫反射 IBL、Emissive 合并
- [ ] 更新描述符预算与资源生命周期

> **提示**：BRDF LUT 与环境无关，可在启动时生成一次或离线生成后加载；Irradiance / Prefiltered 依赖当前天空盒，应在天空盒加载或更换时计算一次并缓存，避免每帧重算。

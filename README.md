# Focus Video

Focus Video 是一个面向 Windows PC 的实时视频超分播放器原型。第一版目标是在 NVIDIA GeForce RTX 3060 Ti 及以上显卡上，稳定播放常见本地视频并提供低延迟 2x 超分。

## 第一版目标

- **平台**：Windows 10/11 x64 桌面端。
- **最低 GPU**：NVIDIA GeForce RTX 3060 Ti，优先走 CUDA / TensorRT 推理链路。
- **典型输入**：720p 或 1080p，24/30/60 FPS 的 H.264 / H.265 本地文件。
- **典型输出**：2K（2560x1440）或 4K（3840x2160），保持音画同步和可交互拖动。
- **体验目标**：3060 Ti 上 1080p30 -> 2K/4K30 使用 Balanced 档流畅运行；1080p60 -> 4K60 自动降级到 Performance 档或跳过部分帧的超分处理。

## 技术路线

播放器拆成五个可替换模块：

1. **解封装 / 解码**：FFmpeg + NVDEC，尽量输出 GPU 纹理，减少 CPU/GPU 拷贝。
2. **帧调度**：按视频时钟管理解码帧、超分帧和显示帧，允许超分队列背压。
3. **实时超分**：优先 TensorRT FP16 模型，3060 Ti 默认使用轻量 2x 模型。
4. **后处理**：色彩空间转换、锐化、去振铃和字幕合成。
5. **渲染**：Direct3D 11/12 交换链，第一版建议 D3D11 以降低集成难度。

更详细的架构说明见 [`docs/architecture.md`](docs/architecture.md)，里程碑见 [`docs/roadmap.md`](docs/roadmap.md)。

## 当前仓库内容

当前提交提供第一版的产品/工程骨架：

- C++20 核心配置模型，用于描述设备能力、视频参数、目标输出分辨率和超分档位。
- 一个命令行原型，用于验证配置选择逻辑。
- 一个依赖最小化的帧级真实超分参考实现：读取 P6 PPM 图像，执行 2x/3x/4x 双线性放大，并在 Balanced/Quality 档叠加轻量边缘锐化，然后写出更高分辨率 PPM 文件。
- 单元测试，覆盖 3060 Ti 目标机型的默认档位、降级策略和帧级超分导出路径。
- Windows 播放器、实时超分和离线视频导出的架构文档。

## 构建与测试

本仓库当前不依赖 FFmpeg、CUDA 或 TensorRT；核心策略代码可在任意 C++20 编译器上构建。每次改动都需要同步更新 [`CHANGELOG.md`](CHANGELOG.md)。

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

验证仓库内的 superpowers 技能不需要安装 PyYAML，可直接运行：

```bash
python scripts/validate_superpowers_skill.py
```

运行策略选择原型：

```bash
./build/focus-video --width 1920 --height 1080 --fps 30 --target-width 3840 --target-height 2160 --gpu "NVIDIA GeForce RTX 3060 Ti"
```

运行帧级真实超分参考导出：

```bash
./build/focus-video --upscale-ppm input.ppm --output output-2x.ppm --scale 2 --sr-mode balanced
```

> 说明：PPM 导出路径已经会实际生成更高分辨率图像，用于验证超分核心、文件写出和测试闭环；完整本地视频导出会在 FFmpeg demux/decode 与 mux/encode 接入后，将同一个帧级超分接口串到视频逐帧处理管线。

Windows 上可使用 Visual Studio 2022 或 Ninja 生成器：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Superpowers 开发技能

仓库内提供 `.codex/skills/superpowers` 技能，用于后续开发时统一执行“理解需求 -> 设定播放稳定性兜底 -> 小步实现 -> 测试 -> 文档 -> 提交/PR”的流程。之后涉及播放器架构、实时超分、Windows/NVIDIA GPU、C++/CMake 或文档测试变更时，应优先使用该技能。

## 2K / 4K 支持说明

第一版目标是支持输出到 2K 和 4K，而不是承诺所有输入都能满质量超分：

- 720p60 -> 2K60：默认 Quality 档。
- 1080p30 -> 2K/4K30：默认 Balanced 档，是 3060 Ti 的主目标。
- 1080p60 -> 4K60：默认 Performance 档，并允许跳过部分超分帧。
- 1440p30 -> 4K30：可进入 Performance 档，作为第一版的保守支持。
- 超过 4K 输出或输入负载超过 3060 Ti 预算时，自动关闭超分，保留稳定播放。

## 开发原则

- 默认以 3060 Ti 为性能预算，不把第一版建立在高端 40/50 系显卡假设上。
- 超分链路必须可关闭、可降级、可旁路，播放稳定性优先于画质。
- GPU 内存预算第一版控制在 2 GB 以内，避免影响系统和其他应用。
- 所有关键路径都需要埋点：解码耗时、推理耗时、渲染耗时、队列深度、丢帧数。

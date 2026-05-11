#include "focus_video.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string value_after(int& index, int argc, char** argv) {
    if (index + 1 >= argc) {
        std::cerr << "Missing value for " << argv[index] << '\n';
        std::exit(2);
    }
    return argv[++index];
}

}  // namespace

int main(int argc, char** argv) {
    focus_video::VideoInfo video;
    focus_video::GpuInfo gpu;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--width") {
            video.width = static_cast<std::uint32_t>(std::stoul(value_after(i, argc, argv)));
        } else if (arg == "--height") {
            video.height = static_cast<std::uint32_t>(std::stoul(value_after(i, argc, argv)));
        } else if (arg == "--fps") {
            video.fps = std::stod(value_after(i, argc, argv));
        } else if (arg == "--target-width") {
            video.target_width = static_cast<std::uint32_t>(std::stoul(value_after(i, argc, argv)));
        } else if (arg == "--target-height") {
            video.target_height = static_cast<std::uint32_t>(std::stoul(value_after(i, argc, argv)));
        } else if (arg == "--gpu") {
            gpu.name = value_after(i, argc, argv);
        } else if (arg == "--vram-mb") {
            gpu.dedicated_memory_mb = static_cast<std::uint32_t>(std::stoul(value_after(i, argc, argv)));
        } else if (arg == "--no-tensorrt") {
            gpu.supports_tensor_rt = false;
        } else if (arg == "--no-fp16") {
            gpu.supports_fp16 = false;
        } else if (arg == "--help") {
            std::cout << "Usage: focus-video [--width N] [--height N] [--fps N] [--target-width N] [--target-height N] [--gpu NAME] [--vram-mb N]\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            return 2;
        }
    }

    const auto profile = focus_video::choose_profile(video, gpu);
    std::cout << "Input: " << video.width << 'x' << video.height << " @ " << video.fps << " FPS\n";
    std::cout << "GPU: " << gpu.name << " (" << gpu.dedicated_memory_mb << " MB VRAM)\n";
    std::cout << "Target: " << profile.output_width << 'x' << profile.output_height << "\n";
    std::cout << "Super resolution: " << focus_video::to_string(profile.mode) << "\n";
    std::cout << "Model scale: " << profile.model_scale << "x\n";
    std::cout << "Queue depth: " << profile.max_queue_depth << "\n";
    std::cout << "Frame skip allowed: " << (profile.allow_frame_skip ? "yes" : "no") << "\n";
    std::cout << "Reason: " << profile.reason << "\n";

    return 0;
}

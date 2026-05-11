#include "focus_video.hpp"
#include "image_super_resolution.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

focus_video::UpscaleMode parse_mode(const std::string& value) {
    if (value == "off") {
        return focus_video::UpscaleMode::Off;
    }
    if (value == "performance") {
        return focus_video::UpscaleMode::Performance;
    }
    if (value == "balanced") {
        return focus_video::UpscaleMode::Balanced;
    }
    if (value == "quality") {
        return focus_video::UpscaleMode::Quality;
    }
    throw std::runtime_error("Unknown SR mode: " + value);
}

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
    focus_video::OfflineExportOptions export_options;
    bool export_ppm = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--upscale-ppm") {
            export_options.input_path = value_after(i, argc, argv);
            export_ppm = true;
        } else if (arg == "--output") {
            export_options.output_path = value_after(i, argc, argv);
        } else if (arg == "--scale") {
            export_options.scale = static_cast<std::uint32_t>(std::stoul(value_after(i, argc, argv)));
        } else if (arg == "--sr-mode") {
            export_options.mode = parse_mode(value_after(i, argc, argv));
        } else if (arg == "--width") {
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
            std::cout << "       focus-video --upscale-ppm INPUT.ppm --output OUTPUT.ppm [--scale 2] [--sr-mode performance|balanced|quality]\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            return 2;
        }
    }

    if (export_ppm) {
        if (export_options.output_path.empty()) {
            std::cerr << "Missing --output for --upscale-ppm\n";
            return 2;
        }
        try {
            focus_video::export_upscaled_ppm(export_options);
        } catch (const std::exception& error) {
            std::cerr << "PPM export failed: " << error.what() << '\n';
            return 1;
        }
        std::cout << "Wrote upscaled PPM: " << export_options.output_path << "\n";
        std::cout << "Scale: " << export_options.scale << "x\n";
        std::cout << "Mode: " << focus_video::to_string(export_options.mode) << "\n";
        return 0;
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

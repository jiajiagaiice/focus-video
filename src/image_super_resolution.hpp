#pragma once

#include "focus_video.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace focus_video {

struct RgbImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> pixels;
};

struct OfflineExportOptions {
    std::filesystem::path input_path;
    std::filesystem::path output_path;
    std::uint32_t scale = 2;
    UpscaleMode mode = UpscaleMode::Balanced;
};

RgbImage read_ppm(const std::filesystem::path& path);
void write_ppm(const std::filesystem::path& path, const RgbImage& image);
RgbImage upscale_image(const RgbImage& source, std::uint32_t scale, UpscaleMode mode);
void export_upscaled_ppm(const OfflineExportOptions& options);

}  // namespace focus_video

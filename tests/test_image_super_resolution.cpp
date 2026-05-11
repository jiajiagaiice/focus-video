#include "image_super_resolution.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    const focus_video::RgbImage source{
        2,
        2,
        {
            0, 0, 0,       255, 0, 0,
            0, 255, 0,     0, 0, 255,
        },
    };

    const auto performance = focus_video::upscale_image(source, 2, focus_video::UpscaleMode::Performance);
    assert(performance.width == 4);
    assert(performance.height == 4);
    assert(performance.pixels.size() == 4 * 4 * 3);
    assert(performance.pixels.front() == 0);
    assert(performance.pixels[3] > 0);

    const auto off = focus_video::upscale_image(source, 2, focus_video::UpscaleMode::Off);
    assert(off.width == source.width);
    assert(off.height == source.height);
    assert(off.pixels == source.pixels);

    const auto temp = std::filesystem::temp_directory_path();
    const auto input_path = temp / "focus_video_sr_input.ppm";
    const auto output_path = temp / "focus_video_sr_output.ppm";
    focus_video::write_ppm(input_path, source);
    focus_video::export_upscaled_ppm({input_path, output_path, 2, focus_video::UpscaleMode::Balanced});
    const auto round_trip = focus_video::read_ppm(output_path);
    assert(round_trip.width == 4);
    assert(round_trip.height == 4);
    assert(round_trip.pixels.size() == 4 * 4 * 3);
    std::filesystem::remove(input_path);
    std::filesystem::remove(output_path);

    std::cout << "image super-resolution tests passed\n";
    return 0;
}

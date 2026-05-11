#include "focus_video.hpp"

#include <cassert>
#include <iostream>

int main() {
    const focus_video::GpuInfo rtx3060ti{"NVIDIA GeForce RTX 3060 Ti", 8192, true, true};

    const auto p720 = focus_video::choose_profile({1280, 720, 60.0, 2560, 1440}, rtx3060ti);
    assert(p720.mode == focus_video::UpscaleMode::Quality);
    assert(p720.model_scale == 2);
    assert(p720.output_width == 2560);
    assert(p720.output_height == 1440);
    assert(!p720.allow_frame_skip);

    const auto p1080_30 = focus_video::choose_profile({1920, 1080, 30.0, 3840, 2160}, rtx3060ti);
    assert(p1080_30.mode == focus_video::UpscaleMode::Balanced);
    assert(p1080_30.max_queue_depth == 3);

    const auto p1080_60 = focus_video::choose_profile({1920, 1080, 60.0, 3840, 2160}, rtx3060ti);
    assert(p1080_60.mode == focus_video::UpscaleMode::Performance);
    assert(p1080_60.allow_frame_skip);

    const auto p1440_30 = focus_video::choose_profile({2560, 1440, 30.0, 3840, 2160}, rtx3060ti);
    assert(p1440_30.mode == focus_video::UpscaleMode::Performance);
    assert(p1440_30.allow_frame_skip);

    const auto above_4k = focus_video::choose_profile({1920, 1080, 30.0, 7680, 4320}, rtx3060ti);
    assert(above_4k.mode == focus_video::UpscaleMode::Off);
    assert(above_4k.model_scale == 1);

    const focus_video::GpuInfo low_gpu{"NVIDIA GeForce GTX 1660", 6144, false, false};
    const auto unsupported = focus_video::choose_profile({1920, 1080, 30.0}, low_gpu);
    assert(unsupported.mode == focus_video::UpscaleMode::Off);
    assert(unsupported.model_scale == 1);

    std::cout << "profile tests passed\n";
    return 0;
}

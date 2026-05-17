#include "video_pipeline.hpp"

#include <cassert>
#include <iostream>
#include <string>

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

}  // namespace

int main() {
    const auto status = focus_video::accelerator_backend_status();
    const auto report = focus_video::format_accelerator_backend_status(status);
    assert(contains(report, "D3D11 renderer compiled:"));
    assert(contains(report, "NVDEC frame pool compiled: no"));
    assert(contains(report, "TensorRT backend compiled: no"));
    assert(!status.blockers.empty());

    const auto compiled = focus_video::ffmpeg_backend_compiled();
    if (!compiled) {
        try {
            (void)focus_video::probe_video("missing.mp4");
            assert(false && "probe_video should fail when FFmpeg is not compiled");
        } catch (const std::exception& error) {
            assert(contains(error.what(), "without in-process FFmpeg"));
        }
    }

    std::cout << "video pipeline tests passed\n";
    return 0;
}

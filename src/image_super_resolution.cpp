#include "image_super_resolution.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace focus_video {
namespace {

constexpr std::uint8_t clamp_byte(double value) {
    if (value <= 0.0) {
        return 0;
    }
    if (value >= 255.0) {
        return 255;
    }
    return static_cast<std::uint8_t>(value + 0.5);
}

std::size_t pixel_offset(const RgbImage& image, std::uint32_t x, std::uint32_t y) {
    return (static_cast<std::size_t>(y) * image.width + x) * 3;
}

std::array<double, 3> sample_bilinear(const RgbImage& image, double x, double y) {
    const auto max_x = image.width - 1;
    const auto max_y = image.height - 1;
    x = std::clamp(x, 0.0, static_cast<double>(max_x));
    y = std::clamp(y, 0.0, static_cast<double>(max_y));

    const auto x0 = static_cast<std::uint32_t>(std::floor(x));
    const auto y0 = static_cast<std::uint32_t>(std::floor(y));
    const auto x1 = std::min(x0 + 1, max_x);
    const auto y1 = std::min(y0 + 1, max_y);
    const auto wx = x - x0;
    const auto wy = y - y0;

    std::array<double, 3> result{};
    for (std::size_t channel = 0; channel < result.size(); ++channel) {
        const auto p00 = image.pixels[pixel_offset(image, x0, y0) + channel];
        const auto p10 = image.pixels[pixel_offset(image, x1, y0) + channel];
        const auto p01 = image.pixels[pixel_offset(image, x0, y1) + channel];
        const auto p11 = image.pixels[pixel_offset(image, x1, y1) + channel];
        const auto top = p00 * (1.0 - wx) + p10 * wx;
        const auto bottom = p01 * (1.0 - wx) + p11 * wx;
        result[channel] = top * (1.0 - wy) + bottom * wy;
    }
    return result;
}

std::uint8_t channel_at(const RgbImage& image, std::uint32_t x, std::uint32_t y, std::size_t channel) {
    x = std::min(x, image.width - 1);
    y = std::min(y, image.height - 1);
    return image.pixels[pixel_offset(image, x, y) + channel];
}

void sharpen_luma_edges(RgbImage& image, UpscaleMode mode) {
    if (mode == UpscaleMode::Off || mode == UpscaleMode::Performance) {
        return;
    }

    const double amount = mode == UpscaleMode::Quality ? 0.65 : 0.38;
    const auto original = image.pixels;
    const RgbImage source{image.width, image.height, original};

    for (std::uint32_t y = 0; y < image.height; ++y) {
        for (std::uint32_t x = 0; x < image.width; ++x) {
            for (std::size_t channel = 0; channel < 3; ++channel) {
                const double center = channel_at(source, x, y, channel);
                const double neighbors = channel_at(source, x > 0 ? x - 1 : x, y, channel) +
                                         channel_at(source, std::min(x + 1, image.width - 1), y, channel) +
                                         channel_at(source, x, y > 0 ? y - 1 : y, channel) +
                                         channel_at(source, x, std::min(y + 1, image.height - 1), channel);
                const double blur = neighbors * 0.25;
                const double edge = center - blur;
                image.pixels[pixel_offset(image, x, y) + channel] = clamp_byte(center + amount * edge);
            }
        }
    }
}

std::string read_token(std::istream& stream) {
    std::string token;
    char ch = 0;
    while (stream.get(ch)) {
        if (ch == '#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            token.push_back(ch);
            break;
        }
    }

    while (stream.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            break;
        }
        if (ch == '#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        token.push_back(ch);
    }

    if (token.empty()) {
        throw std::runtime_error("Unexpected end of PPM header.");
    }
    return token;
}

}  // namespace

RgbImage read_ppm(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open input PPM: " + path.string());
    }

    const auto magic = read_token(input);
    if (magic != "P6") {
        throw std::runtime_error("Only binary P6 PPM images are supported by the reference exporter.");
    }

    const auto width = static_cast<std::uint32_t>(std::stoul(read_token(input)));
    const auto height = static_cast<std::uint32_t>(std::stoul(read_token(input)));
    const auto max_value = std::stoul(read_token(input));
    if (width == 0 || height == 0 || max_value != 255) {
        throw std::runtime_error("PPM must have non-zero dimensions and 8-bit max value 255.");
    }

    RgbImage image{width, height, std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * 3)};
    input.read(reinterpret_cast<char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size()));
    if (input.gcount() != static_cast<std::streamsize>(image.pixels.size())) {
        throw std::runtime_error("PPM pixel payload is shorter than expected.");
    }
    return image;
}

void write_ppm(const std::filesystem::path& path, const RgbImage& image) {
    if (image.width == 0 || image.height == 0 || image.pixels.size() != static_cast<std::size_t>(image.width) * image.height * 3) {
        throw std::runtime_error("Invalid RGB image dimensions or pixel payload.");
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Unable to open output PPM: " + path.string());
    }
    output << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    output.write(reinterpret_cast<const char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size()));
}

RgbImage upscale_image(const RgbImage& source, std::uint32_t scale, UpscaleMode mode) {
    if (scale < 1 || scale > 4) {
        throw std::runtime_error("Reference upscaler supports integer scales from 1 to 4.");
    }
    if (source.width == 0 || source.height == 0 || source.pixels.size() != static_cast<std::size_t>(source.width) * source.height * 3) {
        throw std::runtime_error("Invalid RGB source image.");
    }
    if (mode == UpscaleMode::Off || scale == 1) {
        return source;
    }

    RgbImage output{source.width * scale, source.height * scale,
                    std::vector<std::uint8_t>(static_cast<std::size_t>(source.width) * source.height * scale * scale * 3)};

    for (std::uint32_t y = 0; y < output.height; ++y) {
        for (std::uint32_t x = 0; x < output.width; ++x) {
            const double source_x = (static_cast<double>(x) + 0.5) / scale - 0.5;
            const double source_y = (static_cast<double>(y) + 0.5) / scale - 0.5;
            const auto rgb = sample_bilinear(source, source_x, source_y);
            const auto offset = pixel_offset(output, x, y);
            output.pixels[offset] = clamp_byte(rgb[0]);
            output.pixels[offset + 1] = clamp_byte(rgb[1]);
            output.pixels[offset + 2] = clamp_byte(rgb[2]);
        }
    }

    sharpen_luma_edges(output, mode);
    return output;
}

void export_upscaled_ppm(const OfflineExportOptions& options) {
    const auto input = read_ppm(options.input_path);
    const auto output = upscale_image(input, options.scale, options.mode);
    write_ppm(options.output_path, output);
}

}  // namespace focus_video

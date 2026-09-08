#include "common.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<OutputArtifact> monochrome(const std::string& name,
    const std::vector<std::uint8_t>& input, const std::string& mode, int size, int density)
{
    conv::TempDir tmp("conv-monochrome-");
    const auto in = tmp.path() / "input";
    const auto gray = tmp.path() / "gray.pgm";
    const auto stem = conv::basename_no_ext(std::filesystem::path(name.empty() ? "input.png" : name).filename().string());
    const auto out = tmp.path() / (stem + "_" + mode + ".png");
    const std::string cli = conv::program_exists("magick") ? "magick" : "convert";
    conv::write_file_bytes(in, input);
    std::vector<std::string> cmd = {cli, in.string() + "[0]", "-background", "white",
        "-alpha", "remove", "-alpha", "off", "-colorspace", "Gray"};
    if (mode == "dither") {
        cmd.insert(cmd.end(), {"-dither", "FloydSteinberg", "-remap", "pattern:gray50", out.string()});
        conv::require_success(conv::run_process(cmd), cli);
    } else {
        cmd.insert(cmd.end(), {"-depth", "8", "-compress", "none", gray.string()});
        conv::require_success(conv::run_process(cmd), cli);
        std::ifstream stream(gray, std::ios::binary);
        auto token = [&]() {
            std::string value;
            while (stream >> value) {
                if (value[0] != '#') return value;
                std::getline(stream, value);
            }
            throw std::runtime_error("Invalid grayscale image");
        };
        const auto magic = token();
        const int width = std::stoi(token()), height = std::stoi(token());
        const int maxval = std::stoi(token());
        if ((magic != "P5" && magic != "P2") || width < 1 || height < 1 || maxval != 255 ||
            static_cast<std::uint64_t>(width) * height > 100000000)
            throw std::runtime_error("Unsupported grayscale image dimensions or depth");
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height);
        if (magic == "P5") {
            stream.get();
            stream.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
            if (!stream) throw std::runtime_error("Truncated grayscale image");
        } else {
            for (auto& p : pixels) p = static_cast<std::uint8_t>(std::stoi(token()));
        }
        std::vector<int> ranks(size * size);
        if (mode == "halftone") {
            // Rank samples by distance from the cell center: increasing ink grows a dot.
            std::vector<int> order(size * size);
            std::iota(order.begin(), order.end(), 0);
            auto distance = [size](int i) {
                const double x = i % size - (size - 1) / 2.0, y = i / size - (size - 1) / 2.0;
                return x*x + y*y;
            };
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return distance(a) < distance(b); });
            for (int i = 0; i < size*size; ++i) ranks[order[i]] = i;
        } else {
            // Recursive Bayer matrix, supporting power-of-two grids.
            for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
                int rank = 0;
                for (int bit = 0; (1 << bit) < size; ++bit)
                    rank = 4 * rank + 2 * (((x >> bit) ^ (y >> bit)) & 1) + ((y >> bit) & 1);
                ranks[y*size+x] = rank;
            }
        }
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            auto& p = pixels[static_cast<std::size_t>(y)*width+x];
            const double ink = 1.0 - std::pow(p / 255.0, density / 50.0);
            const double threshold = (ranks[(y%size)*size+x%size] + 0.5) / (size*size);
            p = ink > threshold ? 0 : 255;
        }
        std::ofstream result(gray, std::ios::binary | std::ios::trunc);
        result << "P5\n" << width << ' ' << height << "\n255\n";
        result.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
        result.close();
        conv::require_success(conv::run_process({cli, gray.string(), out.string()}), cli);
    }
    return {conv::make_artifact_from_file(out)};
}
}

std::vector<OutputArtifact> convert_dither(const std::string& n, const std::vector<std::uint8_t>& i) {
    return monochrome(n, i, "dither", 4, 50);
}
std::vector<OutputArtifact> convert_halftone(const std::string& n, const std::vector<std::uint8_t>& i, int density, int size) {
    return monochrome(n, i, "halftone", std::clamp(size, 2, 32), std::clamp(density, 1, 100));
}
std::vector<OutputArtifact> convert_bayer(const std::string& n, const std::vector<std::uint8_t>& i, int size) {
    if (size != 2 && size != 4 && size != 8 && size != 16) throw std::runtime_error("Bayer grid must be 2, 4, 8 or 16");
    return monochrome(n, i, "bayer", size, 50);
}

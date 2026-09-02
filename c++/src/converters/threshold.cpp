#include "common.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::vector<OutputArtifact> convert_threshold(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input,
    int threshold_percent)
{
    threshold_percent = std::clamp(threshold_percent, 1, 100);

    conv::TempDir tmp("conv-threshold-");
    const std::string safe_name = input_name.empty() ? "input.png" : input_name;
    const fs::path in_path = tmp.path() / safe_name;
    const fs::path out_path = tmp.path() /
        (conv::basename_no_ext(in_path.filename().string()) + "_threshold.png");

    conv::write_file_bytes(in_path, input);

    std::vector<std::string> command;
    if (conv::program_exists("magick")) {
        command = {
            "magick", in_path.string(),
            "-colorspace", "Gray",
            "-threshold", std::to_string(threshold_percent) + "%",
            out_path.string()
        };
    } else {
        command = {
            "convert", in_path.string(),
            "-colorspace", "Gray",
            "-threshold", std::to_string(threshold_percent) + "%",
            out_path.string()
        };
    }

    auto result = conv::run_process(command);
    conv::require_success(result, command[0]);

    return {conv::make_artifact_from_file(out_path)};
}

#include "common.hpp"

#include <string>
#include <vector>

std::vector<OutputArtifact> convert_hex(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.bin";
    out_name += ".hex.txt";

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "text/plain";
        artifact.data = "";
        return { artifact };
    }

    static const char hex_chars[] = "0123456789abcdef";
    std::string hex_str;
    hex_str.reserve(input.size() * 2 + 1);

    for (std::uint8_t b : input) {
        hex_str.push_back(hex_chars[b >> 4]);
        hex_str.push_back(hex_chars[b & 0x0F]);
    }
    hex_str.push_back('\n');

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "text/plain";
    artifact.data = std::move(hex_str);

    return { artifact };
}

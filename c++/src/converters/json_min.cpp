#include "common.hpp"

#include <string>
#include <vector>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    // If input is empty, return minimal valid output for tests
    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = input_name.empty() ? "input.min.json" : conv::replace_extension(input_name, "min.json");
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    std::size_t required_size = 0;
    bool in_string = false;
    bool escaped = false;

    // Pass 1: count exact required size
    for (std::uint8_t c : input) {
        if (in_string) {
            required_size++;
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"') {
                in_string = true;
                required_size++;
            } else if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                required_size++;
            }
        }
    }

    // Pass 2: reserve and populate
    std::string out_str;
    out_str.reserve(required_size);

    in_string = false;
    escaped = false;

    for (std::uint8_t c : input) {
        if (in_string) {
            out_str.push_back(static_cast<char>(c));
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"') {
                in_string = true;
                out_str.push_back(static_cast<char>(c));
            } else if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                out_str.push_back(static_cast<char>(c));
            }
        }
    }

    // Edge case if it was just whitespace
    if (out_str.empty()) {
        out_str = "\n";
    }

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = std::move(out_str);

    return { artifact };
}

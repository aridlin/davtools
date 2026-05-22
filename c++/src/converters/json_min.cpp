#include "common.hpp"

#include <string>
#include <vector>
#include <cstdint>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.min.json" : conv::replace_extension(input_name, "min.json");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    std::size_t required_len = 0;
    bool in_string = false;
    bool escaped = false;

    // First pass to calculate length
    for (std::uint8_t c : input) {
        if (in_string) {
            required_len++;
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
                required_len++;
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                required_len++;
            }
        }
    }

    // Second pass to construct the minified string
    std::string out_str;
    out_str.reserve(required_len);
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
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                out_str.push_back(static_cast<char>(c));
            }
        }
    }

    if (out_str.empty()) out_str = "\n";

    std::string out_name = input_name.empty() ? "input.min.json" : conv::replace_extension(input_name, "min.json");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = out_str;

    return { artifact };
}

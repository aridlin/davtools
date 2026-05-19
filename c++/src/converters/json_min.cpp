#include "common.hpp"

#include <vector>
#include <string>
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

    std::size_t required_size = 0;
    bool in_string = false;
    bool in_escape = false;

    // First pass: Calculate required size
    for (auto c : input) {
        if (in_string) {
            required_size++;
            if (in_escape) {
                in_escape = false;
            } else if (c == '\\') {
                in_escape = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"') {
                in_string = true;
                required_size++;
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                required_size++;
            }
        }
    }

    std::vector<std::uint8_t> output;
    output.reserve(required_size + 1); // +1 for trailing newline

    in_string = false;
    in_escape = false;

    // Second pass: append
    for (auto c : input) {
        if (in_string) {
            output.push_back(c);
            if (in_escape) {
                in_escape = false;
            } else if (c == '\\') {
                in_escape = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"') {
                in_string = true;
                output.push_back(c);
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                output.push_back(c);
            }
        }
    }

    if (output.empty() || output.back() != '\n') {
        output.push_back('\n');
    }

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = std::string(output.begin(), output.end());

    return { artifact };
}

#include "common.hpp"

#include <cstdint>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    // First pass: calculate total size needed
    std::size_t required_size = 0;
    bool in_string = false;
    bool escape = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (in_string) {
            required_size++;
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
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

    // Second pass: append to string
    std::string out_data;
    out_data.reserve(required_size);

    in_string = false;
    escape = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (in_string) {
            out_data.push_back(c);
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"') {
                in_string = true;
                out_data.push_back(c);
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                out_data.push_back(c);
            }
        }
    }

    if (out_data.empty()) {
        out_data = "\n";
    }

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = std::move(out_data);

    return { artifact };
}

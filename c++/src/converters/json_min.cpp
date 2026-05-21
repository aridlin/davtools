#include "common.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::size_t required_size = 0;
    bool in_string = false;
    bool escape = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        std::uint8_t c = input[i];
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
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                // skip whitespace
            } else {
                required_size++;
                if (c == '"') {
                    in_string = true;
                }
            }
        }
    }

    if (in_string) {
        throw std::runtime_error("invalid JSON: unclosed string");
    }

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    if (required_size == 0) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    std::string out_str;
    out_str.reserve(required_size + 1);

    in_string = false;
    escape = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        std::uint8_t c = input[i];
        if (in_string) {
            out_str.push_back(static_cast<char>(c));
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                // skip whitespace
            } else {
                out_str.push_back(static_cast<char>(c));
                if (c == '"') {
                    in_string = true;
                }
            }
        }
    }

    out_str.push_back('\n');

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = out_str;

    return { artifact };
}

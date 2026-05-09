#include "common.hpp"

#include <string>
#include <vector>
#include <cstdint>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.json" : input_name;
        out_name = conv::replace_extension(out_name, "min.json");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    // Two-pass approach to avoid reallocations:
    // Pass 1: count required bytes
    std::size_t required_len = 0;
    bool in_string = false;
    bool escape = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (in_string) {
            required_len++;
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
                required_len++;
            } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                // skip whitespace
            } else {
                required_len++;
            }
        }
    }

    // In case the input is entirely whitespace, return a minimal valid response (or just empty as `\n`)
    if (required_len == 0) {
        std::string out_name = input_name.empty() ? "input.json" : input_name;
        out_name = conv::replace_extension(out_name, "min.json");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    // Pass 2: reserve and append
    std::string minified;
    minified.reserve(required_len + 1); // +1 for the newline at the end

    in_string = false;
    escape = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (in_string) {
            minified += c;
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
                minified += c;
            } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                // skip whitespace
            } else {
                minified += c;
            }
        }
    }

    // Add newline at the end for Unix conventions
    minified += '\n';

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = minified;

    return { artifact };
}

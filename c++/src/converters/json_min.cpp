#include "common.hpp"

#include <cstdint>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::size_t size = 0;
    bool in_string = false;
    bool escaped = false;

    // First pass: calculate required size to avoid reallocations
    for (std::uint8_t c : input) {
        if (in_string) {
            size++;
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
                size++;
            } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                // skip whitespace
            } else {
                size++;
            }
        }
    }

    std::string out;
    if (size == 0) {
        out = "\n"; // Minimal non-empty output for 0-byte or fully stripped
    } else {
        out.reserve(size);

        in_string = false;
        escaped = false;

        // Second pass: append to pre-allocated string
        for (std::uint8_t c : input) {
            if (in_string) {
                out.push_back(c);
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
                    out.push_back(c);
                } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                    // skip whitespace
                } else {
                    out.push_back(c);
                }
            }
        }
    }

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = out;

    return { artifact };
}
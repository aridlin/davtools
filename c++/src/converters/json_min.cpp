#include "common.hpp"

#include <vector>
#include <string>

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    // Scan first to calculate total required size
    std::size_t required_size = 0;
    bool in_string = false;
    bool escape = false;

    for (std::uint8_t c : input) {
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

    // Reserve and append pass
    std::string minified;
    minified.reserve(required_size);

    in_string = false;
    escape = false;
    for (std::uint8_t c : input) {
        if (in_string) {
            minified.push_back(static_cast<char>(c));
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
                minified.push_back(static_cast<char>(c));
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                minified.push_back(static_cast<char>(c));
            }
        }
    }

    if (minified.empty()) {
        minified = "\n";
    }

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";
    out_name = conv::replace_extension(out_name, "min.json");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = std::move(minified);

    return { artifact };
}

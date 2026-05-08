#include "common.hpp"

#include <string>
#include <vector>
#include <cstdint>

std::vector<OutputArtifact> convert_json_minify(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.json.min.json" : input_name + ".min.json";
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    std::size_t count = 0;
    bool in_string = false;
    bool escaped = false;

    // First pass: count required characters
    for (std::size_t i = 0; i < input.size(); ++i) {
        std::uint8_t c = input[i];
        if (in_string) {
            count++;
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
                count++;
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                count++;
            }
        }
    }

    if (count == 0) {
        std::string out_name = input_name.empty() ? "input.json.min.json" : input_name + ".min.json";
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    // Second pass: append to string
    std::string result;
    result.reserve(count);

    in_string = false;
    escaped = false;
    for (std::size_t i = 0; i < input.size(); ++i) {
        std::uint8_t c = input[i];
        if (in_string) {
            result.push_back(static_cast<char>(c));
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
                result.push_back(static_cast<char>(c));
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                result.push_back(static_cast<char>(c));
            }
        }
    }

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.json";

    // Attempt to rename to .min.json
    std::string test_name = conv::replace_extension(out_name, "min.json");
    if (test_name == out_name) {
        out_name += ".min.json";
    } else {
        out_name = test_name;
    }

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/json";
    artifact.data = result;

    return { artifact };
}

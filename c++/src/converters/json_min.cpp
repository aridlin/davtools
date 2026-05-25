#include "common.hpp"

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

    bool in_string = false;
    bool escaped = false;
    std::size_t count = 0;

    for (auto c : input) {
        if (in_string) {
            count++;
            if (escaped) {
                escaped = false;
            } else {
                if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_string = false;
                }
            }
        } else {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                continue;
            }
            count++;
            if (c == '"') {
                in_string = true;
            }
        }
    }

    std::string out;
    out.reserve(count);

    in_string = false;
    escaped = false;

    for (auto c : input) {
        if (in_string) {
            out.push_back(static_cast<char>(c));
            if (escaped) {
                escaped = false;
            } else {
                if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_string = false;
                }
            }
        } else {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                continue;
            }
            out.push_back(static_cast<char>(c));
            if (c == '"') {
                in_string = true;
            }
        }
    }

    if (out.empty()) {
        out = "\n";
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

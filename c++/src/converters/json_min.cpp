#include "common.hpp"

#include <string>

namespace {

bool is_json_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

std::string minify_json_bytes(const std::vector<std::uint8_t>& input) {
    std::string out;
    out.reserve(input.size());

    bool in_string = false;
    bool escaped = false;

    for (std::uint8_t byte : input) {
        const char c = static_cast<char>(byte);

        if (in_string) {
            out.push_back(c);
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }

        if (c == '"') {
            in_string = true;
            out.push_back(c);
        } else if (!is_json_whitespace(c)) {
            out.push_back(c);
        }
    }

    return out;
}

} // namespace

std::vector<OutputArtifact> convert_json_min(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.min.json" : conv::replace_extension(input_name, "min.json");
    std::string data = minify_json_bytes(input);
    if (data.empty()) {
        data = "\n";
    }

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

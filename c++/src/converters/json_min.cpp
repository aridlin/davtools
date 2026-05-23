#include "registry.hpp"
#include "common.hpp"

#include <cstdint>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_json_min(const std::string& input_name, const std::vector<std::uint8_t>& input) {
    std::size_t reserved_size = 0;
    bool in_string = false;
    bool escape = false;

    // First pass to calculate required size
    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (!in_string) {
            if (c == '"') {
                in_string = true;
                reserved_size++;
            } else if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                reserved_size++;
            }
        } else {
            reserved_size++;
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
        }
    }

    if (reserved_size == 0) {
        return {
            OutputArtifact{
                .name = conv::replace_extension(input_name, "min.json"),
                .content_type = "application/json",
                .data = "\n"
            }
        };
    }

    std::string out;
    out.reserve(reserved_size);

    in_string = false;
    escape = false;

    // Second pass to construct the string
    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (!in_string) {
            if (c == '"') {
                in_string = true;
                out.push_back(c);
            } else if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                out.push_back(c);
            }
        } else {
            out.push_back(c);
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
        }
    }

    return {
        OutputArtifact{
            .name = conv::replace_extension(input_name, "min.json"),
            .content_type = "application/json",
            .data = out
        }
    };
}

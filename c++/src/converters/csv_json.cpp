#include "common.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

namespace {

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> result;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i+1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                result.push_back(current);
                current.clear();
            } else if (c == '\r' || c == '\n') {
                // Ignore
            } else {
                current += c;
            }
        }
    }
    result.push_back(current);
    return result;
}

std::string escape_json_string(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 2);
    out += '"';
    for (char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) <= 0x1F) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += '"';
    return out;
}

} // namespace

std::vector<OutputArtifact> convert_csv_json(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.json" : conv::replace_extension(input_name, "json");

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = std::move(out_name);
        artifact.content_type = "application/json";
        artifact.data = "\n";
        return { artifact };
    }

    std::string in_str(input.begin(), input.end());
    std::stringstream ss(in_str);
    std::string line;

    std::vector<std::string> headers;
    if (std::getline(ss, line)) {
        headers = parse_csv_line(line);
    }

    std::string data = "[";
    bool first_row = true;
    while (std::getline(ss, line)) {
        if (line.empty() || line == "\r") continue;
        if (!first_row) {
            data += ",";
        }
        first_row = false;

        std::vector<std::string> values = parse_csv_line(line);
        data += "{";
        for (size_t i = 0; i < values.size(); ++i) {
            std::string header = i < headers.size() ? headers[i] : "col" + std::to_string(i);
            data += escape_json_string(header) + ":" + escape_json_string(values[i]);
            if (i + 1 < values.size()) {
                data += ",";
            }
        }
        data += "}";
    }
    data += "]\n";

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

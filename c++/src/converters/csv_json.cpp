#include "common.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace {

std::string json_escape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
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
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
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
                tokens.push_back(current);
                current.clear();
            } else if (c == '\r' || c == '\n') {
                // ignore
            } else {
                current += c;
            }
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string convert_csv_to_json(const std::vector<std::uint8_t>& input) {
    std::string in_str(input.begin(), input.end());
    std::istringstream stream(in_str);
    std::string line;

    if (!std::getline(stream, line)) {
        return "[]";
    }

    std::vector<std::string> headers = split_csv_line(line);

    std::string out = "[";
    bool first_row = true;

    while (std::getline(stream, line)) {
        if (line.empty() || (line.size() == 1 && line[0] == '\r')) continue;

        if (!first_row) {
            out += ",";
        }
        first_row = false;

        std::vector<std::string> tokens = split_csv_line(line);

        out += "{";
        for (size_t i = 0; i < headers.size() && i < tokens.size(); ++i) {
            if (i > 0) out += ",";
            out += "\"" + json_escape(headers[i]) + "\":\"" + json_escape(tokens[i]) + "\"";
        }
        out += "}";
    }

    out += "]";
    return out;
}

} // namespace

std::vector<OutputArtifact> convert_csv_json(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.json" : conv::replace_extension(input_name, "json");

    std::string data;
    if (input.empty()) {
        data = "";
    } else {
        data = convert_csv_to_json(input);
    }

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

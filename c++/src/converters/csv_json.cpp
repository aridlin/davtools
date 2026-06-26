#include "common.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace {

std::string escape_json_string(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 2);
    out.push_back('"');
    for (char c : input) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (static_cast<unsigned char>(c) <= 0x1F) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i+1] == '"') {
                    current.push_back('"');
                    i++;
                } else {
                    in_quotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else if (c != '\r') {
                current.push_back(c);
            }
        }
    }
    fields.push_back(current);
    return fields;
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
        artifact.data = "";
        return { artifact };
    }

    std::string input_str(input.begin(), input.end());
    std::istringstream stream(input_str);
    std::string line;

    std::vector<std::string> headers;
    if (std::getline(stream, line)) {
        headers = parse_csv_line(line);
    }

    std::string data = "[";
    bool first_row = true;
    while (std::getline(stream, line)) {
        if (line.empty() || (line.size() == 1 && line[0] == '\r')) continue;

        std::vector<std::string> fields = parse_csv_line(line);
        if (fields.empty()) continue;

        if (!first_row) data += ",";
        data += "{";

        for (size_t i = 0; i < fields.size(); ++i) {
            std::string header = (i < headers.size()) ? headers[i] : "field" + std::to_string(i);
            if (i > 0) data += ",";
            data += escape_json_string(header) + ":" + escape_json_string(fields[i]);
        }

        data += "}";
        first_row = false;
    }
    data += "]\n";

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

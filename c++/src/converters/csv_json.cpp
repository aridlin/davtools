#include "common.hpp"

#include <string>
#include <vector>
#include <cstdio>

namespace {

std::vector<std::vector<std::string>> parse_csv(const std::string& data) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> current_row;
    std::string current_field;
    bool in_quotes = false;

    for (size_t i = 0; i < data.size(); ++i) {
        char c = data[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < data.size() && data[i + 1] == '"') {
                    current_field += '"';
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                current_field += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                current_row.push_back(current_field);
                current_field.clear();
            } else if (c == '\n') {
                if (!current_field.empty() && current_field.back() == '\r') {
                    current_field.pop_back();
                }
                current_row.push_back(current_field);
                current_field.clear();
                rows.push_back(current_row);
                current_row.clear();
            } else if (c == '\r') {
                current_field += c;
            } else {
                current_field += c;
            }
        }
    }

    if (!current_row.empty() || !current_field.empty()) {
        if (!current_field.empty() && current_field.back() == '\r') {
            current_field.pop_back();
        }
        current_row.push_back(current_field);
        rows.push_back(current_row);
    }

    return rows;
}

std::string escape_json_string(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 2);
    out += '"';
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
            out += c;
        }
    }
    out += '"';
    return out;
}

std::string csv_to_json(const std::string& csv_data) {
    if (csv_data.empty()) return "[]";
    auto rows = parse_csv(csv_data);
    if (rows.empty()) return "[]";
    if (rows.size() == 1 && rows[0].empty()) return "[]";

    auto headers = rows[0];

    std::string json = "[\n";
    for (size_t i = 1; i < rows.size(); ++i) {
        json += "  {";
        const auto& row = rows[i];
        for (size_t j = 0; j < headers.size(); ++j) {
            json += escape_json_string(headers[j]) + ": ";
            if (j < row.size()) {
                json += escape_json_string(row[j]);
            } else {
                json += "\"\"";
            }
            if (j + 1 < headers.size()) json += ", ";
        }
        json += "}";
        if (i + 1 < rows.size()) json += ",";
        json += "\n";
    }
    json += "]";
    return json;
}

} // namespace

std::vector<OutputArtifact> convert_csv_json(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.json" : conv::replace_extension(input_name, "json");

    std::string csv_str(input.begin(), input.end());
    std::string json_str = csv_to_json(csv_str);

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(json_str);
    return { artifact };
}

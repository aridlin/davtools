#include "common.hpp"

#include <string>
#include <vector>

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
        else if (static_cast<unsigned char>(c) <= 0x1f) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
            out += buf;
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

std::vector<std::vector<std::string>> parse_csv(const std::vector<std::uint8_t>& input) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> current_row;
    std::string current_field;
    bool in_quotes = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < input.size() && static_cast<char>(input[i + 1]) == '"') {
                    current_field.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                current_field.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                current_row.push_back(current_field);
                current_field.clear();
            } else if (c == '\n' || c == '\r') {
                if (c == '\r' && i + 1 < input.size() && static_cast<char>(input[i + 1]) == '\n') {
                    ++i;
                }
                // Skip empty lines unless we already have fields parsed
                if (!current_field.empty() || !current_row.empty()) {
                    current_row.push_back(current_field);
                    current_field.clear();
                    rows.push_back(current_row);
                    current_row.clear();
                }
            } else {
                current_field.push_back(c);
            }
        }
    }
    if (!current_field.empty() || !current_row.empty() || (!input.empty() && static_cast<char>(input.back()) == ',')) {
        current_row.push_back(current_field);
        rows.push_back(current_row);
    }
    return rows;
}

} // namespace

std::vector<OutputArtifact> convert_csv_json(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name;
    if (input_name.empty()) {
        out_name = "input.json";
    } else {
        out_name = conv::replace_extension(input_name, "json");
    }

    std::string data = "[\n";
    if (input.empty()) {
        data = "[]\n";
    } else {
        auto rows = parse_csv(input);
        if (rows.size() > 1) {
            const auto& headers = rows[0];
            for (std::size_t i = 1; i < rows.size(); ++i) {
                data += "  {\n";
                const auto& row = rows[i];
                for (std::size_t j = 0; j < headers.size(); ++j) {
                    std::string key = escape_json_string(headers[j]);
                    std::string val = j < row.size() ? escape_json_string(row[j]) : "\"\"";
                    data += "    " + key + ": " + val;
                    if (j + 1 < headers.size()) data += ",";
                    data += "\n";
                }
                data += "  }";
                if (i + 1 < rows.size()) data += ",";
                data += "\n";
            }
            data += "]\n";
        } else if (rows.size() == 1) {
            data += "  {\n";
            const auto& row = rows[0];
            for (std::size_t j = 0; j < row.size(); ++j) {
                data += "    \"field" + std::to_string(j+1) + "\": " + escape_json_string(row[j]);
                if (j + 1 < row.size()) data += ",";
                data += "\n";
            }
            data += "  }\n]\n";
        } else {
            data = "[]\n";
        }
    }

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

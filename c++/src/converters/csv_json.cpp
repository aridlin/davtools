#include "common.hpp"

#include <string>
#include <vector>

namespace {

std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2); // heuristic
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string parse_csv_to_json(const std::vector<std::uint8_t>& input) {
    if (input.empty()) return "[\n]\n";

    std::string json = "[\n";
    bool in_quotes = false;
    std::string current_field;
    std::vector<std::string> current_row;
    bool first_row = true;

    auto finish_field = [&]() {
        current_row.push_back(current_field);
        current_field.clear();
    };

    auto finish_row = [&]() {
        finish_field();
        if (current_row.size() == 1 && current_row[0].empty() && !in_quotes) {
            current_row.clear();
            return; // ignore completely empty lines
        }

        if (!first_row) {
            json += ",\n";
        }
        first_row = false;
        json += "  [";
        for (std::size_t i = 0; i < current_row.size(); ++i) {
            json += "\"" + escape_json(current_row[i]) + "\"";
            if (i + 1 < current_row.size()) json += ",";
        }
        json += "]";
        current_row.clear();
    };

    for (std::size_t i = 0; i < input.size(); ++i) {
        char c = static_cast<char>(input[i]);
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < input.size() && input[i+1] == '"') {
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
                finish_field();
            } else if (c == '\r') {
                if (i + 1 < input.size() && input[i+1] == '\n') {
                    finish_row();
                    ++i;
                } else {
                    finish_row();
                }
            } else if (c == '\n') {
                finish_row();
            } else {
                current_field += c;
            }
        }
    }

    if (!current_field.empty() || in_quotes || !current_row.empty()) {
        finish_row();
    }

    json += "\n]";
    return json;
}

} // namespace

std::vector<OutputArtifact> convert_csv_json(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.json" : conv::replace_extension(input_name, "json");

    std::string data = parse_csv_to_json(input);

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

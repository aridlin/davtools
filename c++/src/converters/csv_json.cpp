#include "common.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace {

std::string csv_to_json_str(const std::vector<std::uint8_t>& input) {
    if (input.empty()) return "";

    std::string csv(input.begin(), input.end());

    std::vector<std::string> current_row;
    std::vector<std::vector<std::string>> rows;

    bool in_quotes = false;
    std::string current_field;

    for (size_t i = 0; i < csv.size(); ++i) {
        char c = csv[i];
        if (c == '"') {
            if (in_quotes && i + 1 < csv.size() && csv[i + 1] == '"') {
                current_field += '"';
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            current_row.push_back(current_field);
            current_field.clear();
        } else if ((c == '\n' || c == '\r') && !in_quotes) {
            if (c == '\r' && i + 1 < csv.size() && csv[i + 1] == '\n') {
                ++i;
            }
            current_row.push_back(current_field);
            current_field.clear();
            rows.push_back(current_row);
            current_row.clear();
        } else {
            current_field += c;
        }
    }

    if (!current_field.empty() || !current_row.empty()) {
        current_row.push_back(current_field);
        rows.push_back(current_row);
    }

    if (rows.empty()) return "[]";
    if (rows.size() == 1) return "[]"; // Only headers

    auto& headers = rows[0];
    std::ostringstream json;
    json << "[\n";
    for (size_t r = 1; r < rows.size(); ++r) {
        json << "  {\n";
        auto& row = rows[r];
        for (size_t c = 0; c < headers.size(); ++c) {
            json << "    \"";
            // Escape header
            for (char h : headers[c]) {
                if (h == '"') json << "\\\"";
                else if (h == '\\') json << "\\\\";
                else if (h == '\n') json << "\\n";
                else if (h == '\r') json << "\\r";
                else if (h == '\t') json << "\\t";
                else json << h;
            }
            json << "\": \"";
            std::string val = c < row.size() ? row[c] : "";
            // Escape value
            for (char v : val) {
                if (v == '"') json << "\\\"";
                else if (v == '\\') json << "\\\\";
                else if (v == '\n') json << "\\n";
                else if (v == '\r') json << "\\r";
                else if (v == '\t') json << "\\t";
                else json << v;
            }
            json << "\"";
            if (c + 1 < headers.size()) json << ",";
            json << "\n";
        }
        json << "  }";
        if (r + 1 < rows.size()) json << ",\n";
        else json << "\n";
    }
    json << "]"; // Omit trailing newline for output determinism consistency
    return json.str();
}

} // namespace

std::vector<OutputArtifact> convert_csv_json(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.json" : conv::replace_extension(input_name, "json");
    std::string data = csv_to_json_str(input);
    if (data.empty()) {
        data = "";
    }

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

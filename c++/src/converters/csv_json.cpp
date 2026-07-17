#include "common.hpp"

#include <string>
#include <vector>

namespace {

std::string csv_to_json(const std::vector<std::uint8_t>& input) {
    if (input.empty()) return "[]";
    std::string in_str(input.begin(), input.end());

    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> current_row;
    std::string current_cell;
    bool in_quotes = false;

    for (size_t i = 0; i < in_str.size(); ++i) {
        char c = in_str[i];
        if (c == '"') {
            if (in_quotes && i + 1 < in_str.size() && in_str[i + 1] == '"') {
                current_cell += '"';
                i++; // skip escaped quote
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            current_row.push_back(current_cell);
            current_cell.clear();
        } else if ((c == '\n' || c == '\r') && !in_quotes) {
            if (c == '\r' && i + 1 < in_str.size() && in_str[i+1] == '\n') {
                i++;
            }
            current_row.push_back(current_cell);
            current_cell.clear();
            // Don't add completely empty rows unless it's the only thing
            if (!current_row.empty() && !(current_row.size() == 1 && current_row[0].empty())) {
                rows.push_back(current_row);
            }
            current_row.clear();
        } else {
            current_cell += c;
        }
    }
    if (!current_cell.empty() || !current_row.empty()) {
        current_row.push_back(current_cell);
        if (!(current_row.size() == 1 && current_row[0].empty())) {
            rows.push_back(current_row);
        }
    }

    if (rows.empty()) return "[]";
    if (rows.size() == 1) return "[]"; // only headers, no data

    std::vector<std::string> headers = rows[0];

    std::string out = "[\n";
    for (size_t i = 1; i < rows.size(); ++i) {
        out += "  {";
        const auto& row = rows[i];
        bool first_cell = true;
        for (size_t j = 0; j < headers.size() && j < row.size(); ++j) {
            if (!first_cell) out += ",";
            out += "\"";
            for (char cc : headers[j]) {
                if (cc == '"') out += "\\\"";
                else if (cc == '\\') out += "\\\\";
                else if (cc == '\n') out += "\\n";
                else if (cc == '\r') out += "\\r";
                else if (cc == '\t') out += "\\t";
                else out += cc;
            }
            out += "\":\"";
            for (char cc : row[j]) {
                if (cc == '"') out += "\\\"";
                else if (cc == '\\') out += "\\\\";
                else if (cc == '\n') out += "\\n";
                else if (cc == '\r') out += "\\r";
                else if (cc == '\t') out += "\\t";
                else out += cc;
            }
            out += "\"";
            first_cell = false;
        }
        out += "}";
        if (i + 1 < rows.size()) out += ",";
        out += "\n";
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
    std::string data = csv_to_json(input);

    OutputArtifact artifact;
    artifact.name = std::move(out_name);
    artifact.content_type = "application/json";
    artifact.data = std::move(data);
    return { artifact };
}

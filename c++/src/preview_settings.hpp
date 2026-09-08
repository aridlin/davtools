#pragma once
#include "app.hpp"
#include <optional>
#include <utility>

std::vector<std::pair<std::string, std::vector<int>>> preview_fields(std::string_view op);
std::optional<std::pair<std::string, int>> parse_preview_setting(std::string_view op, std::string name);
void apply_preview_setting(ConverterOptions& options, const std::string& field, int value);
std::string settings_preview(std::string_view op, const std::string& name, ConverterOptions options);

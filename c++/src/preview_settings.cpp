#include "preview_settings.hpp"
#include "bliss.hpp"
#include "converters/common.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

std::vector<std::pair<std::string, std::vector<int>>> preview_fields(std::string_view op) {
    auto range = [](int a, int b) { std::vector<int> v(b-a+1); std::iota(v.begin(), v.end(), a); return v; };
    if (op == "threshold") return {{"value", range(1, 100)}};
    if (op == "halftone") return {{"density", range(1, 100)}, {"size", range(2, 64)}};
    if (op == "dither") return {{"method", {1, 2}}, {"tone", range(1, 100)}, {"grain", range(1, 8)}};
    if (op == "bayer") return {{"grid", {2, 4, 8, 16}}};
    return {};
}
std::optional<std::pair<std::string, int>> parse_preview_setting(std::string_view op, std::string name) {
    if (name.ends_with(".png")) name.resize(name.size()-4);
    const auto slash = name.find('/');
    if (slash == std::string::npos) return {};
    const auto field = name.substr(0, slash), raw = name.substr(slash+1);
    if (raw.empty() || !std::all_of(raw.begin(), raw.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return {};
    try {
        const int value = std::stoi(raw);
        for (const auto& [key, values] : preview_fields(op))
            if (field == key && std::find(values.begin(), values.end(), value) != values.end()) return {{field, value}};
    } catch (...) {}
    return {};
}
void apply_preview_setting(ConverterOptions& o, const std::string& f, int v) {
    if (f == "value") o.threshold_percent = v;
    if (f == "density") o.halftone_density = v;
    if (f == "size") o.halftone_size = v;
    if (f == "grid") o.bayer_grid = v;
    if (f == "method") o.dither_method = v;
    if (f == "tone") o.dither_tone = v;
    if (f == "grain") o.dither_grain = v;
}

static const std::vector<std::uint8_t>& source_image() {
    static const std::vector<std::uint8_t> source(std::begin(bliss_bytes), std::end(bliss_bytes));
    return source;
}
std::string settings_preview(std::string_view op, const std::string& name, ConverterOptions options) {
    static std::mutex mutex;
    static std::unordered_map<std::string, std::string> cache;
    if (auto setting = parse_preview_setting(op, name)) apply_preview_setting(options, setting->first, setting->second);
    else if (name != "source.png" && name != "current.png") throw std::runtime_error("Unknown preview");
    const auto key = name == "source.png" ? "source" : std::string(op) + ':' + std::to_string(options.threshold_percent) + ':' +
        std::to_string(options.halftone_density) + ':' + std::to_string(options.halftone_size) + ':' + std::to_string(options.bayer_grid) + ':' + std::to_string(options.dither_method) + ':' + std::to_string(options.dither_tone) + ':' + std::to_string(options.dither_grain);
    std::scoped_lock lock(mutex);
    if (auto it = cache.find(key); it != cache.end()) return it->second;
    std::string png;
    if (name == "source.png") {
        conv::TempDir tmp("conv-preview-source-");
        const auto in=tmp.path()/"source.jpg", out=tmp.path()/"source.png";
        conv::write_file_bytes(in,source_image());
        const std::string cli=conv::program_exists("magick")?"magick":"convert";
        conv::require_success(conv::run_process({cli,in.string(),"-strip",out.string()}),cli);
        png=conv::make_artifact_from_file(out).data;
    } else png=run_converter(op, "source.png", source_image(), options).at(0).data;
    if (cache.size() >= 32) cache.clear();
    cache.emplace(key, png);
    return png;
}

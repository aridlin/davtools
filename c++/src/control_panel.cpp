#define FTUI_IMPLEMENTATION
#include "ftui.hpp"

#include "app.hpp"
#include "converters/registry.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <stop_token>
#include <string>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

struct PanelState {
    char bind_ip[64] = "0.0.0.0";
    char port[16] = "8080";
    std::atomic_bool running = false;
    std::atomic_bool busy = false;
    std::atomic_bool last_run_failed = false;
    std::stop_source stop_source;
    std::jthread server_thread;
    std::mutex log_mtx;
    std::string log;
};

void append_log(PanelState& state, const std::string& line) {
    std::scoped_lock lock(state.log_mtx);
    state.log += line;
    state.log += "\n";
    constexpr std::size_t max_log = 64 * 1024;
    if (state.log.size() > max_log) {
        state.log.erase(0, state.log.size() - max_log);
    }
}

std::string log_snapshot(PanelState& state) {
    std::scoped_lock lock(state.log_mtx);
    return state.log;
}

std::string explorer_path(const PanelState& state) {
    std::string host = state.bind_ip;
    if (host.empty() || host == "0.0.0.0" || host == "::") {
        host = "127.0.0.1";
    }
    return "\\\\" + host + "@" + state.port + "\\convert";
}

#if defined(_WIN32)
bool copy_text_to_clipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();

    const int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wide_len <= 0) {
        CloseClipboard();
        return false;
    }

    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(wide_len) * sizeof(wchar_t));
    if (!memory) {
        CloseClipboard();
        return false;
    }

    auto* wide = static_cast<wchar_t*>(GlobalLock(memory));
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide, wide_len);
    GlobalUnlock(memory);

    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}
#else
bool copy_text_to_clipboard(const std::string&) {
    return false;
}
#endif

unsigned short parse_port(const char* text) {
    int value = std::atoi(text);
    if (value < 1 || value > 65535) {
        throw std::runtime_error("Port must be between 1 and 65535");
    }
    return static_cast<unsigned short>(value);
}

void start_server(PanelState& state) {
    if (state.running || state.busy) return;

    std::string bind_ip = state.bind_ip;
    unsigned short port = 0;
    try {
        port = parse_port(state.port);
    } catch (const std::exception& e) {
        append_log(state, std::string("Cannot start: ") + e.what());
        state.last_run_failed = true;
        return;
    }

    state.stop_source = std::stop_source();
    state.running = true;
    state.busy = true;
    state.last_run_failed = false;
    append_log(state, "Starting server...");

    state.server_thread = std::jthread([&state, bind_ip, port](std::stop_token thread_stop) {
        try {
            append_log(state, "[converters] startup self-tests...");
            auto statuses = converter_self_test_all(true);
            std::size_t enabled = 0;
            for (const auto& status : statuses) {
                std::string line = "  - " + status.name + ": " + (status.enabled ? "ENABLED" : "DISABLED");
                if (!status.enabled && !status.reason.empty()) {
                    line += " (" + status.reason + ")";
                }
                append_log(state, line);
                if (status.enabled) ++enabled;
            }
            append_log(state, "[converters] " + std::to_string(enabled) + "/" + std::to_string(statuses.size()) + " enabled");

            std::stop_callback thread_stop_bridge(thread_stop, [&state]() {
                state.stop_source.request_stop();
            });

            run_server(bind_ip, port, state.stop_source.get_token(), [&state](std::string line) {
                append_log(state, line);
            });
        } catch (const std::exception& e) {
            append_log(state, std::string("Fatal: ") + e.what());
            state.last_run_failed = true;
        }
        state.running = false;
        state.busy = false;
    });
}

void stop_server(PanelState& state) {
    if (!state.running) return;
    append_log(state, "Stopping server...");
    state.stop_source.request_stop();
}

void join_stopped_server(PanelState& state) {
    if (!state.running && state.server_thread.joinable()) {
        state.server_thread.join();
    }
}

void shutdown_server(PanelState& state) {
    if (state.running) {
        stop_server(state);
    }
    if (state.server_thread.joinable()) {
        state.server_thread.request_stop();
        state.server_thread.join();
    }
}

void draw_status_line(PanelState& state) {
    const bool running = state.running.load();
    const bool failed = state.last_run_failed.load();
    ftui::row({1.0f, 2.0f}, [&]() {
        if (running) {
            ftui::push_color(ftui::ColorRole::Text, ftui::get_style().success);
            ftui::text("Status: running");
            ftui::pop_color();
        } else if (failed) {
            ftui::push_color(ftui::ColorRole::Text, ftui::get_style().warning);
            ftui::text("Status: stopped after error");
            ftui::pop_color();
        } else {
            ftui::text("Status: stopped");
        }

        char endpoint[160];
        std::snprintf(endpoint, sizeof(endpoint), "Endpoint: http://%s:%s/convert/", state.bind_ip, state.port);
        ftui::text(endpoint);
    });
}

} // namespace

int main() {
    PanelState state;

    ftui::Config cfg;
    cfg.title = "davtools server";
    cfg.width = 880;
    cfg.height = 620;
    cfg.resizable = true;
    cfg.enable_effects = true;

    ftui::set_style(ftui::one_dark_style());
    if (!ftui::create_window(cfg)) {
        return 1;
    }
    ftui::set_window_icon_builtin(ftui::BuiltinIcon::Symbol);

    append_log(state, "Panel ready.");

    while (ftui::pump()) {
        join_stopped_server(state);

        ftui::begin();

        ftui::text("davtools");
        draw_status_line(state);
        ftui::separator();

        const bool controls_disabled = state.running.load() || state.busy.load();
        if (controls_disabled) ftui::begin_disabled();
        ftui::row({2.0f, 1.0f}, [&]() {
            ftui::input("Bind IP", state.bind_ip, sizeof(state.bind_ip));
            ftui::input("Port", state.port, sizeof(state.port), ftui::InputFlags::CharsDecimal);
        });
        if (controls_disabled) ftui::end_disabled();

        std::string explorer_path_text = explorer_path(state);
        char explorer_buffer[256];
        std::snprintf(explorer_buffer, sizeof(explorer_buffer), "%s", explorer_path_text.c_str());
        ftui::row({3.0f, 1.0f}, [&]() {
            ftui::input("Windows Explorer path", explorer_buffer, sizeof(explorer_buffer), ftui::InputFlags::ReadOnly);
            if (ftui::button("Copy path", ftui::ColorRole::Accent)) {
                if (copy_text_to_clipboard(explorer_path_text)) {
                    append_log(state, "Copied Explorer path: " + explorer_path_text);
                } else {
                    append_log(state, "Could not copy Explorer path to clipboard");
                }
            }
        });

        ftui::row(3, [&]() {
            if (state.running.load() || state.busy.load()) ftui::begin_disabled();
            if (ftui::button("Start", ftui::ColorRole::Success)) {
                start_server(state);
            }
            if (state.running.load() || state.busy.load()) ftui::end_disabled();

            if (!state.running.load()) ftui::begin_disabled();
            if (ftui::button("Stop", ftui::ColorRole::Warning)) {
                stop_server(state);
            }
            if (!state.running.load()) ftui::end_disabled();

            if (ftui::button("Clear log")) {
                std::scoped_lock lock(state.log_mtx);
                state.log.clear();
            }
        });

        ftui::spacing(4.0f);
        std::string logs = log_snapshot(state);
        ftui::log_view("Server log", logs.c_str(), 20,
                       ftui::LogViewFlags::WordWrap | ftui::LogViewFlags::AutoScrollBottom);

        ftui::end();
    }

    shutdown_server(state);
    ftui::shutdown();
    return 0;
}

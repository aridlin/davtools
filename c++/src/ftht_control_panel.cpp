#define FTHT_IMPLEMENTATION
#include "ftht.hpp"

#include "app.hpp"
#include "converters/registry.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <stop_token>
#include <string>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>
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

std::string html_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

std::string js_string_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '\'': out += "\\'"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

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

std::string status_html(PanelState& state) {
    const bool running = state.running.load();
    const bool failed = state.last_run_failed.load();
    const char* klass = running ? "ok" : (failed ? "warn" : "idle");
    const char* label = running ? "running" : (failed ? "stopped after error" : "stopped");

    char endpoint[192];
    std::snprintf(endpoint, sizeof(endpoint), "http://%s:%s/convert/", state.bind_ip, state.port);

    std::string html;
    html += "<section class=\"dav-status\">";
    html += "<div><b>Status</b><span class=\"";
    html += klass;
    html += "\">";
    html += label;
    html += "</span></div><div><b>Endpoint</b><code>";
    html += html_escape(endpoint);
    html += "</code></div></section>";
    return html;
}

void draw_copy_path(const std::string& path) {
    std::string html;
    html += "<div class=\"copyline\"><label><span class=\"ft-label\">Windows Explorer path</span>";
    html += "<input readonly value=\"";
    html += html_escape(path);
    html += "\"></label><button type=\"button\" class=\"accent\" onclick=\"navigator.clipboard.writeText('";
    html += js_string_escape(path);
    html += "')\">Copy path</button></div>";
    ftht::html(html.c_str());
}

int parse_panel_port(int argc, char** argv) {
    if (argc < 2) return 8079;
    int value = std::atoi(argv[1]);
    if (value < 1 || value > 65535) return 8079;
    return value;
}

void open_browser_to_panel() {
#if defined(_WIN32)
    if (std::getenv("DAVTOOLS_NO_BROWSER")) return;
    ShellExecuteA(nullptr, "open", ftht::url(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

} // namespace

int main(int argc, char** argv) {
    PanelState state;

    ftht::Config cfg;
    cfg.title = "davtools FTHt panel";
    cfg.host = "127.0.0.1";
    cfg.port = parse_panel_port(argc, argv);
    cfg.client_poll_ms = 1000;
    cfg.dark_mode = true;
    cfg.print_url = true;
    cfg.extra_head_html =
        "<style>"
        ".dav-status{display:grid;grid-template-columns:1fr 2fr;gap:10px;margin-bottom:10px}"
        ".dav-status>div{border:2px solid var(--border);background:var(--paper2);padding:10px;box-shadow:3px 3px 0 var(--border)}"
        ".dav-status b{display:block;text-transform:uppercase;font-size:12px;color:var(--muted);margin-bottom:5px}"
        ".dav-status span{font-weight:900;text-transform:uppercase}.dav-status code{word-break:break-all}"
        ".ok{color:#62d28f}.warn{color:#ffbd6e}.idle{color:var(--muted)}"
        ".copyline{display:grid;grid-template-columns:minmax(0,3fr) auto;gap:10px;align-items:end;margin:10px 0}"
        ".copyline label{display:block}.copyline button{height:38px;white-space:nowrap}"
        "@media(max-width:720px){.dav-status,.copyline{grid-template-columns:1fr}}"
        "</style>";

    ftht::set_style(ftht::one_dark_style());
    if (!ftht::create_server(cfg)) {
        return 1;
    }

    append_log(state, std::string("FTHt panel ready at ") + ftht::url());
    open_browser_to_panel();

    while (ftht::pump()) {
        join_stopped_server(state);

        ftht::begin();

        ftht::html(status_html(state).c_str());
        ftht::text_wrapped("Use this browser panel to start and stop the davtools WebDAV converter server.");

        ftht::separator();

        const bool controls_disabled = state.running.load() || state.busy.load();
        if (controls_disabled) ftht::begin_disabled();
        ftht::row({2.0f, 1.0f}, [&]() {
            ftht::input("Bind IP", state.bind_ip, sizeof(state.bind_ip));
            ftht::input("Port", state.port, sizeof(state.port), ftht::InputFlags::CharsDecimal);
        });
        if (controls_disabled) ftht::end_disabled();

        draw_copy_path(explorer_path(state));

        ftht::row(3, [&]() {
            if (state.running.load() || state.busy.load()) ftht::begin_disabled();
            if (ftht::button("Start", ftht::ColorRole::Success)) {
                start_server(state);
            }
            if (state.running.load() || state.busy.load()) ftht::end_disabled();

            if (!state.running.load()) ftht::begin_disabled();
            if (ftht::button("Stop", ftht::ColorRole::Warning)) {
                stop_server(state);
            }
            if (!state.running.load()) ftht::end_disabled();

            if (ftht::button("Clear log")) {
                std::scoped_lock lock(state.log_mtx);
                state.log.clear();
            }
        });

        ftht::separator();
        const std::string logs = log_snapshot(state);
        ftht::log_view("Server log", logs.c_str(), 20,
                       ftht::LogViewFlags::WordWrap | ftht::LogViewFlags::AutoScrollBottom);

        ftht::end();
    }

    shutdown_server(state);
    ftht::shutdown();
    return 0;
}

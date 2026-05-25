// ftui.hpp — Tiny immediate-mode GUI for Windows and Linux
// Single-header, no external dependencies beyond platform SDKs.
//
// Usage: #define FTUI_IMPLEMENTATION in exactly one .cpp before including.
//
// Windows: cl main.cpp /link d2d1.lib dwrite.lib gdi32.lib ole32.lib uuid.lib user32.lib
//          (MSVC auto-links via #pragma comment)
// Linux:   g++ main.cpp -o app -DFTUI_IMPLEMENTATION \
//            $(pkg-config --cflags --libs cairo x11) -std=c++17
//
// Optional defines before FTUI_IMPLEMENTATION:
//   FTUI_KEEP_CONSOLE    — keep console window on Windows
//   FTUI_LINUX_FONT "x"  — override default "sans-serif" on Linux
//   FTUI_DISABLE_EFFECTS — strip the Windows effects layer entirely

// Global startup theme. Change this line to another built-in preset if you
// want a different default across all new FTUI windows.
#ifndef FTUI_DEFAULT_STYLE
#define FTUI_DEFAULT_STYLE ftui::one_dark_style
#endif

#ifndef FTUI_VERSION_MAJOR
#define FTUI_VERSION_MAJOR 1
#endif
#ifndef FTUI_VERSION_MINOR
#define FTUI_VERSION_MINOR 1
#endif
#ifndef FTUI_VERSION_PATCH
#define FTUI_VERSION_PATCH 0
#endif

#pragma once
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <cassert>

// ============================================================
// Public API
// ============================================================

namespace ftui {

enum class Align { Start, Center, End };

struct Color { float r, g, b, a; };

struct Style {
    Color background    = {0.055f,0.055f,0.059f,1.0f};
    Color panel         = {0.090f,0.094f,0.102f,1.0f};
    Color text          = {0.910f,0.910f,0.910f,1.0f};
    Color text_dim      = {0.663f,0.678f,0.702f,1.0f};
    Color border        = {0.169f,0.176f,0.192f,1.0f};
    Color button        = {0.090f,0.094f,0.102f,1.0f};
    Color button_hover  = {0.137f,0.149f,0.169f,1.0f};
    Color button_active = {0.176f,0.192f,0.220f,1.0f};
    Color input_bg      = {0.071f,0.075f,0.082f,1.0f};
    Color input_focus   = {0.310f,0.420f,0.780f,1.0f};
    Color accent        = {0.310f,0.420f,0.780f,1.0f};
    Color warning       = {0.894f,0.486f,0.353f,1.0f};
    Color success       = {0.420f,0.741f,0.522f,1.0f};
    float window_padding = 20.0f;
    float item_spacing   = 10.0f;
    float item_height    = 36.0f;
    float rounding       = 8.0f;
    float border_width   = 1.0f;
    float font_size      = 16.0f;
};

enum class ColorRole {
    Background,
    Panel,
    Text,
    TextDim,
    Border,
    Button,
    ButtonHover,
    ButtonActive,
    InputBg,
    InputFocus,
    Accent,
    Warning,
    Success,
};

Style default_dark_style();
Style catppuccin_mocha_style();
Style nord_style();
Style gruvbox_dark_style();
Style one_dark_style();

void         set_style(const Style& s);
const Style& get_style();
Color        color_from_hex(const char* hex);
Color        color_from_hex(std::string_view hex);
void         push_color(ColorRole role, Color color);
void         pop_color();
void         set_next_color(ColorRole role, Color color);

enum class BuiltinIcon {
    Symbol,
    SymbolWithText,
};

struct Config {
    const char* title         = "FTUI App"; // UTF-8; converted internally on Windows
    int         width         = 960;
    int         height        = 640;
    bool        resizable     = true;
    bool        center_window = true;
    void*       icon          = nullptr;    // HICON on Windows; ignored on Linux
    bool        enable_effects = true;      // Windows-only visual effects layer
};

bool create_window(const Config& cfg = {});
bool pump();
void begin();
void end();
void shutdown();
void set_quit_on_ctrl_q(bool enabled);
void set_window_icon(void* native_icon);
void set_window_icon_builtin(BuiltinIcon variant = BuiltinIcon::Symbol);

void text(const char* label);
void text_wrapped(const char* text);
void separator();
void spacing(float px = 8.0f);

enum class InputFlags : unsigned {
    Default = 0,
    Password = 1 << 0,
    ReadOnly = 1 << 1,
    CharsDecimal = 1 << 2,
    CharsHexadecimal = 1 << 3,
    CharsUppercase = 1 << 4,
    CharsNoBlank = 1 << 5,
};
inline InputFlags operator|(InputFlags a, InputFlags b) {
    return static_cast<InputFlags>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
inline bool operator&(InputFlags a, InputFlags b) {
    return (static_cast<unsigned>(a) & static_cast<unsigned>(b)) != 0;
}

enum class TextAreaFlags : unsigned {
    Default = 0,
    ReadOnly = 1 << 0,
    WordWrap = 1 << 1,
    AutoScrollBottom = 1 << 2,
};
inline TextAreaFlags operator|(TextAreaFlags a, TextAreaFlags b) {
    return static_cast<TextAreaFlags>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
inline bool operator&(TextAreaFlags a, TextAreaFlags b) {
    return (static_cast<unsigned>(a) & static_cast<unsigned>(b)) != 0;
}

enum class LogViewFlags : unsigned {
    Default = 0,
    WordWrap = 1 << 0,
    AutoScrollBottom = 1 << 1,
};
inline LogViewFlags operator|(LogViewFlags a, LogViewFlags b) {
    return static_cast<LogViewFlags>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
inline bool operator&(LogViewFlags a, LogViewFlags b) {
    return (static_cast<unsigned>(a) & static_cast<unsigned>(b)) != 0;
}

bool input(const char* label, char* buffer, int buffer_size,
           InputFlags flags = InputFlags::Default, bool* enter_pressed = nullptr);

// Multi-line text input. Enter inserts newline; Tab cycles focus.
bool text_area(const char* label, char* buffer, int buffer_size, int rows = 5);
bool text_area_ex(const char* label, char* buffer, int buffer_size, int rows = 5,
                  TextAreaFlags flags = TextAreaFlags::Default);
void log_view(const char* label, const char* text, int rows = 8,
              LogViewFlags flags = LogViewFlags::AutoScrollBottom);

bool checkbox(const char* label, bool* value);
bool slider_float(const char* label, float* value, float min_v, float max_v);
bool button(const char* label);
bool button(const char* label, ColorRole role);
bool button(const char* label, Color color);
bool dropdown(const char* label, const char* const* items, int count, int* selected, int popup_rows = 8);
bool listbox(const char* label, const char* const* items, int count, int* selected, int visible_rows = 6);
bool radio_group(const char* label, const char* const* items, int count, int* selected, int columns = 1);
bool collapsing_header(const char* label, bool* open = nullptr);

// Horizontal tab bar. Returns true when selected changes.
bool tabs(const char* const* labels, int count, int* selected);

void row(int cols, std::function<void()> fn);
void row(std::initializer_list<float> weights, std::function<void()> fn);
void scroll_area(const char* label, float height, std::function<void()> fn);
void set_next_width(float px);
void set_next_fill();
void set_next_percent(float pct);
void set_next_limits(float min_px, float max_px);
void set_next_align(Align align);
void open_modal(const char* label);
bool modal(const char* label, std::function<void()> fn);
void close_modal();
void begin_disabled();
void end_disabled();
void tooltip(const char* text);
void request_focus(const char* label);
float calc_text_width(const char* text);
float calc_text_height(const char* text, float wrap_width);

// Opaque image handle — platform-specific payload in _impl.
struct ImageHandle { void* _impl = nullptr; };
void         image(ImageHandle* img, float width, float height);
ImageHandle* load_image(const char* utf8_path);
void         free_image(ImageHandle* img);

// File dialog — UTF-8 strings on both platforms.
// On Linux: requires zenity; returns "" if not available or cancelled.
struct FileFilter {
    const char* name; // e.g. "PNG Images"
    const char* spec; // e.g. "*.png;*.jpg"  (Windows wildcard; Linux space-sep)
};
std::string open_file_dialog(const char* title = "Open File",
                              const FileFilter* filters = nullptr,
                              int filter_count = 0);

void open_child_window(const Config& cfg, std::function<void()> fn);

struct DebugState {
    bool show_layout_rects = false;
    bool show_hovered_id   = false;
    bool show_active_id    = false;
    bool show_fps          = false;
    bool log_widget_calls  = false;
};
DebugState& debug();

} // namespace ftui


// ============================================================
// Implementation
// ============================================================

#ifdef FTUI_IMPLEMENTATION

#if defined(_WIN32) && !defined(FTUI_DISABLE_EFFECTS)
#define FTUI_WINDOWS_EFFECTS 1
#else
#define FTUI_WINDOWS_EFFECTS 0
#endif

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <shobjidl.h>
#include <wincodec.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#ifndef FTUI_KEEP_CONSOLE
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#endif

#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <time.h>
#include <unistd.h>

#endif

namespace ftui {
namespace internal {

// ---- Shared utilities -----------------------------------------------

static Style startup_style() {
    return FTUI_DEFAULT_STYLE();
}

static const Style& fallback_style() {
    static Style s = startup_style();
    return s;
}

static constexpr int kColorRoleCount = 13;
static constexpr int kColorOverrideStackCap = 64;

static int hash_str(const char* s) {
    unsigned h = 2166136261u;
    for (; *s; ++s) h = (h ^ (unsigned char)*s) * 16777619u;
    return (int)(h & 0x7fffffff) + 1;
}

static void split_label(const char* label, char* visible, int vis_len, const char** hash_src) {
    const char* sep = strstr(label, "##");
    int n = sep ? (int)(sep - label) : (int)strlen(label);
    if (n >= vis_len) n = vis_len - 1;
    memcpy(visible, label, n); visible[n] = '\0';
    *hash_src = label;
}

struct Rect { float x, y, w, h; };
static bool rect_contains(Rect r, float px, float py) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static int utf8_advance(const char* s, int pos) {
    if (!s[pos]) return pos;
    unsigned char c = (unsigned char)s[pos];
    return pos + (c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4);
}
static int utf8_retreat(const char* s, int pos) {
    if (pos == 0) return 0;
    do { --pos; } while (pos > 0 && ((unsigned char)s[pos] & 0xC0) == 0x80);
    return pos;
}
static int utf8_char_count(const char* s, int byte_offset) {
    int n = 0;
    for (int i = 0; i < byte_offset && s[i]; ) {
        unsigned char c = (unsigned char)s[i];
        i += c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4; n++;
    }
    return n;
}

// ---- Shared state structs -------------------------------------------

struct InputState {
    float mouse_x = 0, mouse_y = 0;
    bool  mouse_down = false, mouse_pressed = false, mouse_released = false;
    float wheel_y = 0;
    bool  key_backspace = false, key_enter = false;
    bool  key_space = false, key_escape = false;
    bool  key_tab = false, key_shift_tab = false;
    bool  key_left = false, key_right = false;
    bool  key_up   = false, key_down  = false;
    bool  key_ctrl_c = false, key_ctrl_v = false;
    bool  shift_held = false, ctrl_held = false;
    char  text_input[64] = {};
    int   text_input_count = 0;
    bool  focused = true;
};

struct RowContext {
    bool  active = false;
    bool  weighted = false;
    int   cols = 0;
    float cell_w = 0, gap = 0, start_x = 0, start_y = 0;
    float total_w = 0, used_x = 0, total_weight = 0;
    const float* weights = nullptr;
    int   col_index = 0;
    float row_height = 0;
};

struct UIContext {
    int  hot_id = 0, active_id = 0, focused_input_id = 0, focused_widget_id = 0, frame_index = 0;
    Rect content_region = {};
    float cursor_x = 0, cursor_y = 0;
    RowContext row_ctx;
    std::vector<int> tab_stops, tab_stops_prev;
    Rect last_item_rect = {};
    int  last_item_id = 0;
    bool last_item_hovered = false;
    bool last_item_focused = false;
};

struct CmdState { bool active = false; char buf[16] = {}; int len = 0; };

struct MotionSlot {
    int   key = 0;
    float hover = 0;
    float active = 0;
    float focus = 0;
};

struct TabFxSlot {
    int   key = 0;
    int   selected = 0;
    int   previous = 0;
    int   dir = 1;
    bool  initialized = false;
    float switch_t = 1.0f;
    float underline_x = 0;
    float underline_w = 0;
};

struct FrameContentFx {
    bool  active = false;
    int   owner = 0;
    float start_y = 0;
    float progress = 1.0f;
    int   dir = 1;
};

struct ScrollSlot {
    int   key = 0;
    float current = 0;
    float target = 0;
    float content = 0;
    bool  dragging = false;
    float drag_mouse = 0;
    float drag_scroll0 = 0;
};

struct CollapseSlot {
    int   key = 0;
    bool  initialized = false;
    bool  open = false;
    float progress = 0;
    float body_height = 0;
};

struct ActiveCollapseFx {
    int   key = 0;
    float top_target = 0;
    float top_display = 0;
    float body_height = 0;
    float progress = 1.0f;
};

struct CollapseRectFx {
    Rect rect = {};
    Rect clip = {};
    bool clip_active = false;
};

struct NextLayoutState {
    bool  active = false;
    bool  fill = false;
    bool  has_width = false;
    bool  has_percent = false;
    bool  has_limits = false;
    float width = 0;
    float percent = 1.0f;
    float min = 0;
    float max = 0;
    Align align = Align::Start;
};

struct TooltipState {
    bool active = false;
    bool requested = false;
    bool visible = false;
    int  request_id = 0;
    int  hover_id = 0;
    Rect anchor = {};
    Rect request_anchor = {};
    std::string text;
    std::string request_text;
    float hover_time = 0;
    float opacity = 0;
    float visible_time = 0;
    float calm_time = 0;
    float static_x = 0;
    float static_y = 0;
    int   dismiss_streak = 0;
};

struct DropdownOverlayState {
    bool active = false;
    bool open = false;
    Rect popup_r = {};
    int count = 0;
    int selected_index = -1;
    float item_h = 0;
    float scroll = 0;
    float popup_h = 0;
    float popup_p = 1.0f;
    Color popup_fill = {};
    Color popup_border = {};
    Color item_hover = {};
    Color item_selected = {};
    Color item_text = {};
    Color item_text_selected = {};
    Color scrollbar_thumb = {};
    Color scrollbar_track = {};
    std::vector<std::string> labels;
};

struct ColorOverrideEntry {
    ColorRole role = ColorRole::Text;
    Color color = {};
};

struct ColorOverrideTable {
    bool used[kColorRoleCount] = {};
    Color colors[kColorRoleCount] = {};
};

// ---- Shared globals -------------------------------------------------

static InputState g_input;
static UIContext  g_ctx;
static DebugState g_debug;
static Style      g_style;
static float      g_fps = 0, g_fps_accum = 0;
static int        g_fps_frames = 0;
static bool       g_shortcuts_enabled = true;
static CmdState   g_cmd;
static float      g_dt = 1.0f / 60.0f;
static bool       g_effects_enabled = false;
static float      g_scroll_y = 0, g_content_height = 0;
static float      g_scroll_target_y = 0;
static bool       g_sb_dragging = false;
static float      g_sb_drag_mouse_y = 0, g_sb_drag_scroll0 = 0;
static int        g_text_cursor_id = 0, g_text_cursor = 0, g_text_sel_anchor = 0;
static int        g_ta_cursor_id = 0, g_ta_cursor = 0, g_ta_sel_anchor = 0;
static float      g_ta_scroll_y = 0;
static float      g_ta_scroll_target_y = 0;
static bool       g_drawing = false;
static MotionSlot g_motion_slots[256];
static TabFxSlot  g_tab_fx_slots[32];
static ScrollSlot g_scroll_slots[128];
static CollapseSlot g_collapse_slots[64];
static int        g_tab_content_owner = 0;
static FrameContentFx g_frame_content_fx;
static float      g_draw_fx_off_x = 0;
static float      g_draw_fx_off_y = 0;
static float      g_draw_fx_opacity = 1.0f;
static NextLayoutState g_next_layout;
static TooltipState g_tooltip;
static DropdownOverlayState g_dropdown_overlay;
static DropdownOverlayState g_dropdown_overlay_prev;
static ColorOverrideEntry g_color_stack[kColorOverrideStackCap];
static int        g_color_stack_count = 0;
static ColorOverrideTable g_next_color_pending;
static ColorOverrideTable g_active_widget_colors;
static int        g_active_widget_color_depth = 0;
static ActiveCollapseFx g_active_collapse_fx[16];
static int        g_active_collapse_fx_count = 0;
static int        g_pending_collapse_key = 0;
static float      g_pending_collapse_start_y = 0.0f;
static int        g_disabled_depth = 0;
static int        g_focus_request_id = 0;
static int        g_modal_open_id = 0;
static int        g_modal_request_id = 0;
static bool       g_inside_modal = false;
static bool       g_modal_drawn = false;
static int        g_dropdown_open_id = 0;
static bool       g_dropdown_capture_input = false;

// ---- Forward declarations of platform-specific functions -----------
// (defined in the platform blocks below; used by shared widget code)

static void        dbg(const char* fmt, ...);
static float       measure_text_width(const char* utf8);
static int         byte_from_x(const char* utf8, float rel_x);
static void        fill_round_rect(Rect r, float radius, Color c);
static void        stroke_round_rect(Rect r, float radius, float thickness, Color c);
static void        fill_rect(Rect r, Color c);
static void        draw_line(float x0, float y0, float x1, float y1, float thickness, Color c);
static void        fill_triangle(Rect r, int dir, Color c);
static void        draw_text_utf8(const char* utf8, Rect r, Color c);
static void        draw_text_utf8_centered(const char* utf8, Rect r, Color c);
static void        draw_image_handle(ImageHandle* img, Rect r);
static void        clipboard_set(const char* utf8);
static std::string clipboard_get();
static void        push_clip(Rect r);
static void        pop_clip();
static void        sync_native_window_chrome();
static void        apply_window_icon_handles(void* icon_big, void* icon_small, bool owned, BuiltinIcon variant);

#if FTUI_WINDOWS_EFFECTS
static void        draw_previous_frame_overlay(Rect clip_r, float dx, float opacity);
static void        draw_previous_frame_blur_panel(Rect r, float opacity);
static void        release_frame_snapshot();
static void        capture_frame_snapshot();
#endif

// ---- Shared implementations (call platform fns above) ---------------

static float text_line_height() { return g_style.font_size + 6.0f; }

static float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static Color lerp_color(Color a, Color b, float t) {
    t = clamp01(t);
    return {
        lerpf(a.r, b.r, t),
        lerpf(a.g, b.g, t),
        lerpf(a.b, b.b, t),
        lerpf(a.a, b.a, t),
    };
}

static Color with_alpha(Color c, float a) {
    c.a = a;
    return c;
}

static int color_role_index(ColorRole role) {
    switch (role) {
        case ColorRole::Background:  return 0;
        case ColorRole::Panel:       return 1;
        case ColorRole::Text:        return 2;
        case ColorRole::TextDim:     return 3;
        case ColorRole::Border:      return 4;
        case ColorRole::Button:      return 5;
        case ColorRole::ButtonHover: return 6;
        case ColorRole::ButtonActive:return 7;
        case ColorRole::InputBg:     return 8;
        case ColorRole::InputFocus:  return 9;
        case ColorRole::Accent:      return 10;
        case ColorRole::Warning:     return 11;
        case ColorRole::Success:     return 12;
    }
    return 2;
}

static Color style_color_for_role(const Style& style, ColorRole role) {
    switch (role) {
        case ColorRole::Background:   return style.background;
        case ColorRole::Panel:        return style.panel;
        case ColorRole::Text:         return style.text;
        case ColorRole::TextDim:      return style.text_dim;
        case ColorRole::Border:       return style.border;
        case ColorRole::Button:       return style.button;
        case ColorRole::ButtonHover:  return style.button_hover;
        case ColorRole::ButtonActive: return style.button_active;
        case ColorRole::InputBg:      return style.input_bg;
        case ColorRole::InputFocus:   return style.input_focus;
        case ColorRole::Accent:       return style.accent;
        case ColorRole::Warning:      return style.warning;
        case ColorRole::Success:      return style.success;
    }
    return style.text;
}

static Color resolve_color(ColorRole role) {
    int idx = color_role_index(role);
    if (g_active_widget_color_depth > 0 && g_active_widget_colors.used[idx]) {
        return g_active_widget_colors.colors[idx];
    }
    for (int i = g_color_stack_count - 1; i >= 0; --i) {
        if (g_color_stack[i].role == role) return g_color_stack[i].color;
    }
    return style_color_for_role(g_style, role);
}

struct WidgetColorScope {
    ColorOverrideTable prev = {};
    ColorOverrideTable own = {};
    int prev_depth = 0;
    bool active = false;

    WidgetColorScope() {
        prev = g_active_widget_colors;
        prev_depth = g_active_widget_color_depth;
        own = g_next_color_pending;
        g_active_widget_colors = own;
        g_active_widget_color_depth = 1;
        g_next_color_pending = {};
        active = true;
    }

    void suspend_for_children() {
        if (!active) return;
        g_active_widget_colors = {};
        g_active_widget_color_depth = 0;
    }

    void resume_after_children() {
        if (!active) return;
        g_active_widget_colors = own;
        g_active_widget_color_depth = 1;
    }

    ~WidgetColorScope() {
        if (!active) return;
        g_active_widget_colors = prev;
        g_active_widget_color_depth = prev_depth;
    }
};

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static Color parse_hex_color(std::string_view hex) {
    constexpr Color fallback = {1.0f, 1.0f, 1.0f, 1.0f};
    auto fail = [&]() -> Color {
#ifndef NDEBUG
        assert(false && "ftui::color_from_hex: invalid hex color");
#endif
        return fallback;
    };

    if (hex.size() != 7 && hex.size() != 9) return fail();
    if (hex.empty() || hex[0] != '#') return fail();

    unsigned bytes[4] = {255u, 255u, 255u, 255u};
    int count = hex.size() == 7 ? 3 : 4;
    for (int i = 0; i < count; ++i) {
        int hi = hex_nibble(hex[1 + i * 2]);
        int lo = hex_nibble(hex[2 + i * 2]);
        if (hi < 0 || lo < 0) return fail();
        bytes[i] = (unsigned)((hi << 4) | lo);
    }

    return {
        bytes[0] / 255.0f,
        bytes[1] / 255.0f,
        bytes[2] / 255.0f,
        bytes[3] / 255.0f,
    };
}

static Rect offset_rect(Rect r, float dx, float dy) {
    r.x += dx; r.y += dy; return r;
}

static float ease_smooth(float t) {
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

static float step_anim(float value, float target, float dt, float sharpness) {
    if (dt <= 0.0f) return target;
    float a = 1.0f - expf(-sharpness * dt);
    float out = value + (target - value) * a;
    if (fabsf(out - target) < 0.001f) return target;
    return out;
}

static bool effects_enabled() {
#if FTUI_WINDOWS_EFFECTS
    return g_effects_enabled;
#else
    return false;
#endif
}

static void reset_draw_fx() {
    g_draw_fx_off_x = 0;
    g_draw_fx_off_y = 0;
    g_draw_fx_opacity = 1.0f;
}

static void reset_effect_state(bool enable_effects) {
    g_effects_enabled = enable_effects;
    g_dt = 1.0f / 60.0f;
    g_scroll_target_y = g_scroll_y;
    g_ta_scroll_target_y = g_ta_scroll_y;
    memset(g_motion_slots, 0, sizeof(g_motion_slots));
    memset(g_tab_fx_slots, 0, sizeof(g_tab_fx_slots));
    g_tab_content_owner = 0;
    g_frame_content_fx = {};
    reset_draw_fx();
}

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static Color disabled_color(Color c) {
    Color out = lerp_color(c, resolve_color(ColorRole::Panel), 0.42f);
    out.a *= 0.70f;
    return out;
}

static Color maybe_disabled(Color c) {
    return g_disabled_depth > 0 ? disabled_color(c) : c;
}

static void clear_text_focus() {
    g_ctx.focused_input_id = 0;
    g_text_cursor_id = 0;
    g_ta_cursor_id = 0;
}

static void set_widget_focus(int id, bool text_like = false) {
    g_ctx.focused_widget_id = id;
    if (text_like) g_ctx.focused_input_id = id;
    else clear_text_focus();
}

static bool is_widget_focused(int id) {
    return g_ctx.focused_widget_id == id;
}

static bool widget_interaction_enabled() {
    return g_disabled_depth == 0 && (!g_modal_open_id || g_inside_modal) && !g_dropdown_capture_input;
}

static void register_focusable(int id) {
    g_ctx.tab_stops.push_back(id);
}

static void mark_last_item(int id, Rect r, bool hovered, bool focused) {
    g_ctx.last_item_id = id;
    g_ctx.last_item_rect = r;
    g_ctx.last_item_hovered = hovered;
    g_ctx.last_item_focused = focused;
}

static ScrollSlot& scroll_slot_for(int key) {
    unsigned idx = (unsigned)key & 127u;
    for (int n = 0; n < 128; ++n) {
        ScrollSlot& slot = g_scroll_slots[(idx + (unsigned)n) & 127u];
        if (slot.key == key || slot.key == 0) {
            if (slot.key == 0) slot.key = key;
            return slot;
        }
    }
    ScrollSlot& slot = g_scroll_slots[idx];
    slot = {};
    slot.key = key;
    return slot;
}

static CollapseSlot& collapse_slot_for(int key) {
    unsigned idx = (unsigned)key & 63u;
    for (int n = 0; n < 64; ++n) {
        CollapseSlot& slot = g_collapse_slots[(idx + (unsigned)n) & 63u];
        if (slot.key == key || slot.key == 0) {
            if (slot.key == 0) slot.key = key;
            return slot;
        }
    }
    CollapseSlot& slot = g_collapse_slots[idx];
    slot = {};
    slot.key = key;
    return slot;
}

static void finalize_pending_collapse_measure() {
    if (!g_pending_collapse_key) return;
    CollapseSlot& slot = collapse_slot_for(g_pending_collapse_key);
    float measured = g_ctx.cursor_y - g_pending_collapse_start_y;
    if (measured < 0.0f) measured = 0.0f;
    slot.body_height = measured;
    g_pending_collapse_key = 0;
    g_pending_collapse_start_y = 0.0f;
}

static MotionSlot& motion_slot_for(int key) {
    unsigned idx = (unsigned)key & 255u;
    for (int n = 0; n < 256; ++n) {
        MotionSlot& slot = g_motion_slots[(idx + (unsigned)n) & 255u];
        if (slot.key == key || slot.key == 0) {
            if (slot.key == 0) slot.key = key;
            return slot;
        }
    }
    MotionSlot& slot = g_motion_slots[idx];
    slot = {};
    slot.key = key;
    return slot;
}

static TabFxSlot& tab_fx_slot_for(int key) {
    unsigned idx = (unsigned)key & 31u;
    for (int n = 0; n < 32; ++n) {
        TabFxSlot& slot = g_tab_fx_slots[(idx + (unsigned)n) & 31u];
        if (slot.key == key || slot.key == 0) {
            if (slot.key == 0) slot.key = key;
            return slot;
        }
    }
    TabFxSlot& slot = g_tab_fx_slots[idx];
    slot = {};
    slot.key = key;
    return slot;
}

static void update_motion_slot(MotionSlot& slot, bool hover_on, bool active_on, bool focus_on) {
    if (!effects_enabled()) {
        slot.hover = hover_on ? 1.0f : 0.0f;
        slot.active = active_on ? 1.0f : 0.0f;
        slot.focus = focus_on ? 1.0f : 0.0f;
        return;
    }
    slot.hover = step_anim(slot.hover, hover_on ? 1.0f : 0.0f, g_dt, 16.0f);
    slot.active = step_anim(slot.active, active_on ? 1.0f : 0.0f, g_dt, 22.0f);
    slot.focus = step_anim(slot.focus, focus_on ? 1.0f : 0.0f, g_dt, 14.0f);
}

static void draw_widget_chrome(Rect r, float rounding, Color fill, Color border,
                               float hover_p, float active_p, float focus_p) {
    if (!effects_enabled()) {
        fill_round_rect(r, rounding, fill);
        stroke_round_rect(r, rounding, g_style.border_width, border);
        return;
    }

    float glow = clamp01(hover_p * 0.45f + active_p * 0.65f + focus_p * 0.80f);
    Color shadow = {0.0f, 0.0f, 0.0f, 0.07f + glow * 0.08f};
    Color shell = lerp_color(fill, resolve_color(ColorRole::Panel), 0.12f);
    shell = lerp_color(shell, resolve_color(ColorRole::InputFocus), hover_p * 0.03f + focus_p * 0.05f);
    shell.a = clamp01(0.94f + hover_p * 0.02f + focus_p * 0.02f);
    Color inner = {1.0f, 1.0f, 1.0f, 0.025f + hover_p * 0.015f + focus_p * 0.020f};
    Color rim = lerp_color(border, resolve_color(ColorRole::InputFocus), clamp01(active_p * 0.35f + focus_p * 0.55f));
    rim.a = clamp01(0.50f + glow * 0.18f);

    fill_round_rect(offset_rect(r, 0.0f, 1.0f), rounding + 0.5f, shadow);
    fill_round_rect(r, rounding, shell);
    if (r.w > 2.0f && r.h > 2.0f) {
        stroke_round_rect({r.x + 1.0f, r.y + 1.0f, r.w - 2.0f, r.h - 2.0f},
                          fmaxf(1.0f, rounding - 1.0f), 1.0f, inner);
    }
    stroke_round_rect(r, rounding, 1.0f + focus_p * 0.7f, rim);
}

static float measure_text_at(const char* utf8, int byte_len) {
    if (byte_len <= 0 || !utf8 || !utf8[0]) return 0.0f;
    std::string sub(utf8, utf8 + byte_len);
    return measure_text_width(sub.c_str());
}

struct TextRange {
    int start = 0;
    int end = 0;
};

static bool wrap_space(char c) {
    return c == ' ' || c == '\t';
}

static void compute_wrapped_ranges(const char* text, float max_width, bool word_wrap,
                                   std::vector<TextRange>& out) {
    out.clear();
    if (!text) { out.push_back({0, 0}); return; }

    int len = (int)strlen(text);
    if (len == 0) { out.push_back({0, 0}); return; }

    int line_start = 0;
    while (line_start <= len) {
        if (line_start == len) { out.push_back({len, len}); break; }

        int logical_end = line_start;
        while (logical_end < len && text[logical_end] != '\n') logical_end++;

        if (!word_wrap || max_width <= 1.0f || measure_text_at(text + line_start, logical_end - line_start) <= max_width) {
            out.push_back({line_start, logical_end});
            if (logical_end >= len) break;
            line_start = logical_end + 1;
            continue;
        }

        int seg_start = line_start;
        while (seg_start < logical_end) {
            int pos = seg_start;
            int last_break = -1;
            int best_end = seg_start;
            while (pos < logical_end) {
                int next = utf8_advance(text, pos);
                float w = measure_text_at(text + seg_start, next - seg_start);
                if (wrap_space(text[pos])) last_break = pos;
                if (w > max_width && best_end > seg_start) break;
                if (w > max_width) {
                    best_end = last_break >= seg_start ? last_break : pos;
                    break;
                }
                best_end = next;
                pos = next;
            }
            if (best_end <= seg_start) best_end = utf8_advance(text, seg_start);
            out.push_back({seg_start, best_end});
            seg_start = best_end;
            while (seg_start < logical_end && wrap_space(text[seg_start])) seg_start++;
        }

        if (logical_end >= len) break;
        line_start = logical_end + 1;
    }

    if (out.empty()) out.push_back({0, 0});
}

static void draw_tooltip_overlay() {
    if (g_tooltip.opacity <= 0.01f || g_tooltip.text.empty()) return;

    float pad = 8.0f;
    float max_w = fminf(320.0f, fmaxf(180.0f, g_ctx.content_region.w * 0.45f));
    std::vector<TextRange> lines;
    compute_wrapped_ranges(g_tooltip.text.c_str(), max_w - pad * 2.0f, true, lines);

    float lh = text_line_height();
    float box_h = pad * 2.0f + (float)lines.size() * lh;
    Rect r = {
        g_tooltip.anchor.x,
        g_tooltip.anchor.y + g_tooltip.anchor.h + 8.0f,
        max_w,
        box_h
    };

    float max_x = g_ctx.content_region.x + g_ctx.content_region.w - r.w - 8.0f;
    float max_y = g_ctx.content_region.y + g_ctx.content_region.h - r.h - 8.0f;
    if (r.x > max_x) r.x = max_x;
    if (r.y > max_y) r.y = g_tooltip.anchor.y - r.h - 8.0f;
    if (r.x < 8.0f) r.x = 8.0f;
    if (r.y < 8.0f) r.y = 8.0f;

    Color fill = lerp_color(resolve_color(ColorRole::Panel), resolve_color(ColorRole::Background), 0.18f);
    fill.a = 0.98f * g_tooltip.opacity;
    Color border = resolve_color(ColorRole::Border); border.a *= g_tooltip.opacity;
    draw_widget_chrome(r, fmaxf(4.0f, g_style.rounding * 0.75f), fill, border, 0.0f, 0.0f, 0.0f);

    for (size_t i = 0; i < lines.size(); ++i) {
        TextRange tr = lines[i];
        std::string line(g_tooltip.text.c_str() + tr.start, g_tooltip.text.c_str() + tr.end);
        Rect lr = {r.x + pad, r.y + pad + (float)i * lh, r.w - pad * 2.0f, lh};
        Color text = resolve_color(ColorRole::Text); text.a *= g_tooltip.opacity;
        draw_text_utf8(line.c_str(), lr, text);
    }
}

static void draw_dropdown_overlay() {
    if (!g_dropdown_overlay.active || g_dropdown_overlay.labels.empty() || g_dropdown_overlay.count <= 0) return;

    const DropdownOverlayState& ov = g_dropdown_overlay;
#if FTUI_WINDOWS_EFFECTS
    if (effects_enabled()) {
        draw_previous_frame_blur_panel(ov.popup_r, ov.popup_p);
    }
#endif
    draw_widget_chrome(ov.popup_r, g_style.rounding, maybe_disabled(ov.popup_fill), maybe_disabled(ov.popup_border), 0.0f, 0.0f, 0.0f);

    Rect clip_r = {ov.popup_r.x + 2.0f, ov.popup_r.y + 2.0f, ov.popup_r.w - 4.0f, ov.popup_r.h - 4.0f};
    push_clip(clip_r);
    for (int i = 0; i < ov.count; ++i) {
        Rect ir = {ov.popup_r.x + 4.0f, ov.popup_r.y + 4.0f - ov.scroll + i * ov.item_h, ov.popup_r.w - 8.0f, ov.item_h};
        if (ir.y + ir.h < ov.popup_r.y || ir.y > ov.popup_r.y + ov.popup_r.h) continue;
        bool item_hov = ov.open && rect_contains(ir, g_input.mouse_x, g_input.mouse_y);
        bool item_sel = (ov.selected_index == i);
        Color item_bg = item_sel ? ov.item_selected :
                        item_hov ? ov.item_hover :
                                   Color{0.0f, 0.0f, 0.0f, 0.0f};
        if (item_bg.a > 0.0f) fill_round_rect(ir, g_style.rounding * 0.6f, maybe_disabled(item_bg));
        Rect itr = {ir.x + 8.0f, ir.y, ir.w - 16.0f, ir.h};
        draw_text_utf8(ov.labels[i].c_str(), itr, maybe_disabled(item_sel ? ov.item_text_selected : ov.item_text));
    }
    pop_clip();

    float content_h = ov.count * ov.item_h;
    if (content_h > ov.popup_h - 8.0f) {
        float max_scroll = content_h - (ov.popup_h - 8.0f);
        if (max_scroll < 0.0f) max_scroll = 0.0f;
        if (max_scroll > 0.0f) {
            float visible_h = ov.popup_h - 8.0f;
            float thumb_h = (visible_h / content_h) * visible_h;
            if (thumb_h < 12.0f) thumb_h = 12.0f;
            float thumb_y = ov.popup_r.y + 4.0f + (ov.scroll / max_scroll) * (visible_h - thumb_h);
            Rect track_r = {ov.popup_r.x + ov.popup_r.w - 8.0f, ov.popup_r.y + 4.0f, 4.0f, ov.popup_r.h - 8.0f};
            Rect thumb_r = {track_r.x, thumb_y, track_r.w, thumb_h};
            fill_round_rect(track_r, 2.0f, maybe_disabled(ov.scrollbar_track));
            fill_round_rect(thumb_r, 2.0f, maybe_disabled(ov.scrollbar_thumb));
        }
    }
}

static float widget_label_height(const char* vis) {
    return (vis && vis[0]) ? (text_line_height() + 6.0f) : 0.0f;
}

static Rect next_rect(float height);

static Rect next_labeled_rect(const char* vis, float body_h, Rect* body_out) {
    float label_h = widget_label_height(vis);
    Rect outer = next_rect(body_h + label_h);
    if (body_out) *body_out = {outer.x, outer.y + label_h, outer.w, body_h};
    return outer;
}

static void draw_widget_label(const char* vis, Rect outer, bool emphasized = false) {
    if (!vis || !vis[0]) return;
    float label_h = widget_label_height(vis);
    Rect lr = {outer.x, outer.y, outer.w, label_h};
    draw_text_utf8(vis, lr, maybe_disabled(lerp_color(resolve_color(ColorRole::TextDim), resolve_color(ColorRole::Text), emphasized ? 0.22f : 0.0f)));
}

static Rect rect_intersection(Rect a, Rect b) {
    float x0 = a.x > b.x ? a.x : b.x;
    float y0 = a.y > b.y ? a.y : b.y;
    float x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    float y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;
    return {x0, y0, x1 - x0, y1 - y0};
}

static float transformed_layout_y(float target_y) {
    float y = target_y;
    for (int i = 0; i < g_active_collapse_fx_count; ++i) {
        const ActiveCollapseFx& fx = g_active_collapse_fx[i];
        if (fx.body_height <= 0.0f) continue;
        float body_end = fx.top_target + fx.body_height;
        if (target_y >= body_end - 0.5f) {
            y -= fx.body_height * (1.0f - fx.progress);
        }
    }
    return y;
}

static CollapseRectFx collapse_rect_fx(Rect target) {
    CollapseRectFx out = {};
    out.rect = target;

    for (int i = 0; i < g_active_collapse_fx_count; ++i) {
        const ActiveCollapseFx& fx = g_active_collapse_fx[i];
        if (fx.body_height <= 0.0f) continue;
        float body_end = fx.top_target + fx.body_height;
        float hidden_h = fx.body_height * (1.0f - fx.progress);
        if (target.y >= body_end - 0.5f) {
            out.rect.y -= hidden_h;
        } else if (target.y + target.h > fx.top_target && target.y < body_end) {
            float visible_h = fx.body_height * fx.progress;
            Rect clip = {
                g_ctx.content_region.x - 4.0f,
                fx.top_display,
                g_ctx.content_region.w + 8.0f,
                visible_h
            };
            if (out.rect.y < clip.y) {
                float delta = clip.y - out.rect.y;
                out.rect.y = clip.y;
                out.rect.h -= delta;
            }
            float clip_bottom = clip.y + clip.h;
            float rect_bottom = out.rect.y + out.rect.h;
            if (rect_bottom > clip_bottom) out.rect.h = clip_bottom - out.rect.y;
            if (out.rect.h < 0.0f) out.rect.h = 0.0f;
            out.clip = out.clip_active ? rect_intersection(out.clip, clip) : clip;
            out.clip_active = true;
        }
    }

    return out;
}

static Rect labeled_body_rect(const char* vis, Rect outer) {
    float label_h = widget_label_height(vis);
    float shown_label_h = outer.h < label_h ? outer.h : label_h;
    float body_h = outer.h - shown_label_h;
    if (body_h < 0.0f) body_h = 0.0f;
    return {outer.x, outer.y + shown_label_h, outer.w, body_h};
}

static float tooltip_delay_seconds() {
    if (g_tooltip.dismiss_streak >= 4) return 10.0f;
    if (g_tooltip.dismiss_streak >= 2) return 5.0f;
    return 1.0f;
}

static void begin_tooltip_frame() {
    g_tooltip.requested = false;
    g_tooltip.request_id = 0;
    g_tooltip.request_text.clear();
}

static void finalize_tooltip_frame() {
    if (g_tooltip.requested) {
        if (g_tooltip.hover_id != g_tooltip.request_id) {
            if (g_tooltip.visible && g_tooltip.visible_time < 0.8f) g_tooltip.dismiss_streak++;
            g_tooltip.hover_id = g_tooltip.request_id;
            g_tooltip.hover_time = 0.0f;
            g_tooltip.visible_time = 0.0f;
            g_tooltip.calm_time = 0.0f;
            g_tooltip.static_x = g_input.mouse_x;
            g_tooltip.static_y = g_input.mouse_y;
        } else {
            float dx = g_input.mouse_x - g_tooltip.static_x;
            float dy = g_input.mouse_y - g_tooltip.static_y;
            float moved2 = dx * dx + dy * dy;
            if (moved2 <= 4.0f) {
                g_tooltip.hover_time += g_dt;
            } else {
                if (g_tooltip.visible && g_tooltip.visible_time < 0.8f) g_tooltip.dismiss_streak++;
                g_tooltip.hover_time = 0.0f;
                g_tooltip.visible_time = 0.0f;
                g_tooltip.calm_time = 0.0f;
                g_tooltip.static_x = g_input.mouse_x;
                g_tooltip.static_y = g_input.mouse_y;
            }
        }
        g_tooltip.anchor = g_tooltip.request_anchor;
        g_tooltip.text = g_tooltip.request_text;
    } else {
        if (g_tooltip.visible && g_tooltip.visible_time < 0.8f) g_tooltip.dismiss_streak++;
        g_tooltip.hover_id = 0;
        g_tooltip.hover_time = 0.0f;
        g_tooltip.visible_time = 0.0f;
        g_tooltip.calm_time = 0.0f;
    }

    g_tooltip.visible = g_tooltip.requested && g_tooltip.hover_time >= tooltip_delay_seconds();
    if (g_tooltip.visible) {
        g_tooltip.visible_time += g_dt;
        g_tooltip.calm_time += g_dt;
        if (g_tooltip.calm_time >= 1.25f && g_tooltip.dismiss_streak > 0) {
            g_tooltip.dismiss_streak--;
            g_tooltip.calm_time = 0.0f;
        }
    }
    g_tooltip.opacity = step_anim(g_tooltip.opacity, g_tooltip.visible ? 1.0f : 0.0f, g_dt, 10.0f);
    g_tooltip.active = g_tooltip.opacity > 0.01f;
}

static bool filter_ascii_input_char(char c, InputFlags flags, char& out) {
    out = c;
    if (flags & InputFlags::CharsUppercase) {
        if (c >= 'a' && c <= 'z') out = (char)(c - 'a' + 'A');
    }
    if (flags & InputFlags::CharsNoBlank) {
        if (c == ' ' || c == '\t') return false;
    }
    if (flags & InputFlags::CharsDecimal) {
        if (!((out >= '0' && out <= '9') || out == '.' || out == '-' || out == '+')) return false;
    }
    if (flags & InputFlags::CharsHexadecimal) {
        if (!((out >= '0' && out <= '9') || (out >= 'a' && out <= 'f') || (out >= 'A' && out <= 'F'))) return false;
    }
    return true;
}

static Rect next_rect(float height) {
    float region_x = g_ctx.cursor_x;
    float region_y = g_ctx.cursor_y;
    float region_w = g_ctx.content_region.w;

    if (g_ctx.row_ctx.active) {
        auto& rc = g_ctx.row_ctx;
        int row_index = rc.col_index < rc.cols ? rc.col_index : (rc.cols - 1);
        region_x = rc.start_x + rc.used_x;
        region_y = rc.start_y;
        region_w = rc.weighted && rc.weights
            ? (rc.total_w - rc.gap * (rc.cols - 1)) * (rc.weights[row_index] / rc.total_weight)
            : rc.cell_w;
        rc.used_x += region_w + (rc.col_index + 1 < rc.cols ? rc.gap : 0.0f);
        rc.col_index++;
        if (height > rc.row_height) rc.row_height = height;
    } else {
        g_ctx.cursor_y += height + g_style.item_spacing;
    }

    float width = region_w;
    if (g_next_layout.active) {
        if (g_next_layout.has_width) width = g_next_layout.width;
        else if (g_next_layout.has_percent) width = region_w * g_next_layout.percent;
        else if (g_next_layout.fill) width = region_w;

        if (g_next_layout.has_limits) {
            width = clampf(width, g_next_layout.min, g_next_layout.max);
        }
        if (width > region_w) width = region_w;
        if (width < 1.0f) width = 1.0f;
    }

    float x = region_x;
    Align align = g_next_layout.active ? g_next_layout.align : Align::Start;
    if (width < region_w) {
        if (align == Align::Center) x += (region_w - width) * 0.5f;
        else if (align == Align::End) x += (region_w - width);
    }

    Rect r = {x, region_y, width, height};
    g_next_layout = {};

    if (g_ctx.row_ctx.active) return r;
    return r;
}

static void cmd_clear() { g_cmd.active = false; g_cmd.len = 0; g_cmd.buf[0] = '\0'; }

static void apply_cmd_theme() {
    if      (strcmp(g_cmd.buf, "td") == 0) g_style = default_dark_style();
    else if (strcmp(g_cmd.buf, "tc") == 0) g_style = catppuccin_mocha_style();
    else if (strcmp(g_cmd.buf, "tn") == 0) g_style = nord_style();
    else if (strcmp(g_cmd.buf, "tg") == 0) g_style = gruvbox_dark_style();
    else if (strcmp(g_cmd.buf, "to") == 0) g_style = one_dark_style();
}

} // namespace internal

// ---- Public helpers (before platform split) -------------------------

void set_quit_on_ctrl_q(bool e) { internal::g_shortcuts_enabled = e; }
void set_style(const Style& s)  { internal::g_style = s; internal::sync_native_window_chrome(); }
const Style& get_style()         { return internal::g_style; }
Color color_from_hex(const char* hex) { return internal::parse_hex_color(hex ? std::string_view(hex) : std::string_view{}); }
Color color_from_hex(std::string_view hex) { return internal::parse_hex_color(hex); }
void push_color(ColorRole role, Color color) {
    if (internal::g_color_stack_count >= internal::kColorOverrideStackCap) {
#ifndef NDEBUG
        assert(false && "ftui::push_color: override stack overflow");
#endif
        return;
    }
    internal::g_color_stack[internal::g_color_stack_count++] = {role, color};
}
void pop_color() {
    if (internal::g_color_stack_count <= 0) {
#ifndef NDEBUG
        assert(false && "ftui::pop_color: override stack underflow");
#endif
        return;
    }
    internal::g_color_stack_count--;
}
void set_next_color(ColorRole role, Color color) {
    int idx = internal::color_role_index(role);
    internal::g_next_color_pending.used[idx] = true;
    internal::g_next_color_pending.colors[idx] = color;
}
void set_window_icon(void* native_icon) { internal::apply_window_icon_handles(native_icon, native_icon, false, BuiltinIcon::Symbol); }
void set_window_icon_builtin(BuiltinIcon variant) { internal::apply_window_icon_handles(nullptr, nullptr, true, variant); }
DebugState&  debug()             { return internal::g_debug; }
void set_next_width(float px) {
    internal::g_next_layout.active = true;
    internal::g_next_layout.has_width = true;
    internal::g_next_layout.width = px;
}
void set_next_fill() {
    internal::g_next_layout.active = true;
    internal::g_next_layout.fill = true;
}
void set_next_percent(float pct) {
    internal::g_next_layout.active = true;
    internal::g_next_layout.has_percent = true;
    internal::g_next_layout.percent = pct < 0.0f ? 0.0f : (pct > 1.0f ? 1.0f : pct);
}
void set_next_limits(float min_px, float max_px) {
    internal::g_next_layout.active = true;
    internal::g_next_layout.has_limits = true;
    internal::g_next_layout.min = min_px < 0.0f ? 0.0f : min_px;
    internal::g_next_layout.max = max_px < min_px ? min_px : max_px;
}
void set_next_align(Align align) {
    internal::g_next_layout.active = true;
    internal::g_next_layout.align = align;
}
void begin_disabled() { internal::g_disabled_depth++; }
void end_disabled() { if (internal::g_disabled_depth > 0) internal::g_disabled_depth--; }
void request_focus(const char* label) { if (label) internal::g_focus_request_id = internal::hash_str(label); }
void open_modal(const char* label) { if (label) internal::g_modal_request_id = internal::hash_str(label); }
void close_modal() { internal::g_modal_open_id = 0; internal::g_modal_request_id = 0; }
float calc_text_width(const char* text) { return internal::measure_text_width(text ? text : ""); }
float calc_text_height(const char* text, float wrap_width) {
    std::vector<internal::TextRange> lines;
    internal::compute_wrapped_ranges(text ? text : "", wrap_width, true, lines);
    return (float)lines.size() * internal::text_line_height();
}
void tooltip(const char* text) {
    if (!text || !text[0]) return;
    if (!internal::g_ctx.last_item_hovered) return;
    internal::g_tooltip.requested = true;
    internal::g_tooltip.request_id = internal::g_ctx.last_item_id;
    internal::g_tooltip.request_anchor = internal::g_ctx.last_item_rect;
    internal::g_tooltip.request_text = text;
}

// ============================================================
// Themes
// ============================================================

Style default_dark_style() {
    Style s;
    s.background = {0.055f,0.055f,0.059f,1}; s.panel = {0.090f,0.094f,0.102f,1};
    s.text = {0.910f,0.910f,0.910f,1}; s.text_dim = {0.663f,0.678f,0.702f,1};
    s.border = {0.169f,0.176f,0.192f,1}; s.button = {0.090f,0.094f,0.102f,1};
    s.button_hover = {0.137f,0.149f,0.169f,1}; s.button_active = {0.176f,0.192f,0.220f,1};
    s.input_bg = {0.071f,0.075f,0.082f,1}; s.input_focus = {0.310f,0.420f,0.780f,1};
    s.accent = s.input_focus; s.warning = {0.894f,0.486f,0.353f,1}; s.success = {0.420f,0.741f,0.522f,1};
    s.window_padding=20; s.item_spacing=10; s.item_height=36; s.rounding=8; s.border_width=1; s.font_size=16;
    return s;
}
Style catppuccin_mocha_style() {
    Style s;
    s.background = {0.118f,0.118f,0.180f,1}; s.panel = {0.192f,0.196f,0.267f,1};
    s.text = {0.804f,0.839f,0.957f,1}; s.text_dim = {0.729f,0.761f,0.871f,1};
    s.border = {0.271f,0.278f,0.353f,1}; s.button = {0.192f,0.196f,0.267f,1};
    s.button_hover = {0.271f,0.278f,0.353f,1}; s.button_active = {0.341f,0.349f,0.431f,1};
    s.input_bg = {0.149f,0.149f,0.220f,1}; s.input_focus = {0.537f,0.706f,0.980f,1};
    s.accent = s.input_focus; s.warning = {0.961f,0.471f,0.561f,1}; s.success = {0.651f,0.890f,0.631f,1};
    s.window_padding=20; s.item_spacing=10; s.item_height=36; s.rounding=8; s.border_width=1; s.font_size=16;
    return s;
}
Style nord_style() {
    Style s;
    s.background = {0.180f,0.204f,0.251f,1}; s.panel = {0.231f,0.259f,0.322f,1};
    s.text = {0.847f,0.871f,0.914f,1}; s.text_dim = {0.596f,0.635f,0.702f,1};
    s.border = {0.263f,0.298f,0.369f,1}; s.button = {0.231f,0.259f,0.322f,1};
    s.button_hover = {0.263f,0.298f,0.369f,1}; s.button_active = {0.298f,0.337f,0.416f,1};
    s.input_bg = {0.200f,0.227f,0.282f,1}; s.input_focus = {0.533f,0.753f,0.816f,1};
    s.accent = s.input_focus; s.warning = {0.816f,0.529f,0.440f,1}; s.success = {0.639f,0.745f,0.549f,1};
    s.window_padding=20; s.item_spacing=10; s.item_height=36; s.rounding=6; s.border_width=1; s.font_size=16;
    return s;
}
Style gruvbox_dark_style() {
    Style s;
    s.background = {0.157f,0.157f,0.157f,1}; s.panel = {0.235f,0.220f,0.212f,1};
    s.text = {0.922f,0.859f,0.698f,1}; s.text_dim = {0.835f,0.769f,0.576f,1};
    s.border = {0.314f,0.294f,0.282f,1}; s.button = {0.235f,0.220f,0.212f,1};
    s.button_hover = {0.314f,0.294f,0.282f,1}; s.button_active = {0.400f,0.373f,0.329f,1};
    s.input_bg = {0.196f,0.188f,0.188f,1}; s.input_focus = {0.271f,0.522f,0.533f,1};
    s.accent = s.input_focus; s.warning = {0.984f,0.490f,0.247f,1}; s.success = {0.722f,0.733f,0.247f,1};
    s.window_padding=20; s.item_spacing=10; s.item_height=36; s.rounding=4; s.border_width=1; s.font_size=16;
    return s;
}
Style one_dark_style() {
    Style s;
    s.background = {0.157f,0.173f,0.204f,1}; s.panel = {0.173f,0.192f,0.227f,1};
    s.text = {0.671f,0.698f,0.749f,1}; s.text_dim = {0.435f,0.467f,0.522f,1};
    s.border = {0.208f,0.231f,0.271f,1}; s.button = {0.173f,0.192f,0.227f,1};
    s.button_hover = {0.208f,0.231f,0.271f,1}; s.button_active = {0.247f,0.278f,0.329f,1};
    s.input_bg = {0.145f,0.161f,0.192f,1}; s.input_focus = {0.380f,0.686f,0.937f,1};
    s.accent = s.input_focus; s.warning = {0.878f,0.423f,0.458f,1}; s.success = {0.596f,0.757f,0.420f,1};
    s.window_padding=20; s.item_spacing=10; s.item_height=36; s.rounding=6; s.border_width=1; s.font_size=16;
    return s;
}


// ============================================================
// Windows Implementation
// ============================================================
#ifdef _WIN32

namespace internal {

static std::wstring utf8_to_wide(const char* u) {
    if (!u || !u[0]) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, u, -1, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(n - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, u, -1, &w[0], n);
    return w;
}

struct PlatformState {
    HWND hwnd = nullptr; HINSTANCE instance = nullptr;
    bool running = false; int width = 0, height = 0;
    HICON icon_big = nullptr;
    HICON icon_small = nullptr;
    bool  icon_owned = false;
    BuiltinIcon icon_variant = BuiltinIcon::Symbol;
};
struct RendererState {
    ID2D1Factory*           d2d_factory      = nullptr;
    ID2D1HwndRenderTarget*  target           = nullptr;
    ID2D1SolidColorBrush*   brush            = nullptr;
    IDWriteFactory*         dwrite_factory   = nullptr;
    IDWriteTextFormat*      text_format      = nullptr;
    IDWriteRenderingParams* rendering_params = nullptr;
    IWICImagingFactory*     wic_factory      = nullptr;
    float dpi_scale = 1.0f; bool com_inited = false;
};

static PlatformState  g_platform;
static RendererState  g_renderer;
static LARGE_INTEGER  g_freq, g_last_time;
static ID2D1Bitmap*   g_prev_frame_bitmap = nullptr;
static std::vector<BYTE> g_prev_frame_pixels;
static UINT           g_prev_frame_w = 0, g_prev_frame_h = 0;

static void dbg(const char* fmt, ...) {
    char buf[512]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    OutputDebugStringA(buf);
}

static D2D1_COLOR_F tod(Color c) { return {c.r, c.g, c.b, c.a}; }
static Color apply_draw_color(Color c) {
    c.a *= g_draw_fx_opacity;
    return c;
}
static Rect apply_draw_rect(Rect r) {
    r.x += g_draw_fx_off_x;
    r.y += g_draw_fx_off_y;
    return r;
}
static void set_brush(Color c)   { g_renderer.brush->SetColor(tod(apply_draw_color(c))); }
static void clear_bg(Color c)    { g_renderer.target->Clear(tod(c)); }

static void fill_round_rect(Rect r, float rad, Color c) {
    r = apply_draw_rect(r);
    set_brush(c);
    D2D1_RECT_F rc = {r.x, r.y, r.x+r.w, r.y+r.h};
    g_renderer.target->FillRoundedRectangle(D2D1::RoundedRect(rc, rad, rad), g_renderer.brush);
}
static void stroke_round_rect(Rect r, float rad, float thick, Color c) {
    r = apply_draw_rect(r);
    set_brush(c);
    D2D1_RECT_F rc = {r.x, r.y, r.x+r.w, r.y+r.h};
    g_renderer.target->DrawRoundedRectangle(D2D1::RoundedRect(rc, rad, rad), g_renderer.brush, thick);
}
static void fill_rect(Rect r, Color c) {
    r = apply_draw_rect(r);
    set_brush(c);
    D2D1_RECT_F rc = {r.x, r.y, r.x+r.w, r.y+r.h};
    g_renderer.target->FillRectangle(rc, g_renderer.brush);
}
static void fill_triangle(Rect r, int dir, Color c) {
    r = apply_draw_rect(r);
    ID2D1PathGeometry* tri = nullptr;
    if (FAILED(g_renderer.d2d_factory->CreatePathGeometry(&tri)) || !tri) return;
    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(tri->Open(&sink)) || !sink) { tri->Release(); return; }

    D2D1_POINT_2F pts[3];
    if (dir == 0) {
        pts[0] = {r.x + r.w * 0.5f, r.y + r.h};
        pts[1] = {r.x, r.y};
        pts[2] = {r.x + r.w, r.y};
    } else if (dir == 1) {
        pts[0] = {r.x + r.w * 0.5f, r.y};
        pts[1] = {r.x, r.y + r.h};
        pts[2] = {r.x + r.w, r.y + r.h};
    } else {
        pts[0] = {r.x + r.w, r.y + r.h * 0.5f};
        pts[1] = {r.x, r.y};
        pts[2] = {r.x, r.y + r.h};
    }
    sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(pts[1]);
    sink->AddLine(pts[2]);
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    sink->Release();

    set_brush(c);
    g_renderer.target->FillGeometry(tri, g_renderer.brush);
    tri->Release();
}
static void draw_line(float x0, float y0, float x1, float y1, float thick, Color c) {
    set_brush(c);
    g_renderer.target->DrawLine({x0 + g_draw_fx_off_x, y0 + g_draw_fx_off_y},
                                {x1 + g_draw_fx_off_x, y1 + g_draw_fx_off_y},
                                g_renderer.brush, thick);
}
static void push_clip(Rect r) {
    r = apply_draw_rect(r);
    D2D1_RECT_F rc = {r.x, r.y, r.x+r.w, r.y+r.h};
    g_renderer.target->PushAxisAlignedClip(rc, D2D1_ANTIALIAS_MODE_ALIASED);
}
static void pop_clip() { g_renderer.target->PopAxisAlignedClip(); }

static void draw_text_utf8(const char* utf8, Rect r, Color c) {
    std::wstring w = utf8_to_wide(utf8);
    if (w.empty()) return;
    r = apply_draw_rect(r);
    set_brush(c);
    D2D1_RECT_F rc = {r.x, r.y, r.x+r.w, r.y+r.h};
    DWRITE_TRIMMING trim = {DWRITE_TRIMMING_GRANULARITY_NONE,0,0};
    g_renderer.text_format->SetTrimming(&trim, nullptr);
    g_renderer.target->DrawText(w.c_str(),(UINT32)w.size(),g_renderer.text_format,rc,g_renderer.brush,D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
static void draw_text_utf8_centered(const char* utf8, Rect r, Color c) {
    g_renderer.text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    draw_text_utf8(utf8, r, c);
    g_renderer.text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

static float measure_text_width(const char* utf8) {
    if (!utf8 || !utf8[0]) return 0.0f;
    std::wstring w = utf8_to_wide(utf8);
    IDWriteTextLayout* lay = nullptr;
    if (FAILED(g_renderer.dwrite_factory->CreateTextLayout(w.c_str(),(UINT32)w.size(),g_renderer.text_format,10000,1000,&lay))||!lay) return 0;
    DWRITE_TEXT_METRICS m; lay->GetMetrics(&m); lay->Release(); return m.width;
}
static int byte_from_x(const char* utf8, float rel_x) {
    if (!utf8||!utf8[0]||rel_x<=0) return 0;
    std::wstring w = utf8_to_wide(utf8);
    IDWriteTextLayout* lay = nullptr;
    if (FAILED(g_renderer.dwrite_factory->CreateTextLayout(w.c_str(),(UINT32)w.size(),g_renderer.text_format,10000,1000,&lay))||!lay) return (int)strlen(utf8);
    BOOL trail=0,inside=0; DWRITE_HIT_TEST_METRICS m{};
    lay->HitTestPoint(rel_x,0,&trail,&inside,&m);
    UINT32 wpos = m.textPosition + (trail?1u:0u);
    if (wpos > (UINT32)w.size()) wpos=(UINT32)w.size();
    lay->Release();
    const char* p = utf8; UINT32 wc=0;
    while (*p && wc < wpos) {
        unsigned char ch=(unsigned char)*p;
        int sl = ch<0x80?1:ch<0xE0?2:ch<0xF0?3:4;
        wc += (sl==4)?2:1; p+=sl;
    }
    return (int)(p-utf8);
}

static void draw_image_handle(ImageHandle* img, Rect r) {
    auto* bmp = img ? static_cast<ID2D1Bitmap*>(img->_impl) : nullptr;
    if (!bmp) return;
    r = apply_draw_rect(r);
    D2D1_RECT_F dst = {r.x, r.y, r.x+r.w, r.y+r.h};
    g_renderer.target->DrawBitmap(bmp, dst, g_draw_fx_opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

static void clipboard_set(const char* utf8) {
    if (!g_platform.hwnd||!utf8) return;
    std::wstring w = utf8_to_wide(utf8);
    size_t n = (w.size()+1)*sizeof(wchar_t);
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE,n);
    if (!hg) return;
    memcpy(GlobalLock(hg),w.c_str(),n); GlobalUnlock(hg);
    if (OpenClipboard(g_platform.hwnd)) { EmptyClipboard(); SetClipboardData(CF_UNICODETEXT,hg); CloseClipboard(); }
    else GlobalFree(hg);
}
static std::string clipboard_get() {
    if (!g_platform.hwnd||!OpenClipboard(g_platform.hwnd)) return "";
    std::string result;
    HGLOBAL hg = GetClipboardData(CF_UNICODETEXT);
    if (hg) {
        wchar_t* w=(wchar_t*)GlobalLock(hg);
        if (w) { int n=WideCharToMultiByte(CP_UTF8,0,w,-1,nullptr,0,nullptr,nullptr);
                 if (n>1){result.resize(n-1);WideCharToMultiByte(CP_UTF8,0,w,-1,&result[0],n,nullptr,nullptr);}
                 GlobalUnlock(hg); }
    }
    CloseClipboard(); return result;
}

static void release_frame_snapshot() {
    if (g_prev_frame_bitmap) { g_prev_frame_bitmap->Release(); g_prev_frame_bitmap = nullptr; }
    g_prev_frame_pixels.clear();
    g_prev_frame_w = g_prev_frame_h = 0;
}

static void draw_previous_frame_overlay(Rect clip_r, float dx, float opacity) {
    if (!g_prev_frame_bitmap || opacity <= 0.001f) return;
    D2D1_RECT_F clip = {clip_r.x, clip_r.y, clip_r.x + clip_r.w, clip_r.y + clip_r.h};
    D2D1_RECT_F dst = {dx, 0.0f, dx + (float)g_prev_frame_w, (float)g_prev_frame_h};
    g_renderer.target->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);
    g_renderer.target->DrawBitmap(g_prev_frame_bitmap, dst, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    g_renderer.target->PopAxisAlignedClip();
}

static void draw_previous_frame_blur_panel(Rect r, float opacity) {
    if (!g_prev_frame_bitmap || opacity <= 0.001f) return;
    r = apply_draw_rect(r);
    D2D1_RECT_F dst = {r.x, r.y, r.x + r.w, r.y + r.h};
    struct BlurSample { float ox, oy, weight; };
    static const BlurSample samples[] = {
        { 0.0f,  0.0f, 0.24f},
        {-2.0f,  0.0f, 0.10f}, { 2.0f,  0.0f, 0.10f},
        { 0.0f, -2.0f, 0.10f}, { 0.0f,  2.0f, 0.10f},
        {-1.5f, -1.5f, 0.08f}, { 1.5f, -1.5f, 0.08f},
        {-1.5f,  1.5f, 0.08f}, { 1.5f,  1.5f, 0.08f},
        {-3.0f,  0.0f, 0.04f}, { 3.0f,  0.0f, 0.04f},
        { 0.0f, -3.0f, 0.04f}, { 0.0f,  3.0f, 0.04f}
    };
    g_renderer.target->PushAxisAlignedClip(dst, D2D1_ANTIALIAS_MODE_ALIASED);
    for (const BlurSample& s : samples) {
        D2D1_RECT_F src = {
            dst.left + s.ox,
            dst.top + s.oy,
            dst.right + s.ox,
            dst.bottom + s.oy
        };
        g_renderer.target->DrawBitmap(g_prev_frame_bitmap, dst, opacity * s.weight,
                                      D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, src);
    }
    g_renderer.target->PopAxisAlignedClip();
}

static void capture_frame_snapshot() {
    if (!effects_enabled() || !g_platform.hwnd || !g_renderer.target ||
        g_tab_content_owner != 0 || g_dropdown_open_id != 0) return;

    RECT rc{};
    GetClientRect(g_platform.hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) { release_frame_snapshot(); return; }

    UINT stride = (UINT)w * 4u;
    size_t bytes = (size_t)stride * (size_t)h;
    if (g_prev_frame_pixels.size() != bytes) g_prev_frame_pixels.resize(bytes);

    HDC src_dc = GetDC(g_platform.hwnd);
    if (!src_dc) return;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(src_dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC mem_dc = CreateCompatibleDC(src_dc);
    if (!dib || !mem_dc || !bits) {
        if (mem_dc) DeleteDC(mem_dc);
        if (dib) DeleteObject(dib);
        ReleaseDC(g_platform.hwnd, src_dc);
        return;
    }

    HGDIOBJ old = SelectObject(mem_dc, dib);
    BitBlt(mem_dc, 0, 0, w, h, src_dc, 0, 0, SRCCOPY);
    memcpy(g_prev_frame_pixels.data(), bits, bytes);

    SelectObject(mem_dc, old);
    DeleteDC(mem_dc);
    DeleteObject(dib);
    ReleaseDC(g_platform.hwnd, src_dc);

    UINT dpi = GetDpiForWindow(g_platform.hwnd); if (!dpi) dpi = 96;
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        (float)dpi, (float)dpi);

    if (!g_prev_frame_bitmap || g_prev_frame_w != (UINT)w || g_prev_frame_h != (UINT)h) {
        if (g_prev_frame_bitmap) { g_prev_frame_bitmap->Release(); g_prev_frame_bitmap = nullptr; }
        if (FAILED(g_renderer.target->CreateBitmap(D2D1::SizeU((UINT32)w, (UINT32)h),
                                                   g_prev_frame_pixels.data(), stride,
                                                   props, &g_prev_frame_bitmap))) {
            g_prev_frame_w = g_prev_frame_h = 0;
            return;
        }
        g_prev_frame_w = (UINT)w;
        g_prev_frame_h = (UINT)h;
    } else {
        g_prev_frame_bitmap->CopyFromMemory(nullptr, g_prev_frame_pixels.data(), stride);
    }
}

static bool init_d2d() {
    D2D1_FACTORY_OPTIONS opts = {};
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,__uuidof(ID2D1Factory),&opts,(void**)&g_renderer.d2d_factory))) return false;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),(IUnknown**)&g_renderer.dwrite_factory))) return false;
    return true;
}
static bool create_render_target() {
    RECT rc; GetClientRect(g_platform.hwnd, &rc);
    D2D1_SIZE_U psz = {(UINT32)(rc.right-rc.left),(UINT32)(rc.bottom-rc.top)};
    UINT dpi = GetDpiForWindow(g_platform.hwnd); if (!dpi) dpi=96;
    g_renderer.dpi_scale = (float)dpi/96.0f;
    g_platform.width  = (int)(psz.width  / g_renderer.dpi_scale + 0.5f);
    g_platform.height = (int)(psz.height / g_renderer.dpi_scale + 0.5f);
    D2D1_RENDER_TARGET_PROPERTIES rtp = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN,D2D1_ALPHA_MODE_UNKNOWN),(float)dpi,(float)dpi);
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwp = D2D1::HwndRenderTargetProperties(g_platform.hwnd,psz);
    if (FAILED(g_renderer.d2d_factory->CreateHwndRenderTarget(rtp,hwp,&g_renderer.target))) return false;
    if (FAILED(g_renderer.target->CreateSolidColorBrush(D2D1::ColorF(1,1,1,1),&g_renderer.brush))) return false;
    g_renderer.target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    IDWriteRenderingParams* dp=nullptr; g_renderer.dwrite_factory->CreateRenderingParams(&dp);
    if (dp) { g_renderer.dwrite_factory->CreateCustomRenderingParams(dp->GetGamma(),dp->GetEnhancedContrast(),dp->GetClearTypeLevel(),dp->GetPixelGeometry(),DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC,&g_renderer.rendering_params); dp->Release(); }
    if (g_renderer.rendering_params) g_renderer.target->SetTextRenderingParams(g_renderer.rendering_params);
    return true;
}
static bool create_text_format() {
    if (g_renderer.text_format) { g_renderer.text_format->Release(); g_renderer.text_format=nullptr; }
    if (FAILED(g_renderer.dwrite_factory->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,g_style.font_size,L"en-us",&g_renderer.text_format))) return false;
    g_renderer.text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    g_renderer.text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    g_renderer.text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    return true;
}

static COLORREF to_colorref(Color c) {
    int r = (int)(clampf(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
    int g = (int)(clampf(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
    int b = (int)(clampf(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
    return RGB(r, g, b);
}

static void destroy_owned_icons() {
    if (!g_platform.icon_owned) return;
    if (g_platform.icon_big) { DestroyIcon(g_platform.icon_big); g_platform.icon_big = nullptr; }
    if (g_platform.icon_small) { DestroyIcon(g_platform.icon_small); g_platform.icon_small = nullptr; }
    g_platform.icon_owned = false;
}

static HICON make_ftui_icon(BuiltinIcon variant, int size) {
    if (!g_renderer.d2d_factory||!g_renderer.dwrite_factory) return nullptr;
    HDC screen_dc=GetDC(nullptr); HDC hdc=CreateCompatibleDC(screen_dc); ReleaseDC(nullptr,screen_dc);
    BITMAPV5HEADER bmi={}; bmi.bV5Size=sizeof(bmi); bmi.bV5Width=size; bmi.bV5Height=-size;
    bmi.bV5Planes=1; bmi.bV5BitCount=32; bmi.bV5Compression=BI_BITFIELDS;
    bmi.bV5RedMask=0x00FF0000; bmi.bV5GreenMask=0x0000FF00; bmi.bV5BlueMask=0x000000FF; bmi.bV5AlphaMask=0xFF000000;
    void* bits=nullptr; HBITMAP hbm=CreateDIBSection(hdc,(BITMAPINFO*)&bmi,DIB_RGB_COLORS,&bits,nullptr,0);
    if (!hbm){DeleteDC(hdc);return nullptr;}
    HGDIOBJ old=SelectObject(hdc,hbm);
    D2D1_RENDER_TARGET_PROPERTIES rtp=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED));
    ID2D1DCRenderTarget* dc_rt=nullptr; g_renderer.d2d_factory->CreateDCRenderTarget(&rtp,&dc_rt);
    if (!dc_rt){SelectObject(hdc,old);DeleteObject(hbm);DeleteDC(hdc);return nullptr;}
    RECT br={0,0,size,size}; dc_rt->BindDC(hdc,&br);
    ID2D1SolidColorBrush* ibr=nullptr; dc_rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,1),&ibr);
    float s=(float)size, margin=s*0.1114f, sw=fmaxf(1.f,s/56.f);
    D2D1_COLOR_F orange={1.f,130.f/255.f,0.f,1.f};
    D2D1_COLOR_F black={0.f,0.f,0.f,1.f};
    dc_rt->BeginDraw(); dc_rt->Clear(D2D1::ColorF(0,0,0,0));
    { ID2D1PathGeometry* tri=nullptr; g_renderer.d2d_factory->CreatePathGeometry(&tri);
      ID2D1GeometrySink* sink=nullptr; tri->Open(&sink);
      sink->BeginFigure({s*.5f,margin},D2D1_FIGURE_BEGIN_FILLED);
      D2D1_POINT_2F pts[2]={{s-margin,s-margin},{margin,s-margin}}; sink->AddLines(pts,2);
      sink->EndFigure(D2D1_FIGURE_END_CLOSED); sink->Close(); sink->Release();
      ibr->SetColor(D2D1::ColorF(1,1,1,1)); dc_rt->FillGeometry(tri,ibr); tri->Release(); }
    ibr->SetColor(orange); dc_rt->DrawLine({margin,margin},{s-margin,s-margin},ibr,sw*1.5f);
    { D2D1_RECT_F brd={margin,margin,s-margin,s-margin};
      ibr->SetColor(black); dc_rt->DrawRectangle(brd,ibr,sw*1.35f);
      ibr->SetColor(orange); dc_rt->DrawRectangle(brd,ibr,sw); }
    if (variant == BuiltinIcon::SymbolWithText) {
        IDWriteTextFormat* tf = nullptr;
        IDWriteTextLayout* tl = nullptr;
        float font_sz = size * 0.26f;
        if (SUCCEEDED(g_renderer.dwrite_factory->CreateTextFormat(
                L"Segoe UI Semibold", nullptr,
                DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                font_sz, L"en-us", &tf)) && tf) {
            tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            if (SUCCEEDED(g_renderer.dwrite_factory->CreateTextLayout(
                    L"FTUI", 4, tf, s - margin * 2.0f, size * 0.22f, &tl)) && tl) {
                D2D1_POINT_2F origin = {margin, s * 0.70f};
                ibr->SetColor(orange);
                dc_rt->DrawTextLayout(origin, tl, ibr, D2D1_DRAW_TEXT_OPTIONS_NONE);
                tl->Release();
            }
            tf->Release();
        }
    }
    dc_rt->EndDraw(); ibr->Release(); dc_rt->Release();
    int ms=((size+31)/32)*4; std::vector<BYTE> mb(ms*size,0);
    HBITMAP hbm_mask=CreateBitmap(size,size,1,1,mb.data());
    ICONINFO ii={TRUE,0,0,hbm_mask,hbm}; HICON icon=CreateIconIndirect(&ii);
    SelectObject(hdc,old); DeleteObject(hbm_mask); DeleteObject(hbm); DeleteDC(hdc);
    return icon;
}

static void apply_window_icon_handles(void* icon_big, void* icon_small, bool owned, BuiltinIcon variant) {
    if (owned && !g_renderer.d2d_factory) {
        g_platform.icon_owned = true;
        g_platform.icon_variant = variant;
        g_platform.icon_big = nullptr;
        g_platform.icon_small = nullptr;
        return;
    }
    if (owned) {
        destroy_owned_icons();
        g_platform.icon_big = (HICON)(icon_big ? icon_big : make_ftui_icon(variant, 256));
        g_platform.icon_small = (HICON)(icon_small ? icon_small : make_ftui_icon(variant, 32));
        g_platform.icon_owned = true;
        g_platform.icon_variant = variant;
    } else {
        destroy_owned_icons();
        g_platform.icon_big = (HICON)icon_big;
        g_platform.icon_small = (HICON)(icon_small ? icon_small : icon_big);
        g_platform.icon_owned = false;
    }
    if (g_platform.hwnd) {
        SendMessageW(g_platform.hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_platform.icon_big);
        SendMessageW(g_platform.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_platform.icon_small);
    }
}

static void sync_native_window_chrome() {
    if (!g_platform.hwnd) return;
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    using DwmSetWindowAttributeFn = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    auto fn = (DwmSetWindowAttributeFn)GetProcAddress(dwm, "DwmSetWindowAttribute");
    if (!fn) { FreeLibrary(dwm); return; }

    BOOL dark = TRUE;
    COLORREF caption = to_colorref(lerp_color(g_style.panel, g_style.background, 0.15f));
    COLORREF text = RGB(255, 255, 255);
    COLORREF border = to_colorref(g_style.border);
    fn(g_platform.hwnd, 20, &dark, sizeof(dark));
    fn(g_platform.hwnd, 34, &border, sizeof(border));
    fn(g_platform.hwnd, 35, &caption, sizeof(caption));
    fn(g_platform.hwnd, 36, &text, sizeof(text));
    FreeLibrary(dwm);
}

static void release_render_target() {
    if (g_renderer.rendering_params){g_renderer.rendering_params->Release();g_renderer.rendering_params=nullptr;}
    if (g_renderer.brush){g_renderer.brush->Release();g_renderer.brush=nullptr;}
    if (g_renderer.target){g_renderer.target->Release();g_renderer.target=nullptr;}
    release_frame_snapshot();
}

static void execute_command() {
    if (strcmp(g_cmd.buf,"q")==0) PostQuitMessage(0);
    else apply_cmd_theme();
    cmd_clear();
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (hwnd!=g_platform.hwnd&&g_platform.hwnd) return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg) {
    case WM_DESTROY: g_platform.running=false; PostQuitMessage(0); return 0;
    case WM_SIZE: {
        int pw=LOWORD(lp),ph=HIWORD(lp);
        g_platform.width=(int)(pw/g_renderer.dpi_scale+.5f); g_platform.height=(int)(ph/g_renderer.dpi_scale+.5f);
        if (g_renderer.target&&pw>0&&ph>0) g_renderer.target->Resize({(UINT32)pw,(UINT32)ph});
        return 0; }
    case WM_DPICHANGED: {
        UINT ndpi=HIWORD(wp); g_renderer.dpi_scale=(float)ndpi/96.f;
        if (g_renderer.target) g_renderer.target->SetDpi((float)ndpi,(float)ndpi);
        const RECT* r=(const RECT*)lp;
        SetWindowPos(hwnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);
        return 0; }
    case WM_MOUSEMOVE:
        g_input.mouse_x=GET_X_LPARAM(lp)/g_renderer.dpi_scale;
        g_input.mouse_y=GET_Y_LPARAM(lp)/g_renderer.dpi_scale; return 0;
    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        g_input.mouse_down=g_input.mouse_pressed=true;
        g_input.mouse_x=GET_X_LPARAM(lp)/g_renderer.dpi_scale;
        g_input.mouse_y=GET_Y_LPARAM(lp)/g_renderer.dpi_scale;
        g_input.shift_held=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        if (g_cmd.active) cmd_clear(); return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        g_input.mouse_down=false; g_input.mouse_released=true;
        g_input.mouse_x=GET_X_LPARAM(lp)/g_renderer.dpi_scale;
        g_input.mouse_y=GET_Y_LPARAM(lp)/g_renderer.dpi_scale; return 0;
    case WM_CHAR: {
        wchar_t wc=(wchar_t)wp;
        if (g_cmd.active) {
            if (wc==L'\b'){if(g_cmd.len>0)g_cmd.buf[--g_cmd.len]='\0';}
            else if(wc==L'\r'||wc==L'\n') execute_command();
            else if(wc==27) cmd_clear();
            else if(wc>=32&&g_cmd.len<15){g_cmd.buf[g_cmd.len++]=(char)wc;g_cmd.buf[g_cmd.len]='\0';}
            return 0;
        }
        if (wc==L'\b') g_input.key_backspace=true;
        else if(wc==L'\r'||wc==L'\n') g_input.key_enter=true;
        else if(wc==L':'&&g_ctx.focused_input_id==0&&g_shortcuts_enabled){g_cmd.active=true;g_cmd.len=0;g_cmd.buf[0]='\0';}
        else if(wc>=32){
            char buf[8]={}; int n=WideCharToMultiByte(CP_UTF8,0,&wc,1,buf,sizeof(buf)-1,nullptr,nullptr);
            if(n>0&&g_input.text_input_count+n<(int)sizeof(g_input.text_input)){memcpy(g_input.text_input+g_input.text_input_count,buf,n);g_input.text_input_count+=n;}
        }
        return 0; }
    case WM_KEYDOWN:
        g_input.shift_held=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        g_input.ctrl_held=(GetKeyState(VK_CONTROL)&0x8000)!=0;
        if (g_cmd.active){if(wp==VK_ESCAPE)cmd_clear();return 0;}
        if(wp==VK_BACK)  g_input.key_backspace=true;
        if(wp==VK_RETURN)g_input.key_enter=true;
        if(wp==VK_SPACE) g_input.key_space=true;
        if(wp==VK_ESCAPE)g_input.key_escape=true;
        if(wp==VK_TAB){if(g_input.shift_held)g_input.key_shift_tab=true;else g_input.key_tab=true;}
        if(wp==VK_LEFT) g_input.key_left=true;
        if(wp==VK_RIGHT)g_input.key_right=true;
        if(wp==VK_UP)   g_input.key_up=true;
        if(wp==VK_DOWN) g_input.key_down=true;
        if(g_input.ctrl_held){
            if(wp=='A'&&g_ctx.focused_input_id!=0){
                g_text_cursor_id=g_ctx.focused_input_id; g_text_sel_anchor=0; g_text_cursor=0x7FFFFFFF;
                g_ta_cursor_id  =g_ctx.focused_input_id; g_ta_sel_anchor  =0; g_ta_cursor  =0x7FFFFFFF;
            }
            if(wp=='C')g_input.key_ctrl_c=true;
            if(wp=='V')g_input.key_ctrl_v=true;
            if(wp=='Q'&&g_shortcuts_enabled)PostQuitMessage(0);
        }
        return 0;
    case WM_KEYUP:
        g_input.shift_held=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        g_input.ctrl_held=(GetKeyState(VK_CONTROL)&0x8000)!=0; return 0;
    case WM_MOUSEWHEEL: {
        short delta=(short)HIWORD(wp);
        if (effects_enabled()) {
            g_input.wheel_y += (float)delta / WHEEL_DELTA * 80.0f;
        } else {
            g_scroll_y -= (float)delta/WHEEL_DELTA*80.f;
            g_scroll_target_y = g_scroll_y;
            if(g_scroll_y<0)g_scroll_y=0;
        }
        return 0; }
    case WM_SETFOCUS:   g_input.focused=true;  return 0;
    case WM_KILLFOCUS:  g_input.focused=false;  g_ctx.focused_input_id=0; g_ctx.focused_widget_id=0; return 0;
    case WM_ERASEBKGND: return 1;
    default: return DefWindowProcW(hwnd,msg,wp,lp);
    }
}

} // namespace internal

bool create_window(const Config& cfg) {
    using namespace internal;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    g_renderer.com_inited = (hr==S_OK||hr==S_FALSE);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    g_style = startup_style();
    g_scroll_y = g_scroll_target_y = 0;
    g_ta_scroll_y = g_ta_scroll_target_y = 0;
    reset_effect_state(cfg.enable_effects && FTUI_WINDOWS_EFFECTS);
    g_platform.instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc={sizeof(wc)}; wc.style=CS_HREDRAW|CS_VREDRAW; wc.lpfnWndProc=wndproc;
    wc.hInstance=g_platform.instance; wc.hCursor=LoadCursorW(nullptr,(LPCWSTR)IDC_ARROW);
    wc.lpszClassName=L"FTUI_Window";
    wc.hIcon=cfg.icon?(HICON)cfg.icon:LoadIconW(nullptr,(LPCWSTR)IDI_APPLICATION); wc.hIconSm=wc.hIcon;
    if (!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;

    DWORD style=WS_OVERLAPPEDWINDOW; if(!cfg.resizable)style&=~(WS_THICKFRAME|WS_MAXIMIZEBOX);
    UINT sys_dpi=GetDpiForSystem(); float pre=(float)sys_dpi/96.f;
    RECT rc={0,0,(LONG)(cfg.width*pre+.5f),(LONG)(cfg.height*pre+.5f)};
    using AdjFn=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);
    auto adj=(AdjFn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi");
    if(adj) adj(&rc,style,FALSE,0,sys_dpi); else AdjustWindowRect(&rc,style,FALSE);
    int w=rc.right-rc.left,h=rc.bottom-rc.top,x=CW_USEDEFAULT,y=CW_USEDEFAULT;
    if(cfg.center_window){int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);x=(sw-w)/2;y=(sh-h)/2;}

    std::wstring wtitle = utf8_to_wide(cfg.title ? cfg.title : "FTUI App");
    g_platform.hwnd=CreateWindowExW(0,L"FTUI_Window",wtitle.c_str(),style,x,y,w,h,nullptr,nullptr,g_platform.instance,nullptr);
    if(!g_platform.hwnd) return false;
    g_platform.width=cfg.width; g_platform.height=cfg.height; g_platform.running=true;
    if(!init_d2d()||!create_render_target()||!create_text_format()) return false;
    if(g_platform.width!=cfg.width||g_platform.height!=cfg.height){
        RECT rc2={0,0,(LONG)(cfg.width*g_renderer.dpi_scale+.5f),(LONG)(cfg.height*g_renderer.dpi_scale+.5f)};
        UINT adpi=(UINT)(g_renderer.dpi_scale*96+.5f);
        if(adj)adj(&rc2,style,FALSE,0,adpi);else AdjustWindowRect(&rc2,style,FALSE);
        int nw=rc2.right-rc2.left,nh=rc2.bottom-rc2.top,nx=x,ny=y;
        if(cfg.center_window){nx=(GetSystemMetrics(SM_CXSCREEN)-nw)/2;ny=(GetSystemMetrics(SM_CYSCREEN)-nh)/2;}
        SetWindowPos(g_platform.hwnd,nullptr,nx,ny,nw,nh,SWP_NOZORDER|SWP_NOACTIVATE);
    }
    if (cfg.icon) apply_window_icon_handles(cfg.icon, cfg.icon, false, g_platform.icon_variant);
    else if (!g_platform.icon_owned && (g_platform.icon_big || g_platform.icon_small))
        apply_window_icon_handles(g_platform.icon_big, g_platform.icon_small, false, g_platform.icon_variant);
    else
        apply_window_icon_handles(nullptr, nullptr, true, g_platform.icon_variant);
    sync_native_window_chrome();
    ShowWindow(g_platform.hwnd,SW_SHOW); UpdateWindow(g_platform.hwnd);
    QueryPerformanceFrequency(&g_freq); QueryPerformanceCounter(&g_last_time);
    return true;
}

bool pump() {
    using namespace internal;
    g_input.mouse_pressed=g_input.mouse_released=false;
    g_input.wheel_y = 0;
    g_input.key_backspace=g_input.key_enter=g_input.key_space=g_input.key_escape=g_input.key_tab=g_input.key_shift_tab=false;
    g_input.key_left=g_input.key_right=g_input.key_up=g_input.key_down=false;
    g_input.key_ctrl_c=g_input.key_ctrl_v=false;
    g_input.text_input_count=0; memset(g_input.text_input,0,sizeof(g_input.text_input));
    MSG msg;
    while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){
        if(msg.message==WM_QUIT){g_platform.running=false;return false;}
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    return g_platform.running;
}

void begin() {
    using namespace internal;
    g_ctx.hot_id=0;
    g_frame_content_fx = {};
    g_active_collapse_fx_count = 0;
    g_pending_collapse_key = 0;
    g_pending_collapse_start_y = 0.0f;
    g_dropdown_capture_input = g_dropdown_overlay_prev.active;
    g_dropdown_overlay = {};
    begin_tooltip_frame();
    g_modal_drawn = false;
    reset_draw_fx();
    if((g_input.key_tab||g_input.key_shift_tab)&&!g_ctx.tab_stops_prev.empty()){
        auto& stops=g_ctx.tab_stops_prev;
        int cur=g_ctx.focused_widget_id,idx=-1;
        for(int i=0;i<(int)stops.size();i++){if(stops[i]==cur){idx=i;break;}}
        int next_id = g_input.key_shift_tab?stops[(idx<=0?(int)stops.size():idx)-1]:stops[(idx+1)%(int)stops.size()];
        set_widget_focus(next_id, false);
    }
    if (g_focus_request_id) {
        set_widget_focus(g_focus_request_id, false);
        g_focus_request_id = 0;
    }
    g_ctx.tab_stops.clear();
    float pad=g_style.window_padding;
    g_ctx.content_region={pad,pad,(float)g_platform.width-2*pad,(float)g_platform.height-2*pad};
    g_ctx.cursor_x=g_ctx.content_region.x;
    const float kSW=14;
    float vh=g_ctx.content_region.h;
    bool nsb=g_content_height>vh+1;
    float ms=nsb?g_content_height-vh:0;
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    g_dt=(float)(now.QuadPart-g_last_time.QuadPart)/(float)g_freq.QuadPart;
    g_last_time=now; g_fps_accum+=g_dt; g_fps_frames++;
    if(g_fps_accum>=0.5f){g_fps=(float)g_fps_frames/g_fps_accum;g_fps_frames=0;g_fps_accum=0;}

    if(!nsb) { g_scroll_y=0; g_scroll_target_y=0; }
    if (effects_enabled()) {
        g_scroll_target_y=g_scroll_target_y<0?0:(g_scroll_target_y>ms?ms:g_scroll_target_y);
        g_scroll_y = step_anim(g_scroll_y, g_scroll_target_y, g_dt, 16.0f);
    } else {
        g_scroll_target_y = g_scroll_y;
    }
    g_scroll_y=g_scroll_y<0?0:(g_scroll_y>ms?ms:g_scroll_y);
    if(nsb)g_ctx.content_region.w-=kSW+pad;
    g_ctx.cursor_y=g_ctx.content_region.y-g_scroll_y;
    if(!g_renderer.target) return;
    g_renderer.target->BeginDraw(); g_drawing=true;
    clear_bg(g_style.background);
    D2D1_RECT_F cr={0,g_ctx.content_region.y,(float)g_platform.width,g_ctx.content_region.y+vh};
    g_renderer.target->PushAxisAlignedClip(cr,D2D1_ANTIALIAS_MODE_ALIASED);
}

void end() {
    using namespace internal;
    if(!g_drawing) return;
    finalize_pending_collapse_measure();
    g_renderer.target->PopAxisAlignedClip();

    if (g_frame_content_fx.active) {
        reset_draw_fx();
    }

    float new_ch=g_ctx.cursor_y+g_scroll_y-g_ctx.content_region.y;
    if(new_ch<0)new_ch=0;
    {
        const float kSW=14; float vh=g_ctx.content_region.h;
        bool nsb=g_content_height>vh+1;
        if(nsb){
            float ms=g_content_height-vh;
            if (effects_enabled() && g_input.wheel_y != 0.0f) {
                g_scroll_target_y -= g_input.wheel_y;
                g_input.wheel_y = 0.0f;
            }
            g_scroll_target_y=g_scroll_target_y<0?0:(g_scroll_target_y>ms?ms:g_scroll_target_y);
            float tx=g_ctx.content_region.x+g_ctx.content_region.w+g_style.window_padding;
            Rect track={tx,g_ctx.content_region.y,kSW,vh};
            float th=fmaxf(20,(vh/g_content_height)*vh);
            float tt=ms>0?g_scroll_y/ms:0;
            float ty2=g_ctx.content_region.y+tt*(vh-th);
            Rect thumb={tx,ty2,kSW,th};
            bool thov=rect_contains(thumb,g_input.mouse_x,g_input.mouse_y);
            bool trhov=rect_contains(track,g_input.mouse_x,g_input.mouse_y);
            if(g_input.mouse_pressed&&thov){g_sb_dragging=true;g_sb_drag_mouse_y=g_input.mouse_y;g_sb_drag_scroll0=g_scroll_y;}
            if(g_sb_dragging){
                if(g_input.mouse_down||g_input.mouse_released){
                    float sc=ms/fmaxf(1,vh-th);
                    g_scroll_y=g_sb_drag_scroll0+(g_input.mouse_y-g_sb_drag_mouse_y)*sc;
                    g_scroll_y=g_scroll_y<0?0:(g_scroll_y>ms?ms:g_scroll_y);
                    g_scroll_target_y = g_scroll_y;
                }
                if(g_input.mouse_released)g_sb_dragging=false;
            }
            if(g_input.mouse_pressed&&trhov&&!thov){
                if (effects_enabled()) {
                    g_scroll_target_y += (g_input.mouse_y<ty2?-vh:vh);
                    g_scroll_target_y=g_scroll_target_y<0?0:(g_scroll_target_y>ms?ms:g_scroll_target_y);
                } else {
                    g_scroll_y+=(g_input.mouse_y<ty2?-vh:vh);
                    g_scroll_target_y = g_scroll_y;
                    g_scroll_y=g_scroll_y<0?0:(g_scroll_y>ms?ms:g_scroll_y);
                }
            }
            float thumb_hover = thov ? 1.0f : 0.0f;
            draw_widget_chrome(track, 6, g_style.input_bg, g_style.border, 0.0f, 0.0f, 0.0f);
            draw_widget_chrome(thumb, 6,
                               g_sb_dragging ? g_style.button_active : g_style.button_hover,
                               g_sb_dragging ? g_style.input_focus : g_style.border,
                               thumb_hover, g_sb_dragging ? 1.0f : 0.0f, 0.0f);
        } else {
            if (effects_enabled() && g_input.wheel_y != 0.0f) {
                g_scroll_target_y = 0;
                g_input.wheel_y = 0.0f;
            }
        }
        g_content_height=new_ch;
    }
    if(g_debug.show_fps){
        char buf[64]; snprintf(buf,sizeof(buf),"FPS: %.0f  frame:%d  DPI:%.0f%%",g_fps,g_ctx.frame_index,g_renderer.dpi_scale*100);
        float lh=text_line_height(); Rect r={4,4,300,lh}; fill_rect(r,{0,0,0,.6f}); draw_text_utf8(buf,r,g_style.text_dim);
    }
    if(g_debug.show_hovered_id||g_debug.show_active_id){
        char buf[128]; snprintf(buf,sizeof(buf),"hot=%d active=%d focused=%d",g_ctx.hot_id,g_ctx.active_id,g_ctx.focused_widget_id);
        float lh=text_line_height(),oy=g_debug.show_fps?lh+6:4; Rect r={4,oy,360,lh}; fill_rect(r,{0,0,0,.6f}); draw_text_utf8(buf,r,g_style.text_dim);
    }
    if(g_cmd.active){
        char buf[32]; snprintf(buf,sizeof(buf),":%s_",g_cmd.buf);
        float lh=text_line_height(),oy=(float)g_platform.height-g_style.window_padding-lh;
        Rect r={g_style.window_padding,oy,200,lh};
        fill_rect({r.x-4,r.y-2,r.w+8,r.h+4},{0,0,0,.75f}); draw_text_utf8(buf,r,g_style.text);
    }
    draw_dropdown_overlay();
    finalize_tooltip_frame();
    if (g_modal_open_id && !g_modal_drawn) close_modal();
    draw_tooltip_overlay();
    HRESULT hr=g_renderer.target->EndDraw(); g_drawing=false;
    if(hr==D2DERR_RECREATE_TARGET){release_render_target();create_render_target();}
    g_dropdown_overlay_prev = g_dropdown_overlay;
    g_ctx.tab_stops_prev=g_ctx.tab_stops; g_ctx.frame_index++;
}

void shutdown() {
    using namespace internal;
    release_render_target();
    if(g_renderer.wic_factory)   {g_renderer.wic_factory->Release();   g_renderer.wic_factory=nullptr;}
    if(g_renderer.text_format)   {g_renderer.text_format->Release();   g_renderer.text_format=nullptr;}
    if(g_renderer.dwrite_factory){g_renderer.dwrite_factory->Release();g_renderer.dwrite_factory=nullptr;}
    if(g_renderer.d2d_factory)   {g_renderer.d2d_factory->Release();   g_renderer.d2d_factory=nullptr;}
    destroy_owned_icons();
    if(g_platform.hwnd)          {DestroyWindow(g_platform.hwnd);      g_platform.hwnd=nullptr;}
    if(g_renderer.com_inited)    {CoUninitialize();g_renderer.com_inited=false;}
}

void open_child_window(const Config& cfg, std::function<void()> fn) {
    using namespace internal;
    if(!g_platform.hwnd) return;
    struct Snap {
        PlatformState p; RendererState r; InputState in; UIContext ctx; Style sty; DebugState dbg;
        float sc,sct,ch,sbmy,sbms0; bool sbd;
        CmdState cmd; float fps,fpsa; int fpsf;
        LARGE_INTEGER lt;
        int tci,tc,tsa,taci,tac,taas; float tasc,tasct,dt;
        bool effects;
        MotionSlot motion[256];
        TabFxSlot tab_fx[32];
        int tab_owner;
        FrameContentFx frame_fx;
        float draw_off_x,draw_off_y,draw_opacity;
        ID2D1Bitmap* prev_frame_bitmap;
        std::vector<BYTE> prev_frame_pixels;
        UINT prev_frame_w, prev_frame_h;
        ScrollSlot scroll_slots[128];
        CollapseSlot collapse_slots[64];
        ActiveCollapseFx active_collapse_fx[16];
        int active_collapse_fx_count;
        int pending_collapse_key;
        float pending_collapse_start_y;
        NextLayoutState next_layout;
        ColorOverrideEntry color_stack[kColorOverrideStackCap];
        int color_stack_count;
        ColorOverrideTable next_color_pending, active_widget_colors;
        int active_widget_color_depth;
        TooltipState tooltip;
        DropdownOverlayState dropdown_overlay, dropdown_overlay_prev;
        int disabled_depth, focus_request_id, modal_open_id, modal_request_id, dropdown_open_id;
        bool inside_modal, modal_drawn, dropdown_capture_input;
        bool shortcuts,drawing;
    } s;
    s.p=g_platform;s.r=g_renderer;s.in=g_input;s.ctx=g_ctx;s.sty=g_style;s.dbg=g_debug;
    s.sc=g_scroll_y;s.sct=g_scroll_target_y;s.ch=g_content_height;s.sbd=g_sb_dragging;s.sbmy=g_sb_drag_mouse_y;s.sbms0=g_sb_drag_scroll0;
    s.cmd=g_cmd;s.fps=g_fps;s.fpsa=g_fps_accum;s.fpsf=g_fps_frames;s.lt=g_last_time;
    s.tci=g_text_cursor_id;s.tc=g_text_cursor;s.tsa=g_text_sel_anchor;
    s.taci=g_ta_cursor_id;s.tac=g_ta_cursor;s.taas=g_ta_sel_anchor;s.tasc=g_ta_scroll_y;s.tasct=g_ta_scroll_target_y;s.dt=g_dt;
    s.effects=g_effects_enabled;
    memcpy(s.motion, g_motion_slots, sizeof(g_motion_slots));
    memcpy(s.tab_fx, g_tab_fx_slots, sizeof(g_tab_fx_slots));
    s.tab_owner=g_tab_content_owner;
    s.frame_fx=g_frame_content_fx;
    s.draw_off_x=g_draw_fx_off_x;s.draw_off_y=g_draw_fx_off_y;s.draw_opacity=g_draw_fx_opacity;
    s.prev_frame_bitmap=g_prev_frame_bitmap;
    s.prev_frame_pixels=std::move(g_prev_frame_pixels);
    s.prev_frame_w=g_prev_frame_w;s.prev_frame_h=g_prev_frame_h;
    memcpy(s.scroll_slots, g_scroll_slots, sizeof(g_scroll_slots));
    memcpy(s.collapse_slots, g_collapse_slots, sizeof(g_collapse_slots));
    memcpy(s.active_collapse_fx, g_active_collapse_fx, sizeof(g_active_collapse_fx));
    s.active_collapse_fx_count = g_active_collapse_fx_count;
    s.pending_collapse_key = g_pending_collapse_key;
    s.pending_collapse_start_y = g_pending_collapse_start_y;
    s.next_layout=g_next_layout;
    memcpy(s.color_stack, g_color_stack, sizeof(g_color_stack));
    s.color_stack_count = g_color_stack_count;
    s.next_color_pending = g_next_color_pending;
    s.active_widget_colors = g_active_widget_colors;
    s.active_widget_color_depth = g_active_widget_color_depth;
    s.tooltip=g_tooltip;
    s.dropdown_overlay=g_dropdown_overlay;
    s.dropdown_overlay_prev=g_dropdown_overlay_prev;
    s.disabled_depth=g_disabled_depth;
    s.focus_request_id=g_focus_request_id;
    s.modal_open_id=g_modal_open_id;
    s.modal_request_id=g_modal_request_id;
    s.dropdown_open_id=g_dropdown_open_id;
    s.inside_modal=g_inside_modal;
    s.modal_drawn=g_modal_drawn;
    s.dropdown_capture_input=g_dropdown_capture_input;
    s.shortcuts=g_shortcuts_enabled;s.drawing=g_drawing;

    auto* d2d = g_renderer.d2d_factory;
    auto* wic = g_renderer.wic_factory;
    auto* dw = g_renderer.dwrite_factory;
    auto  inst = g_platform.instance;
    auto  owner = g_platform.hwnd;

    g_platform={}; g_platform.instance=inst;
    g_renderer={}; g_renderer.d2d_factory=d2d; g_renderer.dwrite_factory=(IDWriteFactory*)dw; g_renderer.wic_factory=(IWICImagingFactory*)wic; g_renderer.dpi_scale=1;
    g_input={}; g_ctx={}; g_style=s.sty; g_debug=s.dbg;
    g_scroll_y=g_content_height=0; g_sb_dragging=false; g_cmd={};
    g_fps=g_fps_accum=0; g_fps_frames=0;
    g_text_cursor_id=g_text_cursor=g_text_sel_anchor=0;
    g_ta_cursor_id=g_ta_cursor=g_ta_sel_anchor=0; g_ta_scroll_y=0;
    g_prev_frame_bitmap=nullptr; g_prev_frame_pixels.clear(); g_prev_frame_w=0; g_prev_frame_h=0;
    memset(g_scroll_slots, 0, sizeof(g_scroll_slots));
    memset(g_collapse_slots, 0, sizeof(g_collapse_slots));
    memset(g_active_collapse_fx, 0, sizeof(g_active_collapse_fx));
    g_active_collapse_fx_count = 0;
    g_pending_collapse_key = 0;
    g_pending_collapse_start_y = 0.0f;
    g_next_layout={}; g_tooltip={}; g_dropdown_overlay={}; g_dropdown_overlay_prev={}; g_disabled_depth=0; g_focus_request_id=0;
    memcpy(g_color_stack, s.color_stack, sizeof(g_color_stack));
    g_color_stack_count = s.color_stack_count;
    g_next_color_pending = s.next_color_pending;
    g_active_widget_colors = s.active_widget_colors;
    g_active_widget_color_depth = s.active_widget_color_depth;
    g_modal_open_id=0; g_modal_request_id=0; g_dropdown_open_id=0; g_inside_modal=false; g_modal_drawn=false;
    g_dropdown_capture_input=false;
    reset_effect_state(cfg.enable_effects && FTUI_WINDOWS_EFFECTS);
    g_shortcuts_enabled=s.shortcuts; g_drawing=false;

    DWORD ws=WS_OVERLAPPEDWINDOW; if(!cfg.resizable)ws&=~(WS_THICKFRAME|WS_MAXIMIZEBOX);
    UINT sd=GetDpiForSystem(); float ps=(float)sd/96;
    RECT rc={0,0,(LONG)(cfg.width*ps+.5f),(LONG)(cfg.height*ps+.5f)};
    using AdjFn=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);
    auto adj=(AdjFn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi");
    if(adj)adj(&rc,ws,FALSE,0,sd);else AdjustWindowRect(&rc,ws,FALSE);
    int cw=rc.right-rc.left,ch2=rc.bottom-rc.top,cx=CW_USEDEFAULT,cy=CW_USEDEFAULT;
    if(cfg.center_window){cx=(GetSystemMetrics(SM_CXSCREEN)-cw)/2;cy=(GetSystemMetrics(SM_CYSCREEN)-ch2)/2;}
    std::wstring wt=utf8_to_wide(cfg.title?cfg.title:"FTUI");
    g_platform.hwnd=CreateWindowExW(0,L"FTUI_Window",wt.c_str(),ws,cx,cy,cw,ch2,owner,nullptr,inst,nullptr);
    if(g_platform.hwnd){
        g_platform.width=cfg.width; g_platform.height=cfg.height; g_platform.running=true;
        if(create_render_target()&&create_text_format()){
            QueryPerformanceFrequency(&g_freq); QueryPerformanceCounter(&g_last_time);
            if (cfg.icon) apply_window_icon_handles(cfg.icon, cfg.icon, false, s.p.icon_variant);
            else if (!s.p.icon_owned && (s.p.icon_big || s.p.icon_small))
                apply_window_icon_handles(s.p.icon_big, s.p.icon_small, false, s.p.icon_variant);
            else
                apply_window_icon_handles(nullptr, nullptr, true, s.p.icon_variant);
            sync_native_window_chrome();
            ShowWindow(g_platform.hwnd,SW_SHOW); UpdateWindow(g_platform.hwnd);
            while(pump()){begin();fn();end();}
        }
        release_render_target();
        if(g_renderer.text_format){g_renderer.text_format->Release();g_renderer.text_format=nullptr;}
        g_renderer.d2d_factory=nullptr; g_renderer.dwrite_factory=nullptr; g_renderer.wic_factory=nullptr;
        if(g_platform.hwnd){DestroyWindow(g_platform.hwnd);g_platform.hwnd=nullptr;}
    }
    destroy_owned_icons();
    g_platform=s.p;g_renderer=s.r;g_input=s.in;g_ctx=s.ctx;g_style=s.sty;g_debug=s.dbg;
    g_scroll_y=s.sc;g_scroll_target_y=s.sct;g_content_height=s.ch;g_sb_dragging=s.sbd;g_sb_drag_mouse_y=s.sbmy;g_sb_drag_scroll0=s.sbms0;
    g_cmd=s.cmd;g_fps=s.fps;g_fps_accum=s.fpsa;g_fps_frames=s.fpsf;g_last_time=s.lt;
    g_text_cursor_id=s.tci;g_text_cursor=s.tc;g_text_sel_anchor=s.tsa;
    g_ta_cursor_id=s.taci;g_ta_cursor=s.tac;g_ta_sel_anchor=s.taas;g_ta_scroll_y=s.tasc;g_ta_scroll_target_y=s.tasct;g_dt=s.dt;
    g_effects_enabled=s.effects;
    memcpy(g_motion_slots, s.motion, sizeof(g_motion_slots));
    memcpy(g_tab_fx_slots, s.tab_fx, sizeof(g_tab_fx_slots));
    g_tab_content_owner=s.tab_owner;
    g_frame_content_fx=s.frame_fx;
    g_draw_fx_off_x=s.draw_off_x;g_draw_fx_off_y=s.draw_off_y;g_draw_fx_opacity=s.draw_opacity;
    g_prev_frame_bitmap=s.prev_frame_bitmap;
    g_prev_frame_pixels=std::move(s.prev_frame_pixels);
    g_prev_frame_w=s.prev_frame_w;g_prev_frame_h=s.prev_frame_h;
    memcpy(g_scroll_slots, s.scroll_slots, sizeof(g_scroll_slots));
    memcpy(g_collapse_slots, s.collapse_slots, sizeof(g_collapse_slots));
    memcpy(g_active_collapse_fx, s.active_collapse_fx, sizeof(g_active_collapse_fx));
    g_active_collapse_fx_count = s.active_collapse_fx_count;
    g_pending_collapse_key = s.pending_collapse_key;
    g_pending_collapse_start_y = s.pending_collapse_start_y;
    g_next_layout=s.next_layout;
    memcpy(g_color_stack, s.color_stack, sizeof(g_color_stack));
    g_color_stack_count = s.color_stack_count;
    g_next_color_pending = s.next_color_pending;
    g_active_widget_colors = s.active_widget_colors;
    g_active_widget_color_depth = s.active_widget_color_depth;
    g_tooltip=s.tooltip;
    g_dropdown_overlay=s.dropdown_overlay;
    g_dropdown_overlay_prev=s.dropdown_overlay_prev;
    g_disabled_depth=s.disabled_depth;
    g_focus_request_id=s.focus_request_id;
    g_modal_open_id=s.modal_open_id;
    g_modal_request_id=s.modal_request_id;
    g_dropdown_open_id=s.dropdown_open_id;
    g_inside_modal=s.inside_modal;
    g_modal_drawn=s.modal_drawn;
    g_dropdown_capture_input=s.dropdown_capture_input;
    g_shortcuts_enabled=s.shortcuts;g_drawing=s.drawing;
    InvalidateRect(owner,nullptr,FALSE);
}

ImageHandle* load_image(const char* utf8_path) {
    using namespace internal;
    if(!g_renderer.target||!utf8_path||!utf8_path[0]) return nullptr;
    if(!g_renderer.wic_factory){
        if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&g_renderer.wic_factory)))) return nullptr;
    }
    std::wstring wp=utf8_to_wide(utf8_path);
    IWICBitmapDecoder* dec=nullptr;
    if(FAILED(g_renderer.wic_factory->CreateDecoderFromFilename(wp.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&dec))) return nullptr;
    IWICBitmapFrameDecode* frame=nullptr;
    HRESULT hr=dec->GetFrame(0,&frame); dec->Release(); if(FAILED(hr)) return nullptr;
    IWICFormatConverter* conv=nullptr; g_renderer.wic_factory->CreateFormatConverter(&conv);
    hr=conv->Initialize(frame,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeMedianCut);
    frame->Release(); if(FAILED(hr)){conv->Release();return nullptr;}
    ID2D1Bitmap* bmp=nullptr;
    hr=g_renderer.target->CreateBitmapFromWicBitmap(conv,nullptr,&bmp); conv->Release();
    if(FAILED(hr)) return nullptr;
    ImageHandle* h=new ImageHandle(); h->_impl=bmp; return h;
}
void free_image(ImageHandle* img) {
    if(!img) return;
    if(img->_impl){static_cast<ID2D1Bitmap*>(img->_impl)->Release();img->_impl=nullptr;}
    delete img;
}

std::string open_file_dialog(const char* title, const FileFilter* filters, int filter_count) {
    using namespace internal;
    IFileOpenDialog* dlg=nullptr;
    if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dlg)))) return "";
    if(title){std::wstring wt=utf8_to_wide(title);dlg->SetTitle(wt.c_str());}
    if(filters&&filter_count>0){
        std::vector<COMDLG_FILTERSPEC> specs(filter_count);
        std::vector<std::wstring> wnames(filter_count),wspecs(filter_count);
        for(int i=0;i<filter_count;i++){
            wnames[i]=utf8_to_wide(filters[i].name?filters[i].name:"");
            wspecs[i]=utf8_to_wide(filters[i].spec?filters[i].spec:"*.*");
            specs[i]={wnames[i].c_str(),wspecs[i].c_str()};
        }
        dlg->SetFileTypes((UINT)filter_count,specs.data()); dlg->SetFileTypeIndex(1);
    }
    std::string result;
    if(SUCCEEDED(dlg->Show(g_platform.hwnd))){
        IShellItem* item=nullptr;
        if(SUCCEEDED(dlg->GetResult(&item))){
            PWSTR wpath=nullptr;
            if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&wpath))){
                int n=WideCharToMultiByte(CP_UTF8,0,wpath,-1,nullptr,0,nullptr,nullptr);
                if(n>1){result.resize(n-1);WideCharToMultiByte(CP_UTF8,0,wpath,-1,&result[0],n,nullptr,nullptr);}
                CoTaskMemFree(wpath);
            }
            item->Release();
        }
    }
    dlg->Release(); return result;
}


// ============================================================
// Linux Implementation (X11 + Cairo)
// ============================================================
#elif defined(__linux__)

namespace internal {

struct LinuxImage { cairo_surface_t* surface = nullptr; };

struct PlatformState {
    Display* display = nullptr; Window window = 0;
    Atom wm_delete = 0; bool running = false; int width = 0, height = 0;
};
struct RendererState {
    cairo_surface_t* xlib_surf = nullptr;
    cairo_surface_t* back_surf = nullptr;
    Pixmap           back_px   = 0;
    cairo_t*         cr        = nullptr;
    float            dpi_scale = 1.0f;
    char             font_face[64] = "sans-serif";
};

static PlatformState  g_platform;
static RendererState  g_renderer;
static struct timespec g_last_time;
static XIM  g_xim = nullptr;
static XIC  g_xic = nullptr;
static std::string g_clipboard_buf;

static void dbg(const char* fmt, ...) {
    char buf[512]; va_list a; va_start(a,fmt); vsnprintf(buf,sizeof(buf),fmt,a); va_end(a);
    fputs(buf, stderr);
}

static void set_color(Color c) { cairo_set_source_rgba(g_renderer.cr,c.r,c.g,c.b,c.a); }
static void sync_native_window_chrome() {}
static void apply_window_icon_handles(void*, void*, bool, BuiltinIcon) {}

static void rrect_path(cairo_t* cr, float x, float y, float w, float h, float r) {
    if (r <= 0.0f || w <= 0.0f || h <= 0.0f) { cairo_rectangle(cr,x,y,w,h); return; }
    float rr = r < w*0.5f ? (r < h*0.5f ? r : h*0.5f) : w*0.5f;
    const double PI = M_PI;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x+w-rr, y+rr,     rr, -PI/2,  0);
    cairo_arc(cr, x+w-rr, y+h-rr,   rr,  0,      PI/2);
    cairo_arc(cr, x+rr,   y+h-rr,   rr,  PI/2,   PI);
    cairo_arc(cr, x+rr,   y+rr,     rr,  PI,      3*PI/2);
    cairo_close_path(cr);
}

static void fill_round_rect(Rect r, float rad, Color c) {
    set_color(c); rrect_path(g_renderer.cr, r.x,r.y,r.w,r.h,rad); cairo_fill(g_renderer.cr);
}
static void stroke_round_rect(Rect r, float rad, float thick, Color c) {
    set_color(c); cairo_set_line_width(g_renderer.cr, thick);
    rrect_path(g_renderer.cr, r.x,r.y,r.w,r.h,rad); cairo_stroke(g_renderer.cr);
}
static void fill_rect(Rect r, Color c) {
    set_color(c); cairo_rectangle(g_renderer.cr,r.x,r.y,r.w,r.h); cairo_fill(g_renderer.cr);
}
static void fill_triangle(Rect r, int dir, Color c) {
    set_color(c);
    cairo_new_path(g_renderer.cr);
    if (dir == 0) {
        cairo_move_to(g_renderer.cr, r.x + r.w * 0.5f, r.y + r.h);
        cairo_line_to(g_renderer.cr, r.x, r.y);
        cairo_line_to(g_renderer.cr, r.x + r.w, r.y);
    } else if (dir == 1) {
        cairo_move_to(g_renderer.cr, r.x + r.w * 0.5f, r.y);
        cairo_line_to(g_renderer.cr, r.x, r.y + r.h);
        cairo_line_to(g_renderer.cr, r.x + r.w, r.y + r.h);
    } else {
        cairo_move_to(g_renderer.cr, r.x + r.w, r.y + r.h * 0.5f);
        cairo_line_to(g_renderer.cr, r.x, r.y);
        cairo_line_to(g_renderer.cr, r.x, r.y + r.h);
    }
    cairo_close_path(g_renderer.cr);
    cairo_fill(g_renderer.cr);
}
static void draw_line(float x0, float y0, float x1, float y1, float thick, Color c) {
    set_color(c); cairo_set_line_width(g_renderer.cr,thick);
    cairo_move_to(g_renderer.cr,x0,y0); cairo_line_to(g_renderer.cr,x1,y1); cairo_stroke(g_renderer.cr);
}
static void push_clip(Rect r) {
    cairo_save(g_renderer.cr); cairo_rectangle(g_renderer.cr,r.x,r.y,r.w,r.h); cairo_clip(g_renderer.cr);
}
static void pop_clip() { cairo_restore(g_renderer.cr); }

static void apply_font() {
#ifdef FTUI_LINUX_FONT
    cairo_select_font_face(g_renderer.cr, FTUI_LINUX_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
#else
    cairo_select_font_face(g_renderer.cr, g_renderer.font_face, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
#endif
    cairo_set_font_size(g_renderer.cr, g_style.font_size);
}

static void draw_text_utf8(const char* utf8, Rect r, Color c) {
    if (!utf8||!utf8[0]) return;
    cairo_t* cr = g_renderer.cr;
    apply_font();
    cairo_font_extents_t fe; cairo_font_extents(cr,&fe);
    double by = r.y + (r.h - fe.height)*0.5 + fe.ascent;
    set_color(c);
    cairo_save(cr); cairo_rectangle(cr,r.x,r.y,r.w,r.h); cairo_clip(cr);
    cairo_move_to(cr, r.x, by); cairo_show_text(cr, utf8); cairo_restore(cr);
}

static void draw_text_utf8_centered(const char* utf8, Rect r, Color c) {
    if (!utf8||!utf8[0]) return;
    cairo_t* cr = g_renderer.cr;
    apply_font();
    cairo_text_extents_t te; cairo_text_extents(cr,utf8,&te);
    cairo_font_extents_t fe; cairo_font_extents(cr,&fe);
    double cx = r.x + (r.w - te.x_advance)*0.5;
    double by = r.y + (r.h - fe.height)*0.5 + fe.ascent;
    set_color(c);
    cairo_save(cr); cairo_rectangle(cr,r.x,r.y,r.w,r.h); cairo_clip(cr);
    cairo_move_to(cr,cx,by); cairo_show_text(cr,utf8); cairo_restore(cr);
}

static float measure_text_width(const char* utf8) {
    if (!utf8||!utf8[0]) return 0;
    apply_font();
    cairo_text_extents_t te; cairo_text_extents(g_renderer.cr,utf8,&te);
    return (float)te.x_advance;
}

static int byte_from_x(const char* utf8, float rel_x) {
    if (!utf8||!utf8[0]||rel_x<=0) return 0;
    int len=(int)strlen(utf8), pos=0;
    float prev=0;
    while (pos<len) {
        int np=utf8_advance(utf8,pos);
        float w=measure_text_at(utf8,np);
        float mid=(prev+w)*0.5f;
        if (rel_x<mid) return pos;
        prev=w; pos=np;
    }
    return len;
}

static void draw_image_scaled(cairo_surface_t* surf, Rect dst) {
    if (!surf) return;
    double sw=cairo_image_surface_get_width(surf);
    double sh=cairo_image_surface_get_height(surf);
    if (sw<=0||sh<=0) return;
    cairo_t* cr=g_renderer.cr;
    cairo_save(cr);
    cairo_translate(cr,dst.x,dst.y);
    cairo_scale(cr,dst.w/sw,dst.h/sh);
    cairo_set_source_surface(cr,surf,0,0);
    cairo_pattern_set_filter(cairo_get_source(cr),CAIRO_FILTER_BILINEAR);
    cairo_rectangle(cr,0,0,sw,sh); cairo_fill(cr);
    cairo_restore(cr);
}

static void draw_image_handle(ImageHandle* img, Rect r) {
    if (!img||!img->_impl) return;
    auto* li=static_cast<LinuxImage*>(img->_impl);
    if (li&&li->surface) draw_image_scaled(li->surface,r);
}

static float detect_dpi(Display* dpy) {
    char* xrm=XResourceManagerString(dpy);
    if (xrm) {
        XrmDatabase db=XrmGetStringDatabase(xrm);
        if (db) {
            XrmValue val; char* type=nullptr;
            if (XrmGetResource(db,"Xft.dpi","Xft.Dpi",&type,&val)&&val.addr) {
                float dpi=(float)atof(val.addr); XrmDestroyDatabase(db);
                if (dpi>0) return dpi;
            }
            XrmDestroyDatabase(db);
        }
    }
    int s=DefaultScreen(dpy);
    int pw=DisplayWidth(dpy,s),mm=DisplayWidthMM(dpy,s);
    if (mm>0){float d=(float)pw/((float)mm/25.4f);if(d>0&&d<1000)return d;}
    return 96.0f;
}

static bool create_back_buffer(int w, int h) {
    if (!g_platform.display||!g_platform.window) return false;
    Display* dpy=g_platform.display; int s=DefaultScreen(dpy);
    if (g_renderer.cr)       { cairo_destroy(g_renderer.cr);          g_renderer.cr=nullptr; }
    if (g_renderer.back_surf){ cairo_surface_destroy(g_renderer.back_surf); g_renderer.back_surf=nullptr; }
    if (g_renderer.back_px)  { XFreePixmap(dpy,g_renderer.back_px);  g_renderer.back_px=0; }
    g_renderer.back_px=XCreatePixmap(dpy,g_platform.window,w,h,DefaultDepth(dpy,s));
    g_renderer.back_surf=cairo_xlib_surface_create(dpy,g_renderer.back_px,DefaultVisual(dpy,s),w,h);
    g_renderer.cr=cairo_create(g_renderer.back_surf);
    return cairo_status(g_renderer.cr)==CAIRO_STATUS_SUCCESS;
}

static bool init_cairo() {
    Display* dpy=g_platform.display; int s=DefaultScreen(dpy);
    g_renderer.xlib_surf=cairo_xlib_surface_create(dpy,g_platform.window,DefaultVisual(dpy,s),g_platform.width,g_platform.height);
    return create_back_buffer(g_platform.width,g_platform.height);
}

static void teardown_cairo() {
    if (g_renderer.cr)       { cairo_destroy(g_renderer.cr);          g_renderer.cr=nullptr; }
    if (g_renderer.back_surf){ cairo_surface_destroy(g_renderer.back_surf); g_renderer.back_surf=nullptr; }
    if (g_renderer.back_px&&g_platform.display){ XFreePixmap(g_platform.display,g_renderer.back_px); g_renderer.back_px=0; }
    if (g_renderer.xlib_surf){ cairo_surface_destroy(g_renderer.xlib_surf); g_renderer.xlib_surf=nullptr; }
}

static void swap_buffers() {
    if (!g_renderer.xlib_surf||!g_renderer.back_surf) return;
    cairo_t* fc=cairo_create(g_renderer.xlib_surf);
    cairo_set_source_surface(fc,g_renderer.back_surf,0,0);
    cairo_paint(fc); cairo_destroy(fc);
    XFlush(g_platform.display);
}

static void clipboard_set(const char* utf8) {
    if (!utf8) return;
    g_clipboard_buf=utf8;
    Atom clip=XInternAtom(g_platform.display,"CLIPBOARD",False);
    XSetSelectionOwner(g_platform.display,clip,g_platform.window,CurrentTime);
    XSetSelectionOwner(g_platform.display,XA_PRIMARY,g_platform.window,CurrentTime);
}

static void serve_selection(XEvent& ev) {
    XSelectionRequestEvent* req=&ev.xselectionrequest;
    XEvent resp={}; resp.xselection.type=SelectionNotify;
    resp.xselection.display=req->display; resp.xselection.requestor=req->requestor;
    resp.xselection.selection=req->selection; resp.xselection.target=req->target;
    resp.xselection.time=req->time; resp.xselection.property=None;
    Atom utf8a=XInternAtom(g_platform.display,"UTF8_STRING",False);
    Atom tgts=XInternAtom(g_platform.display,"TARGETS",False);
    if (req->target==tgts) {
        Atom t[]={tgts,utf8a,XA_STRING};
        XChangeProperty(req->display,req->requestor,req->property,XA_ATOM,32,PropModeReplace,(unsigned char*)t,3);
        resp.xselection.property=req->property;
    } else if (req->target==utf8a||req->target==XA_STRING) {
        XChangeProperty(req->display,req->requestor,req->property,req->target,8,PropModeReplace,(unsigned char*)g_clipboard_buf.c_str(),(int)g_clipboard_buf.size());
        resp.xselection.property=req->property;
    }
    XSendEvent(req->display,req->requestor,False,0,&resp);
}

static std::string clipboard_get() {
    Display* dpy=g_platform.display; Window win=g_platform.window;
    Atom clip=XInternAtom(dpy,"CLIPBOARD",False);
    if (XGetSelectionOwner(dpy,clip)==win) return g_clipboard_buf;
    Atom utf8a=XInternAtom(dpy,"UTF8_STRING",False);
    Atom prop=XInternAtom(dpy,"FTUI_CLIP",False);
    XConvertSelection(dpy,clip,utf8a,prop,win,CurrentTime); XFlush(dpy);
    for (int i=0;i<50;i++) {
        if (XPending(dpy)) {
            XEvent ev; XNextEvent(dpy,&ev);
            if (ev.type==SelectionNotify) {
                if (ev.xselection.property==None) return "";
                Atom at; int fmt; unsigned long n,ba; unsigned char* data=nullptr;
                XGetWindowProperty(dpy,win,prop,0,0x7fffffff,True,AnyPropertyType,&at,&fmt,&n,&ba,&data);
                if (data) { std::string r((char*)data,n); XFree(data); return r; }
                return "";
            }
            if (ev.type==SelectionRequest) serve_selection(ev);
        }
        struct timespec ts={0,2000000}; nanosleep(&ts,nullptr);
    }
    return "";
}

static void execute_command() {
    if (strcmp(g_cmd.buf,"q")==0) g_platform.running=false;
    else apply_cmd_theme();
    cmd_clear();
}

static void handle_xevent(XEvent& ev) {
    if (ev.type==SelectionRequest) { serve_selection(ev); return; }
    if (ev.xany.window!=g_platform.window) return;
    switch (ev.type) {
    case ConfigureNotify: {
        int nw=ev.xconfigure.width,nh=ev.xconfigure.height;
        if (nw!=g_platform.width||nh!=g_platform.height) {
            g_platform.width=nw; g_platform.height=nh;
            if (g_renderer.xlib_surf) cairo_xlib_surface_set_size(g_renderer.xlib_surf,nw,nh);
            create_back_buffer(nw,nh);
        }
        break; }
    case ButtonPress:
        if (ev.xbutton.button==Button1) {
            g_input.mouse_pressed=g_input.mouse_down=true;
            g_input.mouse_x=(float)ev.xbutton.x; g_input.mouse_y=(float)ev.xbutton.y;
            if (g_cmd.active) cmd_clear();
        } else if (ev.xbutton.button==Button4) {
            g_scroll_y-=80; if(g_scroll_y<0)g_scroll_y=0;
        } else if (ev.xbutton.button==Button5) {
            g_scroll_y+=80;
        }
        break;
    case ButtonRelease:
        if (ev.xbutton.button==Button1) {
            g_input.mouse_released=true; g_input.mouse_down=false;
            g_input.mouse_x=(float)ev.xbutton.x; g_input.mouse_y=(float)ev.xbutton.y;
        }
        break;
    case MotionNotify:
        g_input.mouse_x=(float)ev.xmotion.x; g_input.mouse_y=(float)ev.xmotion.y; break;
    case KeyRelease:
        g_input.shift_held=(ev.xkey.state&ShiftMask)!=0;
        g_input.ctrl_held =(ev.xkey.state&ControlMask)!=0; break;
    case KeyPress: {
        g_input.shift_held=(ev.xkey.state&ShiftMask)!=0;
        g_input.ctrl_held =(ev.xkey.state&ControlMask)!=0;
        char buf[32]={}; KeySym ks=0;
        int n=0;
        if (g_xic) {
            Status st; n=Xutf8LookupString(g_xic,&ev.xkey,buf,sizeof(buf)-1,&ks,&st);
            if (st==XLookupNone||st==XLookupChars) { if(st==XLookupNone) n=0; ks=XLookupKeysym(&ev.xkey,0); }
        } else {
            ks=XLookupKeysym(&ev.xkey,0);
            n=XLookupString(&ev.xkey,buf,sizeof(buf)-1,nullptr,nullptr);
        }
        // navigation keys
        if (ks==XK_BackSpace) g_input.key_backspace=true;
        if (ks==XK_Return||ks==XK_KP_Enter) g_input.key_enter=true;
        if (ks==XK_space) g_input.key_space=true;
        if (ks==XK_Escape) g_input.key_escape=true;
        if (ks==XK_Tab) { if(g_input.shift_held)g_input.key_shift_tab=true;else g_input.key_tab=true; }
        if (ks==XK_Left)  g_input.key_left=true;
        if (ks==XK_Right) g_input.key_right=true;
        if (ks==XK_Up)    g_input.key_up=true;
        if (ks==XK_Down)  g_input.key_down=true;
        if (ks==XK_Escape && g_cmd.active) { cmd_clear(); break; }
        if (g_input.ctrl_held) {
            if (ks==XK_q||ks==XK_Q) { if(g_shortcuts_enabled)g_platform.running=false; break; }
            if (ks==XK_c||ks==XK_C) g_input.key_ctrl_c=true;
            if (ks==XK_v||ks==XK_V) g_input.key_ctrl_v=true;
            if ((ks==XK_a||ks==XK_A)&&g_ctx.focused_input_id!=0) {
                g_text_cursor_id=g_ctx.focused_input_id; g_text_sel_anchor=0; g_text_cursor=0x7FFFFFFF;
                g_ta_cursor_id  =g_ctx.focused_input_id; g_ta_sel_anchor  =0; g_ta_cursor  =0x7FFFFFFF;
            }
            break;
        }
        if (g_cmd.active) {
            if (ks==XK_BackSpace){if(g_cmd.len>0)g_cmd.buf[--g_cmd.len]='\0';}
            else if(ks==XK_Return||ks==XK_KP_Enter) execute_command();
            else if(n>0&&buf[0]>=32&&g_cmd.len<15){g_cmd.buf[g_cmd.len++]=buf[0];g_cmd.buf[g_cmd.len]='\0';}
            break;
        }
        if (n>0&&buf[0]==':'&&g_ctx.focused_input_id==0&&g_shortcuts_enabled) {
            g_cmd.active=true; g_cmd.len=0; g_cmd.buf[0]='\0'; break;
        }
        if (n>0&&(unsigned char)buf[0]>=32) {
            if (g_input.text_input_count+n<(int)sizeof(g_input.text_input)) {
                memcpy(g_input.text_input+g_input.text_input_count,buf,n);
                g_input.text_input_count+=n;
            }
        }
        break; }
    case FocusIn:  g_input.focused=true; break;
    case FocusOut: g_input.focused=false; g_ctx.focused_input_id=0; g_ctx.focused_widget_id=0; break;
    case ClientMessage:
        if ((Atom)ev.xclient.data.l[0]==g_platform.wm_delete) g_platform.running=false; break;
    }
}

} // namespace internal

bool create_window(const Config& cfg) {
    using namespace internal;
    Display* dpy=XOpenDisplay(nullptr); if (!dpy) return false;
    g_platform.display=dpy;
    g_renderer.dpi_scale=detect_dpi(dpy)/96.0f;
    int s=DefaultScreen(dpy); Window root=RootWindow(dpy,s);
    int x=0,y=0;
    if (cfg.center_window){x=(DisplayWidth(dpy,s)-cfg.width)/2;y=(DisplayHeight(dpy,s)-cfg.height)/2;}
    g_platform.window=XCreateSimpleWindow(dpy,root,x,y,cfg.width,cfg.height,0,BlackPixel(dpy,s),BlackPixel(dpy,s));
    g_platform.wm_delete=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
    XSetWMProtocols(dpy,g_platform.window,&g_platform.wm_delete,1);
    const char* title=cfg.title?cfg.title:"FTUI App";
    XStoreName(dpy,g_platform.window,title);
    Atom net_wm_name=XInternAtom(dpy,"_NET_WM_NAME",False);
    Atom utf8str=XInternAtom(dpy,"UTF8_STRING",False);
    XChangeProperty(dpy,g_platform.window,net_wm_name,utf8str,8,PropModeReplace,(unsigned char*)title,(int)strlen(title));
    XClassHint ch={(char*)"ftui",(char*)"ftui"}; XSetClassHint(dpy,g_platform.window,&ch);
    if (!cfg.resizable){XSizeHints sh={};sh.flags=PMinSize|PMaxSize;sh.min_width=sh.max_width=cfg.width;sh.min_height=sh.max_height=cfg.height;XSetWMNormalHints(dpy,g_platform.window,&sh);}
    XSelectInput(dpy,g_platform.window,ExposureMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask|FocusChangeMask);
    g_xim=XOpenIM(dpy,nullptr,nullptr,nullptr);
    if (g_xim) g_xic=XCreateIC(g_xim,XNInputStyle,(XIMPreeditNothing|XIMStatusNothing),XNClientWindow,g_platform.window,XNFocusWindow,g_platform.window,(void*)nullptr);
    XMapWindow(dpy,g_platform.window); XFlush(dpy);
    g_platform.width=cfg.width; g_platform.height=cfg.height; g_platform.running=true;
    g_style=startup_style();
    g_scroll_y = g_scroll_target_y = 0;
    g_ta_scroll_y = g_ta_scroll_target_y = 0;
    reset_effect_state(false);
    if (!init_cairo()) return false;
    clock_gettime(CLOCK_MONOTONIC,&g_last_time);
    return true;
}

bool pump() {
    using namespace internal;
    g_input.mouse_pressed=g_input.mouse_released=false;
    g_input.wheel_y = 0;
    g_input.key_backspace=g_input.key_enter=g_input.key_space=g_input.key_escape=g_input.key_tab=g_input.key_shift_tab=false;
    g_input.key_left=g_input.key_right=g_input.key_up=g_input.key_down=false;
    g_input.key_ctrl_c=g_input.key_ctrl_v=false;
    g_input.text_input_count=0; memset(g_input.text_input,0,sizeof(g_input.text_input));
    while (XPending(g_platform.display)) {
        XEvent ev; XNextEvent(g_platform.display,&ev); handle_xevent(ev);
        if (!g_platform.running) return false;
    }
    return g_platform.running;
}

void begin() {
    using namespace internal;
    g_ctx.hot_id=0;
    g_frame_content_fx = {};
    g_active_collapse_fx_count = 0;
    g_pending_collapse_key = 0;
    g_pending_collapse_start_y = 0.0f;
    g_dropdown_capture_input = g_dropdown_overlay_prev.active;
    g_dropdown_overlay = {};
    begin_tooltip_frame();
    g_modal_drawn = false;
    reset_draw_fx();
    if ((g_input.key_tab||g_input.key_shift_tab)&&!g_ctx.tab_stops_prev.empty()) {
        auto& stops=g_ctx.tab_stops_prev;
        int cur=g_ctx.focused_widget_id,idx=-1;
        for(int i=0;i<(int)stops.size();i++){if(stops[i]==cur){idx=i;break;}}
        int next_id=g_input.key_shift_tab?stops[(idx<=0?(int)stops.size():idx)-1]:stops[(idx+1)%(int)stops.size()];
        set_widget_focus(next_id, false);
    }
    if (g_focus_request_id) {
        set_widget_focus(g_focus_request_id, false);
        g_focus_request_id = 0;
    }
    g_ctx.tab_stops.clear();
    float pad=g_style.window_padding;
    g_ctx.content_region={pad,pad,(float)g_platform.width-2*pad,(float)g_platform.height-2*pad};
    g_ctx.cursor_x=g_ctx.content_region.x;
    const float kSW=14; float vh=g_ctx.content_region.h;
    bool nsb=g_content_height>vh+1;
    float ms=nsb?g_content_height-vh:0;
    struct timespec now; clock_gettime(CLOCK_MONOTONIC,&now);
    g_dt=(now.tv_sec-g_last_time.tv_sec)+(now.tv_nsec-g_last_time.tv_nsec)*1e-9f;
    g_last_time=now; g_fps_accum+=g_dt; g_fps_frames++;
    if(g_fps_accum>=0.5f){g_fps=(float)g_fps_frames/g_fps_accum;g_fps_frames=0;g_fps_accum=0;}
    if(!nsb){g_scroll_y=0; g_scroll_target_y=0;} else g_scroll_target_y = g_scroll_y;
    g_scroll_y=g_scroll_y<0?0:(g_scroll_y>ms?ms:g_scroll_y);
    if(nsb)g_ctx.content_region.w-=kSW+pad;
    g_ctx.cursor_y=g_ctx.content_region.y-g_scroll_y;
    if(!g_renderer.cr) return;
    apply_font();
    // clear
    set_color(g_style.background); cairo_paint(g_renderer.cr);
    // content clip
    cairo_save(g_renderer.cr);
    cairo_rectangle(g_renderer.cr,0,g_ctx.content_region.y,(float)g_platform.width,vh);
    cairo_clip(g_renderer.cr);
    g_drawing=true;
}

void end() {
    using namespace internal;
    if (!g_drawing) return;
    finalize_pending_collapse_measure();
    cairo_restore(g_renderer.cr); // pop content clip
    float new_ch=g_ctx.cursor_y+g_scroll_y-g_ctx.content_region.y;
    if(new_ch<0)new_ch=0;
    {
        const float kSW=14; float vh=g_ctx.content_region.h;
        bool nsb=g_content_height>vh+1;
        if(nsb){
            float ms=g_content_height-vh;
            float tx=g_ctx.content_region.x+g_ctx.content_region.w+g_style.window_padding;
            Rect track={tx,g_ctx.content_region.y,kSW,vh};
            float th=fmaxf(20,(vh/g_content_height)*vh);
            float tt=ms>0?g_scroll_y/ms:0;
            float ty2=g_ctx.content_region.y+tt*(vh-th);
            Rect thumb={tx,ty2,kSW,th};
            bool thov=rect_contains(thumb,g_input.mouse_x,g_input.mouse_y);
            bool trhov=rect_contains(track,g_input.mouse_x,g_input.mouse_y);
            if(g_input.mouse_pressed&&thov){g_sb_dragging=true;g_sb_drag_mouse_y=g_input.mouse_y;g_sb_drag_scroll0=g_scroll_y;}
            if(g_sb_dragging){
                if(g_input.mouse_down||g_input.mouse_released){float sc=ms/fmaxf(1,vh-th);g_scroll_y=g_sb_drag_scroll0+(g_input.mouse_y-g_sb_drag_mouse_y)*sc;g_scroll_y=g_scroll_y<0?0:(g_scroll_y>ms?ms:g_scroll_y);}
                if(g_input.mouse_released)g_sb_dragging=false;
            }
            if(g_input.mouse_pressed&&trhov&&!thov){g_scroll_y+=(g_input.mouse_y<ty2?-vh:vh);g_scroll_y=g_scroll_y<0?0:(g_scroll_y>ms?ms:g_scroll_y);}
            fill_round_rect(track,6,g_style.input_bg);
            fill_round_rect(thumb,6,g_sb_dragging?g_style.input_focus:thov?g_style.button_hover:g_style.border);
        }
        g_content_height=new_ch;
    }
    if(g_debug.show_fps){
        char buf[64]; snprintf(buf,sizeof(buf),"FPS: %.0f  frame:%d  DPI:%.0f%%",g_fps,g_ctx.frame_index,g_renderer.dpi_scale*100);
        float lh=text_line_height(); Rect r={4,4,300,lh}; fill_rect(r,{0,0,0,.6f}); draw_text_utf8(buf,r,g_style.text_dim);
    }
    if(g_debug.show_hovered_id||g_debug.show_active_id){
        char buf[128]; snprintf(buf,sizeof(buf),"hot=%d active=%d focused=%d",g_ctx.hot_id,g_ctx.active_id,g_ctx.focused_widget_id);
        float lh=text_line_height(),oy=g_debug.show_fps?lh+6:4; Rect r={4,oy,360,lh}; fill_rect(r,{0,0,0,.6f}); draw_text_utf8(buf,r,g_style.text_dim);
    }
    if(g_cmd.active){
        char buf[32]; snprintf(buf,sizeof(buf),":%s_",g_cmd.buf);
        float lh=text_line_height(),oy=(float)g_platform.height-g_style.window_padding-lh;
        Rect r={g_style.window_padding,oy,200,lh};
        fill_rect({r.x-4,r.y-2,r.w+8,r.h+4},{0,0,0,.75f}); draw_text_utf8(buf,r,g_style.text);
    }
    draw_dropdown_overlay();
    finalize_tooltip_frame();
    if (g_modal_open_id && !g_modal_drawn) close_modal();
    draw_tooltip_overlay();
    g_drawing=false;
    swap_buffers();
    g_dropdown_overlay_prev = g_dropdown_overlay;
    g_ctx.tab_stops_prev=g_ctx.tab_stops; g_ctx.frame_index++;
}

void shutdown() {
    using namespace internal;
    teardown_cairo();
    if(g_xic){XDestroyIC(g_xic);g_xic=nullptr;}
    if(g_xim){XCloseIM(g_xim);g_xim=nullptr;}
    if(g_platform.window){XDestroyWindow(g_platform.display,g_platform.window);g_platform.window=0;}
    if(g_platform.display){XCloseDisplay(g_platform.display);g_platform.display=nullptr;}
}

void open_child_window(const Config& cfg, std::function<void()> fn) {
    using namespace internal;
    if(!g_platform.display) return;
    struct Snap {
        PlatformState p; RendererState r; InputState in; UIContext ctx; Style sty; DebugState dbg;
        float sc,sct,ch,sbmy,sbms0; bool sbd;
        CmdState cmd; float fps,fpsa; int fpsf;
        struct timespec lt;
        int tci,tc,tsa,taci,tac,taas; float tasc,tasct,dt;
        bool effects;
        MotionSlot motion[256];
        TabFxSlot tab_fx[32];
        int tab_owner;
        FrameContentFx frame_fx;
        float draw_off_x,draw_off_y,draw_opacity;
        ScrollSlot scroll_slots[128];
        CollapseSlot collapse_slots[64];
        ActiveCollapseFx active_collapse_fx[16];
        int active_collapse_fx_count;
        int pending_collapse_key;
        float pending_collapse_start_y;
        NextLayoutState next_layout;
        ColorOverrideEntry color_stack[kColorOverrideStackCap];
        int color_stack_count;
        ColorOverrideTable next_color_pending, active_widget_colors;
        int active_widget_color_depth;
        TooltipState tooltip;
        DropdownOverlayState dropdown_overlay, dropdown_overlay_prev;
        int disabled_depth, focus_request_id, modal_open_id, modal_request_id, dropdown_open_id;
        bool inside_modal, modal_drawn, dropdown_capture_input;
        bool shortcuts,drawing;
        XIM xim; XIC xic; std::string cb;
    } s;
    s.p=g_platform;s.r=g_renderer;s.in=g_input;s.ctx=g_ctx;s.sty=g_style;s.dbg=g_debug;
    s.sc=g_scroll_y;s.sct=g_scroll_target_y;s.ch=g_content_height;s.sbd=g_sb_dragging;s.sbmy=g_sb_drag_mouse_y;s.sbms0=g_sb_drag_scroll0;
    s.cmd=g_cmd;s.fps=g_fps;s.fpsa=g_fps_accum;s.fpsf=g_fps_frames;s.lt=g_last_time;
    s.tci=g_text_cursor_id;s.tc=g_text_cursor;s.tsa=g_text_sel_anchor;
    s.taci=g_ta_cursor_id;s.tac=g_ta_cursor;s.taas=g_ta_sel_anchor;s.tasc=g_ta_scroll_y;s.tasct=g_ta_scroll_target_y;s.dt=g_dt;
    s.effects=g_effects_enabled;
    memcpy(s.motion, g_motion_slots, sizeof(g_motion_slots));
    memcpy(s.tab_fx, g_tab_fx_slots, sizeof(g_tab_fx_slots));
    s.tab_owner=g_tab_content_owner;
    s.frame_fx=g_frame_content_fx;
    s.draw_off_x=g_draw_fx_off_x;s.draw_off_y=g_draw_fx_off_y;s.draw_opacity=g_draw_fx_opacity;
    memcpy(s.scroll_slots, g_scroll_slots, sizeof(g_scroll_slots));
    memcpy(s.collapse_slots, g_collapse_slots, sizeof(g_collapse_slots));
    memcpy(s.active_collapse_fx, g_active_collapse_fx, sizeof(g_active_collapse_fx));
    s.active_collapse_fx_count = g_active_collapse_fx_count;
    s.pending_collapse_key = g_pending_collapse_key;
    s.pending_collapse_start_y = g_pending_collapse_start_y;
    s.next_layout=g_next_layout;
    memcpy(s.color_stack, g_color_stack, sizeof(g_color_stack));
    s.color_stack_count = g_color_stack_count;
    s.next_color_pending = g_next_color_pending;
    s.active_widget_colors = g_active_widget_colors;
    s.active_widget_color_depth = g_active_widget_color_depth;
    s.tooltip=g_tooltip;
    s.dropdown_overlay=g_dropdown_overlay;
    s.dropdown_overlay_prev=g_dropdown_overlay_prev;
    s.disabled_depth=g_disabled_depth;
    s.focus_request_id=g_focus_request_id;
    s.modal_open_id=g_modal_open_id;
    s.modal_request_id=g_modal_request_id;
    s.dropdown_open_id=g_dropdown_open_id;
    s.inside_modal=g_inside_modal;
    s.modal_drawn=g_modal_drawn;
    s.dropdown_capture_input=g_dropdown_capture_input;
    s.shortcuts=g_shortcuts_enabled;s.drawing=g_drawing;s.xim=g_xim;s.xic=g_xic;s.cb=g_clipboard_buf;
    Display* dpy=g_platform.display; Window parent=g_platform.window;
    g_platform={}; g_platform.display=dpy;
    g_renderer={}; g_input={}; g_ctx={}; g_style=s.sty; g_debug=s.dbg;
    g_scroll_y=g_content_height=0; g_sb_dragging=false; g_cmd={};
    g_fps=g_fps_accum=0; g_fps_frames=0;
    g_text_cursor_id=g_text_cursor=g_text_sel_anchor=0;
    g_ta_cursor_id=g_ta_cursor=g_ta_sel_anchor=0; g_ta_scroll_y=0;
    memset(g_scroll_slots, 0, sizeof(g_scroll_slots));
    memset(g_collapse_slots, 0, sizeof(g_collapse_slots));
    memset(g_active_collapse_fx, 0, sizeof(g_active_collapse_fx));
    g_active_collapse_fx_count = 0;
    g_pending_collapse_key = 0;
    g_pending_collapse_start_y = 0.0f;
    g_next_layout={}; g_tooltip={}; g_dropdown_overlay={}; g_dropdown_overlay_prev={}; g_disabled_depth=0; g_focus_request_id=0;
    memcpy(g_color_stack, s.color_stack, sizeof(g_color_stack));
    g_color_stack_count = s.color_stack_count;
    g_next_color_pending = s.next_color_pending;
    g_active_widget_colors = s.active_widget_colors;
    g_active_widget_color_depth = s.active_widget_color_depth;
    g_modal_open_id=0; g_modal_request_id=0; g_dropdown_open_id=0; g_inside_modal=false; g_modal_drawn=false;
    g_dropdown_capture_input=false;
    reset_effect_state(false);
    g_shortcuts_enabled=s.shortcuts; g_drawing=false; g_clipboard_buf=s.cb;
    g_xim=nullptr; g_xic=nullptr;
    int screen=DefaultScreen(dpy); int cx=0,cy=0;
    if(cfg.center_window){cx=(DisplayWidth(dpy,screen)-cfg.width)/2;cy=(DisplayHeight(dpy,screen)-cfg.height)/2;}
    g_platform.window=XCreateSimpleWindow(dpy,RootWindow(dpy,screen),cx,cy,cfg.width,cfg.height,0,BlackPixel(dpy,screen),BlackPixel(dpy,screen));
    g_platform.wm_delete=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
    XSetWMProtocols(dpy,g_platform.window,&g_platform.wm_delete,1);
    XSetTransientForHint(dpy,g_platform.window,parent);
    const char* t=cfg.title?cfg.title:"FTUI"; XStoreName(dpy,g_platform.window,t);
    Atom nwn=XInternAtom(dpy,"_NET_WM_NAME",False),us=XInternAtom(dpy,"UTF8_STRING",False);
    XChangeProperty(dpy,g_platform.window,nwn,us,8,PropModeReplace,(unsigned char*)t,(int)strlen(t));
    if(!cfg.resizable){XSizeHints sh={};sh.flags=PMinSize|PMaxSize;sh.min_width=sh.max_width=cfg.width;sh.min_height=sh.max_height=cfg.height;XSetWMNormalHints(dpy,g_platform.window,&sh);}
    XSelectInput(dpy,g_platform.window,ExposureMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask|FocusChangeMask);
    g_xim=XOpenIM(dpy,nullptr,nullptr,nullptr);
    if(g_xim) g_xic=XCreateIC(g_xim,XNInputStyle,(XIMPreeditNothing|XIMStatusNothing),XNClientWindow,g_platform.window,XNFocusWindow,g_platform.window,(void*)nullptr);
    g_platform.width=cfg.width;g_platform.height=cfg.height;g_platform.running=true;
    XMapWindow(dpy,g_platform.window); XFlush(dpy);
    g_renderer.dpi_scale=s.r.dpi_scale;
    if(init_cairo()){clock_gettime(CLOCK_MONOTONIC,&g_last_time);while(pump()){begin();fn();end();}}
    teardown_cairo();
    if(g_xic){XDestroyIC(g_xic);g_xic=nullptr;}
    if(g_xim){XCloseIM(g_xim);g_xim=nullptr;}
    if(g_platform.window){XDestroyWindow(dpy,g_platform.window);g_platform.window=0;} XFlush(dpy);
    g_platform=s.p;g_renderer=s.r;g_input=s.in;g_ctx=s.ctx;g_style=s.sty;g_debug=s.dbg;
    g_scroll_y=s.sc;g_scroll_target_y=s.sct;g_content_height=s.ch;g_sb_dragging=s.sbd;g_sb_drag_mouse_y=s.sbmy;g_sb_drag_scroll0=s.sbms0;
    g_cmd=s.cmd;g_fps=s.fps;g_fps_accum=s.fpsa;g_fps_frames=s.fpsf;g_last_time=s.lt;
    g_text_cursor_id=s.tci;g_text_cursor=s.tc;g_text_sel_anchor=s.tsa;
    g_ta_cursor_id=s.taci;g_ta_cursor=s.tac;g_ta_sel_anchor=s.taas;g_ta_scroll_y=s.tasc;g_ta_scroll_target_y=s.tasct;g_dt=s.dt;
    g_effects_enabled=s.effects;
    memcpy(g_motion_slots, s.motion, sizeof(g_motion_slots));
    memcpy(g_tab_fx_slots, s.tab_fx, sizeof(g_tab_fx_slots));
    g_tab_content_owner=s.tab_owner;
    g_frame_content_fx=s.frame_fx;
    g_draw_fx_off_x=s.draw_off_x;g_draw_fx_off_y=s.draw_off_y;g_draw_fx_opacity=s.draw_opacity;
    memcpy(g_scroll_slots, s.scroll_slots, sizeof(g_scroll_slots));
    memcpy(g_collapse_slots, s.collapse_slots, sizeof(g_collapse_slots));
    memcpy(g_active_collapse_fx, s.active_collapse_fx, sizeof(g_active_collapse_fx));
    g_active_collapse_fx_count = s.active_collapse_fx_count;
    g_pending_collapse_key = s.pending_collapse_key;
    g_pending_collapse_start_y = s.pending_collapse_start_y;
    g_next_layout=s.next_layout;
    memcpy(g_color_stack, s.color_stack, sizeof(g_color_stack));
    g_color_stack_count = s.color_stack_count;
    g_next_color_pending = s.next_color_pending;
    g_active_widget_colors = s.active_widget_colors;
    g_active_widget_color_depth = s.active_widget_color_depth;
    g_tooltip=s.tooltip;
    g_dropdown_overlay=s.dropdown_overlay;
    g_dropdown_overlay_prev=s.dropdown_overlay_prev;
    g_disabled_depth=s.disabled_depth;
    g_focus_request_id=s.focus_request_id;
    g_modal_open_id=s.modal_open_id;
    g_modal_request_id=s.modal_request_id;
    g_dropdown_open_id=s.dropdown_open_id;
    g_inside_modal=s.inside_modal;
    g_modal_drawn=s.modal_drawn;
    g_dropdown_capture_input=s.dropdown_capture_input;
    g_shortcuts_enabled=s.shortcuts;g_drawing=s.drawing;g_xim=s.xim;g_xic=s.xic;g_clipboard_buf=s.cb;
    // trigger repaint of parent
    XExposeEvent xe={}; xe.type=Expose; xe.window=g_platform.window; xe.count=0;
    XSendEvent(dpy,g_platform.window,False,ExposureMask,(XEvent*)&xe); XFlush(dpy);
}

ImageHandle* load_image(const char* utf8_path) {
    if (!utf8_path||!utf8_path[0]) return nullptr;
    cairo_surface_t* surf=cairo_image_surface_create_from_png(utf8_path);
    if (!surf||cairo_surface_status(surf)!=CAIRO_STATUS_SUCCESS) {
        if(surf)cairo_surface_destroy(surf);
        internal::dbg("ftui: load_image failed for \"%s\"\n",utf8_path);
        return nullptr;
    }
    auto* li=new internal::LinuxImage{surf};
    auto* h=new ImageHandle(); h->_impl=li; return h;
}
void free_image(ImageHandle* img) {
    if (!img) return;
    if (img->_impl) { auto* li=static_cast<internal::LinuxImage*>(img->_impl); if(li->surface)cairo_surface_destroy(li->surface); delete li; img->_impl=nullptr; }
    delete img;
}

std::string open_file_dialog(const char* title, const FileFilter* filters, int filter_count) {
    std::string cmd="zenity --file-selection";
    if (title&&title[0]) { cmd+=" --title='"; for(const char*p=title;*p;p++){if(*p=='\'')cmd+="'\\''";else cmd+=*p;} cmd+="'"; }
    for (int i=0;i<filter_count;i++) {
        cmd+=" --file-filter='";
        if(filters[i].name){for(const char*p=filters[i].name;*p;p++){if(*p=='\'')cmd+="'\\''";else cmd+=*p;}}
        cmd+=" | ";
        if(filters[i].spec){std::string sp=filters[i].spec;for(char&c:sp)if(c==';')c=' ';cmd+=sp;}
        cmd+="'";
    }
    cmd+=" 2>/dev/null";
    FILE* f=popen(cmd.c_str(),"r"); if(!f) return "";
    std::string result; char buf[4096];
    while(fgets(buf,sizeof(buf),f)) result+=buf;
    pclose(f);
    while(!result.empty()&&(result.back()=='\n'||result.back()=='\r')) result.pop_back();
    return result;
}

#else
    #error "ftui.hpp: unsupported platform (Windows and Linux only)"
#endif // platform


// ============================================================
// Shared Widget Implementations
// ============================================================

using namespace internal;

void text(const char* label) {
    if (!g_drawing) return;
    WidgetColorScope color_scope;
    char vis[256]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    float h = g_style.item_height;
    Rect r = next_rect(h);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    if (cfx.clip_active) push_clip(cfx.clip);
    if (g_debug.show_layout_rects) stroke_round_rect(r, 0, 1, {1,0,0,0.4f});
    draw_text_utf8(vis, r, maybe_disabled(resolve_color(ColorRole::Text)));
    if (cfx.clip_active) pop_clip();
    mark_last_item(0, r, rect_contains(r, g_input.mouse_x, g_input.mouse_y), false);
}

void text_wrapped(const char* text) {
    if (!g_drawing) return;
    WidgetColorScope color_scope;
    std::vector<TextRange> lines;
    compute_wrapped_ranges(text ? text : "", g_ctx.content_region.w, true, lines);
    float lh = text_line_height();
    Rect r = next_rect((float)lines.size() * lh);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    if (cfx.clip_active) push_clip(cfx.clip);
    Color text_col = maybe_disabled(resolve_color(ColorRole::Text));
    float y = r.y;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line((text ? text : "") + lines[i].start, (text ? text : "") + lines[i].end);
        Rect lr = {r.x, y, r.w, lh};
        draw_text_utf8(line.c_str(), lr, text_col);
        y += lh;
    }
    if (cfx.clip_active) pop_clip();
    mark_last_item(0, r, rect_contains(r, g_input.mouse_x, g_input.mouse_y), false);
}

void separator() {
    if (!g_drawing) return;
    WidgetColorScope color_scope;
    float h = g_style.item_spacing * 2 + 1;
    Rect r = next_rect(h);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    if (cfx.clip_active) push_clip(cfx.clip);
    float my = r.y + r.h * 0.5f;
    draw_line(r.x, my, r.x + r.w, my, 1.0f, resolve_color(ColorRole::Border));
    if (cfx.clip_active) pop_clip();
}

void spacing(float px) {
    if (!g_drawing) return;
    next_rect(px);
}

static bool button_impl(const char* label, const Color* tint_override, const ColorRole* tint_role) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[256]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);

    Rect r = next_rect(g_style.item_height);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    bool raw_hov = rect_contains(r, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;
    bool clicked = false;

    if (hov && g_input.mouse_pressed) { g_ctx.active_id = id; set_widget_focus(id, false); }
    if (g_ctx.active_id == id && g_input.mouse_released) {
        if (hov) clicked = true;
        g_ctx.active_id = 0;
    }
    if (hov) g_ctx.hot_id = id;
    bool focused = is_widget_focused(id);
    if (focused && enabled && (g_input.key_enter || g_input.key_space)) clicked = true;

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, raw_hov, enabled && g_ctx.active_id == id, focused);

    Color panel = resolve_color(ColorRole::Panel);
    Color tint = tint_override ? *tint_override : (tint_role ? resolve_color(*tint_role) : resolve_color(ColorRole::Button));
    Color button = tint_override || tint_role ? tint : resolve_color(ColorRole::Button);
    Color button_hover = tint_override || tint_role ? lerp_color(tint, resolve_color(ColorRole::ButtonHover), 0.35f)
                                       : resolve_color(ColorRole::ButtonHover);
    Color button_active = tint_override || tint_role ? lerp_color(tint, resolve_color(ColorRole::ButtonActive), 0.30f)
                                        : resolve_color(ColorRole::ButtonActive);
    Color border_base = tint_override || tint_role ? lerp_color(resolve_color(ColorRole::Border), tint, 0.10f)
                                      : resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color text = resolve_color(ColorRole::Text);

    Color bg = lerp_color(button, panel, 0.30f);
    bg = lerp_color(bg, button_hover, 0.20f + motion.hover * 0.35f);
    bg = lerp_color(bg, focus, 0.06f + motion.hover * 0.05f + motion.active * 0.14f);
    bg = lerp_color(bg, button_active, motion.active * 0.35f);
    Color border = lerp_color(border_base, focus, 0.12f + motion.hover * 0.14f + motion.active * 0.22f);
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(bg), maybe_disabled(border), motion.hover, motion.active, focused ? 1.0f : 0.0f);
    Rect text_r = offset_rect(r, 0.0f, motion.active * 1.0f);
    draw_text_utf8_centered(vis, text_r, maybe_disabled(lerp_color(text_dim, text, 0.72f + motion.hover * 0.20f + motion.active * 0.08f)));
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, r, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(r, 0, 1, {0,1,0,0.4f});
    return clicked;
}

bool button(const char* label) {
    return button_impl(label, nullptr, nullptr);
}

bool button(const char* label, ColorRole role) {
    return button_impl(label, nullptr, &role);
}

bool button(const char* label, Color color) {
    return button_impl(label, &color, nullptr);
}

bool input(const char* label, char* buffer, int buffer_size,
           InputFlags flags, bool* enter_pressed) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);

    register_focusable(id);

    Rect target_r = {}, target_outer = next_labeled_rect(vis, g_style.item_height, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;

    // Click to focus
    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, true);
        g_text_cursor_id = id;
        int len = (int)strlen(buffer);
        // Place cursor at click position
        float inner_x = r.x + 8.0f;
        g_text_cursor = byte_from_x(buffer, g_input.mouse_x - inner_x);
        g_text_sel_anchor = g_text_cursor;
    }

    bool focused = is_widget_focused(id);
    bool changed = false;

    if (focused) {
        g_ctx.focused_input_id = id;
        if (g_text_cursor_id != id) {
            g_text_cursor_id = id;
            g_text_cursor = (int)strlen(buffer);
            g_text_sel_anchor = g_text_cursor;
        }
        int len = (int)strlen(buffer);
        if (g_text_cursor > len) g_text_cursor = len;
        if (g_text_sel_anchor > len) g_text_sel_anchor = len;

        bool ro = (flags & InputFlags::ReadOnly);

        // Ctrl+A: select all
        if (g_input.ctrl_held && g_input.text_input_count > 0 && g_input.text_input[0] == 1) {
            g_text_sel_anchor = 0; g_text_cursor = len;
        }

        // Arrow keys
        if (g_input.key_left) {
            if (g_input.shift_held) {
                g_text_cursor = utf8_retreat(buffer, g_text_cursor);
            } else if (g_text_cursor != g_text_sel_anchor) {
                g_text_cursor = g_text_sel_anchor < g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                g_text_sel_anchor = g_text_cursor;
            } else {
                g_text_cursor = utf8_retreat(buffer, g_text_cursor);
                g_text_sel_anchor = g_text_cursor;
            }
        }
        if (g_input.key_right) {
            if (g_input.shift_held) {
                g_text_cursor = utf8_advance(buffer, g_text_cursor);
            } else if (g_text_cursor != g_text_sel_anchor) {
                g_text_cursor = g_text_sel_anchor > g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                g_text_sel_anchor = g_text_cursor;
            } else {
                g_text_cursor = utf8_advance(buffer, g_text_cursor);
                g_text_sel_anchor = g_text_cursor;
            }
        }

        // Ctrl+C
        if (g_input.key_ctrl_c && g_text_sel_anchor != g_text_cursor) {
            int lo = g_text_sel_anchor < g_text_cursor ? g_text_sel_anchor : g_text_cursor;
            int hi = g_text_sel_anchor > g_text_cursor ? g_text_sel_anchor : g_text_cursor;
            std::string sel(buffer + lo, buffer + hi);
            clipboard_set(sel.c_str());
        }

        // Ctrl+V
        if (g_input.key_ctrl_v && !ro) {
            std::string cb = clipboard_get();
            if (!cb.empty()) {
                int lo = g_text_sel_anchor < g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                int hi = g_text_sel_anchor > g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                if (lo != hi) { memmove(buffer+lo, buffer+hi, len-hi+1); len -= (hi-lo); g_text_cursor = lo; }
                int ins = (int)cb.size();
                if (len + ins < buffer_size - 1) {
                    memmove(buffer+g_text_cursor+ins, buffer+g_text_cursor, len-g_text_cursor+1);
                    memcpy(buffer+g_text_cursor, cb.c_str(), ins);
                    g_text_cursor += ins;
                }
                g_text_sel_anchor = g_text_cursor;
                changed = true;
            }
        }

        // Backspace
        if (g_input.key_backspace && !ro) {
            if (g_text_cursor != g_text_sel_anchor) {
                int lo = g_text_sel_anchor < g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                int hi = g_text_sel_anchor > g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                memmove(buffer+lo, buffer+hi, len-hi+1);
                g_text_cursor = g_text_sel_anchor = lo;
                changed = true;
            } else if (g_text_cursor > 0) {
                int prev = utf8_retreat(buffer, g_text_cursor);
                memmove(buffer+prev, buffer+g_text_cursor, len-g_text_cursor+1);
                g_text_cursor = g_text_sel_anchor = prev;
                changed = true;
            }
        }

        // Printable input
        if (!ro && g_input.text_input_count > 0) {
            // Delete selection first
            if (g_text_cursor != g_text_sel_anchor) {
                int lo = g_text_sel_anchor < g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                int hi = g_text_sel_anchor > g_text_cursor ? g_text_sel_anchor : g_text_cursor;
                memmove(buffer+lo, buffer+hi, len-hi+1);
                len -= (hi-lo); g_text_cursor = g_text_sel_anchor = lo;
            }
            for (int i = 0; i < g_input.text_input_count && g_input.text_input[i]; ) {
                unsigned char c = (unsigned char)g_input.text_input[i];
                int cs = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
                if ((flags & InputFlags::CharsDecimal) || (flags & InputFlags::CharsHexadecimal) ||
                    (flags & InputFlags::CharsUppercase) || (flags & InputFlags::CharsNoBlank)) {
                    if (cs == 1) {
                        char outc = (char)c;
                        if (!filter_ascii_input_char((char)c, flags, outc)) { i += cs; continue; }
                        if (len + 1 < buffer_size - 1) {
                            memmove(buffer+g_text_cursor+1, buffer+g_text_cursor, len-g_text_cursor+1);
                            buffer[g_text_cursor] = outc;
                            g_text_cursor += 1; len += 1;
                        }
                        i += cs;
                        continue;
                    }
                    if ((flags & InputFlags::CharsDecimal) || (flags & InputFlags::CharsHexadecimal)) { i += cs; continue; }
                }
                if (len + cs < buffer_size - 1) {
                    memmove(buffer+g_text_cursor+cs, buffer+g_text_cursor, len-g_text_cursor+1);
                    memcpy(buffer+g_text_cursor, g_input.text_input+i, cs);
                    g_text_cursor += cs; len += cs;
                }
                i += cs;
            }
            g_text_sel_anchor = g_text_cursor;
            changed = true;
        }

        // Enter
        if (g_input.key_enter) {
            if (enter_pressed) *enter_pressed = true;
        }
    }

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, raw_hov, enabled && hov && g_input.mouse_down, focused);

    // Draw
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text = resolve_color(ColorRole::Text);
    Color panel_col = lerp_color(input_bg, panel, motion.hover * 0.18f + motion.focus * 0.08f);
    Color border_col = lerp_color(border, focus, motion.focus);
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(panel_col), maybe_disabled(border_col), motion.hover, motion.active, motion.focus);

    float pad = 8.0f;
    float inner_w = r.w - pad * 2.0f - 4.0f;
    if (inner_w < 4.0f) inner_w = 4.0f;
    Rect inner = {r.x + pad, r.y, inner_w, r.h};

    // Build display string
    int len = (int)strlen(buffer);
    std::string disp;
    if (flags & InputFlags::Password) disp = std::string(utf8_char_count(buffer, len), '*');
    else disp = buffer;

    // Scroll so cursor is visible
    static float s_scroll[256] = {}; // per-hash scroll offset (limited slots)
    int slot = (id ^ (id >> 8)) & 0xFF;
    float& scroll_x = s_scroll[slot];
    if (focused) {
        float cx = measure_text_at(disp.c_str(), g_text_cursor);
        float vis_w = inner.w;
        if (cx - scroll_x > vis_w - 4) scroll_x = cx - vis_w + 4;
        if (cx - scroll_x < 0)         scroll_x = cx;
        if (scroll_x < 0) scroll_x = 0;
    }

    push_clip(inner);

    // Selection highlight
    if (focused && g_text_sel_anchor != g_text_cursor) {
        int lo = g_text_sel_anchor < g_text_cursor ? g_text_sel_anchor : g_text_cursor;
        int hi = g_text_sel_anchor > g_text_cursor ? g_text_sel_anchor : g_text_cursor;
        float sx = inner.x - scroll_x + measure_text_at(disp.c_str(), lo);
        float ex = inner.x - scroll_x + measure_text_at(disp.c_str(), hi);
        Rect sel_r = {sx, r.y + 4, ex - sx, r.h - 8};
        Color sel_col = focus; sel_col.a = 0.35f;
        fill_rect(sel_r, sel_col);
    }

    Rect text_r = {inner.x - scroll_x, inner.y, inner.w + scroll_x, inner.h};
    draw_text_utf8(disp.c_str(), text_r, maybe_disabled(text));

    // Cursor blink
    if (focused && (g_ctx.frame_index / 30) % 2 == 0) {
        float cx = inner.x - scroll_x + measure_text_at(disp.c_str(), g_text_cursor);
        draw_line(cx, r.y + 5, cx, r.y + r.h - 5, 1.5f, maybe_disabled(text));
    }

    pop_clip();

    draw_widget_label(vis, outer, focused);
    if (cfx.clip_active) pop_clip();
    mark_last_item(id, outer, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0,0,1,0.4f});
    return changed;
}

bool text_area_ex(const char* label, char* buffer, int buffer_size, int rows, TextAreaFlags flags) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    bool ro = (flags & TextAreaFlags::ReadOnly);
    bool wrap = (flags & TextAreaFlags::WordWrap);

    register_focusable(id);
    ScrollSlot& tslot = scroll_slot_for(id ^ 0x54A54A);
    float& ta_scroll_y = tslot.current;
    float& ta_scroll_target_y = tslot.target;

    float lh = text_line_height();
    float widget_h = rows * lh + 8.0f;
    Rect target_r = {}, target_outer = next_labeled_rect(vis, widget_h, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;

    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, true);
        g_ta_cursor_id = id;
    }

    bool focused = is_widget_focused(id);
    bool changed = false;

    float pad_x = 6.0f, pad_y = 4.0f;
    float text_w = r.w - pad_x * 2.0f - 8.0f;

    auto rebuild_lines = [&](std::vector<TextRange>& lines) {
        compute_wrapped_ranges(buffer, text_w, wrap, lines);
        if (lines.empty()) lines.push_back({0, 0});
    };
    auto line_index_for = [&](const std::vector<TextRange>& lines, int pos) {
        if (lines.empty()) return 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (pos >= lines[i].start && pos <= lines[i].end) return (int)i;
        }
        return (int)lines.size() - 1;
    };

    std::vector<TextRange> lines;
    rebuild_lines(lines);

    if (focused) {
        g_ctx.focused_input_id = id;
        int len = (int)strlen(buffer);
        if (g_ta_cursor_id != id) {
            g_ta_cursor_id = id;
            g_ta_cursor = len;
            g_ta_sel_anchor = g_ta_cursor;
            ta_scroll_y = 0;
            ta_scroll_target_y = 0;
        }
        if (g_ta_cursor > len) g_ta_cursor = len;
        if (g_ta_sel_anchor > len) g_ta_sel_anchor = len;

        if (g_input.ctrl_held && g_input.text_input_count > 0 && g_input.text_input[0] == 1) {
            g_ta_sel_anchor = 0;
            g_ta_cursor = len;
        }

        if (g_input.key_ctrl_c && g_ta_sel_anchor != g_ta_cursor) {
            int lo = g_ta_sel_anchor < g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
            int hi = g_ta_sel_anchor > g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
            std::string sel(buffer + lo, buffer + hi);
            clipboard_set(sel.c_str());
        }

        auto delete_selection = [&]() {
            if (g_ta_cursor == g_ta_sel_anchor) return;
            int lo = g_ta_sel_anchor < g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
            int hi = g_ta_sel_anchor > g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
            memmove(buffer + lo, buffer + hi, len - hi + 1);
            len -= (hi - lo);
            g_ta_cursor = g_ta_sel_anchor = lo;
            changed = true;
        };

        if (!ro && g_input.key_ctrl_v) {
            std::string cb = clipboard_get();
            if (!cb.empty()) {
                delete_selection();
                int ins = (int)cb.size();
                if (len + ins < buffer_size - 1) {
                    memmove(buffer + g_ta_cursor + ins, buffer + g_ta_cursor, len - g_ta_cursor + 1);
                    memcpy(buffer + g_ta_cursor, cb.c_str(), ins);
                    g_ta_cursor += ins;
                    len += ins;
                    g_ta_sel_anchor = g_ta_cursor;
                    changed = true;
                }
            }
        }

        if (!ro && g_input.key_backspace) {
            if (g_ta_cursor != g_ta_sel_anchor) {
                delete_selection();
            } else if (g_ta_cursor > 0) {
                int prev = utf8_retreat(buffer, g_ta_cursor);
                int removed = g_ta_cursor - prev;
                memmove(buffer + prev, buffer + g_ta_cursor, len - g_ta_cursor + 1);
                g_ta_cursor = g_ta_sel_anchor = prev;
                len -= removed;
                changed = true;
            }
        }

        if (!ro && g_input.key_enter) {
            delete_selection();
            if (len + 1 < buffer_size - 1) {
                memmove(buffer + g_ta_cursor + 1, buffer + g_ta_cursor, len - g_ta_cursor + 1);
                buffer[g_ta_cursor] = '\n';
                g_ta_cursor++;
                g_ta_sel_anchor = g_ta_cursor;
                len += 1;
                changed = true;
            }
        }

        if (g_input.key_left) {
            if (g_input.shift_held) {
                g_ta_cursor = utf8_retreat(buffer, g_ta_cursor);
            } else if (g_ta_cursor != g_ta_sel_anchor) {
                g_ta_cursor = g_ta_sel_anchor < g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
                g_ta_sel_anchor = g_ta_cursor;
            } else {
                g_ta_cursor = utf8_retreat(buffer, g_ta_cursor);
                g_ta_sel_anchor = g_ta_cursor;
            }
        }
        if (g_input.key_right) {
            if (g_input.shift_held) {
                g_ta_cursor = utf8_advance(buffer, g_ta_cursor);
            } else if (g_ta_cursor != g_ta_sel_anchor) {
                g_ta_cursor = g_ta_sel_anchor > g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
                g_ta_sel_anchor = g_ta_cursor;
            } else {
                g_ta_cursor = utf8_advance(buffer, g_ta_cursor);
                g_ta_sel_anchor = g_ta_cursor;
            }
        }

        rebuild_lines(lines);

        if (g_input.key_up || g_input.key_down) {
            int cur_line = line_index_for(lines, g_ta_cursor);
            int next_line = g_input.key_up ? cur_line - 1 : cur_line + 1;
            if (next_line >= 0 && next_line < (int)lines.size()) {
                TextRange cur = lines[cur_line];
                TextRange dst = lines[next_line];
                float col_x = measure_text_at(buffer + cur.start, g_ta_cursor - cur.start);
                int new_pos = dst.start + byte_from_x(buffer + dst.start, col_x);
                if (new_pos > dst.end) new_pos = dst.end;
                g_ta_cursor = new_pos;
                if (!g_input.shift_held) g_ta_sel_anchor = g_ta_cursor;
            }
        }

        if (hov && g_input.mouse_pressed) {
            int line_idx = (int)((g_input.mouse_y - (r.y + pad_y) + ta_scroll_y) / lh);
            if (line_idx < 0) line_idx = 0;
            if (line_idx >= (int)lines.size()) line_idx = (int)lines.size() - 1;
            TextRange tr = lines.empty() ? TextRange{} : lines[line_idx];
            int clicked = tr.start + byte_from_x(buffer + tr.start, g_input.mouse_x - (r.x + pad_x));
            if (clicked > tr.end) clicked = tr.end;
            g_ta_cursor = clicked;
            g_ta_sel_anchor = clicked;
        }

        if (!ro && g_input.text_input_count > 0) {
            delete_selection();
            for (int i = 0; i < g_input.text_input_count && g_input.text_input[i]; ) {
                unsigned char c = (unsigned char)g_input.text_input[i];
                int cs = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
                if (len + cs < buffer_size - 1) {
                    memmove(buffer + g_ta_cursor + cs, buffer + g_ta_cursor, len - g_ta_cursor + 1);
                    memcpy(buffer + g_ta_cursor, g_input.text_input + i, cs);
                    g_ta_cursor += cs;
                    len += cs;
                    changed = true;
                }
                i += cs;
            }
            g_ta_sel_anchor = g_ta_cursor;
        }

        rebuild_lines(lines);
        float visible_h = r.h - 8.0f;
        if (visible_h < lh) visible_h = lh;
        int cur_line = line_index_for(lines, g_ta_cursor);
        float cursor_y_in_widget = (float)cur_line * lh;
        if (cursor_y_in_widget - ta_scroll_target_y < 0.0f) ta_scroll_target_y = cursor_y_in_widget;
        if (cursor_y_in_widget + lh - ta_scroll_target_y > visible_h) ta_scroll_target_y = cursor_y_in_widget + lh - visible_h;
        if (ta_scroll_target_y < 0.0f) ta_scroll_target_y = 0.0f;
    }

    if (hov && g_input.wheel_y != 0.0f) {
        ta_scroll_target_y -= g_input.wheel_y;
        g_input.wheel_y = 0.0f;
    }

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, raw_hov, enabled && hov && g_input.mouse_down, focused);
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color border_col = lerp_color(border, focus, motion.focus);
    Color panel_col = lerp_color(input_bg, panel, motion.hover * 0.18f + motion.focus * 0.08f);
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(panel_col), maybe_disabled(border_col), motion.hover, motion.active, motion.focus);

    Rect clip_r = {r.x + 2, r.y + 2, r.w - 4, r.h - 4};
    if (clip_r.w < 0.0f) clip_r.w = 0.0f;
    if (clip_r.h < 0.0f) clip_r.h = 0.0f;
    push_clip(clip_r);

    rebuild_lines(lines);
    float total_h = (float)lines.size() * lh;
    float ta_visible_h = r.h - 8.0f;
    if (ta_visible_h < lh) ta_visible_h = lh;
    float ta_max_scroll = total_h > ta_visible_h ? (total_h - ta_visible_h) : 0.0f;
    if ((flags & TextAreaFlags::AutoScrollBottom) && !focused) ta_scroll_target_y = ta_max_scroll;
    ta_scroll_target_y = clampf(ta_scroll_target_y, 0.0f, ta_max_scroll);
    if (effects_enabled()) ta_scroll_y = step_anim(ta_scroll_y, ta_scroll_target_y, g_dt, 18.0f);
    else ta_scroll_y = ta_scroll_target_y;
    ta_scroll_y = clampf(ta_scroll_y, 0.0f, ta_max_scroll);

    int sel_lo = g_ta_sel_anchor < g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
    int sel_hi = g_ta_sel_anchor > g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;

    for (size_t li = 0; li < lines.size(); ++li) {
        TextRange tr = lines[li];
        float line_y = r.y + pad_y - ta_scroll_y + (float)li * lh;
        if (line_y + lh < r.y || line_y > r.y + r.h) continue;

        if (focused && sel_lo != sel_hi) {
            int line_sel_lo = sel_lo < tr.start ? tr.start : sel_lo;
            int line_sel_hi = sel_hi > tr.end ? tr.end : sel_hi;
            if (line_sel_lo < line_sel_hi) {
                float sx = r.x + pad_x + measure_text_at(buffer + tr.start, line_sel_lo - tr.start);
                float ex = r.x + pad_x + measure_text_at(buffer + tr.start, line_sel_hi - tr.start);
                Rect sel_r = {sx, line_y, ex - sx, lh};
                Color sc = focus; sc.a = 0.35f;
                fill_rect(sel_r, sc);
            }
        }

        if (tr.end > tr.start) {
            std::string line_str(buffer + tr.start, buffer + tr.end);
            Rect lr = {r.x + pad_x, line_y, text_w, lh};
            draw_text_utf8(line_str.c_str(), lr, maybe_disabled(text));
        }

        if (focused && g_ta_cursor >= tr.start && g_ta_cursor <= tr.end && (g_ctx.frame_index / 30) % 2 == 0) {
            float cx = r.x + pad_x + measure_text_at(buffer + tr.start, g_ta_cursor - tr.start);
            draw_line(cx, line_y + 2, cx, line_y + lh - 2, 1.5f, maybe_disabled(text));
        }
    }

    pop_clip();

    if (total_h > r.h - 8.0f) {
        float sb_w = 6.0f, sb_x = r.x + r.w - sb_w - 2.0f;
        float visible_h = r.h - 8.0f;
        float thumb_h = (visible_h / total_h) * visible_h;
        if (thumb_h < 12.0f) thumb_h = 12.0f;
        float thumb_y = r.y + 4.0f + (ta_scroll_y / (total_h - visible_h)) * (visible_h - thumb_h);
        Rect track_r = {sb_x, r.y + 4.0f, sb_w, visible_h};
        Rect thumb_r = {sb_x, thumb_y, sb_w, thumb_h};
        Color track_c = border; track_c.a = 0.3f;
        fill_round_rect(track_r, sb_w * 0.5f, maybe_disabled(track_c));
        fill_round_rect(thumb_r, sb_w * 0.5f, maybe_disabled(text_dim));
    }

    draw_widget_label(vis, outer, focused);
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, outer, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0,0,1,0.4f});
    return changed;
}

bool text_area(const char* label, char* buffer, int buffer_size, int rows) {
    return text_area_ex(label, buffer, buffer_size, rows, TextAreaFlags::Default);
}

void log_view(const char* label, const char* text, int rows, LogViewFlags flags) {
    if (!g_drawing) return;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);
    ScrollSlot& slot = scroll_slot_for(id ^ 0x6F6A5601);

    float lh = text_line_height();
    float widget_h = rows * lh + 8.0f;
    Rect target_r = {}, target_outer = next_labeled_rect(vis, widget_h, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;
    bool focused = is_widget_focused(id);

    const char* src = text ? text : "";
    int len = (int)strlen(src);

    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, false);
        focused = true;
        g_ta_cursor_id = id;
    }
    if (focused && g_ta_cursor_id != id) {
        g_ta_cursor_id = id;
        g_ta_cursor = len;
        g_ta_sel_anchor = len;
    }

    float pad_x = 6.0f, pad_y = 4.0f;
    float text_w = r.w - pad_x * 2.0f - 8.0f;
    std::vector<TextRange> lines;
    compute_wrapped_ranges(src, text_w, (flags & LogViewFlags::WordWrap), lines);

    float visible_h = r.h - 8.0f;
    if (visible_h < lh) visible_h = lh;
    float total_h = (float)lines.size() * lh;
    float max_scroll = total_h > visible_h ? (total_h - visible_h) : 0.0f;
    if ((flags & LogViewFlags::AutoScrollBottom) && !focused) slot.target = max_scroll;
    if (effects_enabled()) slot.current = step_anim(slot.current, slot.target, g_dt, 18.0f);
    else slot.current = slot.target;
    slot.current = clampf(slot.current, 0.0f, max_scroll);
    slot.target = clampf(slot.target, 0.0f, max_scroll);

    if (hov && g_input.wheel_y != 0.0f) {
        slot.target -= g_input.wheel_y;
        slot.target = clampf(slot.target, 0.0f, max_scroll);
        g_input.wheel_y = 0.0f;
    }

    if (focused) {
        if (g_input.ctrl_held && g_input.text_input_count > 0 && g_input.text_input[0] == 1) {
            g_ta_sel_anchor = 0;
            g_ta_cursor = len;
        }
        if (g_input.key_ctrl_c && g_ta_sel_anchor != g_ta_cursor) {
            int lo = g_ta_sel_anchor < g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
            int hi = g_ta_sel_anchor > g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
            std::string sel(src + lo, src + hi);
            clipboard_set(sel.c_str());
        }
        if (hov && g_input.mouse_pressed) {
            int line_idx = (int)((g_input.mouse_y - (r.y + pad_y) + slot.current) / lh);
            line_idx = line_idx < 0 ? 0 : (line_idx >= (int)lines.size() ? (int)lines.size() - 1 : line_idx);
            TextRange tr = lines.empty() ? TextRange{} : lines[line_idx];
            int clicked = tr.start + byte_from_x(src + tr.start, g_input.mouse_x - (r.x + pad_x));
            if (clicked > tr.end) clicked = tr.end;
            g_ta_cursor = clicked;
            g_ta_sel_anchor = clicked;
        }
    }

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, raw_hov, false, focused);
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text_col = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color border_col = lerp_color(border, focus, focused ? 0.7f : 0.0f);
    Color panel_col = lerp_color(input_bg, panel, motion.hover * 0.18f + (focused ? 0.08f : 0.0f));
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(panel_col), maybe_disabled(border_col), motion.hover, 0.0f, focused ? 1.0f : 0.0f);

    Rect clip_r = {r.x + 2, r.y + 2, r.w - 4, r.h - 4};
    if (clip_r.w < 0.0f) clip_r.w = 0.0f;
    if (clip_r.h < 0.0f) clip_r.h = 0.0f;
    push_clip(clip_r);

    int sel_lo = g_ta_sel_anchor < g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;
    int sel_hi = g_ta_sel_anchor > g_ta_cursor ? g_ta_sel_anchor : g_ta_cursor;

    for (size_t i = 0; i < lines.size(); ++i) {
        float line_y = r.y + pad_y - slot.current + (float)i * lh;
        if (line_y + lh < r.y || line_y > r.y + r.h) continue;

        TextRange tr = lines[i];
        if (focused && sel_lo != sel_hi) {
            int line_sel_lo = sel_lo < tr.start ? tr.start : sel_lo;
            int line_sel_hi = sel_hi > tr.end ? tr.end : sel_hi;
            if (line_sel_lo < line_sel_hi) {
                float sx = r.x + pad_x + measure_text_at(src + tr.start, line_sel_lo - tr.start);
                float ex = r.x + pad_x + measure_text_at(src + tr.start, line_sel_hi - tr.start);
                Rect sel_r = {sx, line_y, ex - sx, lh};
                Color sc = focus; sc.a = 0.30f;
                fill_rect(sel_r, sc);
            }
        }

        std::string line(src + tr.start, src + tr.end);
        Rect lr = {r.x + pad_x, line_y, text_w, lh};
        draw_text_utf8(line.c_str(), lr, maybe_disabled(text_col));
    }
    pop_clip();

    if (total_h > visible_h) {
        float sb_w = 6.0f, sb_x = r.x + r.w - sb_w - 2;
        float thumb_h = (visible_h / total_h) * visible_h;
        if (thumb_h < 12) thumb_h = 12;
        float thumb_y = r.y + 4 + (slot.current / (total_h - visible_h)) * (visible_h - thumb_h);
        Rect track_r = {sb_x, r.y + 4, sb_w, visible_h};
        Rect thumb_r = {sb_x, thumb_y, sb_w, thumb_h};
        Color track_c = border; track_c.a = 0.3f;
        fill_round_rect(track_r, sb_w * 0.5f, maybe_disabled(track_c));
        fill_round_rect(thumb_r, sb_w * 0.5f, maybe_disabled(text_dim));
    }

    draw_widget_label(vis, outer, focused);
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, outer, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0.3f,0.7f,1,0.4f});
}

bool checkbox(const char* label, bool* value) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);

    Rect r = next_rect(g_style.item_height);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    bool raw_hov = rect_contains(r, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;
    bool focused = is_widget_focused(id);
    bool clicked = false;

    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, false);
        g_ctx.active_id = id;
    }
    if (focused && enabled && (g_input.key_space || g_input.key_enter)) {
        *value = !*value;
        clicked = true;
    }
    if (g_ctx.active_id == id && g_input.mouse_released) {
        if (hov) {
            *value = !*value;
            clicked = true;
            set_widget_focus(id, false);
        }
        g_ctx.active_id = 0;
    }

    float box_sz = g_style.item_height - 10.0f;
    Rect box = {r.x, r.y + (r.h - box_sz) * 0.5f, box_sz, box_sz};
    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, hov, enabled && g_ctx.active_id == id, focused || *value);
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color border_base = resolve_color(ColorRole::Border);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color bg = lerp_color(input_bg, panel, 0.14f);
    bg = lerp_color(bg, button_hover, motion.hover * 0.28f);
    bg = lerp_color(bg, focus, *value ? 0.14f : 0.04f);
    Color border = lerp_color(border_base, focus, focused ? 0.85f : (*value ? 0.35f : 0.0f));
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(box, fmaxf(2.0f, g_style.rounding * 0.35f),
                       maybe_disabled(bg), maybe_disabled(border),
                       motion.hover, motion.active, focused ? 1.0f : 0.0f);

    if (*value) {
        float inset = 5.0f;
        Rect mark = {box.x + inset, box.y + inset, box.w - inset * 2.0f, box.h - inset * 2.0f};
        Color mark_fill = focus;
        mark_fill.a = 0.82f;
        fill_rect(mark, maybe_disabled(mark_fill));
    }

    Rect lbl_r = {box.x + box_sz + 8.0f, r.y, r.w - box_sz - 8.0f, r.h};
    draw_text_utf8(vis, lbl_r, maybe_disabled(lerp_color(text_dim, text, 0.52f + motion.hover * 0.42f)));
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, r, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(r, 0, 1, {1,0,1,0.4f});
    return clicked;
}

bool slider_float(const char* label, float* value, float min_v, float max_v) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);

    if (max_v < min_v) { float tmp = min_v; min_v = max_v; max_v = tmp; }
    Rect target_r = {}, target_outer = next_labeled_rect(vis, g_style.item_height, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    float value_w = 80.0f;
    Rect track_r = {r.x, r.y + r.h * 0.5f - 3.0f, r.w - value_w, 6.0f};
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;
    bool focused = is_widget_focused(id);
    bool changed = false;

    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, false);
        g_ctx.active_id = id;
    }
    if (g_ctx.active_id == id) {
        if (g_input.mouse_released) g_ctx.active_id = 0;
        else if (enabled) {
            float t = (g_input.mouse_x - track_r.x) / track_r.w;
            t = clampf(t, 0.0f, 1.0f);
            float nv = min_v + t * (max_v - min_v);
            if (nv != *value) { *value = nv; changed = true; }
        }
    }

    if (focused && enabled && max_v > min_v) {
        float step = (max_v - min_v) / 100.0f;
        if (step <= 0.0f) step = (max_v - min_v) / 20.0f;
        if (step <= 0.0f) step = 1.0f;
        if (g_input.key_left) {
            float nv = *value - step;
            if (nv < min_v) nv = min_v;
            if (nv != *value) { *value = nv; changed = true; }
        }
        if (g_input.key_right) {
            float nv = *value + step;
            if (nv > max_v) nv = max_v;
            if (nv != *value) { *value = nv; changed = true; }
        }
    }

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, hov, enabled && g_ctx.active_id == id, focused);

    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color button = resolve_color(ColorRole::Button);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color button_active = resolve_color(ColorRole::ButtonActive);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color track_bg = lerp_color(input_bg, panel, motion.hover * 0.18f);
    if (cfx.clip_active) push_clip(cfx.clip);
    fill_round_rect(track_r, 3.0f, maybe_disabled(track_bg));
    stroke_round_rect(track_r, 3.0f, g_style.border_width,
                      maybe_disabled(lerp_color(border, focus, focused ? 0.65f : motion.active * 0.25f)));

    float t = max_v > min_v ? ((*value - min_v) / (max_v - min_v)) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    Rect fill_r = {track_r.x, track_r.y, track_r.w * t, track_r.h};
    if (fill_r.w > 0.0f) fill_round_rect(fill_r, 3.0f, maybe_disabled(with_alpha(focus, 0.78f + motion.active * 0.18f)));

    float knob_sz = 14.0f;
    Rect knob = {track_r.x + track_r.w * t - knob_sz * 0.5f, r.y + r.h * 0.5f - knob_sz * 0.5f, knob_sz, knob_sz};
    Color knob_col = lerp_color(button, button_hover, motion.hover);
    knob_col = lerp_color(knob_col, button_active, motion.active);
    draw_widget_chrome(knob, knob_sz * 0.5f, maybe_disabled(knob_col), maybe_disabled(focus),
                       motion.hover, motion.active, focused ? 1.0f : 0.0f);

    char val_str[48];
    snprintf(val_str, sizeof(val_str), "%.2f", *value);
    Rect val_r = {r.x + r.w - value_w + 8.0f, r.y, value_w - 8.0f, r.h};
    draw_text_utf8(val_str, val_r, maybe_disabled(lerp_color(text_dim, text, motion.hover * 0.30f + (focused ? 0.35f : 0.0f))));
    draw_widget_label(vis, outer, focused);
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, outer, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {1,1,0,0.4f});
    return changed;
}

void image(ImageHandle* img, float width, float height) {
    if (!g_drawing) return;
    Rect r = next_rect(height);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    if (width < r.w) r.w = width;
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_image_handle(img, r);
    if (cfx.clip_active) pop_clip();
    mark_last_item(0, r, false, false);
    if (g_debug.show_layout_rects) stroke_round_rect(r, 0, 1, {0,1,1,0.4f});
}

bool tabs(const char* const* labels, int count, int* selected) {
    if (!g_drawing || count <= 0) return false;
    WidgetColorScope color_scope;
    float tab_h = g_style.item_height;
    Rect r = next_rect(tab_h);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    float tab_w = r.w / count;
    bool changed = false;
    size_t ptr_bits = (size_t)(const void*)selected;
    int bar_id = (int)(((ptr_bits >> 4) ^ (ptr_bits >> 9) ^ ((size_t)count * 2654435761u)) & 0x7fffffff);
    if (bar_id == 0) bar_id = count ? count : 1;
    TabFxSlot& fx = tab_fx_slot_for(bar_id);
    if (!fx.initialized) {
        fx.initialized = true;
        fx.selected = *selected;
        fx.previous = *selected;
        fx.switch_t = 1.0f;
    }

    int current_selected = *selected;
    Rect selected_rect = {};
    bool have_selected_rect = false;
    bool enabled = widget_interaction_enabled();
    std::vector<int> ids(count);
    int focused_idx = -1;
    Rect last_rect = r;
    int last_id = bar_id;
    bool last_hov = false;
    bool last_focus = false;

    for (int i = 0; i < count; ++i) {
        char vis[128]; const char* hs;
        split_label(labels[i], vis, sizeof(vis), &hs);
        ids[i] = hash_str(hs) ^ (i * 31337);
        register_focusable(ids[i]);
        if (is_widget_focused(ids[i])) focused_idx = i;
    }

    if (focused_idx >= 0 && enabled) {
        int next_idx = focused_idx;
        if (g_input.key_left) next_idx = focused_idx > 0 ? focused_idx - 1 : 0;
        if (g_input.key_right) next_idx = focused_idx + 1 < count ? focused_idx + 1 : count - 1;
        if (next_idx != focused_idx) {
            current_selected = next_idx;
            set_widget_focus(ids[next_idx], false);
            changed = true;
        } else if ((g_input.key_space || g_input.key_enter) && current_selected != focused_idx) {
            current_selected = focused_idx;
            changed = true;
        }
    }

    if (cfx.clip_active) push_clip(cfx.clip);
    Color button = resolve_color(ColorRole::Button);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color button_active = resolve_color(ColorRole::ButtonActive);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    for (int i = 0; i < count; i++) {
        char vis[128]; const char* hs;
        split_label(labels[i], vis, sizeof(vis), &hs);
        int id = ids[i];

        Rect tr = {r.x + i * tab_w, r.y, tab_w, tab_h};
        bool raw_hov = rect_contains(tr, g_input.mouse_x, g_input.mouse_y);
        bool hov = raw_hov && enabled;

        if (hov && g_input.mouse_pressed) {
            set_widget_focus(id, false);
            g_ctx.active_id = id;
        }
        if (g_ctx.active_id == id && g_input.mouse_released) {
            if (hov && current_selected != i) { current_selected = i; changed = true; }
            g_ctx.active_id = 0;
        }

        bool focused = is_widget_focused(id);
        bool sel = (current_selected == i);
        if (sel) { selected_rect = tr; have_selected_rect = true; }
        if (raw_hov || focused) {
            last_rect = tr;
            last_id = id;
            last_hov = raw_hov;
            last_focus = focused;
        }

        MotionSlot& motion = motion_slot_for(id);
        update_motion_slot(motion, hov, enabled && g_ctx.active_id == id, focused || sel);

        Color bg = lerp_color(button, button_hover, motion.hover * 0.45f);
        bg = lerp_color(bg, panel, focused ? 0.18f : 0.0f);
        bg = lerp_color(bg, button_active, sel ? 0.22f : motion.active * 0.75f);
        Color border_col = lerp_color(border, focus, (focused ? 0.55f : 0.0f) + (sel ? 0.18f : 0.0f));
        draw_widget_chrome(tr, g_style.rounding, maybe_disabled(bg), maybe_disabled(border_col), motion.hover, motion.active, focused ? 1.0f : 0.0f);
        draw_text_utf8_centered(vis, tr, maybe_disabled(lerp_color(text_dim, text, clamp01((sel ? 0.55f : 0.0f) + motion.hover * 0.35f + (focused ? 0.35f : 0.0f)))));
    }

    *selected = current_selected;

    if (current_selected != fx.selected) {
#if FTUI_WINDOWS_EFFECTS
        if (effects_enabled()) capture_frame_snapshot();
#endif
        fx.previous = fx.selected;
        fx.selected = current_selected;
        fx.dir = fx.selected >= fx.previous ? 1 : -1;
        fx.switch_t = 0.0f;
        if (effects_enabled() && (g_tab_content_owner == 0 || g_tab_content_owner == bar_id)) g_tab_content_owner = bar_id;
    }

    if (effects_enabled()) {
        if (fx.switch_t < 1.0f) {
            fx.switch_t += g_dt / 0.18f;
            if (fx.switch_t > 1.0f) fx.switch_t = 1.0f;
        }
    } else {
        fx.switch_t = 1.0f;
    }

    if (have_selected_rect) {
        float target_x = selected_rect.x + 4.0f;
        float target_w = fmaxf(18.0f, selected_rect.w - 8.0f);
        if (fx.underline_w <= 0.0f) {
            fx.underline_x = target_x;
            fx.underline_w = target_w;
        } else if (effects_enabled()) {
            fx.underline_x = step_anim(fx.underline_x, target_x, g_dt, 20.0f);
            fx.underline_w = step_anim(fx.underline_w, target_w, g_dt, 20.0f);
        } else {
            fx.underline_x = target_x;
            fx.underline_w = target_w;
        }
        Rect accent = {fx.underline_x, r.y + r.h - 3, fx.underline_w, 3};
        fill_round_rect(accent, 1.5f, with_alpha(focus, 0.78f));
    }

    if (effects_enabled() && g_tab_content_owner == bar_id && fx.switch_t < 1.0f && !g_frame_content_fx.active) {
        float p = ease_smooth(fx.switch_t);
        g_frame_content_fx.active = true;
        g_frame_content_fx.owner = bar_id;
        g_frame_content_fx.start_y = g_ctx.cursor_y;
        g_frame_content_fx.progress = fx.switch_t;
        g_frame_content_fx.dir = fx.dir;
        g_draw_fx_off_x = (float)fx.dir * (1.0f - p) * 18.0f;
        g_draw_fx_opacity = 1.0f;
    } else if (g_tab_content_owner == bar_id && fx.switch_t >= 1.0f) {
        g_tab_content_owner = 0;
    }
    if (cfx.clip_active) pop_clip();

    mark_last_item(last_id, last_rect, last_hov, last_focus);
    if (g_debug.show_layout_rects) stroke_round_rect(r, 0, 1, {0.5f,0,1,0.4f});
    return changed;
}

bool dropdown(const char* label, const char* const* items, int count, int* selected, int popup_rows) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);

    if (count < 0) count = 0;
    if (popup_rows <= 0) popup_rows = 8;
    if (selected && count > 0) {
        if (*selected < 0) *selected = 0;
        if (*selected >= count) *selected = count - 1;
    }

    Rect target_r = {}, target_outer = next_labeled_rect(vis, g_style.item_height, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    bool open = (g_dropdown_open_id == id);
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool base_enabled = (g_disabled_depth == 0) && (!g_modal_open_id || g_inside_modal);
    bool enabled = base_enabled && (!g_dropdown_capture_input || open);
    bool hov = raw_hov && enabled;
    bool focused = is_widget_focused(id);
    bool changed = false;
    ScrollSlot& slot = scroll_slot_for(id ^ 0x330011);
    CollapseSlot& popup_fx = collapse_slot_for(id ^ 0x330177);

    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, false);
        g_ctx.active_id = id;
    }
    if (g_ctx.active_id == id && g_input.mouse_released) {
        if (hov) {
            bool was_open = open;
#if FTUI_WINDOWS_EFFECTS
            if (effects_enabled() && !was_open) capture_frame_snapshot();
#endif
            g_dropdown_open_id = open ? 0 : id;
            open = !open;
        }
        g_ctx.active_id = 0;
    }
    if (focused && enabled && (g_input.key_space || g_input.key_enter)) {
        bool was_open = open;
#if FTUI_WINDOWS_EFFECTS
        if (effects_enabled() && !was_open) capture_frame_snapshot();
#endif
        g_dropdown_open_id = open ? 0 : id;
        open = !open;
    }
    if (open && g_input.key_escape) {
        g_dropdown_open_id = 0;
        open = false;
    }

    float item_h = g_style.item_height - 4.0f;
    int visible_count = popup_rows < count ? popup_rows : count;
    float full_popup_h = visible_count > 0 ? visible_count * item_h + 8.0f : item_h + 8.0f;
    float space_below = g_ctx.content_region.y + g_ctx.content_region.h - (r.y + r.h) - 4.0f;
    float space_above = r.y - g_ctx.content_region.y - 4.0f;
    bool open_up = full_popup_h > space_below && space_above > space_below;
    float max_popup_h = open_up ? space_above : space_below;
    if (max_popup_h < item_h + 8.0f) max_popup_h = item_h + 8.0f;
    float popup_h = full_popup_h < max_popup_h ? full_popup_h : max_popup_h;

    if (open && focused && count > 0) {
        if (g_input.key_up) {
            int nv = *selected > 0 ? *selected - 1 : 0;
            if (nv != *selected) { *selected = nv; changed = true; }
        }
        if (g_input.key_down) {
            int nv = *selected + 1 < count ? *selected + 1 : count - 1;
            if (nv != *selected) { *selected = nv; changed = true; }
        }
    }

    if (effects_enabled()) popup_fx.progress = step_anim(popup_fx.progress, open ? 1.0f : 0.0f, g_dt, 18.0f);
    else popup_fx.progress = open ? 1.0f : 0.0f;
    float popup_p = ease_smooth(popup_fx.progress);
    bool popup_visible = open || popup_fx.progress > 0.01f;
    float popup_anim_h = popup_h * popup_p;
    Rect popup_r = open_up
        ? Rect{r.x, r.y - 4.0f - popup_anim_h, r.w, popup_anim_h}
        : Rect{r.x, r.y + r.h + 4.0f, r.w, popup_anim_h};
    bool popup_hov = popup_visible && rect_contains(popup_r, g_input.mouse_x, g_input.mouse_y);
    bool consume_mouse = false;

    if (open && g_input.mouse_pressed && !raw_hov && !popup_hov) {
        g_dropdown_open_id = 0;
        open = false;
        consume_mouse = true;
        g_ctx.active_id = 0;
    } else if (open && popup_hov && g_input.mouse_pressed) {
        g_ctx.active_id = 0;
    }

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, hov || popup_hov, enabled && g_ctx.active_id == id, focused);
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color panel_col = lerp_color(input_bg, panel, motion.hover * 0.18f + (open ? 0.10f : 0.0f));
    Color border_col = lerp_color(border, focus, (focused ? 0.65f : 0.0f) + (open ? 0.20f : 0.0f));
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(panel_col), maybe_disabled(border_col),
                       motion.hover, motion.active, focused ? 1.0f : 0.0f);

    const char* current = (count > 0 && selected) ? items[*selected] : "(empty)";
    Rect text_r = {r.x + 10.0f, r.y, r.w - 34.0f, r.h};
    draw_text_utf8(current ? current : "", text_r, maybe_disabled(text));
    Rect arrow_r = {r.x + r.w - 22.0f, r.y + (r.h - 8.0f) * 0.5f, 10.0f, 8.0f};
    fill_triangle(arrow_r, (open && open_up) ? 1 : 0, maybe_disabled(text_dim));
    if (cfx.clip_active) pop_clip();

    if (popup_visible && popup_anim_h > 8.0f) {
        float max_scroll = count * item_h - (popup_h - 8.0f);
        if (max_scroll < 0.0f) max_scroll = 0.0f;
        if (open && popup_hov && g_input.wheel_y != 0.0f) {
            slot.target -= g_input.wheel_y;
            slot.target = clampf(slot.target, 0.0f, max_scroll);
            g_input.wheel_y = 0.0f;
        }
        if (effects_enabled()) slot.current = step_anim(slot.current, slot.target, g_dt, 18.0f);
        else slot.current = slot.target;
        slot.current = clampf(slot.current, 0.0f, max_scroll);

        for (int i = 0; i < count; ++i) {
            Rect ir = {popup_r.x + 4.0f, popup_r.y + 4.0f - slot.current + i * item_h, popup_r.w - 8.0f, item_h};
            if (ir.y + ir.h < popup_r.y || ir.y > popup_r.y + popup_r.h) continue;
            bool item_hov = open && rect_contains(ir, g_input.mouse_x, g_input.mouse_y);
            if (item_hov && g_input.mouse_pressed && enabled) {
                if (selected && *selected != i) changed = true;
                if (selected) *selected = i;
                g_dropdown_open_id = 0;
                open = false;
                consume_mouse = true;
                g_ctx.active_id = 0;
            }
        }
        g_dropdown_overlay.active = true;
        g_dropdown_overlay.open = open;
        g_dropdown_overlay.popup_r = popup_r;
        g_dropdown_overlay.count = count;
        g_dropdown_overlay.selected_index = selected ? *selected : -1;
        g_dropdown_overlay.item_h = item_h;
        g_dropdown_overlay.scroll = slot.current;
        g_dropdown_overlay.popup_h = popup_h;
        g_dropdown_overlay.popup_p = popup_p;
        g_dropdown_overlay.popup_fill = with_alpha(panel, effects_enabled() ? 0.96f : 0.98f);
        g_dropdown_overlay.popup_border = with_alpha(border, effects_enabled() ? 0.92f : 1.0f);
        g_dropdown_overlay.item_hover = with_alpha(button_hover, 0.34f);
        g_dropdown_overlay.item_selected = with_alpha(focus, 0.24f);
        g_dropdown_overlay.item_text = text_dim;
        g_dropdown_overlay.item_text_selected = text;
        Color scrollbar_track = border; scrollbar_track.a = 0.30f;
        g_dropdown_overlay.scrollbar_track = scrollbar_track;
        g_dropdown_overlay.scrollbar_thumb = text_dim;
        g_dropdown_overlay.labels.clear();
        g_dropdown_overlay.labels.reserve((size_t)count);
        for (int i = 0; i < count; ++i) {
            g_dropdown_overlay.labels.emplace_back(items && items[i] ? items[i] : "");
        }
    }

    if ((open || popup_visible) && (popup_hov || raw_hov || g_input.mouse_pressed || g_input.mouse_released || g_input.mouse_down)) {
        consume_mouse = true;
    }
    if (consume_mouse) {
        g_input.mouse_pressed = false;
        g_input.mouse_released = false;
        g_input.mouse_down = false;
    }

    draw_widget_label(vis, outer, focused);
    mark_last_item(id, outer, raw_hov || popup_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0.2f,0.8f,0.8f,0.4f});
    return changed;
}

bool listbox(const char* label, const char* const* items, int count, int* selected, int visible_rows) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);

    if (count < 0) count = 0;
    if (visible_rows <= 0) visible_rows = 6;
    if (selected && count > 0) {
        if (*selected < 0) *selected = 0;
        if (*selected >= count) *selected = count - 1;
    }

    float item_h = g_style.item_height - 4.0f;
    Rect target_r = {}, target_outer = next_labeled_rect(vis, visible_rows * item_h + 8.0f, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;
    bool focused = is_widget_focused(id);
    bool changed = false;
    ScrollSlot& slot = scroll_slot_for(id ^ 0x774411);

    if (hov && g_input.mouse_pressed) set_widget_focus(id, false);
    if (focused && enabled && selected && count > 0) {
        if (g_input.key_up) {
            int nv = *selected > 0 ? *selected - 1 : 0;
            if (nv != *selected) { *selected = nv; changed = true; }
        }
        if (g_input.key_down) {
            int nv = *selected + 1 < count ? *selected + 1 : count - 1;
            if (nv != *selected) { *selected = nv; changed = true; }
        }
    }

    float visible_h = r.h - 8.0f;
    float total_h = count * item_h;
    float max_scroll = total_h > visible_h ? (total_h - visible_h) : 0.0f;
    if (selected && count > 0) {
        float item_top = *selected * item_h;
        float item_bot = item_top + item_h;
        if (item_top < slot.target) slot.target = item_top;
        if (item_bot > slot.target + visible_h) slot.target = item_bot - visible_h;
    }
    slot.target = clampf(slot.target, 0.0f, max_scroll);
    if (hov && g_input.wheel_y != 0.0f) {
        slot.target -= g_input.wheel_y;
        slot.target = clampf(slot.target, 0.0f, max_scroll);
        g_input.wheel_y = 0.0f;
    }
    if (effects_enabled()) slot.current = step_anim(slot.current, slot.target, g_dt, 18.0f);
    else slot.current = slot.target;
    slot.current = clampf(slot.current, 0.0f, max_scroll);

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, hov, false, focused);
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color panel_col = lerp_color(input_bg, panel, motion.hover * 0.18f + (focused ? 0.08f : 0.0f));
    Color border_col = lerp_color(border, focus, focused ? 0.7f : 0.0f);
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(panel_col), maybe_disabled(border_col),
                       motion.hover, 0.0f, focused ? 1.0f : 0.0f);

    Rect clip_r = {r.x + 2.0f, r.y + 2.0f, r.w - 4.0f, r.h - 4.0f};
    if (clip_r.w < 0.0f) clip_r.w = 0.0f;
    if (clip_r.h < 0.0f) clip_r.h = 0.0f;
    push_clip(clip_r);
    for (int i = 0; i < count; ++i) {
        Rect ir = {r.x + 4.0f, r.y + 4.0f - slot.current + i * item_h, r.w - 12.0f, item_h};
        if (ir.y + ir.h < r.y || ir.y > r.y + r.h) continue;
        bool item_hov = rect_contains(ir, g_input.mouse_x, g_input.mouse_y) && enabled;
        if (item_hov && g_input.mouse_pressed) {
            set_widget_focus(id, false);
            if (selected && *selected != i) changed = true;
            if (selected) *selected = i;
        }
        bool item_sel = selected && *selected == i;
        Color item_bg = item_sel ? with_alpha(focus, 0.22f) :
                        item_hov ? with_alpha(button_hover, 0.30f) :
                                   with_alpha(panel, 0.0f);
        if (item_bg.a > 0.0f) fill_round_rect(ir, g_style.rounding * 0.6f, maybe_disabled(item_bg));
        Rect item_text_r = {ir.x + 8.0f, ir.y, ir.w - 16.0f, ir.h};
        draw_text_utf8(items[i] ? items[i] : "", item_text_r, maybe_disabled(item_sel ? text : text_dim));
    }
    if (count == 0) {
        Rect empty_r = {r.x + 8.0f, r.y, r.w - 16.0f, r.h};
        draw_text_utf8("(empty)", empty_r, maybe_disabled(text_dim));
    }
    pop_clip();

    if (total_h > visible_h) {
        float sb_w = 6.0f, sb_x = r.x + r.w - sb_w - 2.0f;
        float thumb_h = (visible_h / total_h) * visible_h;
        if (thumb_h < 12.0f) thumb_h = 12.0f;
        float thumb_y = r.y + 4.0f + (slot.current / (total_h - visible_h)) * (visible_h - thumb_h);
        Rect track_r = {sb_x, r.y + 4.0f, sb_w, visible_h};
        Rect thumb_r = {sb_x, thumb_y, sb_w, thumb_h};
        Color track_c = border; track_c.a = 0.3f;
        fill_round_rect(track_r, sb_w * 0.5f, maybe_disabled(track_c));
        fill_round_rect(thumb_r, sb_w * 0.5f, maybe_disabled(text_dim));
    }

    draw_widget_label(vis, outer, focused);
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, outer, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0.0f,0.9f,0.5f,0.4f});
    return changed;
}

bool radio_group(const char* label, const char* const* items, int count, int* selected, int columns) {
    if (!g_drawing || count <= 0 || !selected) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int base_id = hash_str(hs);
    if (columns <= 0) columns = 1;
    if (columns > count) columns = count;
    if (*selected < 0) *selected = 0;
    if (*selected >= count) *selected = count - 1;

    int rows = (count + columns - 1) / columns;
    float gap = g_style.item_spacing;
    float item_h = g_style.item_height;
    Rect target_r = {}, target_outer = next_labeled_rect(vis, rows * item_h + gap * (rows - 1), &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    float cell_w = (r.w - gap * (columns - 1)) / columns;
    bool enabled = widget_interaction_enabled();
    bool changed = false;
    std::vector<int> ids(count);
    int focused_idx = -1;
    Rect last_rect = r;
    int last_id = base_id;
    bool last_hov = false;
    bool last_focus = false;

    if (cfx.clip_active) push_clip(cfx.clip);
    for (int i = 0; i < count; ++i) {
        ids[i] = base_id ^ (i * 911);
        register_focusable(ids[i]);
        if (is_widget_focused(ids[i])) focused_idx = i;
    }
    if (cfx.clip_active) pop_clip();

    if (focused_idx >= 0 && enabled) {
        int next_idx = focused_idx;
        if (g_input.key_left && (focused_idx % columns) > 0) next_idx = focused_idx - 1;
        if (g_input.key_right && (focused_idx % columns) + 1 < columns && focused_idx + 1 < count) next_idx = focused_idx + 1;
        if (g_input.key_up && focused_idx - columns >= 0) next_idx = focused_idx - columns;
        if (g_input.key_down && focused_idx + columns < count) next_idx = focused_idx + columns;
        if (next_idx != focused_idx) {
            set_widget_focus(ids[next_idx], false);
            if (*selected != next_idx) { *selected = next_idx; changed = true; }
        } else if ((g_input.key_space || g_input.key_enter) && *selected != focused_idx) {
            *selected = focused_idx;
            changed = true;
        }
    }

    Color button = resolve_color(ColorRole::Button);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color button_active = resolve_color(ColorRole::ButtonActive);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    for (int i = 0; i < count; ++i) {
        int row_i = i / columns;
        int col_i = i % columns;
        Rect ir = {r.x + col_i * (cell_w + gap), r.y + row_i * (item_h + gap), cell_w, item_h};
        bool raw_hov = rect_contains(ir, g_input.mouse_x, g_input.mouse_y);
        bool hov = raw_hov && enabled;
        bool focused = is_widget_focused(ids[i]);
        bool sel = (*selected == i);

        if (hov && g_input.mouse_pressed) {
            set_widget_focus(ids[i], false);
            g_ctx.active_id = ids[i];
        }
        if (g_ctx.active_id == ids[i] && g_input.mouse_released) {
            if (hov && !sel) { *selected = i; changed = true; }
            g_ctx.active_id = 0;
        }
        if (raw_hov || focused) {
            last_rect = ir;
            last_id = ids[i];
            last_hov = raw_hov;
            last_focus = focused;
        }

        MotionSlot& motion = motion_slot_for(ids[i]);
        update_motion_slot(motion, hov, enabled && g_ctx.active_id == ids[i], focused || sel);
        Color bg = lerp_color(button, button_hover, motion.hover * 0.42f);
        bg = lerp_color(bg, button_active, sel ? 0.20f : motion.active * 0.65f);
        Color border_col = lerp_color(border, focus, (sel ? 0.28f : 0.0f) + (focused ? 0.55f : 0.0f));
        draw_widget_chrome(ir, g_style.rounding, maybe_disabled(bg), maybe_disabled(border_col), motion.hover, motion.active, focused ? 1.0f : 0.0f);

        float box_sz = 12.0f;
        Rect box = {ir.x + 10.0f, ir.y + (ir.h - box_sz) * 0.5f, box_sz, box_sz};
        stroke_round_rect(box, 3.0f, 1.0f, maybe_disabled(sel ? focus : border));
        if (sel) {
            Rect inner = {box.x + 3.0f, box.y + 3.0f, box.w - 6.0f, box.h - 6.0f};
            fill_rect(inner, maybe_disabled(focus));
        }

        Rect tr = {box.x + box.w + 8.0f, ir.y, ir.w - box.w - 18.0f, ir.h};
        draw_text_utf8(items[i] ? items[i] : "", tr, maybe_disabled(sel ? text : text_dim));
    }

    draw_widget_label(vis, outer, false);

    mark_last_item(last_id, outer, last_hov, last_focus);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0.7f,0.5f,0.1f,0.4f});
    return changed;
}

bool collapsing_header(const char* label, bool* open) {
    if (!g_drawing) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    register_focusable(id);

    CollapseSlot& slot = collapse_slot_for(id);
    if (!slot.initialized) { slot.initialized = true; slot.open = false; }
    bool state = open ? *open : slot.open;

    finalize_pending_collapse_measure();

    Rect r = next_rect(g_style.item_height);
    CollapseRectFx cfx = collapse_rect_fx(r);
    r = cfx.rect;
    bool raw_hov = rect_contains(r, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;
    bool focused = is_widget_focused(id);

    if (hov && g_input.mouse_pressed) {
        set_widget_focus(id, false);
        g_ctx.active_id = id;
    }
    if (focused && enabled && (g_input.key_space || g_input.key_enter)) state = !state;
    if (g_ctx.active_id == id && g_input.mouse_released) {
        if (hov) state = !state;
        g_ctx.active_id = 0;
    }

    if (open) *open = state;
    else slot.open = state;
    if (effects_enabled()) slot.progress = step_anim(slot.progress, state ? 1.0f : 0.0f, g_dt, 18.0f);
    else slot.progress = state ? 1.0f : 0.0f;
    bool show_body = state || (effects_enabled() && slot.progress > 0.001f);

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, hov, enabled && g_ctx.active_id == id, focused || state);
    Color button = resolve_color(ColorRole::Button);
    Color button_hover = resolve_color(ColorRole::ButtonHover);
    Color panel = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text = resolve_color(ColorRole::Text);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color bg = lerp_color(button, button_hover, motion.hover * 0.40f);
    bg = lerp_color(bg, panel, state ? 0.10f : 0.0f);
    Color border_col = lerp_color(border, focus, (focused ? 0.55f : 0.0f) + (state ? 0.16f : 0.0f));
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(bg), maybe_disabled(border_col), motion.hover, motion.active, focused ? 1.0f : 0.0f);

    Rect arrow_r = {r.x + 10.0f, r.y + (r.h - 10.0f) * 0.5f, 10.0f, 10.0f};
    fill_triangle(arrow_r, slot.progress > 0.5f ? 0 : 2, maybe_disabled(text_dim));
    Rect text_r = {r.x + 28.0f, r.y, r.w - 32.0f, r.h};
    draw_text_utf8(vis, text_r, maybe_disabled(text));
    if (cfx.clip_active) pop_clip();

    if (show_body) {
        float body_top_target = g_ctx.cursor_y;
        float body_top_display = transformed_layout_y(body_top_target);
        if (effects_enabled() && slot.body_height > 0.5f && g_active_collapse_fx_count < 16) {
            ActiveCollapseFx& fx = g_active_collapse_fx[g_active_collapse_fx_count++];
            fx.key = id;
            fx.top_target = body_top_target;
            fx.top_display = body_top_display;
            fx.body_height = slot.body_height;
            fx.progress = ease_smooth(slot.progress);
        }
        g_pending_collapse_key = id;
        g_pending_collapse_start_y = body_top_target;
    }

    mark_last_item(id, r, raw_hov, focused);
    if (g_debug.show_layout_rects) stroke_round_rect(r, 0, 1, {0.8f,0.4f,0.1f,0.4f});
    return show_body;
}

void scroll_area(const char* label, float height, std::function<void()> fn) {
    if (!g_drawing || height <= 0.0f || !fn) return;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);
    ScrollSlot& slot = scroll_slot_for(id ^ 0x442211);

    Rect target_r = {}, target_outer = next_labeled_rect(vis, height, &target_r);
    CollapseRectFx cfx = collapse_rect_fx(target_outer);
    Rect outer = cfx.rect;
    Rect r = labeled_body_rect(vis, outer);
    bool raw_hov = rect_contains(outer, g_input.mouse_x, g_input.mouse_y);
    bool enabled = widget_interaction_enabled();
    bool hov = raw_hov && enabled;

    bool had_scroll_prev = slot.content > (r.h - 12.0f);
    float sb_reserve = had_scroll_prev ? 10.0f : 0.0f;
    Rect inner = {r.x + 6.0f, r.y + 6.0f, r.w - 12.0f - sb_reserve, r.h - 12.0f};
    if (inner.w < 0.0f) inner.w = 0.0f;
    if (inner.h < 0.0f) inner.h = 0.0f;

    if (hov && g_input.wheel_y != 0.0f) {
        slot.target -= g_input.wheel_y;
        g_input.wheel_y = 0.0f;
    }

    MotionSlot& motion = motion_slot_for(id);
    update_motion_slot(motion, hov, false, false);
    Color panel = resolve_color(ColorRole::Panel);
    Color input_bg = resolve_color(ColorRole::InputBg);
    Color border = resolve_color(ColorRole::Border);
    Color focus = resolve_color(ColorRole::InputFocus);
    Color text_dim = resolve_color(ColorRole::TextDim);
    Color panel_col = lerp_color(panel, input_bg, motion.hover * 0.12f);
    if (cfx.clip_active) push_clip(cfx.clip);
    draw_widget_chrome(r, g_style.rounding, maybe_disabled(panel_col), maybe_disabled(border), motion.hover, 0.0f, 0.0f);

    Rect clip_r = {r.x + 2.0f, r.y + 2.0f, r.w - 4.0f, r.h - 4.0f};
    if (clip_r.w < 0.0f) clip_r.w = 0.0f;
    if (clip_r.h < 0.0f) clip_r.h = 0.0f;
    push_clip(clip_r);

    Rect saved_region = g_ctx.content_region;
    float saved_cursor_x = g_ctx.cursor_x;
    float saved_cursor_y = g_ctx.cursor_y;
    RowContext saved_row = g_ctx.row_ctx;

    g_ctx.content_region = inner;
    g_ctx.cursor_x = inner.x;
    g_ctx.cursor_y = inner.y - slot.current;
    g_ctx.row_ctx = {};
    color_scope.suspend_for_children();
    fn();
    color_scope.resume_after_children();
    slot.content = g_ctx.cursor_y + slot.current - inner.y;

    g_ctx.content_region = saved_region;
    g_ctx.cursor_x = saved_cursor_x;
    g_ctx.cursor_y = saved_cursor_y;
    g_ctx.row_ctx = saved_row;
    pop_clip();

    float max_scroll = slot.content > inner.h ? (slot.content - inner.h) : 0.0f;
    slot.target = clampf(slot.target, 0.0f, max_scroll);
    if (effects_enabled()) slot.current = step_anim(slot.current, slot.target, g_dt, 18.0f);
    else slot.current = slot.target;
    slot.current = clampf(slot.current, 0.0f, max_scroll);

    if (slot.content > inner.h) {
        float sb_w = 6.0f, visible_h = inner.h;
        float thumb_h = (visible_h / slot.content) * visible_h;
        if (thumb_h < 12.0f) thumb_h = 12.0f;
        float thumb_y = inner.y + (slot.current / (slot.content - visible_h)) * (visible_h - thumb_h);
        Rect track_r = {r.x + r.w - sb_w - 4.0f, inner.y, sb_w, visible_h};
        Rect thumb_r = {track_r.x, thumb_y, sb_w, thumb_h};
        bool thumb_hov = rect_contains(thumb_r, g_input.mouse_x, g_input.mouse_y) && enabled;
        bool track_hov = rect_contains(track_r, g_input.mouse_x, g_input.mouse_y) && enabled;
        if (g_input.mouse_pressed && thumb_hov) {
            slot.dragging = true;
            slot.drag_mouse = g_input.mouse_y;
            slot.drag_scroll0 = slot.current;
        }
        if (slot.dragging) {
            if (g_input.mouse_down || g_input.mouse_released) {
                float scale = (slot.content - visible_h) / fmaxf(1.0f, visible_h - thumb_h);
                slot.current = slot.drag_scroll0 + (g_input.mouse_y - slot.drag_mouse) * scale;
                slot.current = clampf(slot.current, 0.0f, max_scroll);
                slot.target = slot.current;
            }
            if (g_input.mouse_released) slot.dragging = false;
        }
        if (g_input.mouse_pressed && track_hov && !thumb_hov) {
            slot.target += (g_input.mouse_y < thumb_y ? -visible_h : visible_h);
            slot.target = clampf(slot.target, 0.0f, max_scroll);
        }
        Color track_c = border; track_c.a = 0.3f;
        fill_round_rect(track_r, sb_w * 0.5f, maybe_disabled(track_c));
        fill_round_rect(thumb_r, sb_w * 0.5f, maybe_disabled(slot.dragging ? focus : text_dim));
    }

    draw_widget_label(vis, outer, false);
    if (cfx.clip_active) pop_clip();

    mark_last_item(id, outer, raw_hov, false);
    if (g_debug.show_layout_rects) stroke_round_rect(outer, 0, 1, {0.3f,1.0f,0.4f,0.4f});
}

bool modal(const char* label, std::function<void()> fn) {
    if (!g_drawing || !label || !fn) return false;
    WidgetColorScope color_scope;
    char vis[128]; const char* hs;
    split_label(label, vis, sizeof(vis), &hs);
    int id = hash_str(hs);

    if (g_modal_request_id == id) {
        g_modal_open_id = id;
        g_modal_request_id = 0;
    }
    if (g_modal_open_id != id) return false;
    if (g_input.key_escape) {
        close_modal();
        return false;
    }

    g_modal_drawn = true;
    Rect overlay = {g_ctx.content_region.x, g_ctx.content_region.y, g_ctx.content_region.w, g_ctx.content_region.h};
    Color veil = {0.02f, 0.03f, 0.05f, 0.58f};
    fill_rect(overlay, veil);

    float modal_w = fminf(460.0f, fmaxf(280.0f, overlay.w * 0.56f));
    float modal_h = fminf(320.0f, fmaxf(180.0f, overlay.h * 0.42f));
    Rect panel = {
        overlay.x + (overlay.w - modal_w) * 0.5f,
        overlay.y + (overlay.h - modal_h) * 0.5f,
        modal_w,
        modal_h
    };
    Color panel_col = resolve_color(ColorRole::Panel);
    Color border = resolve_color(ColorRole::Border);
    Color text = resolve_color(ColorRole::Text);
    draw_widget_chrome(panel, fmaxf(g_style.rounding, 10.0f), panel_col, border, 0.0f, 0.0f, 1.0f);

    Rect title_r = {panel.x + 16.0f, panel.y + 10.0f, panel.w - 32.0f, g_style.item_height};
    draw_text_utf8(vis, title_r, text);

    Rect saved_region = g_ctx.content_region;
    float saved_cursor_x = g_ctx.cursor_x;
    float saved_cursor_y = g_ctx.cursor_y;
    RowContext saved_row = g_ctx.row_ctx;
    bool saved_inside_modal = g_inside_modal;

    g_inside_modal = true;
    g_ctx.content_region = {panel.x + 16.0f, panel.y + g_style.item_height + 16.0f, panel.w - 32.0f, panel.h - g_style.item_height - 26.0f};
    g_ctx.cursor_x = g_ctx.content_region.x;
    g_ctx.cursor_y = g_ctx.content_region.y;
    g_ctx.row_ctx = {};
    color_scope.suspend_for_children();
    fn();
    color_scope.resume_after_children();

    g_ctx.content_region = saved_region;
    g_ctx.cursor_x = saved_cursor_x;
    g_ctx.cursor_y = saved_cursor_y;
    g_ctx.row_ctx = saved_row;
    g_inside_modal = saved_inside_modal;

    mark_last_item(id, panel, rect_contains(panel, g_input.mouse_x, g_input.mouse_y), true);
    if (g_debug.show_layout_rects) stroke_round_rect(panel, 0, 1, {1.0f,0.4f,0.2f,0.4f});
    return g_modal_open_id == id;
}

void row(int cols, std::function<void()> fn) {
    if (!g_drawing || cols <= 0) return;
    auto& rc = g_ctx.row_ctx;
    float gap = g_style.item_spacing;
    rc.active   = true;
    rc.weighted = false;
    rc.cols     = cols;
    rc.gap      = gap;
    rc.cell_w   = (g_ctx.content_region.w - gap * (cols - 1)) / cols;
    rc.start_x  = g_ctx.cursor_x;
    rc.start_y  = g_ctx.cursor_y;
    rc.total_w  = g_ctx.content_region.w;
    rc.used_x   = 0;
    rc.total_weight = (float)cols;
    rc.weights  = nullptr;
    rc.col_index = 0;
    rc.row_height = 0;
    fn();
    rc.active = false;
    g_ctx.cursor_y += rc.row_height + g_style.item_spacing;
}

void row(std::initializer_list<float> weights, std::function<void()> fn) {
    if (!g_drawing || !fn || weights.size() == 0) return;
    auto& rc = g_ctx.row_ctx;
    float gap = g_style.item_spacing;
    rc.active = true;
    rc.weighted = true;
    rc.cols = (int)weights.size();
    rc.gap = gap;
    rc.cell_w = 0.0f;
    rc.start_x = g_ctx.cursor_x;
    rc.start_y = g_ctx.cursor_y;
    rc.total_w = g_ctx.content_region.w;
    rc.used_x = 0.0f;
    rc.total_weight = 0.0f;
    rc.weights = weights.begin();
    for (float w : weights) rc.total_weight += w > 0.0f ? w : 0.0f;
    if (rc.total_weight <= 0.0f) rc.total_weight = (float)rc.cols;
    rc.col_index = 0;
    rc.row_height = 0.0f;
    fn();
    rc.active = false;
    g_ctx.cursor_y += rc.row_height + g_style.item_spacing;
}

} // namespace ftui

#endif // FTUI_IMPLEMENTATION

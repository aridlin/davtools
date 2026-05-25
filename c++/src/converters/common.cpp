#include "common.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace conv {

#if defined(_WIN32)
static std::wstring utf8_to_wide(std::string_view s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) {
        throw std::runtime_error("UTF-8 to UTF-16 conversion failed");
    }
    std::wstring out(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), len);
    return out;
}

static std::string windows_error_message(DWORD code) {
    LPSTR buffer = nullptr;
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr
    );

    std::string out = len && buffer ? std::string(buffer, len) : ("Windows error " + std::to_string(code));
    if (buffer) LocalFree(buffer);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

static std::wstring quote_arg_windows(std::string_view arg) {
    bool needs_quotes = arg.empty();
    for (char c : arg) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == '"') {
            needs_quotes = true;
            break;
        }
    }

    std::string quoted;
    if (!needs_quotes) {
        quoted.assign(arg);
    } else {
        quoted.push_back('"');
        unsigned backslashes = 0;
        for (char c : arg) {
            if (c == '\\') {
                ++backslashes;
            } else if (c == '"') {
                quoted.append(backslashes * 2 + 1, '\\');
                quoted.push_back('"');
                backslashes = 0;
            } else {
                quoted.append(backslashes, '\\');
                backslashes = 0;
                quoted.push_back(c);
            }
        }
        quoted.append(backslashes * 2, '\\');
        quoted.push_back('"');
    }

    return utf8_to_wide(quoted);
}

static bool is_executable_file(const fs::path& p) {
    std::error_code ec;
    auto st = fs::status(p, ec);
    return !ec && fs::exists(st) && !fs::is_directory(st);
}
#else
static bool is_executable_file(const fs::path& p) {
    std::error_code ec;
    auto st = fs::status(p, ec);
    return !ec && fs::exists(st) && !fs::is_directory(st) && ::access(p.c_str(), X_OK) == 0;
}
#endif

TempDir::TempDir(std::string prefix) {
    if (prefix.empty()) prefix = "convertdav-";
#if defined(_WIN32)
    fs::path base = fs::temp_directory_path();
    for (int i = 0; i < 100; ++i) {
        fs::path candidate = base / (prefix + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()) + "-" + std::to_string(i));
        std::error_code ec;
        if (fs::create_directory(candidate, ec)) {
            path_ = std::move(candidate);
            valid_ = true;
            return;
        }
    }
    throw std::runtime_error("failed to create temporary directory");
#else
    std::string templ = (fs::temp_directory_path() / (prefix + "XXXXXX")).string();

    std::vector<char> buf(templ.begin(), templ.end());
    buf.push_back('\0');

    char* out = ::mkdtemp(buf.data());
    if (!out) {
        throw std::runtime_error(std::string("mkdtemp failed: ") + std::strerror(errno));
    }

    path_ = out;
    valid_ = true;
#endif
}

TempDir::~TempDir() {
    if (valid_) {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
}

TempDir::TempDir(TempDir&& other) noexcept {
    path_ = std::move(other.path_);
    valid_ = other.valid_;
    other.valid_ = false;
}

TempDir& TempDir::operator=(TempDir&& other) noexcept {
    if (this != &other) {
        if (valid_) {
            std::error_code ec;
            fs::remove_all(path_, ec);
        }
        path_ = std::move(other.path_);
        valid_ = other.valid_;
        other.valid_ = false;
    }
    return *this;
}

static std::vector<std::string> split_path_env() {
    std::vector<std::string> out;
    const char* env = std::getenv("PATH");
    if (!env) return out;
    std::string s(env);
    std::stringstream ss(s);
    std::string item;
#if defined(_WIN32)
    constexpr char sep = ';';
#else
    constexpr char sep = ':';
#endif
    while (std::getline(ss, item, sep)) {
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

#if defined(_WIN32)
static std::vector<std::string> executable_extensions() {
    std::vector<std::string> out;
    const char* env = std::getenv("PATHEXT");
    std::string s = env ? env : ".COM;.EXE;.BAT;.CMD";
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ';')) {
        if (item.empty()) continue;
        std::transform(item.begin(), item.end(), item.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        out.push_back(item);
    }
    return out;
}

static bool has_windows_executable_extension(std::string_view name) {
    fs::path p{std::string(name)};
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& candidate : executable_extensions()) {
        if (ext == candidate) return true;
    }
    return false;
}

static std::string lowercase_copy(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

static std::optional<std::string> find_windows_program_fallback(std::string_view name) {
    const std::string requested = lowercase_copy(name);
    const std::string exe_name = has_windows_executable_extension(name)
        ? std::string(name)
        : std::string(name) + ".exe";

    std::vector<fs::path> direct_dirs;
    if (const char* chocolatey = std::getenv("ChocolateyInstall")) {
        direct_dirs.emplace_back(fs::path(chocolatey) / "bin");
    }
    direct_dirs.emplace_back("C:/ProgramData/chocolatey/bin");

    for (const auto& dir : direct_dirs) {
        fs::path candidate = dir / exe_name;
        if (is_executable_file(candidate)) return candidate.string();
    }

    if (requested != "magick" && requested != "magick.exe") {
        return std::nullopt;
    }

    std::vector<fs::path> program_roots;
    if (const char* program_files = std::getenv("ProgramFiles")) {
        program_roots.emplace_back(program_files);
    }
    if (const char* program_files_x86 = std::getenv("ProgramFiles(x86)")) {
        program_roots.emplace_back(program_files_x86);
    }
    program_roots.emplace_back("C:/Program Files");

    for (const auto& root : program_roots) {
        std::error_code ec;
        if (!fs::is_directory(root, ec)) continue;

        for (const auto& entry : fs::directory_iterator(root, ec)) {
            if (ec) break;
            if (!entry.is_directory(ec)) continue;

            const std::string dir_name = lowercase_copy(entry.path().filename().string());
            if (dir_name.rfind("imagemagick-", 0) != 0) continue;

            fs::path candidate = entry.path() / "magick.exe";
            if (is_executable_file(candidate)) return candidate.string();
        }
    }

    return std::nullopt;
}
#endif

std::optional<std::string> find_program_in_path(std::string_view name) {
    if (name.empty()) return std::nullopt;

    if (name.find('/') != std::string_view::npos || name.find('\\') != std::string_view::npos) {
        fs::path p(name);
#if defined(_WIN32)
        if (has_windows_executable_extension(name) && is_executable_file(p)) return p.string();
        for (const auto& ext : executable_extensions()) {
            fs::path with_ext = p;
            with_ext += ext;
            if (is_executable_file(with_ext)) return with_ext.string();
        }
#else
        if (is_executable_file(p)) return p.string();
#endif
        return std::nullopt;
    }

    for (const auto& dir : split_path_env()) {
        fs::path p = fs::path(dir) / std::string(name);
#if defined(_WIN32)
        if (has_windows_executable_extension(name) && is_executable_file(p)) return p.string();
        for (const auto& ext : executable_extensions()) {
            fs::path with_ext = p;
            with_ext += ext;
            if (is_executable_file(with_ext)) return with_ext.string();
        }
#else
        if (is_executable_file(p)) return p.string();
#endif
    }
#if defined(_WIN32)
    if (auto fallback = find_windows_program_fallback(name)) return fallback;
#endif
    return std::nullopt;
}

bool program_exists(std::string_view name) {
    return find_program_in_path(name).has_value();
}

std::string detect_magick_cli() {
    if (program_exists("magick")) return "magick";
#if !defined(_WIN32)
    if (program_exists("convert")) return "convert";
#endif
    throw std::runtime_error("ImageMagick CLI not found (need 'magick' or 'convert' in PATH)");
}

CommandResult run_process(const std::vector<std::string>& args, const fs::path& cwd) {
    if (args.empty()) throw std::runtime_error("run_process: empty args");

#if defined(_WIN32)
    std::vector<std::string> resolved_args = args;
    if (args[0].find('/') == std::string::npos && args[0].find('\\') == std::string::npos) {
        if (auto resolved = find_program_in_path(args[0])) {
            resolved_args[0] = *resolved;
        }
    }

    HANDLE child_stdout_read = nullptr;
    HANDLE child_stdout_write = nullptr;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&child_stdout_read, &child_stdout_write, &sa, 0)) {
        throw std::runtime_error("CreatePipe failed: " + windows_error_message(GetLastError()));
    }
    if (!SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(child_stdout_read);
        CloseHandle(child_stdout_write);
        throw std::runtime_error("SetHandleInformation failed: " + windows_error_message(GetLastError()));
    }

    std::wstring command_line;
    for (const auto& arg : resolved_args) {
        if (!command_line.empty()) command_line.push_back(L' ');
        command_line += quote_arg_windows(arg);
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = child_stdout_write;
    si.hStdError = child_stdout_write;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};
    std::wstring cwd_w = cwd.empty() ? std::wstring() : cwd.wstring();
    BOOL created = CreateProcessW(
        nullptr,
        command_line.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        cwd_w.empty() ? nullptr : cwd_w.c_str(),
        &si,
        &pi
    );

    CloseHandle(child_stdout_write);

    if (!created) {
        DWORD err = GetLastError();
        CloseHandle(child_stdout_read);
        return CommandResult{127, "CreateProcess failed: " + windows_error_message(err)};
    }

    std::string out;
    char buf[4096];
    DWORD read = 0;
    while (ReadFile(child_stdout_read, buf, sizeof(buf), &read, nullptr) && read > 0) {
        out.append(buf, read);
    }
    CloseHandle(child_stdout_read);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return CommandResult{static_cast<int>(exit_code), std::move(out)};
#else
    int pipefd[2];
    if (::pipe(pipefd) != 0) {
        throw std::runtime_error(std::string("pipe failed: ") + std::strerror(errno));
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        throw std::runtime_error(std::string("fork failed: ") + std::strerror(errno));
    }

    if (pid == 0) {
        // child
        if (!cwd.empty()) {
            ::chdir(cwd.c_str());
        }

        ::dup2(pipefd[1], STDOUT_FILENO);
        ::dup2(pipefd[1], STDERR_FILENO);
        ::close(pipefd[0]);
        ::close(pipefd[1]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& s : args) argv.push_back(const_cast<char*>(s.c_str()));
        argv.push_back(nullptr);

        ::execvp(argv[0], argv.data());
        _exit(127);
    }

    // parent
    ::close(pipefd[1]);
    std::string out;
    char buf[4096];
    for (;;) {
        ssize_t n = ::read(pipefd[0], buf, sizeof(buf));
        if (n > 0) out.append(buf, static_cast<std::size_t>(n));
        else break;
    }
    ::close(pipefd[0]);

    int status = 0;
    ::waitpid(pid, &status, 0);

    CommandResult r;
    if (WIFEXITED(status)) r.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) r.exit_code = 128 + WTERMSIG(status);
    else r.exit_code = -1;
    r.output = std::move(out);
    return r;
#endif
}

void write_file_bytes(const fs::path& p, const std::vector<std::uint8_t>& data) {
    std::ofstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("failed to open for write: " + p.string());
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!f) throw std::runtime_error("failed to write file: " + p.string());
}

std::vector<std::uint8_t> read_file_bytes(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("failed to open for read: " + p.string());
    f.seekg(0, std::ios::end);
    auto sz = f.tellg();
    f.seekg(0, std::ios::beg);
    if (sz < 0) throw std::runtime_error("failed to stat file: " + p.string());

    std::vector<std::uint8_t> data(static_cast<std::size_t>(sz));
    if (!data.empty()) {
        f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!f) throw std::runtime_error("failed to read file: " + p.string());
    }
    return data;
}

std::string replace_extension(std::string name, std::string_view new_ext_no_dot) {
    auto pos = name.find_last_of('.');
    if (pos == std::string::npos) return name + "." + std::string(new_ext_no_dot);
    return name.substr(0, pos + 1) + std::string(new_ext_no_dot);
}

std::string basename_no_ext(std::string_view name) {
    std::string s(name);
    auto slash = s.find_last_of("/\\");
    if (slash != std::string::npos) s = s.substr(slash + 1);
    auto dot = s.find_last_of('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    return s;
}

std::string lower_ext(std::string_view name) {
    std::string s(name);
    auto dot = s.find_last_of('.');
    if (dot == std::string::npos) return {};
    std::string ext = s.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::vector<fs::path> list_files_sorted(const fs::path& dir) {
    std::vector<fs::path> out;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.is_regular_file()) out.push_back(e.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

OutputArtifact make_artifact_from_file(const fs::path& p, std::string logical_name) {
    OutputArtifact a{};
    a.name = logical_name.empty() ? p.filename().string() : std::move(logical_name);

    // app.hpp currently uses std::string for binary-safe payload storage
    auto bytes = read_file_bytes(p);
    a.data.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());

    return a;
}

void require_success(const CommandResult& r, std::string_view tool_name) {
    if (r.exit_code != 0) {
        std::string msg = std::string(tool_name) + " failed (exit " + std::to_string(r.exit_code) + ")";
        if (!r.output.empty()) {
            msg += ": " + r.output;
        }
        throw std::runtime_error(msg);
    }
}

std::vector<std::string> magick_args(const std::vector<std::string>& subargs) {
    const std::string cli = detect_magick_cli();
    if (cli == "magick") {
        std::vector<std::string> out;
        out.reserve(subargs.size() + 1);
        out.push_back("magick");
        out.insert(out.end(), subargs.begin(), subargs.end());
        return out;
    }
    return subargs; // "convert" mode expects direct args
}

} // namespace conv

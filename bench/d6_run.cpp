#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
// Escape arguments for Windows argv parsing.
std::wstring quote(const std::wstring& arg) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (wchar_t ch : arg) {
        if (ch == L'\\') {
            ++slashes;
            continue;
        }
        result.append(ch == L'"' ? 2 * slashes + 1 : slashes, L'\\');
        result += ch;
        slashes = 0;
    }
    result.append(2 * slashes, L'\\');
    return result + L'"';
}

bool launch(const std::vector<fs::path>& args, const fs::path& log) {
    std::wstring command;
    for (const auto& arg : args) {
        if (!command.empty()) command += L' ';
        command += quote(arg.native());
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE file = CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open simulator log");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = file;
    startup.hStdError = file;
    PROCESS_INFORMATION child{};
    const BOOL started = CreateProcessW(args.front().c_str(), command.data(), nullptr,
                                         nullptr, TRUE, 0, nullptr, nullptr, &startup, &child);
    CloseHandle(file);
    if (!started) throw std::runtime_error("Cannot start simulator");
    CloseHandle(child.hThread);
    DWORD status = 1;
    const bool completed = WaitForSingleObject(child.hProcess, INFINITE) == WAIT_OBJECT_0
                        && GetExitCodeProcess(child.hProcess, &status);
    CloseHandle(child.hProcess);
    return completed && status == 0;
}
#else
bool launch(const std::vector<fs::path>& args, const fs::path& log) {
    std::vector<char*> argv;
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    const int file = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file < 0) throw std::runtime_error("Cannot open simulator log");
    const pid_t child = fork();
    if (child == 0) {
        if (dup2(file, STDOUT_FILENO) < 0 || dup2(file, STDERR_FILENO) < 0) _exit(126);
        if (file > STDERR_FILENO) close(file);
        execv(argv.front(), argv.data());
        _exit(127);
    }
    close(file);
    if (child < 0) throw std::runtime_error("Cannot start simulator");
    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    return waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

std::string readTiming(const fs::path& log) {
    std::ifstream input(log);
    for (std::string line; std::getline(input, line);) {
        std::cout << line << '\n';
        const auto start = line.find(" time=");
        if (start == std::string::npos || line.back() != 's') continue;
        const auto value = line.substr(start + 6, line.size() - start - 7);
        if (!value.empty() && value.find_first_not_of("0123456789.eE+-") == std::string::npos) {
            return value;
        }
    }
    throw std::runtime_error("Missing simulator timing in " + log.string());
}

int main(int argc, char** argv) try {
    if (argc < 3) {
        throw std::runtime_error("Usage: d6_run SIMULATOR OUTPUT [--schedule NAME] [--smoke]");
    }
    const auto simulator = fs::absolute(fs::u8path(argv[1]));
    const fs::path output = fs::u8path(argv[2]);
    std::string schedule = "uniform";
    bool smoke = false;
    for (int i = 3; i < argc; ++i) {
        const std::string option = argv[i];
        if (option == "--smoke") smoke = true;
        else if (option == "--schedule" && i + 1 < argc) schedule = argv[++i];
        else throw std::runtime_error("Invalid option: " + option);
    }
    if (schedule.empty() || schedule.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos) {
        throw std::runtime_error("Invalid schedule name");
    }
    const bool hibbardStress = schedule == "min_delete_high_insert" ||
                               schedule == "max_delete_low_insert";
    const bool scapegoatStress = schedule == "scapegoat_stress";
    if (schedule != "uniform" && !hibbardStress && !scapegoatStress) {
        throw std::runtime_error("Unknown benchmark schedule: " + schedule);
    }
    if (!fs::is_regular_file(simulator)) throw std::runtime_error("Simulator executable not found");
    if (!fs::create_directories(output)) throw std::runtime_error("Output directory already exists");
    std::ofstream summary(output / "runs.csv");
    summary.exceptions(std::ios::badbit | std::ios::failbit);
    summary << "strategy,n,seed,schedule,pairs,seconds,samples\n";
    for (int n : {1000, 2000, 4000}) {
        // Allow four turnovers for chains to form and persist.
        const int pairs = smoke ? 2 * n : (hibbardStress ? 4 * n : n * n);
        const int sample = hibbardStress ? n / 10 : n;
        for (int seed : {20261117, 20261118, 20261119, 20261120, 20261121}) {
            for (const std::string strategy : {"hibbard_succ", "hibbard_rand",
                                               "scapegoat_array", "scapegoat_inplace"}) {
                const bool hibbard = strategy == "hibbard_succ" || strategy == "hibbard_rand";
                if ((hibbardStress && !hibbard) || (scapegoatStress && hibbard)) continue;
                const auto stem = strategy + "_" + std::to_string(n) + "_" + std::to_string(seed);
                const auto samples = stem + ".csv";
                const auto log = output / "simulator.log";
                if (!launch({simulator, "--strategy", strategy, "--n", std::to_string(n),
                             "--seed", std::to_string(seed), "--schedule", schedule,
                             "--pairs", std::to_string(pairs), "--sample", std::to_string(sample),
                             "--out", output / samples}, log)) {
                    std::cerr << std::ifstream(log).rdbuf();
                    throw std::runtime_error("Simulator failed for " + stem);
                }
                summary << strategy << ',' << n << ',' << seed << ',' << schedule << ',' << pairs
                        << ',' << readTiming(log) << ',' << samples << '\n' << std::flush;
                fs::remove(log);
            }
        }
    }
} catch (const std::exception& error) {
    std::cerr << "d6_run: " << error.what() << '\n';
    return EXIT_FAILURE;
}

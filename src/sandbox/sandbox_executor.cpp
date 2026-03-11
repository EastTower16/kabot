#include "sandbox/sandbox_executor.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <system_error>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace kabot::sandbox {

ExecResult SandboxExecutor::Run(const std::string& command,
                                const std::string& working_dir,
                                std::chrono::seconds timeout) {
    ExecResult result{};
#ifdef _WIN32
    (void)command;
    (void)working_dir;
    (void)timeout;
    result.exit_code = -1;
    result.error = "SandboxExecutor is not supported on Windows";
    return result;
#else
    static const std::vector<std::string> kBlockedTokens = {
        "rm -rf",
        "rm -r",
        "shutdown",
        "reboot",
        "mkfs",
        "dd ",
        ":(){:|:&};:",
        "sudo ",
        "su ",
        "kill -9",
        "killall",
        "chmod 777",
        "chown",
        "curl | sh",
        "wget | sh"
    };
    for (const auto& token : kBlockedTokens) {
        if (command.find(token) != std::string::npos) {
            result.blocked = true;
            result.output = "Error: command blocked by policy";
            return result;
        }
    }
    const auto stamp = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto stdout_path = std::filesystem::temp_directory_path() / ("kabot_stdout_" + stamp + ".log");
    const auto stderr_path = std::filesystem::temp_directory_path() / ("kabot_stderr_" + stamp + ".log");
    const char* kProxyVars[] = {
        "HTTP_PROXY",
        "HTTPS_PROXY",
        "ALL_PROXY",
        "http_proxy",
        "https_proxy",
        "all_proxy",
        "NO_PROXY",
        "no_proxy"
    };

    const int stdout_fd = ::open(stdout_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (stdout_fd < 0) {
        result.exit_code = -1;
        result.error = std::string("Error: failed to open stdout log: ") + std::strerror(errno);
        return result;
    }

    const int stderr_fd = ::open(stderr_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (stderr_fd < 0) {
        const int saved_errno = errno;
        ::close(stdout_fd);
        std::error_code ec;
        std::filesystem::remove(stdout_path, ec);
        result.exit_code = -1;
        result.error = std::string("Error: failed to open stderr log: ") + std::strerror(saved_errno);
        return result;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        const int saved_errno = errno;
        ::close(stdout_fd);
        ::close(stderr_fd);
        std::error_code ec;
        std::filesystem::remove(stdout_path, ec);
        std::filesystem::remove(stderr_path, ec);
        result.exit_code = -1;
        result.error = std::string("Error: exec failed: ") + std::strerror(saved_errno);
        return result;
    }

    if (pid == 0) {
        if (!working_dir.empty() && ::chdir(working_dir.c_str()) != 0) {
            const auto message = std::string("Error: failed to change directory: ") + std::strerror(errno) + "\n";
            ::write(stderr_fd, message.c_str(), message.size());
            _exit(125);
        }

        for (const auto* key : kProxyVars) {
            if (const char* value = std::getenv(key)) {
                ::setenv(key, value, 1);
            }
        }

        if (::dup2(stdout_fd, STDOUT_FILENO) < 0 || ::dup2(stderr_fd, STDERR_FILENO) < 0) {
            const auto message = std::string("Error: failed to redirect output: ") + std::strerror(errno) + "\n";
            ::write(stderr_fd, message.c_str(), message.size());
            _exit(125);
        }

        ::close(stdout_fd);
        ::close(stderr_fd);

        ::execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));

        const auto message = std::string("Error: exec failed: ") + std::strerror(errno) + "\n";
        ::write(STDERR_FILENO, message.c_str(), message.size());
        _exit(127);
    }

    ::close(stdout_fd);
    ::close(stderr_fd);

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    bool finished = false;
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            finished = true;
            break;
        }
        if (waited < 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (!finished) {
        result.timed_out = true;
        ::kill(pid, SIGTERM);
        const auto grace_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (std::chrono::steady_clock::now() < grace_deadline) {
            const auto waited = ::waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                finished = true;
                break;
            }
            if (waited < 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!finished) {
            ::kill(pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            finished = true;
        }
    }

    if (finished) {
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.exit_code = 128 + WTERMSIG(status);
        }
    } else if (result.exit_code == -1) {
        result.exit_code = 124;
    }

    std::ostringstream output_stream;
    std::ostringstream error_stream;
    auto read_file = [&](const std::filesystem::path& path, std::ostringstream& target) {
        std::ifstream input(path);
        if (!input.is_open()) {
            return;
        }
        target << input.rdbuf();
    };
    read_file(stdout_path, output_stream);
    read_file(stderr_path, error_stream);
    result.output = output_stream.str();
    result.error = error_stream.str();

    std::error_code ec;
    std::filesystem::remove(stdout_path, ec);
    std::filesystem::remove(stderr_path, ec);
    return result;
#endif
}

}  // namespace kabot::sandbox

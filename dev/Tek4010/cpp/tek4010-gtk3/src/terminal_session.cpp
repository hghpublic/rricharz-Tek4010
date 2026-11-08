// PTY-backed child-process lifecycle and nonblocking input/output for TubeCore.
module;

#define _GNU_SOURCE

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>
#include <signal.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

module terminal_session;

namespace
{
constexpr int EXEC_FAILURE_STATUS{127};
constexpr int CHILD_EXIT_POLL_ATTEMPTS{20};
constexpr int CHILD_EXIT_POLL_INTERVAL_MICROSECONDS{10000};

bool find_executable(const char* command, std::string& path)
{
    // Resolve the executable before fork so the child can exec without a shell.
    if ((command == nullptr) || (command[0] == '\0'))
    {
        return false;
    }

    if (strchr(command, '/') != nullptr)
    {
        if (access(command, X_OK) == 0)
        {
            path = command;
            return true;
        }
        return false;
    }

    const char* searchPath{getenv("PATH")};
    if (searchPath == nullptr)
    {
        return false;
    }

    const std::string_view directories{searchPath};
    std::size_t directoryStart{0};
    while (directoryStart <= directories.size())
    {
        const std::size_t separator{directories.find(':', directoryStart)};
        const std::size_t directoryLength =
            separator == std::string_view::npos
                ? directories.size() - directoryStart
                : separator - directoryStart;

        std::string candidate{
            directories.substr(directoryStart, directoryLength)};
        if (candidate.empty())
        {
            candidate = ".";
        }
        candidate += '/';
        candidate += command;

        if (access(candidate.c_str(), X_OK) == 0)
        {
            path = std::move(candidate);
            return true;
        }

        if (separator == std::string_view::npos)
        {
            break;
        }
        directoryStart = separator + 1;
    }

    return false;
}
} // namespace

TerminalSession::~TerminalSession()
{
    close();
}

bool TerminalSession::start(int argc, char* argv[], int firstArg, bool useShell,
                            int eraseCharacter)
{
    // Do not replace a live session: its streams and child belong to the caller.
    if ((childPid_ > 0) || (input_ != nullptr) || (output_ != nullptr))
    {
        errno = EBUSY;
        return false;
    }

    childExited_ = false;
    childExitStatus_ = EXIT_SUCCESS;
    std::string commandPath;
    std::vector<char*> childArguments;

    if (!useShell)
    {
        // Resolve and prepare argv in the parent; the child can exec directly.
        if ((firstArg >= argc) || !find_executable(argv[firstArg], commandPath))
        {
            std::println(stderr, "Unknown command {}",
                         firstArg < argc ? argv[firstArg] : "");
            return false;
        }

        childArguments.reserve(static_cast<std::size_t>(argc - firstArg + 1));
        childArguments.push_back(const_cast<char*>(commandPath.c_str()));
        for (int i{firstArg + 1}; i < argc; ++i)
        {
            childArguments.push_back(argv[i]);
        }
        childArguments.push_back(nullptr);
    }

    int ptyMaster{-1};
    const pid_t pid{forkpty(&ptyMaster, nullptr, nullptr, nullptr)};
    if (pid == -1)
    {
        std::println(stderr, "Cannot fork pseudo terminal: {}",
                     strerror(errno));
        return false;
    }

    if (pid == 0)
    {
        // Configure the terminal endpoint before handing control to the child.
        setenv("TERM", "tek4014", 1);

        if (useShell)
        {
            const char* shell{getenv("SHELL")};
            if ((shell == nullptr) || (shell[0] == '\0') ||
                (strstr(shell, "zsh") != nullptr))
            {
                shell = "/bin/sh";
            }

            struct termios terminalSettings;
            if (tcgetattr(STDIN_FILENO, &terminalSettings) == 0)
            {
#ifdef ECHOCTL
                terminalSettings.c_lflag &= ~ECHOCTL;
#endif
                terminalSettings.c_lflag |= ECHOE;
                terminalSettings.c_cc[VERASE] =
                    static_cast<cc_t>(eraseCharacter);
                tcsetattr(STDIN_FILENO, TCSANOW, &terminalSettings);
            }

            char interactiveFlag[]{"-i"};
            char* shellArguments[]{const_cast<char*>(shell), interactiveFlag,
                                   nullptr};
            execvp(shell, shellArguments);
        }
        else
        {
            execv(commandPath.c_str(), childArguments.data());
        }

        perror("Unable to start terminal child");
        _exit(EXEC_FAILURE_STATUS);
    }

    childPid_ = pid;

    // Separate descriptors let the core poll reads while writes remain directed
    // to the same PTY master.
    const int outputDescriptor{dup(ptyMaster)};
    if (outputDescriptor == -1)
    {
        const int error{errno};
        ::close(ptyMaster);
        errno = error;
        std::println(stderr, "Cannot duplicate pseudo terminal: {}",
                     strerror(errno));
        close();
        return false;
    }

    input_ = fdopen(ptyMaster, "r");
    if (input_ == nullptr)
    {
        const int error{errno};
        ::close(ptyMaster);
        ::close(outputDescriptor);
        errno = error;
        std::println(stderr, "Cannot open terminal input stream: {}",
                     strerror(errno));
        close();
        return false;
    }

    output_ = fdopen(outputDescriptor, "w");
    if (output_ == nullptr)
    {
        const int error{errno};
        ::close(outputDescriptor);
        errno = error;
        std::println(stderr, "Cannot open terminal output stream: {}",
                     strerror(errno));
        close();
        return false;
    }

    // Avoid stdio read-ahead: readiness is checked directly on the PTY descriptor.
    setbuf(input_, nullptr);
    setbuf(output_, nullptr);
    return true;
}

bool TerminalSession::hasInput() const
{
    if ((input_ == nullptr) || (feof(input_) != 0) || (ferror(input_) != 0))
    {
        return false;
    }

    fd_set readDescriptors;
    FD_ZERO(&readDescriptors);
    const int inputDescriptor{fileno(input_)};
    FD_SET(inputDescriptor, &readDescriptors);

    timeval timeout{};
    const int result = select(inputDescriptor + 1, &readDescriptors, nullptr,
                              nullptr, &timeout);
    if ((result < 0) && (errno != EINTR))
    {
        std::println(stderr, "Cannot poll terminal input: {}", strerror(errno));
        return false;
    }
    return (result > 0) && FD_ISSET(inputDescriptor, &readDescriptors);
}

int TerminalSession::readInput()
{
    if (input_ == nullptr)
    {
        return -1;
    }

    const int ch{fgetc(input_)};
    // The parser consumes only the seven-bit Tek character code.
    return ch == EOF ? -1 : ch & 0x7F;
}

int TerminalSession::normalizeWaitStatus(int status)
{
    // Match shell convention so exit codes and signal termination share one API.
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status))
    {
        return 128 + WTERMSIG(status);
    }
    return EXIT_FAILURE;
}

bool TerminalSession::pollChildExit()
{
    if (childPid_ <= 0)
    {
        return childExited_;
    }

    // WNOHANG keeps the GTK timer responsive while still reaping completed children.
    int status{0};
    pid_t result;
    do
    {
        result = waitpid(childPid_, &status, WNOHANG);
    } while ((result == -1) && (errno == EINTR));

    if (result == childPid_)
    {
        childPid_ = -1;
        childExited_ = true;
        childExitStatus_ = normalizeWaitStatus(status);
    }
    else if ((result == -1) && (errno == ECHILD))
    {
        childPid_ = -1;
        childExited_ = true;
        childExitStatus_ = EXIT_FAILURE;
    }
    else if ((result == -1) && (errno != EINTR))
    {
        std::println(stderr, "Cannot poll terminal child: {}", strerror(errno));
    }

    return childExited_;
}

bool TerminalSession::childExited() const
{
    return childExited_;
}

int TerminalSession::childExitStatus() const
{
    return childExitStatus_;
}

bool TerminalSession::writeByte(int ch)
{
    if ((output_ == nullptr) || (putc(ch, output_) == EOF) ||
        (fflush(output_) == EOF))
    {
        return false;
    }
    return true;
}

bool TerminalSession::sendBreak()
{
    if (output_ == nullptr)
    {
        errno = EBADF;
        return false;
    }
    return tcsendbreak(fileno(output_), 0) == 0;
}

void TerminalSession::closeStreams()
{
    if (input_ != nullptr)
    {
        if (fclose(input_) == EOF)
        {
            std::println(stderr, "Cannot close terminal input stream: {}",
                         strerror(errno));
        }
        input_ = nullptr;
    }

    if (output_ != nullptr)
    {
        if (fclose(output_) == EOF)
        {
            std::println(stderr, "Cannot close terminal output stream: {}",
                         strerror(errno));
        }
        output_ = nullptr;
    }
}

void TerminalSession::stopChild()
{
    // Close the process group gracefully, then escalate and reap if necessary.
    if (childPid_ <= 0)
    {
        return;
    }

    const pid_t childPid{childPid_};
    childPid_ = -1;
    bool reaped{childExited_};

    if (!reaped)
    {
        if ((kill(-childPid, SIGHUP) == -1) && (errno != ESRCH))
        {
            std::println(stderr, "Cannot signal terminal child: {}",
                         strerror(errno));
        }

        int status{0};
        for (int attempt{0}; attempt < CHILD_EXIT_POLL_ATTEMPTS; ++attempt)
        {
            const pid_t result{waitpid(childPid, &status, WNOHANG)};
            if ((result == childPid) || ((result == -1) && (errno == ECHILD)))
            {
                reaped = true;
                break;
            }
            if ((result == -1) && (errno != EINTR))
            {
                std::println(stderr, "Cannot wait for terminal child: {}",
                             strerror(errno));
                reaped = true;
                break;
            }
            usleep(CHILD_EXIT_POLL_INTERVAL_MICROSECONDS);
        }

        if (!reaped)
        {
            if ((kill(-childPid, SIGKILL) == -1) && (errno != ESRCH))
            {
                std::println(stderr, "Cannot terminate terminal child: {}",
                             strerror(errno));
            }
            pid_t result;
            do
            {
                result = waitpid(childPid, &status, 0);
            } while ((result == -1) && (errno == EINTR));
            if ((result == -1) && (errno != ECHILD))
            {
                std::println(stderr, "Cannot reap terminal child: {}",
                             strerror(errno));
            }
        }
    }

    childExited_ = true;
}

void TerminalSession::close()
{
    // Release parent PTY descriptors before stopping the process that owns them.
    closeStreams();
    stopChild();
}

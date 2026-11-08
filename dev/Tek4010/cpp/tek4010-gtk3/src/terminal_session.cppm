module;

#include <cstdio>
#include <sys/types.h>

// PTY session API without exposing its POSIX process-management implementation.
export module terminal_session;

export {
    // Owns the PTY streams and child process from start() until close().
    class TerminalSession final
    {
    public:
        TerminalSession() = default;
        ~TerminalSession();

        TerminalSession(const TerminalSession&) = delete;
        TerminalSession& operator=(const TerminalSession&) = delete;

        // Launch a command or shell in a PTY; false reports startup failure.
        bool start(int argc, char* argv[], int firstArg, bool useShell,
                   int eraseCharacter);
        // Readiness check is nonblocking; readInput() returns -1 when empty.
        bool hasInput() const;
        int readInput();
        // Poll without blocking and retain the child's normalized exit status.
        bool pollChildExit();
        bool childExited() const;
        int childExitStatus() const;
        // Convert waitpid status to an exit code, mapping signals to 128 + signal.
        static int normalizeWaitStatus(int status);
        bool writeByte(int ch);
        bool sendBreak();
        // Release streams and stop/reap the child, including after partial startup.
        void close();

    private:
        void closeStreams();
        void stopChild();

        pid_t childPid_{-1};
        std::FILE* input_{nullptr};
        std::FILE* output_{nullptr};
        bool childExited_{false};
        int childExitStatus_{0};
    };
}

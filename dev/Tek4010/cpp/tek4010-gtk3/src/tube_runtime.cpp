// Handles terminal input pacing, APL key translation, and font prerequisites.
module;

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <istream>
#include <memory>
#include <print>
#include <pwd.h>
#include <scn/scan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sys/wait.h>
#include <tuple>
#include <unistd.h>

module tube;

import terminal_session;
import :internal;

int tube_isInput(TubeCoreState& state)
// Check queued parser input and PTY readiness without blocking the host loop.
{
    if (state.input().hasQueuedInput())
    {
        return 1;
    }

    if (!state.terminalSession().hasInput())
    {
        state.input().resetCharacterTimer(std::chrono::steady_clock::now());
        return 0;
    }

    return 1;
}

int tube_getInputChar(TubeCoreState& state)
// Return the next paced input byte, or -1 when input is unavailable.
{
    if (tube_isInput(state))
    {
        const auto now{std::chrono::steady_clock::now()};
        // Preserve the configured character interval while allowing redraws.
        if (state.input().shouldDelayCharacter(now))
        {
            return -1;
        }
        const auto queuedByte{state.input().takeQueuedInput()};
        const int c{queuedByte ? *queuedByte
                               : state.terminalSession().readInput()};
        if (c < 0)
        {
            return -1;
        }
        state.input().recordCharacter(now);

        state.diagnostics().traceEscapeCharacter(
            c, state.options().escapeDebugEnabled());

        return c;
    }
    else
    {
        return -1;
    }
}

bool tube_loadKeyTranslationTable(TubeInputState& input,
                                  std::istream& config)
{
    // Ignore non-entry lines, but reject valid tables beyond fixed capacity.
    input.clearKeyTranslations();

    int index{0};
    std::string line;
    while (std::getline(config, line))
    {
        auto result{scn::scan<int, int>(line, "{} {}")};
        if (!result)
        {
            continue;
        }

        if (index >= TUBE_MAX_KEYCODES)
        {
            return false;
        }

        const auto [inputCode, outputCode] = result->values();
        if (!input.addKeyTranslation(index, inputCode, outputCode))
        {
            return false;
        }
        index++;
    }

    return true;
}

static void checkFont(const char* fontName)
// APL drawing requires this font; fail setup if Fontconfig cannot find it.
{
    std::string command{"fc-list '"};
    command += fontName;
    command += '\'';
    std::unique_ptr<FILE, decltype(&pclose)> fontList{
        popen(command.c_str(), "r"), &pclose};
    if (!fontList)
    {
        std::println(stderr, "Cannot start APL font query: {}", fontName);
        exit(EXIT_FAILURE);
    }

    std::string output;
    std::array<char, 256> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()),
                 fontList.get()) != nullptr)
    {
        output += buffer.data();
    }

    const bool readFailed = ferror(fontList.get()) != 0;
    const int status{pclose(fontList.release())};
    if (readFailed || (status == -1) || !WIFEXITED(status) ||
        (WEXITSTATUS(status) != 0))
    {
        std::println(stderr, "APL font query failed: {}", fontName);
        exit(EXIT_FAILURE);
    }

    if (output.find(fontName) != std::string::npos)
    {
        return;
    }

    std::println("Error: APL font '{}' not installed. This font is required "
                 "for the APL mode.",
                 fontName);
    exit(EXIT_FAILURE);
}

static void readKeyTranslationTable(TubeCoreState& state)
// Load the optional per-user map; without it, keys pass through unchanged.
{
    const passwd* const user{getpwuid(getuid())};
    if ((user == nullptr) || (user->pw_dir == nullptr))
    {
        std::println(stderr,
                     "Cannot determine home directory for APL key table");
        exit(EXIT_FAILURE);
    }

    std::filesystem::path configPath{user->pw_dir};
    configPath /= ".tek4010conf";
    configPath /= "aplkeys";
    std::ifstream confFile{configPath};
    if (!tube_loadKeyTranslationTable(state.input(), confFile))
    {
        std::println("Error: APL key code table too large, max {} entries",
                     TUBE_MAX_KEYCODES);
        confFile.close();
        exit(EXIT_FAILURE);
    }
}

int tube_translateKeyCode(TubeCoreState& state, int ch)
{
    return state.input().translateKeyCode(ch);
}

void tube_initializeAplInput(TubeCoreState& state)
{
    // Validate rendering and keyboard prerequisites before entering APL mode.
    checkFont(APL_FONT);
    readKeyTranslationTable(state);
}

// Implements per-instance emulator state and command-line option parsing.
module;

#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <print>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>
#include <system_error>

#include "help.h"

module tube;

import tube_renderer;
import :internal;

int TubeCoordinates::currentX() const
{
    return currentX_;
}

int TubeCoordinates::currentY() const
{
    return currentY_;
}

int TubeCoordinates::targetX() const
{
    return targetX_;
}

int TubeCoordinates::targetY() const
{
    return targetY_;
}

int TubeCoordinates::leftMargin() const
{
    return leftMargin_;
}

void TubeCoordinates::setCurrentX(int x)
{
    currentX_ = x;
}

void TubeCoordinates::setCurrentY(int y)
{
    currentY_ = y;
}

void TubeCoordinates::setTargetX(int x)
{
    targetX_ = x;
}

void TubeCoordinates::setTargetY(int y)
{
    targetY_ = y;
}

void TubeCoordinates::setCurrentPosition(int x, int y)
{
    currentX_ = x;
    currentY_ = y;
}

void TubeCoordinates::setTargetPosition(int x, int y)
{
    targetX_ = x;
    targetY_ = y;
}

void TubeCoordinates::setTargetToCurrent()
{
    targetX_ = currentX_;
    targetY_ = currentY_;
}

void TubeCoordinates::copyTargetToCurrent()
{
    currentX_ = targetX_;
    currentY_ = targetY_;
}

void TubeCoordinates::moveCurrentX(int delta)
{
    currentX_ += delta;
}

void TubeCoordinates::moveCurrentY(int delta)
{
    currentY_ += delta;
}

void TubeCoordinates::setLeftMargin(int margin)
{
    leftMargin_ = margin;
}

void TubeCoordinates::resetForClear(int baselineY)
{
    currentX_ = 0;
    currentY_ = baselineY;
    targetX_ = 0;
    targetY_ = baselineY;
    leftMargin_ = 0;
}

int TubeDisplayGeometry::windowWidth() const
{
    return windowWidth_;
}

int TubeDisplayGeometry::windowHeight() const
{
    return windowHeight_;
}

int TubeDisplayGeometry::horizontalDotsPerCharacter() const
{
    return horizontalDotsPerCharacter_;
}

int TubeDisplayGeometry::verticalDotsPerCharacter() const
{
    return verticalDotsPerCharacter_;
}

void TubeDisplayGeometry::setWindowSize(int width, int height)
{
    windowWidth_ = width;
    windowHeight_ = height;
}

void TubeDisplayGeometry::setCharacterGrid(int charactersPerLine,
                                           int charactersPerPage)
{
    // Cells use whole logical dots; integer division leaves remainder dots unused.
    horizontalDotsPerCharacter_ = windowWidth_ / charactersPerLine;
    verticalDotsPerCharacter_ = windowHeight_ / charactersPerPage;
}

void TubeRenderLifecycle::requestPersistentClear()
{
    persistentClearRequested_ = true;
}

bool TubeRenderLifecycle::persistentClearRequested() const
{
    return persistentClearRequested_;
}

void TubeRenderLifecycle::completePersistentClear()
{
    persistentClearRequested_ = false;
}

void TubeRenderLifecycle::showBrightSpot(int frameCount)
{
    brightFramesRemaining_ = frameCount;
}

bool TubeRenderLifecycle::hasBrightSpot() const
{
    return brightFramesRemaining_ > 0;
}

void TubeRenderLifecycle::advanceFrame()
{
    // Bright-spot lifetime follows timer ticks, keeping decay in core state.
    if (brightFramesRemaining_ > 0)
    {
        brightFramesRemaining_--;
    }
}

void TubeCursorState::show()
{
    visible_ = true;
}

void TubeCursorState::hide()
{
    visible_ = false;
}

bool TubeCursorState::isVisible() const
{
    return visible_;
}

bool TubeCharacterSetState::aplMode() const
{
    return aplMode_;
}

void TubeCharacterSetState::setAplMode(bool enabled)
{
    aplMode_ = enabled;
}

std::chrono::milliseconds TubeFrameTiming::drawBudget() const
{
    return drawBudget_;
}

void TubeFrameTiming::setDrawBudget(std::chrono::milliseconds budget)
{
    drawBudget_ = budget;
}

double TubeStrokeStyle::penSize() const
{
    return penSize_;
}

void TubeStrokeStyle::setPenSize(double penSize)
{
    penSize_ = penSize;
}

LineType TubeStrokeStyle::lineType() const
{
    return lineType_;
}

void TubeStrokeStyle::setLineType(LineType lineType)
{
    lineType_ = lineType;
}

bool TubeCoreOptions::ardsMode() const
{
    return ardsMode_;
}

void TubeCoreOptions::setArdsMode(bool enabled)
{
    ardsMode_ = enabled;
}

bool TubeCoreOptions::aplEnabled() const
{
    return aplEnabled_;
}

void TubeCoreOptions::setAplEnabled(bool enabled)
{
    aplEnabled_ = enabled;
}

bool TubeCoreOptions::autoClear() const
{
    return autoClear_;
}

void TubeCoreOptions::setAutoClear(bool enabled)
{
    autoClear_ = enabled;
}

bool TubeCoreOptions::noExit() const
{
    return noExit_;
}
void TubeCoreOptions::setNoExit(bool enabled)
{
    noExit_ = enabled;
}
bool TubeCoreOptions::expandTabs() const
{
    return expandTabs_;
}
void TubeCoreOptions::setExpandTabs(bool enabled)
{
    expandTabs_ = enabled;
}
bool TubeCoreOptions::keepCharacterSize() const
{
    return keepCharacterSize_;
}
void TubeCoreOptions::setKeepCharacterSize(bool enabled)
{
    keepCharacterSize_ = enabled;
}
bool TubeCoreOptions::fastMode() const
{
    return fastMode_;
}
void TubeCoreOptions::setFastMode(bool enabled)
{
    fastMode_ = enabled;
}
bool TubeCoreOptions::performanceMode() const
{
    return performanceMode_;
}
void TubeCoreOptions::setPerformanceMode(bool enabled)
{
    performanceMode_ = enabled;
    if (enabled)
    {
        // Performance mode also disables fading through the fast-render path.
        setFastMode(true);
    }
}
bool TubeCoreOptions::rawMode() const
{
    return rawMode_;
}
void TubeCoreOptions::setRawMode(bool enabled)
{
    rawMode_ = enabled;
}
int TubeCoreOptions::baudRate() const
{
    return baudRate_;
}
void TubeCoreOptions::setBaudRate(int baudRate)
{
    baudRate_ = baudRate;
}
bool TubeCoreOptions::fullScreen() const
{
    return fullScreen_;
}
void TubeCoreOptions::setFullScreen(bool enabled)
{
    fullScreen_ = enabled;
}
bool TubeCoreOptions::verticalFullScreen() const
{
    return verticalFullScreen_;
}
void TubeCoreOptions::setVerticalFullScreen(bool enabled)
{
    verticalFullScreen_ = enabled;
}
bool TubeCoreOptions::hideCursor() const
{
    return hideCursor_;
}
void TubeCoreOptions::setHideCursor(bool enabled)
{
    hideCursor_ = enabled;
}
int TubeCoreOptions::waitSeconds() const
{
    return waitSeconds_;
}
void TubeCoreOptions::setWaitSeconds(int seconds)
{
    waitSeconds_ = seconds;
}
bool TubeCoreOptions::halfScreen() const
{
    return halfScreen_;
}
void TubeCoreOptions::setHalfScreen(bool enabled)
{
    halfScreen_ = enabled;
}
bool TubeCoreOptions::keyboardDisabled() const
{
    return keyboardDisabled_;
}
void TubeCoreOptions::setKeyboardDisabled(bool disabled)
{
    keyboardDisabled_ = disabled;
}
bool TubeCoreOptions::usesPty() const
{
    return usesPty_;
}
void TubeCoreOptions::setUsesPty(bool enabled)
{
    usesPty_ = enabled;
}
int TubeCoreOptions::eraseCharacter() const
{
    return eraseCharacter_;
}
void TubeCoreOptions::setEraseCharacter(int character)
{
    eraseCharacter_ = character;
}
bool TubeCoreOptions::escapeDebugEnabled() const
{
    return escapeDebugEnabled_;
}
void TubeCoreOptions::setEscapeDebugEnabled(bool enabled)
{
    escapeDebugEnabled_ = enabled;
}

void TubeInputState::enqueueInputByte(unsigned char byte)
{
    queuedInput_.push_back(byte);
}

bool TubeInputState::hasQueuedInput() const
{
    return !queuedInput_.empty();
}

std::optional<unsigned char> TubeInputState::takeQueuedInput()
{
    if (queuedInput_.empty())
    {
        return std::nullopt;
    }

    const unsigned char byte{queuedInput_.front()};
    queuedInput_.pop_front();
    return byte;
}

void TubeInputState::clearKeyTranslations()
{
    keyTable_.fill(TubeKeyCode{});
}

bool TubeInputState::addKeyTranslation(int index, int inputCode, int outputCode)
{
    if (index < 0)
    {
        return false;
    }

    const auto tableIndex{static_cast<std::size_t>(index)};
    if (tableIndex >= keyTable_.size())
    {
        return false;
    }

    keyTable_[tableIndex] =
        TubeKeyCode{.inputCode = inputCode, .outputCode = outputCode};
    return true;
}

int TubeInputState::translateKeyCode(int inputCode) const
{
    // Translation entries are contiguous; unmatched keys pass through unchanged.
    for (const TubeKeyCode& keyCode : keyTable_)
    {
        if (keyCode.inputCode == 0)
        {
            break;
        }
        if (keyCode.inputCode == inputCode)
        {
            return keyCode.outputCode;
        }
    }
    return inputCode;
}

void TubeInputState::setCharacterInterval(std::chrono::microseconds interval)
{
    characterInterval_ = interval;
}

bool TubeInputState::shouldDelayCharacter(
    std::chrono::steady_clock::time_point now) const
{
    // Each byte advances the next deadline from the last idle boundary.
    return characterTimeSinceReset(now) <
           (characterInterval_ * charResetCount_);
}

void TubeInputState::recordCharacter(std::chrono::steady_clock::time_point now)
{
    charCount_++;
    charResetCount_++;
    lastInputTime_ = now;
}

void TubeInputState::resetCharacterTimer(
    std::chrono::steady_clock::time_point now)
{
    // An idle gap starts a fresh pacing burst for the next input byte.
    inputTimerStartTime_ = now;
    charResetCount_ = 0;
}

std::chrono::steady_clock::duration TubeInputState::characterTimeSinceReset(
    std::chrono::steady_clock::time_point now) const
{
    return now - inputTimerStartTime_;
}

void TubeProcessState::requestScreenshot()
{
    screenshotRequested_ = true;
}

bool TubeProcessState::takeScreenshotRequest()
{
    const bool requested{screenshotRequested_};
    screenshotRequested_ = false;
    return requested;
}

bool TubeProcessState::markChildFinished()
{
    if (childFinishedShown_)
    {
        return false;
    }
    childFinishedShown_ = true;
    return true;
}

bool TubeProcessState::shouldQuitAfterWait(
    std::chrono::steady_clock::time_point now, std::chrono::seconds wait)
{
    if (!firstWait_)
    {
        // Start the configured delay when child completion is first observed.
        firstWait_ = now;
        return false;
    }
    return now - *firstWait_ >= wait;
}

void TubeDiagnosticsState::printEscapeDebugCharacter(int ch)
{
    ch &= 0xff;

    if (ch == 27)
    {
        std::print("<ESC>");
    }
    else if (ch == 7)
    {
        std::print("<BEL>");
    }
    else if (ch == '\r')
    {
        std::print("<CR>");
    }
    else if (ch == '\n')
    {
        std::print("<LF>");
    }
    else if ((ch >= 32) && (ch < 127))
    {
        putchar(ch);
    }
    else
    {
        std::print("<{:02X}>", ch);
    }
}

void TubeDiagnosticsState::printEscapeDebug()
{
    std::print("ESCDEBUG: ");
    for (int i{0}; i < escDebugCount_; i++)
    {
        printEscapeDebugCharacter(escDebugBuf_[i]);
    }
    std::println();
    fflush(stdout);
}

void TubeDiagnosticsState::traceEscapeCharacter(int ch, bool enabled)
{
    if (!enabled)
    {
        return;
    }

    ch &= 0xff;

    if (ch == 27)
    {
        // Start a new trace at ESC, flushing any incomplete prior sequence.
        if (escDebugActive_ && escDebugCount_ > 0)
        {
            printEscapeDebug();
        }

        escDebugActive_ = 1;
        escDebugCount_ = 0;
        escDebugBuf_[escDebugCount_++] = ch;
        return;
    }

    if (!escDebugActive_)
    {
        return;
    }

    if (static_cast<std::size_t>(escDebugCount_) < escDebugBuf_.size())
    {
        escDebugBuf_[escDebugCount_++] = ch;
    }

    if ((ch == '\r') || (ch == '\n') ||
        (static_cast<std::size_t>(escDebugCount_) >= escDebugBuf_.size()))
    {
        // Bound trace memory and emit complete lines as soon as they arrive.
        printEscapeDebug();
        escDebugActive_ = 0;
        escDebugCount_ = 0;
    }
}

int TubeCoreOptions::parseCommandLine(int argc, char* argv[])
{
    constexpr int MAX_COMMAND_LINE_ARGUMENT_COUNT{19};
    constexpr int DEFAULT_WAIT_SECONDS{3};

    int firstArg{1};
    // Establish legacy defaults before command-line switches override them.
    setAutoClear(true);
    setNoExit(true);
    if (argc > MAX_COMMAND_LINE_ARGUMENT_COUNT)
    {
        std::println("Error: Too many arguments");
        exit(EXIT_FAILURE);
    }

    if ((argc > firstArg) && ((std::string_view{argv[firstArg]} == "-h") ||
                              (std::string_view{argv[firstArg]} == "--help")))
    {
        std::print("{}", helpStr);
        exit(EXIT_SUCCESS);
    }

    // Retain this legacy option behavior for compatibility with earlier releases.
    if (std::string_view{argv[argc - 1]} == "-noexit")
    {
        setNoExit(true);
        argc--;
    }

    while ((firstArg < argc) &&
           std::string_view{argv[firstArg]}.starts_with('-'))
    {
        const std::string_view argument{argv[firstArg]};
        if (argument == "-raw")
        {
            setRawMode(true);
        }
        else if (argument == "-noexit")
        {
            setNoExit(true);
        }
        else if (argument == "-exit")
        {
            setNoExit(false);
        }
        else if (argument == "-b100000")
        {
            setBaudRate(100000);
        }
        else if (argument == "-b38400")
        {
            setBaudRate(38400);
        }
        else if (argument == "-b19200")
        {
            setBaudRate(19200);
        }
        else if (argument == "-b9600")
        {
            setBaudRate(9600);
        }
        else if (argument == "-b4800")
        {
            setBaudRate(4800);
        }
        else if (argument == "-b2400")
        {
            setBaudRate(2400);
        }
        else if (argument == "-b1200")
        {
            setBaudRate(1200);
        }
        else if (argument == "-b600")
        {
            setBaudRate(600);
        }
        else if (argument == "-b300")
        {
            setBaudRate(300);
        }
        else if (argument == "-tab1")
        {
            setExpandTabs(true);
        }
        else if (argument == "-full")
        {
            setFullScreen(true);
        }
        else if (argument == "-fullv")
        {
            setVerticalFullScreen(true);
        }
        else if (argument == "-autoClear")
        {
            setAutoClear(true);
        }
        else if (argument == "-noAutoClear")
        {
            setAutoClear(false);
        }
        else if (argument == "-keepsize")
        {
            setKeepCharacterSize(true);
        }
        else if (argument == "-hidecursor")
        {
            setHideCursor(true);
        }
        else if (argument == "-APL")
        {
            setAplEnabled(true);
        }
        else if (argument == "-ARDS")
        {
            setArdsMode(true);
        }
        else if (argument == "-fast")
        {
            std::println("Fast mode refresh without fading");
            setFastMode(true);
        }
        else if (argument == "-performance")
        {
            setPerformanceMode(true);
        }
        else if (argument == "-nokey")
        {
            if (!fullScreen())
            {
                std::println("Keyboard input disabled");
                setKeyboardDisabled(true);
            }
            else
            {
                std::print("Keyboard entry must be allowed in full view");
            }
        }
        else if (argument == "-wait")
        {
            // A missing numeric value retains the historical three-second wait.
            setWaitSeconds(DEFAULT_WAIT_SECONDS);
            if (firstArg < argc - 2)
            {
                const std::string_view waitArgument{argv[firstArg + 1]};
                if (!waitArgument.empty() && (waitArgument.front() >= '0') &&
                    (waitArgument.front() <= '9'))
                {
                    int waitSeconds{0};
                    const char* const waitArgumentEnd =
                        waitArgument.data() + waitArgument.size();
                    const auto result = std::from_chars(
                        waitArgument.data(), waitArgumentEnd, waitSeconds);
                    if ((result.ec != std::errc{}) ||
                        (result.ptr != waitArgumentEnd))
                    {
                        std::println(stderr, "Error: Invalid -wait value '{}'",
                                     waitArgument);
                        exit(EXIT_FAILURE);
                    }
                    firstArg++;
                    setWaitSeconds(waitSeconds);
                }
            }
        }
        else if (argument == "-half")
        {
            setHalfScreen(true);
        }
        else if (argument == "-escdebug")
        {
            setEscapeDebugEnabled(true);
        }

        else if (argument == "-eraseBS")
        {
            setEraseCharacter(ERASE_BS);
        }
        else if (argument == "-eraseDEL")
        {
            setEraseCharacter(ERASE_DEL);
        }

        else
        {
            std::println("tek4010: unknown argument {}", argv[firstArg]);
            exit(EXIT_FAILURE);
        }
        firstArg++;
    }

    // With no command remaining, run an interactive shell on the PTY.
    setUsesPty(firstArg >= argc);
    if (baudRate() > 20000)
    {
        // High baud rates cannot use the ordinary fading path responsively.
        setFastMode(true);
    }

    return firstArg;
}

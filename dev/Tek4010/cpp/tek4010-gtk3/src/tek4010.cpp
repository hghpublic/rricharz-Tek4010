/*
 * tek4010.c
 *
 * A tek4010/4014 graphics emulator
 *
 * Copyright 2019  rricharz
 *
 * https://github.com/rricharz/Tek4010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#define _GNU_SOURCE

module;

#include <array>
#include <chrono>
#include <errno.h>
#include <print>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <utility>

module tube:tek4010;

import terminal_session;
import tube_renderer;
import :internal;

// Incremental Tek 4010/4014 protocol decoder; parser state survives draw calls.
static constexpr bool DEBUG{false}; // enable parser diagnostics
static constexpr long TODO{32}; // draw multiple objects until screen updates

// TekParserMode records partial coordinates and escape/protocol states across
// incoming bytes; see its named values in tube_internal.cppm.

// table for special plot point mode
// 4014 manual page F-9
static constexpr int intensityTable[]{
    14, 16, 17, 19, 20, 22, 23, 25,  28, 31, 34, 38, 41, 33, 47, 50,
    56, 62, 69, 75, 81, 88, 94, 100, 56, 62, 69, 75, 81, 88, 96, 100,
    0,  1,  1,  1,  1,  1,  1,  2,   2,  2,  2,  2,  3,  3,  3,  3,
    4,  4,  4,  5,  5,  5,  6,  6,   7,  8,  9,  10, 11, 12, 12, 13,
    14, 16, 17, 19, 20, 22, 23, 25,  28, 31, 34, 38, 41, 44, 47, 50,
    56, 62, 69, 75, 81, 88, 94, 100, 56, 63, 69, 75, 81, 88};

namespace
{
constexpr bool isCoordinateMode(TekParserMode mode)
{
    switch (mode)
    {
    case TekParserMode::MoveYHigh:
    case TekParserMode::MoveYLow:
    case TekParserMode::MoveXHigh:
    case TekParserMode::MoveXLow:
    case TekParserMode::VectorYHigh:
    case TekParserMode::VectorYLow:
    case TekParserMode::VectorXHigh:
    case TekParserMode::VectorXLow:
        return true;
    default:
        return false;
    }
}

constexpr bool isHighCoordinateByte(TekParserMode mode)
{
    return (mode == TekParserMode::MoveYHigh) ||
           (mode == TekParserMode::MoveXHigh) ||
           (mode == TekParserMode::VectorYHigh) ||
           (mode == TekParserMode::VectorXHigh);
}

constexpr TekParserMode nextCoordinateMode(TekParserMode mode)
{
    switch (mode)
    {
    case TekParserMode::MoveYHigh:
        return TekParserMode::MoveYLow;
    case TekParserMode::MoveYLow:
        return TekParserMode::MoveXHigh;
    case TekParserMode::MoveXHigh:
        return TekParserMode::MoveXLow;
    case TekParserMode::MoveXLow:
        return TekParserMode::VectorYHigh;
    case TekParserMode::VectorYHigh:
        return TekParserMode::VectorYLow;
    case TekParserMode::VectorYLow:
        return TekParserMode::VectorXHigh;
    case TekParserMode::VectorXHigh:
        return TekParserMode::VectorXLow;
    default:
        return mode;
    }
}
} // namespace

bool Tek4010State::captureGinKey(int ch)
{
    if (!ginModeActive_)
    {
        return false;
    }

    pendingGinKey_ = ch;
    return true;
}

bool Tek4010State::ginModeActive() const
{
    return ginModeActive_;
}

void Tek4010State::resetPlotModes()
{
    plotPointMode_ = 0;
    specialPlotMode_ = 0;
}

void Tek4010State::resetIntensityControl()
{
    defocussed_ = false;
    intensity_ = 100;
}

bool Tek4010State::writeThroughMode() const
{
    return writeThroughMode_;
}

bool Tek4010State::isDefocussed() const
{
    return defocussed_;
}

int Tek4010State::intensity() const
{
    return intensity_;
}

void Tek4010State::beginGinMode()
{
    ginModeActive_ = true;
    pendingGinKey_.reset();
}

void Tek4010State::endGinMode()
{
    ginModeActive_ = false;
    pendingGinKey_.reset();
}

std::optional<int> Tek4010State::pendingGinKey() const
{
    return pendingGinKey_;
}

void Tek4010State::startGinEchoSuppression(int count)
{
    ginEchoCount_ = count;
}

bool Tek4010State::suppressGinEcho(int ch)
{
    if (ginEchoCount_ == 0)
    {
        return false;
    }

    if (ch == 10)
    {
        return true;
    }

    const int expectedCharacter{ginCharacter[ginEchoCount_ - 1]};
    if ((ch & 0x7F) == expectedCharacter)
    {
        if (DEBUG)
        {
            std::println("GIN echo suppression ({}): suppressing: {}",
                         ginEchoCount_, ch);
        }
        ginEchoCount_--;
        return true;
    }

    if (DEBUG)
    {
        std::println("GIN echo suppression, characters are different ({},{})",
                     ch & 0x7F, expectedCharacter);
    }
    ginEchoCount_ = 0;
    return false;
}

void Tek4010State::checkLimits(TubeCoreState& state, TubeRenderer& renderer)
/* Clamp beam coordinates and handle alpha wrapping at the page boundaries. */
{
    /* Graphics may span the full width; the left margin applies to alpha wrap. */
    if (state.coordinates().currentX() < 0)
    {
        state.coordinates().setCurrentX(0);
    }

    if (state.coordinates().currentX() >
        state.displayGeometry().windowWidth() -
            state.displayGeometry().horizontalDotsPerCharacter())
    {
        state.coordinates().setCurrentX(state.coordinates().leftMargin());
        state.coordinates().moveCurrentY(
            -(state.displayGeometry().verticalDotsPerCharacter()));
    }
    if (state.coordinates().currentY() < 4)
    { // new line at bottom of page
        state.coordinates().setCurrentY(
            state.displayGeometry().windowHeight() -
            state.displayGeometry().verticalDotsPerCharacter());
        if (state.coordinates().leftMargin())
        {
            state.coordinates().setLeftMargin(0);
        }
        else
        {
            state.coordinates().setLeftMargin(
                state.displayGeometry().windowWidth() / 2);
        }
        if (state.options().autoClear())
        {
            state.coordinates().setLeftMargin(0);
            tube_clearPersistent(state, renderer);
        }
        /* Keep alpha text within the active column margin. */
        if (state.coordinates().currentX() < state.coordinates().leftMargin())
        {
            state.coordinates().setCurrentX(state.coordinates().leftMargin());
        }
    }
    if (state.coordinates().currentY() >
        (state.displayGeometry().windowHeight() -
         state.displayGeometry().verticalDotsPerCharacter()))
    {
        state.coordinates().setCurrentY(
            state.displayGeometry().windowHeight() -
            state.displayGeometry().verticalDotsPerCharacter());
    }
}

void Tek4010State::bell(TubeCoreState& state)
{
    // Preserve the terminal's expected 50 ms bell timing.
    state.input().resetCharacterTimer(std::chrono::steady_clock::now());
    usleep(50000);
    state.cursor().hide();
    state.tek().todo = 0;
}

void Tek4010State::sendCoordinates(TubeCoreState& state)
{
    // send 4 coordinate bytes
    int x, y, ch;
    x = (int)((double)state.coordinates().currentX() / state.tek().efactor);
    y = (int)((double)state.coordinates().currentY() / state.tek().efactor);

    if (DEBUG)
    {
        std::println("sendCoordinates, x={}, y={}", x, y);
    }

    ch = (x >> 5) + 0x20;
    tube_write_key(state, ch);
    state.tek().ginCharacter[4] = ch; // save to check for echo
    ch = (x & 31) + 0x20;
    tube_write_key(state, ch);
    state.tek().ginCharacter[3] = ch; // save to check for echo
    ch = (y >> 5) + 0x20;
    tube_write_key(state, ch);
    state.tek().ginCharacter[2] = ch; // save to check for echo
    ch = (y & 31) + 0x20;
    tube_write_key(state, ch);
    state.tek().ginCharacter[1] = ch; // save to check for echo
}

void Tek4010State::enqMode(TubeCoreState& state)
{
    // activated by sending ESC ENQ
    int status;
    status = 0x20;
    if (state.coordinates().leftMargin() == 0)
    {
        status += 2;
    }
    if (state.tek().mode == TekParserMode::Alpha)
    {
        status += 4;
    }
    tube_write_key(state, status); // send status byte
    sendCoordinates(state);
    tube_write_key(state, 13); // cannot send a EOT here
}

void Tek4010State::ginMode(TubeCoreState& state, TubeRenderer& renderer)
{
    // activated by sending ESC SUB
    if (DEBUG)
    {
        std::println("GIN, mode = {}, input mode = {}",
                     std::to_underlying(state.tek().mode),
                     state.tek().ginModeActive());
    }
    tube_crosshair(state, renderer);
    state.tek().mode = TekParserMode::Gin;
    state.tek().todo = 0;
    state.cursor().hide();
    state.tek().beginGinMode();
}

void Tek4010State::ginSend(TubeCoreState& state, int ch)
{
    // Send the key and packed coordinates, then the required carriage return.
    if (DEBUG)
    {
        std::println("ginSend, ch = {}", ch);
    }
    tube_write_key(state, ch);        // user key stroke character
    state.tek().ginCharacter[5] = ch; // save to check for echo
    sendCoordinates(state);           // 4 characters of packed coordinates
    // cannot send a EOT here
    // wait 5 ms, then send CR.
    usleep(5000);
    tube_write_key(state, 13); // cr
    state.tek().ginCharacter[0] = 13;
    // prepare to suppress unwanted echoed characters.
    state.tek().startGinEchoSuppression(6);
}

void Tek4010State::clicked(TubeCoreState& state, int x, int y)
{
    if (DEBUG)
    {
        std::println("Clicked, mode = {}",
                     std::to_underlying(state.tek().mode));
    }

    if (state.tek().mode == TekParserMode::Gin)
    {
        state.coordinates().setCurrentPosition(x, y);

        ginSend(state, ' ');
        state.tek().mode = TekParserMode::Alpha;
        state.tek().todo = 0;
        state.tek().endGinMode();
    }
}

void Tek4010State::escapeCodeHandler(TubeCoreState& state,
                                     TubeRenderer& renderer, int ch)
// Decode Tek ESC commands after draw() has handled controls shared by modes.
{
    if (DEBUG)
    {
        std::println("Escape {:02X}, savemode={}", ch,
                     std::to_underlying(state.tek().saveMode));
    }
    switch (ch)
    {
    case 0:
        break; // ignore filler 0

    case 5: // ENQ: ask for status and position
        enqMode(state);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case 6:
        break;

    case 8: // backspace during ESC
        state.coordinates().moveCurrentX(
            -(state.displayGeometry().horizontalDotsPerCharacter()));
        checkLimits(state, renderer);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case 9: // tab during ESC
        if (state.options().expandTabs())
        {
            state.coordinates().moveCurrentX(
                state.displayGeometry().horizontalDotsPerCharacter());
        }
        else
        {
            state.coordinates().setCurrentX(
                state.coordinates().currentX() -
                (state.coordinates().currentX() %
                 (8 * state.displayGeometry().horizontalDotsPerCharacter())) +
                8 * state.displayGeometry().horizontalDotsPerCharacter());
        }
        checkLimits(state, renderer);
        state.tek().mode = TekParserMode::Alpha;
        break;

    case 11: // VT during ESC, move one line up
        state.coordinates().moveCurrentY(
            state.displayGeometry().verticalDotsPerCharacter());
        checkLimits(state, renderer);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case 12: // FF during ESC
        if (!state.options().keepCharacterSize())
        {
            tube_changeCharacterSize(state, renderer, 74, 35,
                                     state.tek().efactor);
        }
        tube_clearPersistent(state, renderer);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case 13:
        state.tek().mode = TekParserMode::Alpha;
        break;
    case 14: // SO  activate alternative char set
        if (state.options().aplEnabled())
        { // Switch only when APL support is enabled.
            state.characterSet().setAplMode(true);
        }
        state.tek().mode = TekParserMode::Alpha;
        state.tek().todo = 0;
        break;
    case 15: // SI  deactivate alternative char set
        state.characterSet().setAplMode(false);
        state.tek().mode = TekParserMode::Alpha;
        state.tek().todo = 0;
        break;

    case 23:
        tube_request_screenshot(state);
        state.tek().mode = TekParserMode::Alpha;
        break;

    case 26: // sub
        ginMode(state, renderer);
        break;

    // modes 27 and 29 - 31 are identical in all modes
    case 28: { // record separator
        if (DEBUG)
        {
            std::println("Special point plot mode, mode={}",
                         std::to_underlying(state.tek().saveMode));
        }
        state.tek().mode = TekParserMode::IntensityControl;
        state.tek().plotPointMode_ = 1;
        state.tek().specialPlotMode_ = 1;
        double intensity{1.0};
        int defocussed{0};
        break;
    }

    case '8':
        tube_changeCharacterSize(state, renderer, 74, 35, state.tek().efactor);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case '9':
        tube_changeCharacterSize(state, renderer, 81, 38,
                                 state.tek().efactor * 0.9);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case ':':
        tube_changeCharacterSize(state, renderer, 121, 58,
                                 state.tek().efactor * 0.65);
        state.tek().mode = TekParserMode::Alpha;
        break;
    case ';':
        tube_changeCharacterSize(state, renderer, 133, 64,
                                 state.tek().efactor * 0.55);
        state.tek().mode = TekParserMode::Alpha;
        break;

    case '[':
        state.tek().mode = TekParserMode::IgnoreCsi;
        break;

    case ']':
        state.tek().mode = TekParserMode::IgnoreOsc;
        break;

    // Escape codes for stroke style and plot modes.
    case '`':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'a':
        state.strokeStyle().setLineType(LineType::Dotted);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'b':
        state.strokeStyle().setLineType(LineType::DotDash);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'c':
        state.strokeStyle().setLineType(LineType::ShortDash);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'd':
        state.strokeStyle().setLineType(LineType::LongDash);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'e':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'f':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;
    case 'g':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 0;
        state.tek().mode = state.tek().saveMode;
        break;

    // Defocus escape codes are unsupported; ignore them through the next GS.
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
        if (DEBUG)
        {
            std::println("Defocussed mode ESC {} not supported, ignored", ch);
        }
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        break;

    // Write-through plots are drawn on the temporary layer only.
    case 'p':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 'q':
        state.strokeStyle().setLineType(LineType::Dotted);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 'r':
        state.strokeStyle().setLineType(LineType::DotDash);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 's':
        state.strokeStyle().setLineType(LineType::ShortDash);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 't':
        state.strokeStyle().setLineType(LineType::LongDash);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 'u':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 'v':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;
    case 'w':
        state.strokeStyle().setLineType(LineType::Solid);
        state.tek().writeThroughMode_ = 1;
        state.tek().mode = TekParserMode::IgnoreUntilGroupSeparator;
        state.cursor().hide();
        break;

    default:
        state.tek().mode = TekParserMode::Alpha;
        break;
    }
}

int Tek4010State::checkReturnToAlpha(TubeCoreState& state, int ch)
// test for return to alpha character set
// see 4014 manual, page F-10, note 1
{
    if (state.tek().mode == TekParserMode::Alpha)
    {
        return 0;
    }

    if (ch == 27)
    {
        state.tek().saveMode = state.tek().mode;
    }
    if ((ch == 31) || (ch == 13) || (ch == 27))
    {
        if (DEBUG && (state.tek().mode != TekParserMode::Alpha))
        {
            std::println("Going to alpha mode");
        }
        state.tek().mode = TekParserMode::Alpha;
        state.cursor().hide();
        if (ch == 27)
        {
            state.tek().mode = TekParserMode::Escape;
            state.tek().todo = 0;
        }
        state.tek().plotPointMode_ = 0;
        state.tek().specialPlotMode_ = 0;
        if (state.tek().ginModeActive() && DEBUG)
        {
            std::println("clearing GIN mode, char = {}", ch);
        }
        state.tek().endGinMode();
        return 1;
    }
    else
    {
        return 0;
    }
}

int digit(char ch)
{
    return ((ch >= '0') && (ch <= '9'));
}

void Tek4010State::draw(TubeCoreState& state, TubeRenderer& renderer,
                        bool first)
{
    int ch, tag;

    if (first)
    {
        first = 0;
        state.tek().efactor = state.displayGeometry().windowWidth() / 1024.0;
        state.frameTiming().setDrawBudget(std::chrono::milliseconds{50});
        tube_changeCharacterSize(state, renderer, 74, 35, state.tek().efactor);
        if (state.tek().efactor > 0.8)
        {
            state.strokeStyle().setPenSize(state.tek().efactor * 1.25);
        }
        if (state.displayGeometry().windowWidth() != 1024)
        {
            std::println("Scaling: {:.3f}", state.tek().efactor / 4.0);
        }
    }

    state.tek().startPaintTime = std::chrono::steady_clock::now();

    state.cursor().show();

    // Clear the transient layer each frame and the persistent layer on request.
    tube_clearSecond(state, renderer);

    if (state.renderLifecycle().persistentClearRequested())
    {
        tube_clearPersistent(state, renderer);
        if (!state.options().keepCharacterSize())
        {
            tube_changeCharacterSize(state, renderer, 74, 35,
                                     state.tek().efactor);
        }
    }

    if (state.characterSet().aplMode())
    {
        tube_setupPainting(state, renderer, TubeFont::Apl);
    }
    else
    {
        tube_setupPainting(state, renderer, TubeFont::Standard);
    }

    // Bound parser work per draw so dense plots cannot monopolize the host loop.
    if (state.tek().plotPointMode_)
    {
        state.tek().todo = 100 * TODO;
    }
    else if (state.tek().writeThroughMode_)
    {
        state.tek().todo = 8 * TODO;
    }
    else if (state.tek().mode == TekParserMode::Alpha)
    {
        state.tek().todo = 4 * TODO; // for text speed
    }
    else
    {
        state.tek().todo = TODO;
    }

    do
    {
        ch = tube_getInputChar(state);

        if (tube_isInput(state) == 0)
        {
            state.tek().todo = 0;
        }

        if (ch == -1)
        {
            if ((state.tek().mode == TekParserMode::Alpha) &&
                state.cursor().isVisible())
            {
                tube_doCursor(state, renderer);
            }
            if ((state.tek().mode != TekParserMode::Gin) ||
                state.options().performanceMode())
            {
                return; // Yield so the host can process updates.
            }
        }

        // Suppress terminal echoes of the GIN key and coordinate reply.
        if (state.tek().suppressGinEcho(ch))
        {
            return;
        }

        if (DEBUG)
        {
            std::print("mode={}, ch code {:02X}",
                       std::to_underlying(state.tek().mode), ch);
            if ((ch > 0x20) && (ch <= 0x7E))
            {
                std::print(" ({:c})", ch);
            }
            std::println();
        }

        if (checkReturnToAlpha(state, ch))
        {
            state.tek().todo = state.tek().todo - 4;
            goto endDo;
        }

        // Handle protocol controls shared across parser modes before payload data.

        switch (ch)
        {
        case 7:
            bell(state);
            goto endDo;
        case 10: // new line
            state.coordinates().moveCurrentY(
                -(state.displayGeometry().verticalDotsPerCharacter()));
            checkLimits(state, renderer);
            goto endDo;
        case 13: // return
            if (state.tek().mode != TekParserMode::Escape)
            { // special handling in ESC mode
                state.tek().mode = TekParserMode::Alpha;
                state.coordinates().setCurrentX(
                    state.coordinates().leftMargin());
                goto endDo;
            }
            break;
        case 27: // escape code, all modes
            state.tek().saveMode = state.tek().mode;
            state.tek().mode = TekParserMode::Escape;
            goto endDo;
        case 28: // file separator  >> point plot mode
            if (state.tek().mode != TekParserMode::Escape)
            { // special handling in ESC mode
                state.tek().mode = TekParserMode::VectorYHigh;
                state.tek().plotPointMode_ = 1;
                goto endDo;
            }
            break;
        case 29: // group separator >> graphics mode
            state.tek().mode = TekParserMode::MoveYHigh;
            state.tek().plotPointMode_ = 0;
            goto endDo;
        case 30: // record separator >> incremental plot mode
            if (DEBUG)
            {
                std::println("Incremental point plot mode");
            }
            state.tek().penDown = 1;
            state.tek().mode = TekParserMode::IncrementalPlot;
            goto endDo;
        case 31: // US, normal mode
            state.tek().mode = TekParserMode::Alpha;
            goto endDo;
        }


        // Advance coordinate state first so this byte can be decoded immediately
        // under the next mode in the same parser iteration.

        tag = (ch >> 5) & 3;

        if (isCoordinateMode(state.tek().mode))
        {

            if ((state.tek().mode == TekParserMode::VectorYHigh) && (ch == 29))
            {
                if (DEBUG)
                {
                    std::println("group separator, go from mode 5 to mode 1");
                }
                state.tek().mode = TekParserMode::MoveYHigh;
                goto endDo;
            }

            if (DEBUG)
            {
                if (isHighCoordinateByte(state.tek().mode))
                {
                    std::print("    mode={},tag={}-H-val={},",
                               std::to_underlying(state.tek().mode), tag,
                               32 * (ch & 31));
                }
                else
                {
                    std::print("    mode={},tag={}-L-val={},",
                               std::to_underlying(state.tek().mode), tag,
                               ch & 31);
                }
                std::println("xh={},xl={},yh={},yl={}", state.tek().xh,
                             state.tek().xl, state.tek().yh, state.tek().yl);
            }

            if (tag != 0)
            {

                if ((state.tek().mode == TekParserMode::MoveYHigh) &&
                    (tag != 1))
                {
                    state.tek().mode = TekParserMode::MoveYLow;
                }

                if ((state.tek().mode == TekParserMode::MoveXHigh) &&
                    (tag == 3))
                {
                    // Preserve 4014 extra coordinate bits before the next Y byte.
                    state.tek().mode = TekParserMode::MoveYLow;
                    state.tek().xy4014 = state.tek().yl;
                    if (DEBUG)
                    {
                        std::println("4014 coordinates, overwrite last value");
                    }
                }

                if ((state.tek().mode == TekParserMode::MoveYLow) && (tag != 3))
                {
                    state.tek().mode = TekParserMode::MoveXHigh;
                }
                if ((state.tek().mode == TekParserMode::MoveXHigh) &&
                    (tag != 1))
                {
                    state.tek().mode = TekParserMode::MoveXLow;
                }

                if ((state.tek().mode == TekParserMode::VectorYHigh) &&
                    (tag != 1))
                {
                    state.tek().mode = TekParserMode::VectorYLow;
                }

                if ((state.tek().mode == TekParserMode::VectorXHigh) &&
                    (tag == 3))
                {
                    // Preserve 4014 extra coordinate bits before the next Y byte.
                    state.tek().mode = TekParserMode::VectorYLow;
                    state.tek().xy4014 = state.tek().yl;
                    if (DEBUG)
                    {
                        std::println("4014 coordinates, overwrite last value");
                    }
                }

                if ((state.tek().mode == TekParserMode::VectorYLow) &&
                    (tag != 3))
                {
                    state.tek().mode = TekParserMode::VectorXHigh;
                }
                if ((state.tek().mode == TekParserMode::VectorXHigh) &&
                    (tag != 1))
                {
                    state.tek().mode = TekParserMode::VectorXLow;
                }
            }
            else
            {
                if (ch == 0)
                {
                    return;
                }
                if (ch == 29)
                {
                    state.tek().mode =
                        TekParserMode::MoveYHigh; // GS begins a new coordinate.
                }
                else if (ch == 28)
                {
                    state.tek().plotPointMode_ = 1;
                    state.tek().todo = 16 * state.tek().todo;
                }
                else if (DEBUG)
                {
                    std::println("Plot mode, unknown char {}, "
                                 "plotPointMode = {}",
                                 ch, state.tek().plotPointMode_);
                }
                return;
            }
        }


        // Dispatch using the updated mode after shared controls and coordinate bytes.

        switch (state.tek().mode)
        {
        case TekParserMode::MoveYHigh:
            state.tek().plotPointMode_ =
                0; // normal graphics mode, starting coordinates
            state.tek().yh = 32 * (ch & 31);
            state.tek().mode = nextCoordinateMode(state.tek().mode);
            if (DEBUG)
            {
                std::println("setting yh to {}", state.tek().yh);
            }
            break;
        case TekParserMode::MoveYLow:
            state.tek().yl = (ch & 31);
            state.tek().mode = nextCoordinateMode(state.tek().mode);
            if (DEBUG)
            {
                std::println("setting yl to {}", state.tek().yl);
            }
            break;
        case TekParserMode::MoveXHigh:
            if (tag == 1)
            {
                state.tek().xh = 32 * (ch & 31);
            }
            state.tek().mode = nextCoordinateMode(state.tek().mode);
            if (DEBUG)
            {
                std::println("setting xh to {}", state.tek().xh);
            }
            break;
        case TekParserMode::MoveXLow:
            state.tek().xl = (ch & 31);
            if (state.displayGeometry().windowWidth() != 1024)
            {
                int xb{state.tek().xy4014 & 3};
                const int x =
                    (int)(state.tek().efactor *
                          (double)(((state.tek().xh + state.tek().xl) << 2) +
                                   xb) /
                          4.0);
                int yb{(state.tek().xy4014 >> 2) & 3};
                const int y =
                    (int)(state.tek().efactor *
                          (double)(((state.tek().yh + state.tek().yl) << 2) +
                                   yb) /
                          4.0);
                state.coordinates().setCurrentPosition(x, y);
            }
            else
            {
                state.coordinates().setCurrentPosition(
                    state.tek().xh + state.tek().xl,
                    state.tek().yh + state.tek().yl);
            }
            state.tek().mode = nextCoordinateMode(state.tek().mode);
            tube_emulateDeflectionTime(state);
            if (DEBUG)
            {
                std::println("setting xl to {}", state.tek().xl);
            }
            if (DEBUG)
            {
                std::println(
                    "******************************************** Moving to "
                    "({},{})",
                    state.coordinates().currentX(),
                    state.coordinates().currentY());
            }
            break;
        case TekParserMode::VectorYHigh:
            if (ch == 29)
            {
                if (DEBUG)
                {
                    std::println("setting mode to 1");
                }
                state.tek().mode = TekParserMode::MoveYHigh;
            }
            else if (tag != 0)
            {
                state.tek().yh = 32 * (ch & 31);
                state.tek().mode = nextCoordinateMode(state.tek().mode);
            }
            else if (DEBUG)
            {
                std::println("case 5: tag is 0");
            }
            if (DEBUG)
            {
                std::println(">>>>>yh={}", state.tek().yh);
            }
            break;
        case TekParserMode::VectorYLow:
            state.tek().yl = (ch & 31);
            state.tek().mode = nextCoordinateMode(state.tek().mode);
            break;
            if (DEBUG)
            {
                std::println(">>>>>yl={}", state.tek().yl);
            }
        case TekParserMode::VectorXHigh:
            state.tek().xh = 32 * (ch & 31);
            state.tek().mode = nextCoordinateMode(state.tek().mode);
            break;
            if (DEBUG)
            {
                std::println(">>>>>xh={}", state.tek().xh);
            }
        case TekParserMode::VectorXLow:
            state.tek().xl = (ch & 31);
            if (state.displayGeometry().windowWidth() != 1024)
            {
                int xb{state.tek().xy4014 & 3};
                const int x =
                    (int)(state.tek().efactor *
                          (double)(((state.tek().xh + state.tek().xl) << 2) +
                                   xb) /
                          4.0);
                int yb{(state.tek().xy4014 >> 2) & 3};
                const int y =
                    (int)(state.tek().efactor *
                          (double)(((state.tek().yh + state.tek().yl) << 2) +
                                   yb) /
                          4.0);
                state.coordinates().setTargetPosition(x, y);
            }
            else
            {
                state.coordinates().setTargetPosition(
                    state.tek().xh + state.tek().xl,
                    state.tek().yh + state.tek().yl);
            }
            if (DEBUG)
            {
                std::println(">>>>>xl={}", state.tek().xl);
            }

            if (state.tek().plotPointMode_ > 0.0)
            {

                // draw the point
                tube_drawPoint(state, renderer);

                state.tek().todo--;
            }

            else
            {
                tube_drawVector(state, renderer);

                state.tek().todo--;
            }

            state.cursor().hide();

            state.coordinates().copyTargetToCurrent();
            if (state.tek().specialPlotMode_)
            {
                state.tek().mode = TekParserMode::IntensityControl;
            }
            else
            {
                state.tek().mode = TekParserMode::VectorYHigh;
            }
            break;
        case TekParserMode::Escape: // escape code handler
            escapeCodeHandler(state, renderer, ch);
            break;

        case TekParserMode::IgnoreCsi: // ignore ANSI CSI sequence ESC [ ...
                                       // final byte
            if ((ch >= 0x40) && (ch <= 0x7E))
            {
                state.tek().mode = TekParserMode::Alpha;
            }
            break;

        case TekParserMode::IgnoreOsc: // ignore ANSI OSC sequence ESC ] ... BEL
            if (ch == 7)
            {
                state.tek().mode = TekParserMode::Alpha;
            }
            break;

        case TekParserMode::IncrementalPlot: // incremental plot mode
            checkReturnToAlpha(state, ch);   // check for exit
            if (DEBUG)
            {
                std::println("Incremental plot mode, ch = {}, penDown = {}", ch,
                             state.tek().penDown);
            }
            if (ch == 32)
            {
                state.tek().penDown = 0;
            }
            else if (ch == 80)
            {
                state.tek().penDown = 1;
            }
            else if ((ch & 0x70) == 0x40)
            {
                if (ch & 4)
                {
                    state.coordinates().moveCurrentY(1);
                }
                if (ch & 1)
                {
                    state.coordinates().moveCurrentX(1);
                }
                if (ch & 8)
                {
                    state.coordinates().moveCurrentY(-1);
                }
                if (ch & 2)
                {
                    state.coordinates().moveCurrentX(-1);
                }
                if (DEBUG)
                {
                    std::println("point ({},{})",
                                 state.coordinates().currentX(),
                                 state.coordinates().currentY());
                }
                state.coordinates().setTargetToCurrent();
                if (state.tek().penDown)
                {
                    tube_drawPoint(state, renderer);
                }
            }
            else if (DEBUG)
            {
                std::println("Illegal byte 0x{:02X} in incremental plot", ch);
            }
            break;
        case TekParserMode::IntensityControl: // special plot mode
            tag = ch >> 5;
            if ((ch < 32) || (ch >= 126))
            {
                return;
            }
            if (DEBUG)
            {
                std::print("Tek intensity/focus control = {:c}: {}: ", ch, tag);
            }
            state.tek().defocussed_ = (tag == 1);
            state.tek().intensity_ = intensityTable[ch - 32];
            if (DEBUG)
            {
                std::println("defocussed = {}, intensity = {}%",
                             state.tek().defocussed_, state.tek().intensity_);
            }
            state.tek().mode = TekParserMode::VectorYHigh;
            break;
        case TekParserMode::Gin: // crosshair mode
            if (const auto pendingKey{state.tek().pendingGinKey()})
            { // key struck by user
                ginSend(state, *pendingKey);
                state.tek().mode = TekParserMode::Alpha;
                state.tek().todo = 0;
                if (DEBUG)
                {
                    std::println("GIN: key stroken by user, exiting GIN mode");
                }
                state.tek().endGinMode();
            }
            else
            {
                ginMode(state, renderer);
            }
            break;
        case TekParserMode::IgnoreUntilGroupSeparator:
            if (DEBUG)
            {
                std::println("Ignore until group separator, ch = {:02x}", ch);
            }
            if (ch == 29)
            {
                state.tek().mode = TekParserMode::MoveYHigh;
            }
            break;
        case TekParserMode::Alpha: // handle ALPHA mode; 4014 user manual, table
                                   // page G-1
            // some characters are identical for all modes and handled elsewhere
            switch (ch)
            {
            case 0:
                break;
            case 8: // backspace
                state.coordinates().moveCurrentX(
                    -(state.displayGeometry().horizontalDotsPerCharacter()));
                checkLimits(state, renderer);
                break;
            case 9: // tab
                if (state.options().expandTabs())
                {
                    state.coordinates().moveCurrentX(
                        state.displayGeometry().horizontalDotsPerCharacter());
                }
                else
                {
                    state.coordinates().setCurrentX(
                        state.coordinates().currentX() -
                        (state.coordinates().currentX() %
                         (8 * state.displayGeometry()
                                  .horizontalDotsPerCharacter())) +
                        8 * state.displayGeometry()
                                .horizontalDotsPerCharacter());
                }
                checkLimits(state, renderer);
                break;
            case 11: // VT, move one line up
                state.coordinates().moveCurrentY(
                    state.displayGeometry().verticalDotsPerCharacter());
                checkLimits(state, renderer);
                break;
            case 23: // ctrl-w  screen dump
                tube_request_screenshot(state);
                break;

            default:
                if ((ch >= 32) && (ch < 127))
                { // printable character
                    checkLimits(state, renderer);
                    tube_drawCharacter(state, renderer, ch);
                    state.tek().todo -= 2;
                }
                break;
            }
            break;
        default:
            if (DEBUG)
            {
                std::println("Illegal mode - this is a tek4010decoder error "
                             "and should not happen");
            }
            break;
        }
    endDo:;
    } while (state.options().performanceMode() ||
             ((state.tek().todo > 0) &&
              ((std::chrono::steady_clock::now() -
                state.tek().startPaintTime) <
               state.frameTiming().drawBudget())));

    if (state.cursor().isVisible() && (tube_isInput(state) == 0))
    {
        tube_doCursor(state, renderer);
    }
}

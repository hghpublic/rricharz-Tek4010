/*
 * tube.c
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

// Facade that owns the emulator state and routes host lifecycle events.
module;

#include <chrono>
#include <errno.h>
#include <locale.h>
#include <memory>
#include <print>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

module tube;

import terminal_session;
import tube_renderer;
import :internal;

TubeCore::TubeCore() : state_(std::make_unique<TubeCoreState>()) {}

TubeCore::~TubeCore() = default;


TubeOptions TubeCore::initialize(int argc, char* argv[])
{
    TubeCoreState& state{*state_};
    std::println("tek4010 version 2.0.2");
    // Resolve mode-specific options before starting the child process or parser.
    const int firstArg{state.options().parseCommandLine(argc, argv)};
    if (state.options().aplEnabled())
    {
        tube_initializeAplInput(state);
    }

    fflush(stdout);
    if (!state.terminalSession().start(argc, argv, firstArg,
                                       state.options().usesPty(),
                                       state.options().eraseCharacter()))
    {
        exit(EXIT_FAILURE);
    }

    constexpr int REFERENCE_CHARACTER_INTERVAL_MICROSECONDS{100};
    constexpr int REFERENCE_BAUD_RATE{100000};
    const std::chrono::microseconds characterInterval{
        state.options().performanceMode()
            ? std::chrono::microseconds::zero()
            : std::chrono::microseconds{
                  REFERENCE_CHARACTER_INTERVAL_MICROSECONDS *
                  (REFERENCE_BAUD_RATE / state.options().baudRate())}};
    state.input().setCharacterInterval(characterInterval);

    state.renderLifecycle().requestPersistentClear();

    fflush(stdout);

    state.input().resetCharacterTimer(std::chrono::steady_clock::now());

    return TubeOptions{.fullScreen = state.options().fullScreen(),
                       .verticalFullScreen =
                           state.options().verticalFullScreen(),
                       .ardsMode = state.options().ardsMode(),
                       .aplEnabled = state.options().aplEnabled(),
                       .halfScreen = state.options().halfScreen(),
                       .hideCursor = state.options().hideCursor(),
                       .keyboardDisabled = state.options().keyboardDisabled(),
                       .performanceMode = state.options().performanceMode(),
                       .eraseCharacter = state.options().eraseCharacter()};
}


TubeTimerUpdate TubeCore::onTimerEvent()
// Polls the terminal process and reports host actions without managing GTK.
{
    TubeCoreState& state{*state_};
    TubeTimerUpdate update{};
    state.terminalSession().pollChildExit();

    // A completed child either ends its session or leaves the emulator open.
    if (state.terminalSession().childExited())
    {
        update.childExitStatus = state.terminalSession().childExitStatus();
        if (state.options().usesPty() || !state.options().noExit())
        {
            if (state.options().performanceMode() && tube_isInput(state))
            {
                update.redraw = true;
                return update;
            }
            std::println("Process has been terminated");
            update.requestQuit = true;
            return update;
        }

        if (state.process().markChildFinished())
        {
            update.processFinished = true;
        }

        if (state.options().waitSeconds())
        {
            if (state.process().shouldQuitAfterWait(
                    std::chrono::steady_clock::now(),
                    std::chrono::seconds{state.options().waitSeconds()}))
            {
                std::println("Process has been terminated after {} seconds",
                             state.options().waitSeconds());
                update.requestQuit = true;
                return update;
            }
        }
    }

    // Advance core-owned fade state and ask the host to redraw only when needed.
    state.renderLifecycle().advanceFrame();

    update.redraw =
        state.renderLifecycle().hasBrightSpot() || tube_isInput(state);
    return update;
}

void TubeCore::setWindowSize(int width, int height)
{
    state_->displayGeometry().setWindowSize(width, height);
}

bool TubeCore::hasBrightSpot() const
{
    return state_->renderLifecycle().hasBrightSpot();
}

bool TubeCore::isFastMode() const
{
    return state_->options().fastMode();
}

void TubeCore::enableFastMode()
{
    state_->options().setFastMode(true);
}

bool TubeCore::isChildExited() const
{
    return state_->terminalSession().childExited();
}

void TubeCore::requestClear()
{
    state_->renderLifecycle().requestPersistentClear();
}

void TubeCore::setAplMode(bool enabled)
{
    state_->characterSet().setAplMode(enabled);
}

void tube_request_screenshot(TubeCoreState& state)
{
    state.process().requestScreenshot();
}

bool TubeCore::takeScreenshotRequest()
{
    return state_->process().takeScreenshotRequest();
}

void tube_write_key(TubeCoreState& state, int ch)
{
    if (!state.terminalSession().writeByte(ch))
    {
        std::println(stderr, "Cannot write to terminal session: {}",
                     strerror(errno));
    }
}

void TubeCore::sendBreak()
{
    if (!state_->terminalSession().sendBreak())
    {
        std::println(stderr, "Cannot send terminal break: {}", strerror(errno));
    }
}

bool TubeCore::submitKey(int ch)
{
    TubeCoreState& state{*state_};
    // In GIN, a key is a local coordinate response rather than child input.
    if (state.tek().captureGinKey(ch))
    {
        return true;
    }

    if (state.characterSet().aplMode())
    {
        // APL table entries may encode a character as a low/high byte pair.
        ch = tube_translateKeyCode(state, ch);
        tube_write_key(state, ch & 0xFF);
        if (ch >> 8)
        {
            tube_write_key(state, 8);
            tube_write_key(state, ch >> 8);
        }
    }
    else
    {
        tube_write_key(state, ch);
    }

    return false;
}

int TubeCore::clicked(int button, int x, int y)
// Forward left-button clicks to Tek GIN; ARDS mode ignores pointer input.
{
    TubeCoreState& state{*state_};
    if (state.options().ardsMode())
    {
        return 0;
    }
    if (button == 1)
    {
        state.tek().clicked(state, x,
                            state.displayGeometry().windowHeight() - y);
        return 1;
    }
    return 0;
}

void TubeCore::drawTek(TubeRenderer& renderer, bool first)
{
    state_->tek().draw(*state_, renderer, first);
}

void TubeCore::drawArds(TubeRenderer& renderer, bool first)
{
    state_->ards().draw(*state_, renderer, first);
}

void TubeCore::quit()
// Release PTY streams and terminate/reap the child during host shutdown.
{
    state_->terminalSession().close();
}

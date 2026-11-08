/*
 * ards.c
 *
 * ARDS option for tek4010 graphics emulator
 *
 * Copyright 2019  Lars Brinkhoff
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

module;

#include <array>
#include <chrono>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

module tube:ards;

import tube_renderer;
import :internal;

// The ARDS decoder buffers fixed-length packets before applying the selected
// symbol, set-point, extended-vector, or short-vector operation.

void ArdsParser::drawCharacter(TubeCoreState& state, TubeRenderer& renderer,
                               char ch)
{
    state.coordinates().setCurrentPosition((int)(efactor_ * (x0_ + 540)),
                                           (int)(efactor_ * (y0_ + 707)));
    tube_drawCharacter(state, renderer, ch);
    x0_ +=
        (int)(state.displayGeometry().horizontalDotsPerCharacter() / efactor_);
}

void ArdsParser::drawVector(TubeCoreState& state, TubeRenderer& renderer)
{
    state.coordinates().setCurrentPosition((int)(efactor_ * (x0_ + 540)),
                                           (int)(efactor_ * (y0_ + 707)));
    state.coordinates().setTargetPosition((int)(efactor_ * (x2_ + 540)),
                                          (int)(efactor_ * (y2_ + 707)));
    tube_drawVector(state, renderer);
}

void ArdsParser::draw(TubeCoreState& state, TubeRenderer& renderer, bool first)
{
    int ch;

    if (first)
    {
        first = 0;
        // Fit the 1080-unit reference width and bound work per host frame.
        efactor_ = state.displayGeometry().windowWidth() / 1080.0;
        state.frameTiming().setDrawBudget(std::chrono::milliseconds{50});
        tube_changeCharacterSize(state, renderer, 80, 50, efactor_ * 1.1);
    }

    startPaintTime_ = std::chrono::steady_clock::now();

    state.cursor().show();

    // Clear the transient layer each frame and the persistent layer on request.
    tube_clearSecond(state, renderer);

    if (state.renderLifecycle().persistentClearRequested())
    {
        tube_clearPersistent(state, renderer);
    }

    tube_setupPainting(state, renderer, TubeFont::Standard);

    do
    {
        ch = tube_getInputChar(state);

        if (tube_isInput(state) == 0)
        {
        }

        if (ch == -1)
        {
            return; // Yield so the host can process updates.
        }

        // Fill backward so the complete packet matches the decoder's field order.
        if (args_ > 0)
        {
            data_[--args_] = ch;
        }

        // Control bytes discard partial packets and choose the next payload size.
        switch (ch)
        {
        case 007:
            mode_ = Mode::Symbol;
            args_ = 0;
            break;
        case 010:
            mode_ = Mode::Symbol;
            args_ = 0;
            x0_ -= state.displayGeometry().horizontalDotsPerCharacter();
            break;
        case 012:
            mode_ = Mode::Symbol;
            args_ = 0;
            y0_ -= state.displayGeometry().verticalDotsPerCharacter();
            break;
        case 014:
            mode_ = Mode::Symbol;
            args_ = 0;
            x0_ = -485;
            y0_ = 450;
            tube_clearPersistent(state, renderer);
            break;
        case 015:
            mode_ = Mode::Symbol;
            args_ = 0;
            x0_ = -479;
            break;
        case 034:
            mode_ = Mode::Symbol;
            args_ = 0;
            break;
        case 035:
            mode_ = Mode::SetPoint;
            args_ = 4;
            break;
        case 036:
            mode_ = Mode::ExtendedVector;
            args_ = 4;
            break;
        case 037:
            mode_ = Mode::ShortVector;
            args_ = 2;
            break;
        default:
            if (args_ == 0)
            {
                // Decode only complete packets; vectors are relative to the prior
                // point while set-point packets replace it.
                switch (mode_)
                {
                case Mode::Symbol:
                    drawCharacter(state, renderer, ch);
                    break;
                case Mode::SetPoint:
                    args_ = 4;
                    x0_ = (data_[3] & 076) >> 1;
                    x0_ |= (data_[2] & 037) << 5;
                    if (data_[3] & 1)
                    {
                        x0_ = -x0_;
                    }
                    y0_ = (data_[1] & 076) >> 1;
                    y0_ |= (data_[0] & 037) << 5;
                    if (data_[1] & 1)
                    {
                        y0_ = -y0_;
                    }
                    break;
                case Mode::ExtendedVector:
                    args_ = 4;
                    x2_ = (data_[3] & 076) >> 1;
                    x2_ |= (data_[2] & 037) << 5;
                    if (data_[3] & 1)
                    {
                        x2_ = -x2_;
                    }
                    x2_ += x0_;
                    y2_ = (data_[1] & 076) >> 1;
                    y2_ |= (data_[0] & 037) << 5;
                    if (data_[1] & 1)
                    {
                        y2_ = -y2_;
                    }
                    y2_ += y0_;
                    if (data_[2] & 040)
                        ; // The invisibility bit suppresses this vector.
                    else
                    {
                        drawVector(state, renderer);
                    }
                    if (data_[0] & 040)
                        ; // The dotted-style flag is not rendered here.
                    x0_ = x2_;
                    y0_ = y2_;
                    break;
                case Mode::ShortVector:
                    args_ = 2;
                    x2_ = (data_[1] & 076) >> 1;
                    if (data_[1] & 1)
                    {
                        x2_ = -x2_;
                    }
                    x2_ += x0_;
                    y2_ = (data_[0] & 076) >> 1;
                    if (data_[0] & 1)
                    {
                        y2_ = -y2_;
                    }
                    y2_ += y0_;
                    drawVector(state, renderer);
                    x0_ = x2_;
                    y0_ = y2_;
                    break;
                }
            }
            break;
        }
    // Drain input in performance mode; otherwise yield when the frame budget ends.
    } while (state.options().performanceMode() ||
             ((std::chrono::steady_clock::now() - startPaintTime_) <
              state.frameTiming().drawBudget()));

    // Draw the cursor only when no byte is currently ready for parsing.
    if (state.cursor().isVisible() && (tube_isInput(state) == 0))
    {
        tube_doCursor(state, renderer);
    }
}

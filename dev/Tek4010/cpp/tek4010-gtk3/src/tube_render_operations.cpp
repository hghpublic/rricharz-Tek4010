// Translates core state changes into backend-neutral renderer operations.
module;

#include <unistd.h>

module tube;

import tube_renderer;
import :internal;

static constexpr int BRIGHT_TIME_MSEC{1000};
static constexpr int BRIGHT_COUNTER_INIT =
    (BRIGHT_TIME_MSEC + TIME_INTERVAL - 1) / TIME_INTERVAL;
static constexpr int DEFLECTION_DISTANCE_THRESHOLD{300};
static constexpr int DEFLECTION_DELAY_MICROSECONDS_PER_UNIT{2};

TubeRenderState tube_renderState(const TubeCoreState& state)
{
    // Give each backend call one backend-neutral snapshot of the current beam.
    return TubeRenderState{
        .windowWidth = state.displayGeometry().windowWidth(),
        .windowHeight = state.displayGeometry().windowHeight(),
        .argFast = state.options().fastMode(),
        .tube_x0 = state.coordinates().currentX(),
        .tube_y0 = state.coordinates().currentY(),
        .tube_x2 = state.coordinates().targetX(),
        .tube_y2 = state.coordinates().targetY(),
        .hDotsPerChar = state.displayGeometry().horizontalDotsPerCharacter(),
        .vDotsPerChar = state.displayGeometry().verticalDotsPerCharacter(),
        .intensity = state.tek().intensity(),
        .defocussed = state.tek().isDefocussed(),
        .pensize = state.strokeStyle().penSize(),
        .ltype = state.strokeStyle().lineType(),
        .writeThroughMode = state.tek().writeThroughMode(),
    };
}

void tube_doCursor(TubeCoreState& state, TubeRenderer& renderer)
{
    renderer.drawCursor(tube_renderState(state));
}

void tube_clearPersistent(TubeCoreState& state, TubeRenderer& renderer)
{
    renderer.clearPersistent();
    // A protocol clear resets logical beam, plot style, and phosphor state too.
    state.renderLifecycle().completePersistentClear();
    state.coordinates().resetForClear(
        state.displayGeometry().windowHeight() -
        state.displayGeometry().verticalDotsPerCharacter());
    state.renderLifecycle().showBrightSpot(BRIGHT_COUNTER_INIT);
    state.tek().resetPlotModes();
    state.strokeStyle().setLineType(LineType::Solid);
    state.tek().resetIntensityControl();
}

void tube_clearSecond(TubeCoreState& state, TubeRenderer& renderer)
{
    renderer.clearTemporary(state.options().fastMode());
}

void tube_crosshair(TubeCoreState& state, TubeRenderer& renderer)
{
    renderer.crosshair(tube_renderState(state));
}

void tube_drawCharacter(TubeCoreState& state, TubeRenderer& renderer, char ch)
{
    renderer.drawCharacter(tube_renderState(state), ch,
                           state.characterSet().aplMode());
    // Text consumes one character cell and restarts the phosphor hold interval.
    state.coordinates().moveCurrentX(
        state.displayGeometry().horizontalDotsPerCharacter());
    state.renderLifecycle().showBrightSpot(BRIGHT_COUNTER_INIT);
}

void tube_drawPoint(TubeCoreState& state, TubeRenderer& renderer)
{
    renderer.drawPoint(tube_renderState(state));
    state.renderLifecycle().showBrightSpot(BRIGHT_COUNTER_INIT);
}

void tube_drawVector(TubeCoreState& state, TubeRenderer& renderer)
{
    tube_emulateDeflectionTime(state);
    // Give a zero-length vector a visible extent so the renderer can show it.
    if ((state.coordinates().targetX() == state.coordinates().currentX()) &&
        (state.coordinates().targetY() == state.coordinates().currentY()))
    {
        state.coordinates().moveCurrentX(1);
    }

    renderer.drawVector(tube_renderState(state));
    state.renderLifecycle().showBrightSpot(BRIGHT_COUNTER_INIT);
}

void tube_setupPainting(TubeCoreState& state, TubeRenderer& renderer,
                        TubeFont font)
{
    renderer.setupPainting(state.strokeStyle().penSize(), font);
}

void tube_changeCharacterSize(TubeCoreState& state, TubeRenderer& renderer,
                              int charsPerLine, int charsPerPage,
                              double fontSize)
{
    static constexpr double STANDARD_FONT_SIZE{18.0};
    static constexpr double APL_FONT_SIZE{20.0};

    // Keep parser cell spacing and renderer font metrics in the same scale.
    state.displayGeometry().setCharacterGrid(charsPerLine, charsPerPage);
    state.coordinates().setLeftMargin(0);

    const double fontScale =
        state.options().ardsMode() ? APL_FONT_SIZE : STANDARD_FONT_SIZE;
    renderer.setCharacterSize(static_cast<int>(fontSize * fontScale),
                              state.options().ardsMode());
}

void tube_emulateDeflectionTime(TubeCoreState& state)
{
    if (state.options().performanceMode())
    {
        // Performance mode skips the simulated physical beam-slew delay.
        return;
    }

    int l{state.coordinates().targetX() - state.coordinates().currentX()};
    if ((state.coordinates().currentX() - state.coordinates().targetX()) > l)
    {
        l = state.coordinates().currentX() - state.coordinates().targetX();
    }
    if ((state.coordinates().targetY() - state.coordinates().currentY()) > l)
    {
        l = state.coordinates().targetY() - state.coordinates().currentY();
    }
    if ((state.coordinates().currentY() - state.coordinates().targetY()) > l)
    {
        l = state.coordinates().currentY() - state.coordinates().targetY();
    }
    if (l > DEFLECTION_DISTANCE_THRESHOLD)
    {
        usleep((l - DEFLECTION_DISTANCE_THRESHOLD) *
               DEFLECTION_DELAY_MICROSECONDS_PER_UNIT);
    }
}

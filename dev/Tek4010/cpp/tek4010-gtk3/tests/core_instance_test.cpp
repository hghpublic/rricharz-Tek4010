#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

import tube;

namespace
{
// Capture parser-emitted characters without a display server or Cairo.
class RecordingRenderer final : public TubeRenderer
{
public:
    void drawCursor(const TubeRenderState&) override {}
    void clearPersistent() override {}
    void clearTemporary(bool) override {}
    void drawCharacter(const TubeRenderState&, char ch, bool) override
    {
        characters.push_back(ch);
    }
    void drawPoint(const TubeRenderState&) override {}
    void crosshair(const TubeRenderState&) override {}
    void drawVector(const TubeRenderState&) override {}
    void setupPainting(double, TubeFont) override {}
    void setCharacterSize(int, bool) override {}

    std::vector<char> characters;
};
}

// What: verify that fast-mode changes stay local to each TubeCore.
// Why: leaked state can unexpectedly change another emulator's rendering.
TEST_CASE("TubeCore instances keep fast mode isolated", "[core]")
{
    // Arrange: create two cores with independent default state.
    TubeCore first;
    TubeCore second;

    // Assert: neither core starts in fast mode.
    CHECK_FALSE(first.isFastMode());
    CHECK_FALSE(second.isFastMode());

    // Act: enable fast mode on only the first core.
    first.enableFastMode();
    // Assert: the second core remains unchanged.
    CHECK(first.isFastMode());
    CHECK_FALSE(second.isFastMode());

    // Act: enable fast mode on the second core as well.
    second.enableFastMode();
    // Assert: both per-instance settings are retained.
    CHECK(first.isFastMode());
    CHECK(second.isFastMode());
}

// What: verify option defaults and setter effects.
// Why: incorrect configuration can change CLI and host behavior silently.
TEST_CASE("TubeCoreOptions defaults and setters", "[options]")
{
    // Arrange: start with freshly constructed options.
    TubeCoreOptions options;
    // Assert: the default behavior remains the documented baseline.
    CHECK(options.noExit());
    CHECK(options.baudRate() == 19200);
    CHECK_FALSE(options.ardsMode());
    CHECK_FALSE(options.aplEnabled());
    CHECK(options.autoClear());
    CHECK_FALSE(options.expandTabs());
    CHECK_FALSE(options.keepCharacterSize());
    CHECK_FALSE(options.fastMode());
    CHECK_FALSE(options.performanceMode());

    // Act: change each supported option through its public setter.
    options.setBaudRate(9600);
    options.setNoExit(false);
    options.setArdsMode(true);
    options.setAplEnabled(true);
    options.setAutoClear(false);
    options.setExpandTabs(true);
    options.setKeepCharacterSize(true);
    options.setFastMode(true);
    options.setPerformanceMode(true);

    // Assert: every requested option change is observable.
    CHECK(options.baudRate() == 9600);
    CHECK_FALSE(options.noExit());
    CHECK(options.ardsMode());
    CHECK(options.aplEnabled());
    CHECK_FALSE(options.autoClear());
    CHECK(options.expandTabs());
    CHECK(options.keepCharacterSize());
    CHECK(options.fastMode());
    CHECK(options.performanceMode());
}

// What: verify normalization of ordinary and signal-terminated child statuses.
// Why: losing failure codes makes the emulator report unsuccessful commands as
// successful or return the wrong status to its caller.
TEST_CASE("Terminal child wait status preserves failures", "[process]")
{
    // Arrange: create a child that exits normally with a known failure status.
    const pid_t exitChild{fork()};
    REQUIRE(exitChild >= 0);
    if (exitChild == 0)
    {
        _exit(7);
    }

    // Act: collect the child's native status with waitpid.
    int exitStatus{};
    pid_t waited{};
    do
    {
        waited = waitpid(exitChild, &exitStatus, 0);
    } while ((waited == -1) && (errno == EINTR));
    // Assert: waitpid reaps that child and normalization preserves its status.
    REQUIRE(waited == exitChild);
    CHECK(TerminalSession::normalizeWaitStatus(exitStatus) == 7);

    // Arrange: create a second child that terminates itself with a signal.
    const pid_t signalChild{fork()};
    REQUIRE(signalChild >= 0);
    if (signalChild == 0)
    {
        kill(getpid(), SIGKILL);
        _exit(EXIT_FAILURE);
    }

    // Act: collect the signal-terminated child's native status.
    int signalStatus{};
    do
    {
        waited = waitpid(signalChild, &signalStatus, 0);
    } while ((waited == -1) && (errno == EINTR));
    // Assert: signal termination uses the conventional 128 + signal mapping.
    REQUIRE(waited == signalChild);
    CHECK(TerminalSession::normalizeWaitStatus(signalStatus) ==
          128 + SIGKILL);
}

// What: verify CLI flags are parsed before the command argument.
// Why: forwarding emulator flags to the child would launch the wrong command.
TEST_CASE("Command-line options are parsed", "[options][cli]")
{
    // Arrange: place emulator options before the child command in argv.
    char program[]{"tek4010"};
    char fastOption[]{"-fast"};
    char baudOption[]{"-b9600"};
    char command[]{"shell"};
    char* arguments[]{program, fastOption, baudOption, command};

    TubeCoreOptions options;
    // Act: parse and locate the first argument that belongs to the child.
    REQUIRE(options.parseCommandLine(4, arguments) == 3);
    // Assert: the flags were applied and the command does not use a PTY.
    CHECK(options.fastMode());
    CHECK(options.baudRate() == 9600);
    CHECK_FALSE(options.usesPty());
}

// What: verify performance mode bypasses simulated beam-deflection delays.
// Why: a long vector must not stall input or redraw processing in this mode.
TEST_CASE("Performance mode disables emulated drawing delays",
          "[options][cli]")
{
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

    // Arrange: request performance mode and a low baud rate through the CLI.
    char program[]{"tek4010"};
    char performanceOption[]{"-performance"};
    char baudOption[]{"-b300"};
    char command[]{"shell"};
    char* arguments[]{program, performanceOption, baudOption, command};

    TubeCoreOptions options;
    // Act: parse the options and verify the mode's associated settings.
    REQUIRE(options.parseCommandLine(4, arguments) == 3);
    CHECK(options.performanceMode());
    CHECK(options.fastMode());
    CHECK(options.baudRate() == 300);

    // Arrange: use a long deflection that would otherwise incur a delay.
    TubeCoreState state;
    state.options().setPerformanceMode(true);
    state.coordinates().setTargetPosition(200000, 0);

    const Clock::time_point started{Clock::now()};
    // Act: emulate the deflection under performance mode.
    tube_emulateDeflectionTime(state);
    // Assert: the emulation returns promptly rather than sleeping for distance.
    CHECK((Clock::now() - started) < 250ms);
}

// What: verify explicit and default values for the wait option.
// Why: a wrong wait duration can close the display too early or leave it open.
TEST_CASE("Command-line wait option accepts a value or its default",
          "[options][cli]")
{
    // Arrange: provide an explicit wait duration before the child command.
    char program[]{"tek4010"};
    char waitOption[]{"-wait"};
    char waitValue[]{"12"};
    char command[]{"shell"};
    char* arguments[]{program, waitOption, waitValue, command};

    TubeCoreOptions options;
    // Act: parse the explicit duration.
    REQUIRE(options.parseCommandLine(4, arguments) == 3);
    // Assert: the duration is applied without enabling a PTY.
    CHECK(options.waitSeconds() == 12);
    CHECK_FALSE(options.usesPty());

    // Arrange: omit the wait value while leaving the child command in argv.
    char* defaultArguments[]{program, waitOption, command};
    TubeCoreOptions defaultOptions;
    // Act: parse the option with its default duration.
    REQUIRE(defaultOptions.parseCommandLine(3, defaultArguments) == 2);
    // Assert: the default duration is applied.
    CHECK(defaultOptions.waitSeconds() == 3);
    CHECK_FALSE(defaultOptions.usesPty());
}

// What: verify host dimensions are divided into the requested character grid.
// Why: incorrect spacing misplaces text and graphics on the emulated display.
TEST_CASE("Display geometry computes the character grid", "[state]")
{
    // Arrange: choose host dimensions and a fixed row/column count.
    TubeDisplayGeometry geometry;
    // Act: configure the host size and logical character grid.
    geometry.setWindowSize(800, 600);
    geometry.setCharacterGrid(80, 40);

    // Assert: the host size and derived per-character spacing are consistent.
    CHECK(geometry.windowWidth() == 800);
    CHECK(geometry.windowHeight() == 600);
    CHECK(geometry.horizontalDotsPerCharacter() == 10);
    CHECK(geometry.verticalDotsPerCharacter() == 15);
}

// What: verify clear requests and bright-spot lifetimes across frame updates.
// Why: stale clears or premature spot expiry can corrupt visible phosphor state.
TEST_CASE("Render lifecycle tracks clears and bright spots", "[state]")
{
    // Arrange: begin with no clear request and no bright frames.
    TubeRenderLifecycle lifecycle;
    CHECK_FALSE(lifecycle.persistentClearRequested());
    CHECK_FALSE(lifecycle.hasBrightSpot());

    // Act: request a persistent clear and start a two-frame bright spot.
    lifecycle.requestPersistentClear();
    lifecycle.showBrightSpot(2);
    // Assert: both lifecycle conditions become active.
    CHECK(lifecycle.persistentClearRequested());
    CHECK(lifecycle.hasBrightSpot());

    // Act: complete the clear and advance one frame.
    lifecycle.completePersistentClear();
    lifecycle.advanceFrame();
    // Assert: the clear is consumed while the bright spot remains.
    CHECK_FALSE(lifecycle.persistentClearRequested());
    CHECK(lifecycle.hasBrightSpot());

    // Act: advance the second frame.
    lifecycle.advanceFrame();
    // Assert: the bright spot expires at the requested frame count.
    CHECK_FALSE(lifecycle.hasBrightSpot());
}

// What: verify the cursor state can be shown and hidden.
// Why: stale visibility state causes the renderer to draw an unwanted cursor.
TEST_CASE("Cursor visibility can be changed", "[state]")
{
    // Arrange: a newly constructed cursor is hidden.
    TubeCursorState cursor;
    CHECK_FALSE(cursor.isVisible());

    // Act: show the cursor.
    cursor.show();
    // Assert: visibility is observable to the renderer.
    CHECK(cursor.isVisible());

    // Act: hide the cursor.
    cursor.hide();
    // Assert: the visible state is cleared.
    CHECK_FALSE(cursor.isVisible());
}

// What: verify APL character-set mode can be enabled and disabled.
// Why: a stale mode changes how later input bytes are translated and rendered.
TEST_CASE("Character set mode can be changed", "[state]")
{
    // Arrange: begin in the standard character set.
    TubeCharacterSetState characterSet;
    CHECK_FALSE(characterSet.aplMode());

    // Act: enable APL mode.
    characterSet.setAplMode(true);
    // Assert: the mode change is observable.
    CHECK(characterSet.aplMode());

    // Act: return to the standard character set.
    characterSet.setAplMode(false);
    // Assert: the standard mode is restored.
    CHECK_FALSE(characterSet.aplMode());
}

// What: verify GIN captures zero and other control-byte key values.
// Why: treating zero as absent input would drop valid keys during coordinate
// reporting.
TEST_CASE("GIN input preserves control key values", "[state][gin]")
{
    // Arrange: create a parser in its default, non-GIN state.
    Tek4010State tek;

    // Act/Assert: input outside GIN mode is not captured.
    CHECK_FALSE(tek.captureGinKey(0));
    // Act: enter GIN mode and submit a zero byte.
    tek.beginGinMode();
    CHECK(tek.ginModeActive());

    CHECK(tek.captureGinKey(0));
    // Assert: zero is present as a key value, not confused with no value.
    REQUIRE(tek.pendingGinKey().has_value());
    CHECK(*tek.pendingGinKey() == 0);

    // Act: capture a second control key while GIN remains active.
    CHECK(tek.captureGinKey(1));
    // Assert: the pending value is the second key, not an empty-input marker.
    REQUIRE(tek.pendingGinKey().has_value());
    CHECK(*tek.pendingGinKey() == 1);

    // Act: leave GIN mode.
    tek.endGinMode();
    // Assert: GIN is inactive and no pending key remains.
    CHECK_FALSE(tek.ginModeActive());
    CHECK_FALSE(tek.pendingGinKey().has_value());
}

// What: verify the Tek parser consumes queued alpha input and emits a glyph.
// Why: parser behavior must be testable and responsive without a live PTY.
TEST_CASE("Tek parser draws queued alpha input", "[parser]")
{
    // Arrange: set display geometry and queue one byte for the parser.
    TubeCoreState state;
    state.displayGeometry().setWindowSize(800, 600);
    state.displayGeometry().setCharacterGrid(80, 40);
    state.input().enqueueInputByte('A');
    RecordingRenderer renderer;

    // Act: process the queued byte through the real Tek parser.
    state.tek().draw(state, renderer, false);

    // Assert: the renderer receives the glyph, the beam advances, and input is
    // consumed exactly once.
    REQUIRE(renderer.characters.size() == 1);
    CHECK(renderer.characters.front() == 'A');
    CHECK(state.coordinates().currentX() == 10);
    CHECK_FALSE(state.input().hasQueuedInput());
}

// What: verify the frame timing object stores a parser-selected draw budget.
// Why: a lost budget changes how much work is processed before host updates.
TEST_CASE("Frame timing stores a draw budget", "[state]")
{
    using namespace std::chrono_literals;

    // Arrange: start with an unset draw budget.
    TubeFrameTiming frameTiming;
    CHECK(frameTiming.drawBudget() == 0ms);

    // Act: configure the frame budget.
    frameTiming.setDrawBudget(50ms);
    // Assert: the configured budget is retained.
    CHECK(frameTiming.drawBudget() == 50ms);
}

// What: verify pen size and line type can be configured independently.
// Why: incorrect stroke state changes the appearance of later Tek vectors.
TEST_CASE("Stroke style stores pen size and line type", "[state]")
{
    // Arrange: use the default stroke style.
    TubeStrokeStyle strokeStyle;
    CHECK(strokeStyle.penSize() == 1.0);
    CHECK(strokeStyle.lineType() == LineType::Solid);

    // Act: set a non-default size and dash pattern.
    strokeStyle.setPenSize(2.5);
    strokeStyle.setLineType(LineType::Dotted);
    // Assert: both renderer-facing properties reflect the new style.
    CHECK(strokeStyle.penSize() == 2.5);
    CHECK(strokeStyle.lineType() == LineType::Dotted);
}

// What: verify beam/target movement, copying, and clear reset semantics.
// Why: mixing current and target coordinates can draw vectors from the wrong
// point or leave the next plot at a stale position.
TEST_CASE("Coordinates move, copy, and reset positions", "[state]")
{
    // Arrange: start with a fresh coordinate state.
    TubeCoordinates coordinates;
    CHECK(coordinates.currentX() == 0);
    CHECK(coordinates.currentY() == 0);
    CHECK(coordinates.targetX() == 0);
    CHECK(coordinates.targetY() == 0);
    CHECK(coordinates.leftMargin() == 0);

    // Act: set distinct beam/target positions, then move only the beam.
    coordinates.setCurrentPosition(10, 20);
    coordinates.setTargetPosition(30, 40);
    coordinates.setLeftMargin(5);
    coordinates.moveCurrentX(-2);
    coordinates.moveCurrentY(3);
    // Assert: relative movement leaves the target and margin unchanged.
    CHECK(coordinates.currentX() == 8);
    CHECK(coordinates.currentY() == 23);
    CHECK(coordinates.targetX() == 30);
    CHECK(coordinates.targetY() == 40);
    CHECK(coordinates.leftMargin() == 5);

    // Act: copy the beam to the target, move it, then restore from the target.
    coordinates.setTargetToCurrent();
    coordinates.moveCurrentX(1);
    coordinates.copyTargetToCurrent();
    // Assert: copying the target restores the earlier beam position.
    CHECK(coordinates.currentX() == 8);
    CHECK(coordinates.currentY() == 23);

    // Act: reset coordinates for a persistent clear.
    coordinates.resetForClear(500);
    // Assert: both positions share the new baseline and the margin is cleared.
    CHECK(coordinates.currentX() == 0);
    CHECK(coordinates.currentY() == 500);
    CHECK(coordinates.targetX() == 0);
    CHECK(coordinates.targetY() == 500);
    CHECK(coordinates.leftMargin() == 0);
}

// What: verify key translation, capacity limits, and character pacing.
// Why: bad mappings corrupt keys; incorrect pacing drops bytes or stalls input.
TEST_CASE("Input state translates keys and enforces character timing",
          "[state]")
{
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

    // Arrange: add one mapping and verify unmapped input retains its identity.
    TubeInputState input;
    CHECK(input.addKeyTranslation(0, 'a', 'b'));
    CHECK(input.translateKeyCode('a') == 'b');
    CHECK(input.translateKeyCode('c') == 'c');
    CHECK_FALSE(input.addKeyTranslation(TUBE_MAX_KEYCODES, 'x', 'y'));

    // Act: clear mappings and verify translation falls back to the input byte.
    input.clearKeyTranslations();
    CHECK(input.translateKeyCode('a') == 'a');

    // Arrange: fill the fixed-size table through its final valid index.
    for (int i{0}; i < TUBE_MAX_KEYCODES; i++)
    {
        CHECK(input.addKeyTranslation(i, i + 1, i + 2));
    }
    // Act: query the final slot and try to add one more mapping.
    // Assert: the final slot works and the excess entry is rejected.
    CHECK(input.translateKeyCode(TUBE_MAX_KEYCODES) ==
          TUBE_MAX_KEYCODES + 1);
    CHECK_FALSE(
        input.addKeyTranslation(TUBE_MAX_KEYCODES, 'x', 'y'));

    // Arrange: establish a deterministic ten-microsecond character interval.
    const Clock::time_point inputStart{};
    input.setCharacterInterval(10us);
    input.resetCharacterTimer(inputStart);
    // Act: record a byte before the pacing deadline.
    input.recordCharacter(inputStart + 7us);
    // Assert: input is delayed before the interval but allowed at its boundary.
    CHECK(input.shouldDelayCharacter(inputStart + 9us));
    CHECK_FALSE(input.shouldDelayCharacter(inputStart + 10us));

    // Act: reset the pacing timer at a later point.
    input.resetCharacterTimer(inputStart + 100us);
    // Assert: elapsed time is measured from the new origin.
    CHECK(input.characterTimeSinceReset(inputStart + 125us) == 25us);
}

// What: verify the APL loader accepts a full table and rejects an oversized one.
// Why: off-by-one handling can silently lose a valid mapping or overrun capacity.
TEST_CASE("APL key translation loader accepts exactly its capacity",
          "[input][apl]")
{
    // Arrange: serialize exactly the maximum number of key mappings.
    std::ostringstream entries;
    for (int i{0}; i < TUBE_MAX_KEYCODES; i++)
    {
        entries << i + 1 << ' ' << i + 2 << '\n';
    }

    TubeInputState input;
    std::istringstream fullTable{entries.str()};
    // Act: load the table at its supported capacity.
    REQUIRE(tube_loadKeyTranslationTable(input, fullTable));
    // Assert: loading succeeds and the final mapping is available.
    CHECK(input.translateKeyCode(TUBE_MAX_KEYCODES) ==
          TUBE_MAX_KEYCODES + 1);

    // Arrange: append one mapping beyond the supported capacity.
    entries << TUBE_MAX_KEYCODES + 1 << ' ' << TUBE_MAX_KEYCODES + 2 << '\n';
    std::istringstream oversizedTable{entries.str()};
    // Act: attempt to load the oversized table.
    CHECK_FALSE(tube_loadKeyTranslationTable(input, oversizedTable));
    // Assert: the retained capacity boundary remains usable.
    CHECK(input.translateKeyCode(TUBE_MAX_KEYCODES) ==
          TUBE_MAX_KEYCODES + 1);
}

// What: verify screenshot consumption, child-finished transitions, and wait
// deadlines.
// Why: repeated requests or an incorrect deadline can duplicate screenshots or
// quit the host too early/late.
TEST_CASE("Process state consumes screenshot requests and tracks exit wait",
          "[state]")
{
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

    // Arrange: request one screenshot.
    TubeProcessState process;
    process.requestScreenshot();
    // Act/Assert: the first consume returns the pending request.
    CHECK(process.takeScreenshotRequest());
    // Act/Assert: a second consume confirms the request was cleared.
    CHECK_FALSE(process.takeScreenshotRequest());

    // Act/Assert: only the first child-finished transition is reported.
    CHECK(process.markChildFinished());
    CHECK_FALSE(process.markChildFinished());

    // Arrange: use a fixed clock origin so the wait boundary is deterministic.
    const Clock::time_point waitStart{};
    // Act/Assert: the process remains pending before the deadline and quits at it.
    CHECK_FALSE(process.shouldQuitAfterWait(waitStart, 1s));
    CHECK_FALSE(process.shouldQuitAfterWait(waitStart + 999ms, 1s));
    CHECK(process.shouldQuitAfterWait(waitStart + 1s, 1s));
}

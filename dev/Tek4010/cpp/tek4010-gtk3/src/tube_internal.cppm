module;

#include <array>
#include <chrono>
#include <deque>
#include <iosfwd>
#include <optional>
#include <stdio.h>

// State and helpers shared by tube's implementation partitions.
// GTK and Cairo remain outside this state layer.
export module tube:internal;

import terminal_session;
import tube_renderer;

export inline constexpr char APL_FONT[]{"APL385 Unicode"};
export inline constexpr int TUBE_MAX_KEYCODES{256};
export inline constexpr int TUBE_ESCDEBUG_LEN{16};

export {

    // Converts host window dimensions into the emulator's logical character grid.
    class TubeDisplayGeometry
    {
    public:
        int windowWidth() const;
        int windowHeight() const;
        int horizontalDotsPerCharacter() const;
        int verticalDotsPerCharacter() const;
        void setWindowSize(int width, int height);
        void setCharacterGrid(int charactersPerLine, int charactersPerPage);

    private:
        int windowWidth_{0};
        int windowHeight_{0};
        int horizontalDotsPerCharacter_{0};
        int verticalDotsPerCharacter_{0};
    };

    // Tracks persistent clears and the remaining phosphor-bright frames.
    class TubeRenderLifecycle
    {
    public:
        void requestPersistentClear();
        bool persistentClearRequested() const;
        void completePersistentClear();
        void showBrightSpot(int frameCount);
        bool hasBrightSpot() const;
        void advanceFrame();

    private:
        bool persistentClearRequested_{false};
        int brightFramesRemaining_{0};
    };

    // Host cursor visibility is kept separate from Tek drawing state.
    class TubeCursorState
    {
    public:
        void show();
        void hide();
        bool isVisible() const;

    private:
        bool visible_{false};
    };

    // Tracks whether input and rendered characters use the APL mapping.
    class TubeCharacterSetState
    {
    public:
        bool aplMode() const;
        void setAplMode(bool enabled);

    private:
        bool aplMode_{false};
    };

    // Stores the parser's per-frame drawing budget.
    class TubeFrameTiming
    {
    public:
        std::chrono::milliseconds drawBudget() const;
        void setDrawBudget(std::chrono::milliseconds budget);

    private:
        std::chrono::milliseconds drawBudget_{};
    };

    // Rendering style shared by Tek parser operations.
    class TubeStrokeStyle
    {
    public:
        double penSize() const;
        void setPenSize(double penSize);
        LineType lineType() const;
        void setLineType(LineType lineType);

    private:
        double penSize_{1.0};
        LineType lineType_{LineType::Solid};
    };

    // Current beam position, pending target, and alpha-mode left margin.
    class TubeCoordinates
    {
    public:
        int currentX() const;
        int currentY() const;
        int targetX() const;
        int targetY() const;
        int leftMargin() const;
        void setCurrentX(int x);
        void setCurrentY(int y);
        void setTargetX(int x);
        void setTargetY(int y);
        void setCurrentPosition(int x, int y);
        void setTargetPosition(int x, int y);
        void setTargetToCurrent();
        void copyTargetToCurrent();
        void moveCurrentX(int delta);
        void moveCurrentY(int delta);
        void setLeftMargin(int margin);
        void resetForClear(int baselineY);

    private:
        int currentX_{0};
        int currentY_{0};
        int targetX_{0};
        int targetY_{0};
        int leftMargin_{0};
    };

    struct TubeCoreState;

    // Incremental Tek byte-parser state; coordinate modes consume one byte each.
    enum class TekParserMode : int
    {
        Alpha = 0,
        MoveYHigh = 1,
        MoveYLow = 2,
        MoveXHigh = 3,
        MoveXLow = 4,
        VectorYHigh = 5,
        VectorYLow = 6,
        VectorXHigh = 7,
        VectorXLow = 8,
        Escape = 30,
        IgnoreCsi = 31,
        IgnoreOsc = 32,
        IncrementalPlot = 40,
        IntensityControl = 50,
        Gin = 60,
        IgnoreUntilGroupSeparator = 101
    };

    // Parser state persists across draw calls as PTY bytes arrive incrementally.
    class Tek4010State
    {
    public:
        void draw(TubeCoreState& state, TubeRenderer& renderer, bool first);
        void clicked(TubeCoreState& state, int x, int y);
        bool captureGinKey(int ch);
        bool ginModeActive() const;
        void beginGinMode();
        void endGinMode();
        std::optional<int> pendingGinKey() const;
        void resetPlotModes();
        void resetIntensityControl();
        bool writeThroughMode() const;
        bool isDefocussed() const;
        int intensity() const;

    private:
        void checkLimits(TubeCoreState& state, TubeRenderer& renderer);
        void bell(TubeCoreState& state);
        void sendCoordinates(TubeCoreState& state);
        void enqMode(TubeCoreState& state);
        void ginMode(TubeCoreState& state, TubeRenderer& renderer);
        void ginSend(TubeCoreState& state, int ch);
        void startGinEchoSuppression(int count);
        bool suppressGinEcho(int ch);
        void escapeCodeHandler(TubeCoreState& state, TubeRenderer& renderer,
                               int ch);
        int checkReturnToAlpha(TubeCoreState& state, int ch);

        TekParserMode mode{TekParserMode::Alpha};
        TekParserMode saveMode{TekParserMode::Alpha};
        int penDown{1};
        std::chrono::steady_clock::time_point startPaintTime{};
        int xh{0};
        int xl{0};
        int yh{0};
        int yl{0};
        int xy4014{0};
        long todo{0};
        double efactor{0.0};
        std::array<int, 6> ginCharacter{};
        bool ginModeActive_{false};
        std::optional<int> pendingGinKey_{};
        int ginEchoCount_{0};
        int plotPointMode_{0};
        int specialPlotMode_{0};
        bool writeThroughMode_{false};
        bool defocussed_{false};
        int intensity_{100};
    };

    // ARDS packet parser state persists across draw calls like the Tek parser.
    class ArdsParser
    {
    public:
        void draw(TubeCoreState& state, TubeRenderer& renderer, bool first);

    private:
        enum class Mode
        {
            // Preserve the protocol's legacy order: symbol, set point,
            // extended vector, then short vector.
            Symbol,
            SetPoint,
            ExtendedVector,
            ShortVector
        };

        void drawCharacter(TubeCoreState& state, TubeRenderer& renderer,
                           char ch);
        void drawVector(TubeCoreState& state, TubeRenderer& renderer);

        std::chrono::steady_clock::time_point startPaintTime_{};
        Mode mode_{Mode::Symbol};
        double efactor_{0.0};
        int args_{0};
        std::array<unsigned char, 4> data_{};
        int x0_{-485};
        int y0_{450};
        int x2_{0};
        int y2_{0};
    };

    // One key-table entry; an empty input code terminates the translation scan.
    struct TubeKeyCode
    {
        int inputCode{0};
        int outputCode{0};
    };

    // Parsed command-line configuration retained for the lifetime of the core.
    class TubeCoreOptions
    {
    public:
        int parseCommandLine(int argc, char* argv[]);
        bool ardsMode() const;
        void setArdsMode(bool enabled);
        bool aplEnabled() const;
        void setAplEnabled(bool enabled);
        bool autoClear() const;
        void setAutoClear(bool enabled);
        bool noExit() const;
        void setNoExit(bool enabled);
        bool expandTabs() const;
        void setExpandTabs(bool enabled);
        bool keepCharacterSize() const;
        void setKeepCharacterSize(bool enabled);
        bool fastMode() const;
        void setFastMode(bool enabled);
        bool performanceMode() const;
        void setPerformanceMode(bool enabled);
        bool rawMode() const;
        void setRawMode(bool enabled);
        int baudRate() const;
        void setBaudRate(int baudRate);
        bool fullScreen() const;
        void setFullScreen(bool enabled);
        bool verticalFullScreen() const;
        void setVerticalFullScreen(bool enabled);
        bool hideCursor() const;
        void setHideCursor(bool enabled);
        int waitSeconds() const;
        void setWaitSeconds(int seconds);
        bool halfScreen() const;
        void setHalfScreen(bool enabled);
        bool keyboardDisabled() const;
        void setKeyboardDisabled(bool disabled);
        bool usesPty() const;
        void setUsesPty(bool enabled);
        int eraseCharacter() const;
        void setEraseCharacter(int character);
        bool escapeDebugEnabled() const;
        void setEscapeDebugEnabled(bool enabled);

    private:
        bool ardsMode_{false};
        bool aplEnabled_{false};
        bool autoClear_{true};
        bool noExit_{true};
        bool expandTabs_{false};
        bool keepCharacterSize_{false};
        bool fastMode_{false};
        bool performanceMode_{false};
        bool rawMode_{false};
        int baudRate_{19200};
        bool fullScreen_{false};
        bool verticalFullScreen_{false};
        bool hideCursor_{false};
        int waitSeconds_{0};
        bool halfScreen_{false};
        bool keyboardDisabled_{false};
        bool usesPty_{false};
        int eraseCharacter_{0x08};
        bool escapeDebugEnabled_{false};
    };

    // Owns key mappings, character pacing, and deterministic queued parser input.
    class TubeInputState
    {
    public:
        // Queued bytes take precedence over PTY input for deterministic parsing.
        void enqueueInputByte(unsigned char byte);
        bool hasQueuedInput() const;
        std::optional<unsigned char> takeQueuedInput();
        void clearKeyTranslations();
        bool addKeyTranslation(int index, int inputCode, int outputCode);
        int translateKeyCode(int inputCode) const;
        void setCharacterInterval(std::chrono::microseconds interval);
        bool shouldDelayCharacter(
            std::chrono::steady_clock::time_point now) const;
        void recordCharacter(std::chrono::steady_clock::time_point now);
        void resetCharacterTimer(std::chrono::steady_clock::time_point now);
        std::chrono::steady_clock::duration characterTimeSinceReset(
            std::chrono::steady_clock::time_point now) const;

    private:
        std::array<TubeKeyCode, TUBE_MAX_KEYCODES> keyTable_{};
        std::deque<unsigned char> queuedInput_{};
        long charCount_{0};
        long charResetCount_{0};
        std::chrono::microseconds characterInterval_{};
        std::chrono::steady_clock::time_point inputTimerStartTime_{};
        std::chrono::steady_clock::time_point lastInputTime_{};
    };

    // Buffers escape-sequence bytes for optional diagnostic output.
    class TubeDiagnosticsState
    {
    public:
        void traceEscapeCharacter(int ch, bool enabled);

    private:
        void printEscapeDebug();
        void printEscapeDebugCharacter(int ch);

        int escDebugActive_{0};
        int escDebugCount_{0};
        std::array<int, TUBE_ESCDEBUG_LEN> escDebugBuf_{};
    };

    // Tracks one-shot host requests and the post-exit wait interval.
    class TubeProcessState
    {
    public:
        void requestScreenshot();
        bool takeScreenshotRequest();
        bool markChildFinished();
        bool shouldQuitAfterWait(std::chrono::steady_clock::time_point now,
                                 std::chrono::seconds wait);

    private:
        bool screenshotRequested_{false};
        bool childFinishedShown_{false};
        std::optional<std::chrono::steady_clock::time_point> firstWait_;
    };

    // Aggregates all mutable state for one TubeCore instance.
    class TubeCoreState
    {
    public:
        TubeCoreState() = default;

        TubeCoordinates& coordinates() { return coordinates_; }
        const TubeCoordinates& coordinates() const { return coordinates_; }
        TubeDisplayGeometry& displayGeometry() { return displayGeometry_; }
        const TubeDisplayGeometry& displayGeometry() const
        {
            return displayGeometry_;
        }
        TubeRenderLifecycle& renderLifecycle() { return renderLifecycle_; }
        const TubeRenderLifecycle& renderLifecycle() const
        {
            return renderLifecycle_;
        }
        TubeCursorState& cursor() { return cursor_; }
        const TubeCursorState& cursor() const { return cursor_; }
        TubeCharacterSetState& characterSet() { return characterSet_; }
        const TubeCharacterSetState& characterSet() const
        {
            return characterSet_;
        }
        TubeFrameTiming& frameTiming() { return frameTiming_; }
        const TubeFrameTiming& frameTiming() const { return frameTiming_; }
        TubeStrokeStyle& strokeStyle() { return strokeStyle_; }
        const TubeStrokeStyle& strokeStyle() const { return strokeStyle_; }
        Tek4010State& tek() { return tek_; }
        const Tek4010State& tek() const { return tek_; }
        ArdsParser& ards() { return ards_; }
        const ArdsParser& ards() const { return ards_; }
        TerminalSession& terminalSession() { return terminalSession_; }
        const TerminalSession& terminalSession() const
        {
            return terminalSession_;
        }
        TubeCoreOptions& options() { return options_; }
        const TubeCoreOptions& options() const { return options_; }
        TubeInputState& input() { return input_; }
        const TubeInputState& input() const { return input_; }
        TubeDiagnosticsState& diagnostics() { return diagnostics_; }
        const TubeDiagnosticsState& diagnostics() const { return diagnostics_; }
        TubeProcessState& process() { return process_; }
        const TubeProcessState& process() const { return process_; }

    private:
        TubeCoordinates coordinates_;
        TubeDisplayGeometry displayGeometry_;
        TubeRenderLifecycle renderLifecycle_;
        TubeCursorState cursor_;
        TubeCharacterSetState characterSet_;
        TubeFrameTiming frameTiming_;
        TubeStrokeStyle strokeStyle_;
        Tek4010State tek_;
        ArdsParser ards_;
        TerminalSession terminalSession_;
        TubeCoreOptions options_;
        TubeInputState input_;
        TubeDiagnosticsState diagnostics_;
        TubeProcessState process_;
    };

    extern void tube_request_screenshot(TubeCoreState& state);
    extern void tube_write_key(TubeCoreState& state, int ch);
    extern int tube_translateKeyCode(TubeCoreState& state, int ch);
    extern void tube_initializeAplInput(TubeCoreState& state);
    extern TubeRenderState tube_renderState(const TubeCoreState& state);
    extern void tube_doCursor(TubeCoreState& state, TubeRenderer& renderer);
    extern void tube_clearPersistent(TubeCoreState& state,
                                     TubeRenderer& renderer);
    extern void tube_clearSecond(TubeCoreState& state, TubeRenderer& renderer);
    extern int tube_isInput(TubeCoreState& state);
    extern int tube_getInputChar(TubeCoreState& state);
    extern bool tube_loadKeyTranslationTable(TubeInputState& input,
                                             std::istream& config);
    extern void tube_emulateDeflectionTime(TubeCoreState& state);
    extern void tube_crosshair(TubeCoreState& state, TubeRenderer& renderer);
    extern void tube_drawVector(TubeCoreState& state, TubeRenderer& renderer);
    extern void tube_drawCharacter(TubeCoreState& state, TubeRenderer& renderer,
                                   char ch);
    extern void tube_drawPoint(TubeCoreState& state, TubeRenderer& renderer);
    extern void tube_setupPainting(TubeCoreState& state, TubeRenderer& renderer,
                                   TubeFont font);
    extern void tube_changeCharacterSize(TubeCoreState& state,
                                         TubeRenderer& renderer,
                                         int charsPerLine, int charsPerPage,
                                         double fontSize);
}


module;

#include <memory>

// Public per-instance emulator API; GTK and Cairo stay behind host interfaces.
export module tube;

export import terminal_session;
export import tube_renderer;
export import :internal;

export inline constexpr int TIME_INTERVAL{35};
export inline constexpr int ERASE_BS{0x08};
export inline constexpr int ERASE_DEL{0x7F};

// Host configuration returned after core initialization and used to build GTK UI.
export struct TubeOptions
{
    bool fullScreen;
    bool verticalFullScreen;
    bool ardsMode;
    bool aplEnabled;
    bool halfScreen;
    bool hideCursor;
    bool keyboardDisabled;
    bool performanceMode;
    int eraseCharacter;
};

// Host actions and child status returned after one nonblocking timer update.
export struct TubeTimerUpdate
{
    bool redraw;
    bool processFinished;
    bool requestQuit;
    int childExitStatus; // Normalized status, meaningful after child exit.
};

// Owns one emulator instance and exposes host-facing input, timer, and drawing.
export class TubeCore final
{
public:
    TubeCore();
    ~TubeCore();

    TubeCore(const TubeCore&) = delete;
    TubeCore& operator=(const TubeCore&) = delete;

    TubeOptions initialize(int argc, char* argv[]);
    // Poll process/input state without blocking or entering GTK code.
    TubeTimerUpdate onTimerEvent();
    void setWindowSize(int width, int height);
    bool hasBrightSpot() const;
    bool isFastMode() const;
    void enableFastMode();
    bool isChildExited() const;
    void requestClear();
    void setAplMode(bool enabled);
    bool takeScreenshotRequest();
    void sendBreak();
    bool submitKey(int ch);
    int clicked(int button, int x, int y);
    void drawTek(TubeRenderer& renderer, bool first);
    void drawArds(TubeRenderer& renderer, bool first);
    void quit();

private:
    std::unique_ptr<TubeCoreState> state_;
};

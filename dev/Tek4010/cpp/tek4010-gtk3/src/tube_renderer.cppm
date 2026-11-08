// Backend-neutral drawing contract between protocol parsers and Cairo rendering.
export module tube_renderer;

export {
    // Backend-neutral choices shared by the core and rendering implementation.
    enum class LineType
    {
        Solid,
        Dotted,
        DotDash,
        ShortDash,
        LongDash
    };

    enum class TubeFont
    {
        Standard,
        Apl
    };

    // Snapshot consumed by a renderer for one drawing operation.
    struct TubeRenderState
    {
        int windowWidth;
        int windowHeight;
        bool argFast;
        int tube_x0;
        int tube_y0;
        int tube_x2;
        int tube_y2;
        int hDotsPerChar;
        int vDotsPerChar;
        int intensity;
        bool defocussed;
        double pensize;
        LineType ltype;
        bool writeThroughMode;
    };

    // Rendering operations shared by the GTK/Cairo host and Tek parsers.
    class TubeRenderer
    {
    public:
        virtual ~TubeRenderer() = default;

        // Draw commands consume a core snapshot without depending on GTK types.
        virtual void drawCursor(const TubeRenderState& state) = 0;
        // Persistent marks survive frames; transient marks are cleared or faded.
        virtual void clearPersistent() = 0;
        virtual void clearTemporary(bool fastMode) = 0;
        virtual void drawCharacter(const TubeRenderState& state, char ch,
                                   bool aplMode) = 0;
        virtual void drawPoint(const TubeRenderState& state) = 0;
        virtual void crosshair(const TubeRenderState& state) = 0;
        virtual void drawVector(const TubeRenderState& state) = 0;
        // Configure stroke/font state used by subsequent draw operations.
        virtual void setupPainting(double penSize, TubeFont font) = 0;
        virtual void setCharacterSize(int fontSize, bool measureAscent) = 0;
    };
}

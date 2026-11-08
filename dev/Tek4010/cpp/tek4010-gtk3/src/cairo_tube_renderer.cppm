module;

#include <cairo.h>

// Cairo implementation of the renderer contract; it does not own the contexts.
export module cairo_tube_renderer;

import tube_renderer;

export {
    // Adapts the backend-neutral renderer contract to borrowed Cairo contexts.
    class CairoTubeRenderer final : public TubeRenderer
    {
    public:
        // Contexts are owned by CairoCanvas and used only during its draw callback.
        void setContexts(cairo_t* persistent, cairo_t* temporary);

        void drawCursor(const TubeRenderState& state) override;
        void clearPersistent() override;
        void clearTemporary(bool fastMode) override;
        void drawCharacter(const TubeRenderState& state, char ch,
                           bool aplMode) override;
        void drawPoint(const TubeRenderState& state) override;
        void crosshair(const TubeRenderState& state) override;
        void drawVector(const TubeRenderState& state) override;
        void setupPainting(double penSize, TubeFont font) override;
        void setCharacterSize(int fontSize, bool measureAscent) override;

    private:
        cairo_t* persistent_{nullptr};
        cairo_t* temporary_{nullptr};
        // Suppress repeated glow work for points clustered during dense plots.
        int rendererXLast_{0};
        int rendererYLast_{0};
        int currentFontSize_{18};
        // ARDS aligns glyphs to measured font ascent; other modes use zero offset.
        int currentCharacterOffset_{0};
    };
}

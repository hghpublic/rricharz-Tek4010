module;

#include <cairo.h>
#include <memory>

// Owns logical Cairo layers and their composition without depending on GTK.
export module cairo_canvas;

export {
    // Maintains fixed-size Tek drawing layers and fits them into a host viewport.
    class CairoCanvas final
    {
    public:
        // Callback contexts are borrowed and valid only for the duration of render().
        using DrawCallback = void (*)(cairo_t* persistent, cairo_t* temporary,
                                      void* context);

        CairoCanvas() = default;
        ~CairoCanvas() = default;

        CairoCanvas(const CairoCanvas&) = delete;
        CairoCanvas& operator=(const CairoCanvas&) = delete;

        // Logical dimensions are fixed by the first successful initialization.
        bool initialize(cairo_t* target, int logicalWidth, int logicalHeight);
        bool render(cairo_t* target, int viewportWidth, int viewportHeight,
                    bool fastMode, DrawCallback draw, void* context);
        // Returns false for letterboxed points outside the logical canvas.
        bool mapToCanvas(double x, double y, int viewportWidth,
                         int viewportHeight, int& canvasX, int& canvasY) const;

    private:
        struct SurfaceDeleter
        {
            void operator()(cairo_surface_t* surface) const noexcept;
        };

        using Surface = std::unique_ptr<cairo_surface_t, SurfaceDeleter>;

        // Uniform scale and offsets keep the logical canvas centered in the host.
        struct Viewport
        {
            double scale;
            double offsetX;
            double offsetY;
        };

        Viewport fitViewport(int width, int height) const;

        Surface permanentSurface_;
        Surface temporarySurface_;
        int logicalWidth_{0};
        int logicalHeight_{0};
    };
}

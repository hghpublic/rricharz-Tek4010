// Owns persistent and temporary Tek layers and composes them into a Cairo target.
module;

#include <algorithm>
#include <cairo.h>
#include <cstdio>
#include <memory>
#include <print>
#include <utility>

module cairo_canvas;

namespace
{
struct ContextDeleter
{
    void operator()(cairo_t* context) const noexcept
    {
        if (context != nullptr)
        {
            cairo_destroy(context);
        }
    }
};

using Context = std::unique_ptr<cairo_t, ContextDeleter>;

bool report_cairo_error(const char* operation, cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS)
    {
        return false;
    }

    std::println(stderr, "{}: {}", operation, cairo_status_to_string(status));
    return true;
}
} // namespace

void CairoCanvas::SurfaceDeleter::operator()(
    cairo_surface_t* surface) const noexcept
{
    if (surface != nullptr)
    {
        cairo_surface_destroy(surface);
    }
}

bool CairoCanvas::initialize(cairo_t* target, int logicalWidth,
                             int logicalHeight)
{
    if ((target == nullptr) || (logicalWidth <= 0) || (logicalHeight <= 0))
    {
        std::println(stderr,
                     "Cannot initialize Cairo canvas with invalid geometry");
        return false;
    }
    if (report_cairo_error(
            "Cannot initialize Cairo canvas from invalid context",
            cairo_status(target)))
    {
        return false;
    }

    if (permanentSurface_ || temporarySurface_)
    {
        if ((logicalWidth == logicalWidth_) &&
            (logicalHeight == logicalHeight_))
        {
            return true;
        }

        std::println(stderr,
                     "Cannot change Cairo canvas logical dimensions after "
                     "initialization");
        return false;
    }

    // Commit the layers only after both allocations and status checks succeed.
    Surface permanent(cairo_surface_create_similar(
        cairo_get_target(target), CAIRO_CONTENT_COLOR, logicalWidth,
        logicalHeight));
    if (permanent == nullptr)
    {
        std::println(stderr, "Cannot allocate persistent Cairo surface");
        return false;
    }
    if (report_cairo_error("Cannot create persistent Cairo surface",
                           cairo_surface_status(permanent.get())))
    {
        return false;
    }

    Surface temporary(cairo_surface_create_similar(
        cairo_get_target(target), CAIRO_CONTENT_COLOR_ALPHA, logicalWidth,
        logicalHeight));
    if (temporary == nullptr)
    {
        std::println(stderr, "Cannot allocate temporary Cairo surface");
        return false;
    }
    if (report_cairo_error("Cannot create temporary Cairo surface",
                           cairo_surface_status(temporary.get())))
    {
        return false;
    }

    permanentSurface_ = std::move(permanent);
    temporarySurface_ = std::move(temporary);
    logicalWidth_ = logicalWidth;
    logicalHeight_ = logicalHeight;
    return true;
}

bool CairoCanvas::render(cairo_t* target, int viewportWidth, int viewportHeight,
                         bool fastMode, DrawCallback draw, void* context)
{
    if ((target == nullptr) || !permanentSurface_ || !temporarySurface_ ||
        (draw == nullptr))
    {
        std::println(stderr, "Cannot render an uninitialized Cairo canvas");
        return false;
    }
    if (report_cairo_error("Cannot render to an invalid Cairo context",
                           cairo_status(target)))
    {
        return false;
    }

    Context persistent{cairo_create(permanentSurface_.get())};
    if (persistent == nullptr)
    {
        std::println(stderr, "Cannot allocate persistent Cairo context");
        return false;
    }
    if (report_cairo_error("Cannot create persistent Cairo context",
                           cairo_status(persistent.get())))
    {
        return false;
    }

    Context temporary{cairo_create(temporarySurface_.get())};
    if (temporary == nullptr)
    {
        std::println(stderr, "Cannot allocate temporary Cairo context");
        return false;
    }
    if (report_cairo_error("Cannot create temporary Cairo context",
                           cairo_status(temporary.get())))
    {
        return false;
    }

    // The callback borrows these contexts only until this render call returns.
    draw(persistent.get(), temporary.get(), context);

    if (report_cairo_error("Cairo drawing failed",
                           cairo_status(persistent.get())) ||
        report_cairo_error("Temporary Cairo drawing failed",
                           cairo_status(temporary.get())))
    {
        return false;
    }

    persistent.reset();
    temporary.reset();
    cairo_surface_flush(permanentSurface_.get());
    cairo_surface_flush(temporarySurface_.get());
    if (report_cairo_error("Persistent Cairo surface flush failed",
                           cairo_surface_status(permanentSurface_.get())) ||
        report_cairo_error("Temporary Cairo surface flush failed",
                           cairo_surface_status(temporarySurface_.get())))
    {
        return false;
    }

    if ((viewportWidth <= 0) || (viewportHeight <= 0))
    {
        // A temporarily hidden or not-yet-sized widget has nothing to compose.
        return true;
    }

    const Viewport viewport{fitViewport(viewportWidth, viewportHeight)};
    // Rebuild from black, then overlay the transient glow on persistent strokes.
    cairo_save(target);
    cairo_set_operator(target, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgb(target, 0.0, 0.0, 0.0);
    cairo_paint(target);

    cairo_translate(target, viewport.offsetX, viewport.offsetY);
    cairo_scale(target, viewport.scale, viewport.scale);
    cairo_rectangle(target, 0.0, 0.0, logicalWidth_, logicalHeight_);
    cairo_clip(target);
    cairo_set_operator(target, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(target, permanentSurface_.get(), 0.0, 0.0);
    cairo_paint(target);

    cairo_set_operator(target,
                       fastMode ? CAIRO_OPERATOR_OVER : CAIRO_OPERATOR_LIGHTEN);
    cairo_set_source_surface(target, temporarySurface_.get(), 0.0, 0.0);
    cairo_paint(target);
    cairo_restore(target);

    return !report_cairo_error("Cairo canvas composition failed",
                               cairo_status(target));
}

bool CairoCanvas::mapToCanvas(double x, double y, int viewportWidth,
                              int viewportHeight, int& canvasX,
                              int& canvasY) const
{
    if ((logicalWidth_ <= 0) || (logicalHeight_ <= 0) || (viewportWidth <= 0) ||
        (viewportHeight <= 0))
    {
        return false;
    }

    const Viewport viewport{fitViewport(viewportWidth, viewportHeight)};
    const double canvasRight =
        viewport.offsetX + logicalWidth_ * viewport.scale;
    const double canvasBottom =
        viewport.offsetY + logicalHeight_ * viewport.scale;
    // Letterbox clicks are outside the logical drawing surface.
    if ((x < viewport.offsetX) || (x >= canvasRight) ||
        (y < viewport.offsetY) || (y >= canvasBottom))
    {
        return false;
    }

    canvasX = static_cast<int>((x - viewport.offsetX) / viewport.scale);
    canvasY = static_cast<int>((y - viewport.offsetY) / viewport.scale);
    return true;
}

CairoCanvas::Viewport CairoCanvas::fitViewport(int width, int height) const
{
    const double scale = std::min(static_cast<double>(width) / logicalWidth_,
                                  static_cast<double>(height) / logicalHeight_);
    return Viewport{.scale = scale,
                    .offsetX = (width - logicalWidth_ * scale) / 2.0,
                    .offsetY = (height - logicalHeight_ * scale) / 2.0};
}

#include <catch2/catch_test_macros.hpp>
#include <cairo.h>
#include <cstdint>
#include <cstring>
#include <memory>

import cairo_canvas;
import cairo_tube_renderer;
import tube_renderer;

namespace
{

using CairoSurface =
    std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)>;
using CairoContext = std::unique_ptr<cairo_t, decltype(&cairo_destroy)>;

struct DrawVectorContext
{
    CairoTubeRenderer renderer;
    TubeRenderState state{
        .windowWidth = 100,
        .windowHeight = 100,
        .argFast = true,
        .tube_x0 = 10,
        .tube_y0 = 10,
        .tube_x2 = 90,
        .tube_y2 = 90,
        .hDotsPerChar = 10,
        .vDotsPerChar = 10,
        .intensity = 100,
        .defocussed = false,
        .pensize = 2.0,
        .ltype = LineType::Solid,
        .writeThroughMode = false,
    };
};

void drawVector(cairo_t* persistent, cairo_t* temporary, void* rawContext)
{
    // Arrange: clean layers so the pixels below reflect only this vector.
    auto& context{*static_cast<DrawVectorContext*>(rawContext)};

    cairo_set_operator(persistent, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgb(persistent, 0.0, 0.0, 0.0);
    cairo_paint(persistent);
    cairo_set_operator(persistent, CAIRO_OPERATOR_OVER);

    cairo_set_operator(temporary, CAIRO_OPERATOR_CLEAR);
    cairo_paint(temporary);
    cairo_set_operator(temporary, CAIRO_OPERATOR_OVER);

    // Act through the same renderer used by the GTK host.
    context.renderer.setContexts(persistent, temporary);
    context.renderer.setupPainting(context.state.pensize, TubeFont::Standard);
    context.renderer.drawVector(context.state);
}

std::uint32_t pixelAt(cairo_surface_t* surface, int x, int y)
{
    const int stride{cairo_image_surface_get_stride(surface)};
    const unsigned char* const data{cairo_image_surface_get_data(surface)};
    std::uint32_t pixel{};
    std::memcpy(&pixel, data + y * stride + x * sizeof(pixel), sizeof(pixel));
    return pixel;
}

} // namespace

// What: verify letterbox-aware coordinate mapping and Cairo layer composition.
// Why: regressions can misroute GIN clicks or hide/clip vectors; this offscreen
// test exercises those contracts without requiring a GTK display.
TEST_CASE("Cairo canvas composes a rendered Tek vector", "[render][cairo]")
{
    // Arrange: create a wide viewport around a square logical canvas to create bars.
    CairoSurface surface{
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 200, 100),
        cairo_surface_destroy};
    REQUIRE(surface != nullptr);
    REQUIRE(cairo_surface_status(surface.get()) == CAIRO_STATUS_SUCCESS);

    CairoContext target{cairo_create(surface.get()), cairo_destroy};
    REQUIRE(target != nullptr);
    REQUIRE(cairo_status(target.get()) == CAIRO_STATUS_SUCCESS);

    CairoCanvas canvas;
    REQUIRE(canvas.initialize(target.get(), 100, 100));

    int canvasX{};
    int canvasY{};
    // Act: map a point in the centered canvas to logical coordinates.
    REQUIRE(canvas.mapToCanvas(70.0, 20.0, 200, 100, canvasX, canvasY));
    // Assert the viewport offset is removed from the mapped position.
    CHECK(canvasX == 20);
    CHECK(canvasY == 20);
    // Assert that input in the left letterbox cannot become a Tek coordinate.
    CHECK_FALSE(canvas.mapToCanvas(10.0, 50.0, 200, 100, canvasX, canvasY));

    // Arrange: use a solid vector with endpoints inside the logical canvas.
    DrawVectorContext drawContext;
    // Act: compose both drawing layers into the target before reading pixels.
    REQUIRE(canvas.render(target.get(), 200, 100, true, drawVector,
                          &drawContext));
    cairo_surface_flush(surface.get());

    // Assert the vector is visible and the letterbox remains black.
    CHECK(pixelAt(surface.get(), 100, 50) != 0xFF000000U);
    CHECK(pixelAt(surface.get(), 10, 50) == 0xFF000000U);
}

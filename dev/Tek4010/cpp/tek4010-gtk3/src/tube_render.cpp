module;

#include <array>
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <string_view>

module cairo_tube_renderer;

import tube_renderer;

// Implements the Tek drawing primitives over the canvas's persistent and
// temporary Cairo layers.
static constexpr double WRITE_TROUGH_INTENSITY{0.5};
static constexpr double WRITE_TROUGH_SATURATION{0.2};
static constexpr double NORMAL_INTENSITY{0.7};
static constexpr double NORMAL_SATURATION{0.2};
static constexpr double CURSOR_INTENSITY{0.7};
static constexpr double CURSOR_SATURATION{0.2};
static constexpr double BRIGHT_SPOT_INTENSITY{1.0};
static constexpr double BRIGHT_SPOT_SATURATION{0.7};
static constexpr double BLACK_INTENSITY{0.08};
static constexpr double FADE{0.3};

static constexpr double dashset[]{2, 6, 2, 2, 6, 3, 3, 3, 6, 6};

static void tube_set_source_rgb(cairo_t* cr, double intensity,
                                double saturation);

void CairoTubeRenderer::setContexts(cairo_t* persistent, cairo_t* temporary)
{
    persistent_ = persistent;
    temporary_ = temporary;
}

void CairoTubeRenderer::drawCursor(const TubeRenderState& state)
{
    cairo_t* cr2{temporary_};
    tube_set_source_rgb(cr2, CURSOR_INTENSITY, CURSOR_SATURATION);
    cairo_set_line_width(cr2, 1);
    cairo_rectangle(cr2, state.tube_x0,
                    state.windowHeight - state.tube_y0 - state.vDotsPerChar +
                        6 + currentCharacterOffset_,
                    state.hDotsPerChar - 3, state.vDotsPerChar - 3);
    cairo_fill(cr2);
    cairo_stroke(cr2);
}

void CairoTubeRenderer::clearPersistent()
{
    cairo_set_source_rgb(persistent_, 0.0, BLACK_INTENSITY, 0.0);
    cairo_paint(persistent_);
    tube_set_source_rgb(persistent_, NORMAL_INTENSITY, NORMAL_SATURATION);
    tube_set_source_rgb(temporary_, BRIGHT_SPOT_INTENSITY / 2,
                        BRIGHT_SPOT_SATURATION);
    cairo_paint(temporary_);
    fflush(stdout);
    rendererXLast_ = 0;
    rendererYLast_ = 0;
}

void CairoTubeRenderer::clearTemporary(bool fastMode)
{
    if (fastMode)
    {
        // Fast mode removes stale glow outright instead of simulating decay.
        cairo_set_source_rgba(temporary_, 0, 0, 0, 0);
        cairo_set_operator(temporary_, CAIRO_OPERATOR_SOURCE);
        cairo_paint(temporary_);
        cairo_set_operator(temporary_, CAIRO_OPERATOR_OVER);
    }
    else
    {
        // Normal mode fades old glow gradually to preserve phosphor persistence.
        cairo_set_source_rgba(temporary_, 0, 0, 0, FADE);
        cairo_set_operator(temporary_, CAIRO_OPERATOR_MULTIPLY);
        cairo_paint(temporary_);
        cairo_set_operator(temporary_, CAIRO_OPERATOR_OVER);
    }
}

static void tube_line_type(cairo_t* cr, cairo_t* cr2, LineType lineType)
{
    // Keep the transient glow aligned with the persistent stroke's dash pattern.
    int ndash, ndx;
    double ofs{0.5};

    switch (lineType)
    {
    case LineType::Solid:
        ndx = 0;
        ndash = 0;
        break;
    case LineType::Dotted:
        ndx = 0;
        ndash = 2;
        break;
    case LineType::DotDash:
        ndx = 2;
        ndash = 4;
        break;
    case LineType::LongDash:
        ndx = 8;
        ndash = 2;
        break;
    case LineType::ShortDash:
        ndx = 6;
        ndash = 2;
        break;
    }
    cairo_set_dash(cr, &dashset[ndx], ndash, ofs);
    cairo_set_dash(cr2, &dashset[ndx], ndash, ofs);
}

void CairoTubeRenderer::drawCharacter(const TubeRenderState& state, char ch,
                                      bool aplMode)
{
    static constexpr std::array<std::string_view, 95> aplCharacters{
        // 32-47
        " ", "\u00A8", ")", "<", "\u2264", "=", ">", "]", "\u2228", "\u2227",
        "\u2260", "\u00F7", ",", "+", ".", "/",
        // 48-57
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        // 58-79
        "(", "[", ";", "\u00D7", ":", "\\", "\u00AF", "\u237A", "\u22A5",
        "\u2229", "\u230A", "\u220A", "_", "\u2207", "\u2206", "\u2373",
        "\u2218", "'", "\u2395", "\u2223", "\u22A4", "\u25CB",
        // 80-96
        "\u22c6", "?", "\u2374", "\u2308", "\u223C", "\u2193", "\u222A",
        "\u03C9", "\u2283", "\u2191", "\u2282", "\u2190", "\u22A2", "\u2192",
        "\u2265", "-", "\u22C4",
        // 97-122
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
        "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        // 123-126
        "{", "\u22A3", "}", "$"};
    cairo_t* cr{persistent_};
    cairo_t* cr2{temporary_};
    std::array<char, 2> characterBuffer{};
    std::string_view character;

    if (ch < 32)
    {
        return; // non printable control character
    }

    if (aplMode)
    {
        // Map the Tek APL byte range to Unicode glyphs supported by Pango.
        if (ch <= 126)
        {
            character = aplCharacters[static_cast<std::size_t>(ch - 32)];
        }
        else
        {
            character = " ";
        }
    }
    else
    {
        characterBuffer[0] = ch;
        character = std::string_view{characterBuffer.data(), 1};
    }
    cairo_set_font_size(cr, currentFontSize_);
    cairo_set_font_size(cr2, currentFontSize_);

    if (state.writeThroughMode)
    {
        // Write-through bypasses persistent phosphor and draws only transiently.
        tube_set_source_rgb(cr2, WRITE_TROUGH_INTENSITY,
                            WRITE_TROUGH_SATURATION);
        cairo_move_to(cr2, state.tube_x0,
                      state.windowHeight - state.tube_y0 +
                          currentCharacterOffset_);
        cairo_show_text(cr2, character.data());
    }

    else
    {
        // Store the glyph on the phosphor layer and draw a bright transient copy.
        tube_set_source_rgb(
            cr,
            BLACK_INTENSITY +
                ((NORMAL_INTENSITY - BLACK_INTENSITY) * state.intensity) / 100,
            NORMAL_SATURATION);
        cairo_move_to(cr, state.tube_x0,
                      state.windowHeight - state.tube_y0 +
                          currentCharacterOffset_);
        cairo_show_text(cr, character.data());

        tube_set_source_rgb(cr2, BRIGHT_SPOT_INTENSITY, BRIGHT_SPOT_SATURATION);
        cairo_move_to(cr2, state.tube_x0,
                      state.windowHeight - state.tube_y0 +
                          currentCharacterOffset_);
        cairo_show_text(cr2, character.data());
    }

    fflush(stdout);
}

void CairoTubeRenderer::drawPoint(const TubeRenderState& state)
{
    cairo_t* cr{persistent_};
    cairo_t* cr2{temporary_};
    static constexpr double PI2{6.283185307};
    cairo_set_line_width(cr, state.pensize + state.defocussed);
    tube_set_source_rgb(
        cr, BLACK_INTENSITY + ((1.0 - BLACK_INTENSITY) * state.intensity) / 100,
        NORMAL_SATURATION);
    cairo_move_to(cr, state.tube_x2, state.windowHeight - state.tube_y2);
    cairo_line_to(cr, state.tube_x2 + 1,
                  state.windowHeight - state.tube_y2 + 1);
    cairo_stroke(cr);

    // Nearby points share one bright spot to limit Cairo work during plotting.
    if (((state.tube_x2 - rendererXLast_) > 2) ||
        ((rendererXLast_ - state.tube_x2) > 2) ||
        ((state.tube_y2 - rendererYLast_) > 2) ||
        ((rendererYLast_ - state.tube_y2) > 2))
    {

        // draw the bright spot
        cairo_set_line_width(cr2, 0.1);
        tube_set_source_rgb(cr2,
                            (BRIGHT_SPOT_INTENSITY * state.intensity) / 100,
                            BRIGHT_SPOT_SATURATION);
        cairo_arc(cr2, state.tube_x2, state.windowHeight - state.tube_y2,
                  2 + state.defocussed, 0, PI2);
        cairo_fill(cr2);

        rendererXLast_ = state.tube_x2;
        rendererYLast_ = state.tube_y2;
    }
}

void CairoTubeRenderer::crosshair(const TubeRenderState& state)
{
    cairo_t* cr2{temporary_};
    cairo_set_line_width(cr2, 1);
    tube_set_source_rgb(cr2, WRITE_TROUGH_INTENSITY, WRITE_TROUGH_SATURATION);
    cairo_move_to(cr2, state.tube_x0, 0);
    cairo_line_to(cr2, state.tube_x0, state.windowHeight);
    cairo_move_to(cr2, 0, state.windowHeight - state.tube_y0);
    cairo_line_to(cr2, state.windowWidth, state.windowHeight - state.tube_y0);
    cairo_stroke(cr2);
}

void CairoTubeRenderer::drawVector(const TubeRenderState& state)
{
    cairo_t* cr{persistent_};
    cairo_t* cr2{temporary_};
    // Write-through strokes stay transient; ordinary vectors persist and glow.
    if (state.writeThroughMode)
    {
        cairo_set_line_width(cr2, state.pensize + 1);
        tube_set_source_rgb(cr2, WRITE_TROUGH_INTENSITY,
                            WRITE_TROUGH_SATURATION);
        cairo_move_to(cr2, state.tube_x0, state.windowHeight - state.tube_y0);
        cairo_line_to(cr2, state.tube_x2, state.windowHeight - state.tube_y2);
        cairo_stroke(cr2);
    }

    else
    {
        cairo_set_line_width(cr, state.pensize + state.defocussed);
        tube_set_source_rgb(
            cr,
            BLACK_INTENSITY +
                ((NORMAL_INTENSITY - BLACK_INTENSITY) * state.intensity) / 100,
            NORMAL_SATURATION);
        tube_line_type(cr, cr2, state.ltype);
        cairo_move_to(cr, state.tube_x0, state.windowHeight - state.tube_y0);
        cairo_line_to(cr, state.tube_x2, state.windowHeight - state.tube_y2);
        cairo_stroke(cr);


        cairo_set_line_width(cr2, (state.pensize + 1) + state.defocussed);
        tube_set_source_rgb(cr2, BRIGHT_SPOT_INTENSITY, BRIGHT_SPOT_SATURATION);
        cairo_move_to(cr2, state.tube_x0, state.windowHeight - state.tube_y0);
        cairo_line_to(cr2, state.tube_x2, state.windowHeight - state.tube_y2);
        cairo_stroke(cr2);
    }
}

void CairoTubeRenderer::setupPainting(double penSize, TubeFont font)
{
    cairo_t* cr{persistent_};
    cairo_t* cr2{temporary_};
    const char* fontName =
        font == TubeFont::Apl ? "APL385 Unicode" : "monospace";

    // The bold transient copy makes newly drawn glyphs read as bright phosphor.
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    cairo_set_line_width(cr, penSize);
    tube_set_source_rgb(cr, NORMAL_INTENSITY, NORMAL_SATURATION);
    cairo_select_font_face(cr, fontName, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_select_font_face(cr2, fontName, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
}

void CairoTubeRenderer::setCharacterSize(int fontSize, bool measureAscent)
{
    cairo_t* cr{persistent_};
    cairo_t* cr2{temporary_};
    cairo_font_extents_t et;
    currentFontSize_ = fontSize;
    cairo_set_font_size(cr, currentFontSize_);
    cairo_set_font_size(cr2, currentFontSize_);
    if (measureAscent)
    {
        cairo_font_extents(cr, &et);
        currentCharacterOffset_ = (int)et.ascent;
    }
    else
    {
        currentCharacterOffset_ = 0;
    }
}

static void tube_set_source_rgb(cairo_t* cr, double intensity,
                                double saturation)
{
    cairo_set_source_rgb(cr, intensity * saturation, intensity,
                         intensity * saturation);
}

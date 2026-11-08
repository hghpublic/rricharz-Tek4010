/*
 *
 * GTK3 host owns the event loop, window, and Cairo canvas, forwarding input and
 * timer events to the backend-neutral Tek core.
 *
 * Copyright 2016,2019  rricharz
 *
 * https://github.com/rricharz/Tek4010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

module;

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include <cairo.h>
#include <chrono>
#include <gtk/gtk.h>
#include <print>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

module gtk_app;

import cairo_canvas;
import cairo_tube_renderer;
import tube;

namespace
{

/*
 * Automatic performance fallback:
 *
 * On slow systems (e.g. virtual machines or high-DPI displays), the
 * phosphor fading simulation can cause large delays between redraws.
 * This is perceived by the user as sluggish typing response.
 *
 * We measure the actual time between redraws ("lag"). If the lag exceeds
 * SLOW_THRESHOLD (in milliseconds) several times within a time window,
 * we assume that the system cannot keep up with full fading and switch
 * permanently to fast mode (no continuous fading).
 *
 * Detection rule:
 *   - count redraws with lag > SLOW_THRESHOLD
 *   - if ≥ 3 such events occur within WINDOW_MSEC
 *     → switch to fast mode
 *   - ignore startup lag
 *
 * The switch is one-way (no automatic return to normal mode) to avoid
 * unstable or oscillating behavior.
 */

using Clock = std::chrono::steady_clock;

static constexpr std::chrono::milliseconds SLOW_THRESHOLD{100};
static constexpr std::chrono::milliseconds WINDOW_MSEC{10000};
static constexpr std::chrono::milliseconds STARTUP_IGNORE_MSEC{1000};
static constexpr int SLOW_FRAME_COUNT{3};
static constexpr guint PERFORMANCE_TIMER_INTERVAL{1};

// Account for window decorations so the requested client area fits on screen.
static constexpr int BORDER{80};

// Adapts GTK events and window lifetime to the backend-neutral TubeCore API.
class GtkApp final
{
public:
    ~GtkApp();
    int run(int argc, char* argv[]);

private:
    void main_finished();
    void check_graphics_response(Clock::duration lag);
    static gboolean on_draw_event(GtkWidget* widget, cairo_t* cr,
                                  gpointer user_data);
    static void on_screenshot_exit(GPid pid, gint status, gpointer user_data);
    void launch_screenshot();
    static void draw_emulator(cairo_t* persistent, cairo_t* temporary,
                              void* context);
    static gboolean on_timer_event(gpointer user_data);
    static gboolean clicked(GtkWidget* widget, GdkEventButton* event,
                            gpointer user_data);
    static void on_quit_event(GtkWidget* widget, gpointer user_data);
    void quit(int exitStatus = EXIT_SUCCESS);
    void do_drawing(cairo_t* cr, GtkWidget* widget);
    static void on_key_press(GtkWidget* widget, GdkEventKey* event,
                             gpointer user_data);

    Clock::time_point lastQueueDrawTime_{};
    Clock::time_point appStartTime_{};
    TubeOptions options_{};
    TubeCore core_;
    CairoTubeRenderer renderer_;
    bool firstCall_{true};
    GtkWidget* window_{nullptr};
    int windowWidth_{0};
    int windowHeight_{0};
    double aspectRatio_{0.0};
    guint timeoutRef_ = 0;
    int exitStatus_{EXIT_SUCCESS};
    int slowCount_{0};
    Clock::time_point firstSlowTime_{};
    bool fastSwitched_{false};
    CairoCanvas canvas_;
};

void GtkApp::main_finished()
{
    std::print("tek4010 - finished");
    gtk_window_set_title(GTK_WINDOW(window_), "tek4010 - finished");
}

void GtkApp::check_graphics_response(Clock::duration lag)
{
    const auto now{Clock::now()};

    if (!core_.hasBrightSpot())
    {
        return;
    }

    // ignore startup phase (first 1000 ms)
    if ((now - appStartTime_) < STARTUP_IGNORE_MSEC)
    {
        return;
    }

    if (fastSwitched_ || core_.isFastMode())
    {
        return;
    }

    if (lag <= SLOW_THRESHOLD)
    {
        return;
    }

    const auto detectionTime{Clock::now()};

    if (slowCount_ == 0)
    {
        firstSlowTime_ = detectionTime;
        slowCount_ = 1;
    }
    else if ((detectionTime - firstSlowTime_) <= WINDOW_MSEC)
    {
        slowCount_++;
    }
    else
    {
        // restart detection window
        firstSlowTime_ = detectionTime;
        slowCount_ = 1;
    }

    if (slowCount_ >= SLOW_FRAME_COUNT)
    {
        core_.enableFastMode();
        fastSwitched_ = true;

        std::println("Slow graphics response -> switched to fast mode");
        fflush(stdout);
    }
}

gboolean GtkApp::on_draw_event(GtkWidget* widget, cairo_t* cr,
                               gpointer user_data)
{
    auto* app{static_cast<GtkApp*>(user_data)};
    app->do_drawing(cr, widget);
    return FALSE;
}

void GtkApp::on_screenshot_exit(GPid pid, gint status, gpointer user_data)
{
    GError* error{nullptr};
    if (!g_spawn_check_wait_status(status, &error))
    {
        std::println(stderr, "Screenshot command failed: {}", error->message);
        g_error_free(error);
    }
    g_spawn_close_pid(pid);
}

void GtkApp::launch_screenshot()
{
    static gchar scrotCommand[]{"scrot"};
    static gchar focusedOption[]{"--focused"};
    gchar* arguments[]{scrotCommand, focusedOption, nullptr};
    GPid pid{0};
    GError* error{nullptr};
    const GSpawnFlags flags = static_cast<GSpawnFlags>(
        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD);

    // Keep screenshot capture off the GTK event loop; the child watch reaps it.
    if (!g_spawn_async(nullptr, arguments, nullptr, flags, nullptr, nullptr,
                       &pid, &error))
    {
        std::println(stderr, "Cannot start screenshot command: {}",
                     error->message);
        g_error_free(error);
        return;
    }

    g_child_watch_add(pid, on_screenshot_exit, this);
}

gboolean GtkApp::on_timer_event(gpointer user_data)
{
    auto* app{static_cast<GtkApp*>(user_data)};
    // The core reports work as state; this adapter performs GTK side effects.
    const TubeTimerUpdate update{app->core_.onTimerEvent()};
    if (app->core_.takeScreenshotRequest())
    {
        app->launch_screenshot();
    }
    if (update.childExitStatus != EXIT_SUCCESS)
    {
        app->exitStatus_ = update.childExitStatus;
    }
    if (update.processFinished)
    {
        app->main_finished();
    }
    if (update.requestQuit)
    {
        app->quit(update.childExitStatus);
        app->timeoutRef_ = 0;
        return FALSE;
    }
    if (update.redraw)
    {
        app->lastQueueDrawTime_ = Clock::now();
        gtk_widget_queue_draw(app->window_);
    }

    return TRUE;
}

gboolean GtkApp::clicked(GtkWidget* widget, GdkEventButton* event,
                         gpointer user_data)
{
    auto* app{static_cast<GtkApp*>(user_data)};
    int canvasX;
    int canvasY;
    // Map host coordinates through the letterbox viewport before GIN handling.
    if (app->canvas_.mapToCanvas(
            event->x, event->y, gtk_widget_get_allocated_width(widget),
            gtk_widget_get_allocated_height(widget), canvasX, canvasY) &&
        app->core_.clicked(event->button, canvasX, canvasY))
    {
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

void GtkApp::on_quit_event(GtkWidget* widget, gpointer user_data)
{
    static_cast<GtkApp*>(user_data)->quit();
}

void GtkApp::quit(int exitStatus)
{
    if (exitStatus != EXIT_SUCCESS)
    {
        exitStatus_ = exitStatus;
    }
    core_.quit();
    gtk_main_quit();
}

void GtkApp::do_drawing(cairo_t* cr, GtkWidget* widget)
{
    check_graphics_response(Clock::now() - lastQueueDrawTime_);

    if (firstCall_)
    {
        // Fit the client area to the logical canvas, leaving letterbox bars.
        gtk_window_get_size(GTK_WINDOW(window_), &windowWidth_, &windowHeight_);

        if ((windowWidth_ != 1024) || (windowHeight_ != 768))
        {
            if (windowWidth_ >
                (int)((double)windowHeight_ * aspectRatio_ + 0.5))
            {
                windowWidth_ =
                    (int)((double)windowHeight_ * aspectRatio_ + 0.5);
            }
            if (windowHeight_ >
                (int)((double)windowWidth_ / aspectRatio_ + 0.5))
            {
                windowHeight_ =
                    (int)((double)windowWidth_ / aspectRatio_ + 0.5);
            }
        }
        std::println("Window dimensions: {} x {}", windowWidth_, windowHeight_);
        core_.setWindowSize(windowWidth_, windowHeight_);

        if (!canvas_.initialize(cr, windowWidth_, windowHeight_))
        {
            quit(EXIT_FAILURE);
            return;
        }

        if (options_.hideCursor)
        { // hide cursor (does not allow GIN mode)
            GdkCursor* Cursor{gdk_cursor_new(GDK_BLANK_CURSOR)};
            GdkWindow* win{gtk_widget_get_window(window_)};
            gdk_window_set_cursor((win), Cursor);
        }
    }

    if (!canvas_.render(cr, gtk_widget_get_allocated_width(widget),
                        gtk_widget_get_allocated_height(widget),
                        core_.isFastMode(), draw_emulator, this))
    {
        std::println(stderr, "Cannot render emulator canvas");
        quit(EXIT_FAILURE);
        return;
    }
}

void GtkApp::draw_emulator(cairo_t* persistent, cairo_t* temporary,
                           void* context)
{
    auto* app{static_cast<GtkApp*>(context)};
    // Both protocol modes share the same renderer; first is consumed after draw.
    app->renderer_.setContexts(persistent, temporary);
    if (app->options_.ardsMode)
    {
        app->core_.drawArds(app->renderer_, app->firstCall_);
    }
    else
    {
        app->core_.drawTek(app->renderer_, app->firstCall_);
    }
    app->firstCall_ = false;
}

void GtkApp::on_key_press(GtkWidget* widget, GdkEventKey* event,
                          gpointer user_data)
{
    auto* app{static_cast<GtkApp*>(user_data)};
    int ch;
    // Handle local shortcuts first, then translate remaining keys to bytes.
    if (app->options_.keyboardDisabled)
    {
        return;
    }

    if (event->keyval == 0xFF50)
    { // "home" key, use fn <left cursor> on Mac
        app->core_.requestClear();
        gtk_widget_queue_draw(widget);
        return;
    }

    if (event->keyval == 0x8BF)
    { // "option b" sends break code to target
        app->core_.sendBreak();
        return;
    }

    // alt/option keys: local tek4010 functions
    else if (event->state & GDK_MOD1_MASK)
    {

        if ((event->keyval == 0x0071) || // Alt-Q
            (event->keyval == 0x13BD))
        { // Mac left Option-Q
            app->quit();
            return;
        }
        else if (app->options_.aplEnabled &&
                 ((event->keyval == 0x006E) || // Alt-N
                  (event->keyval == 0x02DC)))
        { // Mac left Option-N
            app->core_.setAplMode(true);
            return;
        }
        else if (app->options_.aplEnabled &&
                 ((event->keyval == 0x006F) || // Alt-O
                  (event->keyval == 0x00F8)))
        { // Mac left Option-O
            app->core_.setAplMode(false);
            return;
        }
        else if ((event->keyval == 0x0062) || // Alt-B
                 (event->keyval == 0x222B))
        { // Mac left Option-B
            app->core_.sendBreak();
            return;
        }

        return;
    }

    // Backspace/Delete/Rubout key
    else if ((event->keyval == 0xFF08) || // Backspace
             (event->keyval == 0xFFFF))
    { // Delete/Rubout
        fflush(stdout);
        ch = app->options_.eraseCharacter;
    }

    // exit on ctrl-d if child has exited
    else if (app->core_.isChildExited() && (event->state & GDK_CONTROL_MASK) &&
             (event->keyval == 0x0064))
    {
        app->quit();
        return;
    }

    // control keys
    else if ((event->keyval >= 0xFF00) && (event->keyval <= 0xFF1F))
    {
        ch = event->keyval & 0x1F;
    }

    // ctrl-letter keys: pass through as ASCII control characters
    else if (event->state & GDK_CONTROL_MASK)
    {
        ch = event->keyval & 0x1F;
    }

    // normal keys
    else if ((event->keyval >= 0x0020) && (event->keyval <= 0x007F))
    {
        ch = event->keyval & 0x7F;
    }

    else
    {
        return;
    }

    if (app->core_.submitKey(ch))
    {
        gtk_widget_queue_draw(widget);
    }
}

GtkApp::~GtkApp()
{
    // Remove the timer callback before the stack-owned app and core are destroyed.
    if (timeoutRef_ != 0)
    {
        g_source_remove(timeoutRef_);
    }
    core_.quit();
}

int GtkApp::run(int argc, char* argv[])
{
    GtkWidget* drawingArea;

    int askWindowWidth;
    int askWindowHeight;

    options_ = core_.initialize(argc, argv);
    appStartTime_ = Clock::now();

    // Select the logical canvas aspect ratio before sizing the host window.
    if (options_.ardsMode)
    {
        askWindowWidth = 1080;
        askWindowHeight = 1414;
    }

    else if (options_.fullScreen)
    {
        askWindowWidth = 4096;
        askWindowHeight = 3120;
    }
    else
    {
        askWindowWidth = 1024;
        askWindowHeight = 780;
    }

    aspectRatio_ = (double)askWindowWidth / (double)askWindowHeight;

    gtk_init(&argc, &argv);

    window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    // set the background color
    GdkColor color;
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    gtk_widget_modify_bg(window_, GTK_STATE_NORMAL, &color);

    drawingArea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window_), drawingArea);
    gtk_widget_add_events(window_, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(window_, GDK_KEY_PRESS_MASK);

    gtk_widget_set_events(drawingArea, gtk_widget_get_events(drawingArea) |
                                           GDK_BUTTON_PRESS_MASK |
                                           GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(drawingArea), "draw", G_CALLBACK(on_draw_event),
                     this);
    g_signal_connect(G_OBJECT(drawingArea), "destroy",
                     G_CALLBACK(on_quit_event), this);
    g_signal_connect(G_OBJECT(drawingArea), "button-press-event",
                     G_CALLBACK(clicked), this);
    g_signal_connect(G_OBJECT(window_), "key_press_event",
                     G_CALLBACK(on_key_press), this);

    GdkScreen* screen{gtk_window_get_screen(GTK_WINDOW(window_))};
    int screenWidth{gdk_screen_get_width(screen)};
    int screenHeight{gdk_screen_get_height(screen)};
    std::println("Screen dimensions: {} x {}", screenWidth, screenHeight);

    if (options_.fullScreen)
    {
        // DISPLAY UNDECORATED FULL SCREEN WINDOW
        gtk_window_set_decorated(GTK_WINDOW(window_), FALSE);
        gtk_window_fullscreen(GTK_WINDOW(window_));
        gtk_window_set_keep_above(GTK_WINDOW(window_), FALSE);
        windowWidth_ = screenWidth;
        windowHeight_ = screenHeight;
    }

    else
    {
        // DISPLAY DECORATED WINDOW
        if (options_.verticalFullScreen ||
            (askWindowHeight > (screenHeight - BORDER)))
        {
            askWindowWidth =
                (int)((double)(screenHeight - BORDER) * aspectRatio_);
            askWindowHeight = screenHeight - BORDER;
            gtk_window_move(GTK_WINDOW(window_), screenWidth - askWindowWidth,
                            0);
        }
        else if (options_.halfScreen)
        {
            std::println("Using right half of the screen");
            askWindowWidth = screenWidth / 2;
            askWindowHeight = (int)((double)askWindowWidth / aspectRatio_);
            gtk_window_move(GTK_WINDOW(window_), screenWidth / 2, 0);
        }
        gtk_window_set_decorated(GTK_WINDOW(window_), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(window_), askWindowWidth,
                                    askWindowHeight);
        windowWidth_ = askWindowWidth;
        windowHeight_ = askWindowHeight;
    }
    core_.setWindowSize(windowWidth_, windowHeight_);

    const guint timerInterval{
        options_.performanceMode
            ? PERFORMANCE_TIMER_INTERVAL
            : static_cast<guint>(TIME_INTERVAL)};
    if (timerInterval > 0)
    {
        // Poll the core frequently enough to keep input and redraws responsive.
        timeoutRef_ = g_timeout_add(timerInterval, on_timer_event, this);
    }

    const char* windowName =
        options_.ardsMode
            ? "ARDS emulator"
            : (options_.aplEnabled ? "Tektronix 4013/4015 emulator (APL)"
                                   : "Tektronix 4010/4014 emulator");
    gtk_window_set_title(GTK_WINDOW(window_), windowName);

    gtk_widget_set_can_focus(window_, TRUE);
    gtk_widget_show_all(window_);
    gtk_window_present(GTK_WINDOW(window_));
    gtk_widget_grab_focus(window_);

    // GTK owns dispatch after initialization; callbacks return core results.
    gtk_main();

    return exitStatus_;
}

} // namespace

int run_gtk_app(int argc, char* argv[])
{
    GtkApp app;
    return app.run(argc, argv);
}

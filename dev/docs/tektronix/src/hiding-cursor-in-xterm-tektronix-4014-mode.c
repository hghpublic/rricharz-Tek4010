// Source - https://stackoverflow.com/q/35242335
// Posted by schoppenhauer
// Retrieved 2026-09-19, License - CC BY-SA 3.0

// The solution was that the control terminal needs to be set to not echoing input. This can be done with noecho() from libncurses.

#define _BSD_SOURCE
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#define GS ((char)0x1D)
#define ESC ((char)0x1B)
#define FF ((char)0x0C)

static void xy(int x, int y, char *restrict xy)
{
    xy[3] = (x % 32) + 64; // x low
    xy[2] = (x >> 5) + 32; // x high
    xy[1] = (y % 32) + 96; // y low
    xy[0] = (y >> 5) + 32; // y high
    assert(x == 32 * (xy[2] - 32) + xy[3] - 64);
    assert(y == 32 * (xy[0] - 32) + xy[1] - 96);
}

static void enter_graphic()
{
    printf("%c", GS);
}

static void leave_graphic()
{
    printf("\n");
}

static char pt[] = {ESC, 0, 0, 0, 0, 0, 0};

/* This function is NOT threadsafe! (but why should it be ...) */
static void line_to(int x, int y, char pattern)
{
    pt[1] = pattern ? pattern : '`';
    xy(x, y, pt + 2);
    printf("%s", pt);
}

static void clear()
{
    printf("%c%c", ESC, FF);
}

static void randgrph(int dx, int dy)
{
    int x, y;
    enter_graphic();

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 10; ++j)
        {
            x = (x + 7) % 200;
            y = (y + 39) % 200;
            line_to(x + dx, y + dy, 'b');
        }
    }

    leave_graphic();
}

int main(void)
{
    int i, j;
    for (i = 0; i < 200; ++i)
    {
        for (j = 0; j < 200; ++j)
        {
            clear();
            randgrph(i, j);
            usleep(1000000 / 25);
        }
    }
}

#set document(title: "Tek4010 Quick Start")
#set page(margin: 30mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[Tek4010 Quick Start]
]

#outline(title: [Contents], depth: 2)

This is a Tektronix 4010/4014 graphics terminal emulator. It is not a full
VT100/xterm terminal.

= Basic usage

```sh
tek4010 [options]
```

= Examples

Start tek4010:

```sh
tek4010
```

In the tek4010 window, change to the Tek4010 directory:

```sh
cd Tek4010
```

Display a plot file:

```sh
cat test.plt
```

Animation:

```sh
cat animation.plt
```

Demo script:

```sh
sh demos/demo.sh
```

If typing feels slow, start tek4010 with the `-fast` option.

Without `-fast`, tek4010 automatically switches to fast mode when it detects
slow graphics response. This may take a while.

= Keyboard

The following keys are handled locally by tek4010 and are not passed to the
child process or remote system.

#table(
  columns: (1.2fr, 1fr, 1fr, 1fr),
  [Action], [macOS], [Linux], [Windows],
  [Clear screen], [Fn-Left Arrow], [Home], [Home],
  [Break], [Left Option-B], [Alt-B], [Alt-B],
  [Quit], [Window close button or Ctrl-D], [Window close button or Ctrl-D],
  [Window close button or Ctrl-D],
)

APL mode is available only if APL support is enabled:

- Left Option-N switches to the APL character set.
- Left Option-O switches back to the normal character set.

All normal ASCII keys and control characters (Ctrl-C, Ctrl-D, etc.) are passed
through unchanged.

= Options

By default, the screen clears automatically when the bottom is reached. Use
`-noAutoClear` to disable this.

#table(
  columns: (1fr, 3fr),
  [Option], [Description],
  [`-fast`], [Fast rendering without fading.],
  [`-full`], [Full screen mode (Ctrl-Q to close).],
  [`-half`], [Use half of the screen width.],
  [`-b9600...`], [Emulate terminal baud rates.],
)

Some historical systems expect BS (^H, 0x08) as the erase character, while
others expect DEL (^?, 0x7F).

If the Backspace/Delete key does not work correctly, try:

```sh
tek4010 -eraseBS
```

or:

```sh
tek4010 -eraseDEL
```

= More information

More options and detailed information, including usage with historical
systems, can be found in the #link("manual.pdf")[full manual].

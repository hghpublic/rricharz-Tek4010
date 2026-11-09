#set document(title: "Tek4010 on macOS")
#set page(margin: 30mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[Tek4010 on macOS]
]

#outline(title: [Contents], depth: 2)

This page describes installation on macOS.

= Quick installation

Open a terminal and install the command-line tools:

```sh
xcode-select --install
```

Install #link("https://brew.sh")[Homebrew], then install the required packages:

```sh
brew install gtk+3 cairo pkg-config git
```

Download the source code:

```sh
git clone https://github.com/rricharz/Tek4010
cd Tek4010
```

Build and install:

```sh
make clean
make
make install
```

= First test

Run:

```sh
make check
```

This runs a demonstration and verifies that `tek4010` works.

= Notes

== Running tek4010

See the #link("quickstart.typ")[quick start guide] (also available as
#link("quickstart.md")[Markdown]).

= For experienced users

- The program is built using `make`.
- `make install` installs the binary in a user-local location.

Manual build steps:

```sh
make clean
make
make install
```

If the system changes in the future, adapt package installation and build
commands as required.

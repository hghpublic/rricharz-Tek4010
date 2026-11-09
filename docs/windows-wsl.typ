#set document(title: "Tek4010 on Windows with WSL")
#set page(margin: 30mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[Tek4010 on Windows]
]

#outline(title: [Contents], depth: 2)

This page describes installation on Windows using WSL (Windows Subsystem for
Linux) with Ubuntu.

= Quick installation

Open PowerShell and type:

```powershell
wsl --install -d Ubuntu
```

Reboot your system, then start Ubuntu from the Start menu.

Install the required packages:

```sh
sudo apt-get update
sudo apt-get install git libgtk-3-dev make gcc pkg-config
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

== Ubuntu setup

When Ubuntu starts for the first time, it will ask you to choose a user name
and password.

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

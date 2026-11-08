#set document(title: "MU/TH/UR")
#set par(justify: true)
#set page(margin: 40mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[MU/TH/UR]
]

#outline(title: [Contents], depth: 2)

There is no native or official vector font from Tektronix that looks like the MU/TH/UR interface, primarily because they come from two completely different design families.

The actual terminal screens in Alien were custom-built using a font called Berthold City Light (manually stretched horizontally to roughly 200% width), whereas genuine Tektronix 4010-series terminals used a rigid, hardware-coded monospaced dot/stroke font designed purely for telemetry data. [1, 2, 3, 4]

However, the retro-computing and Alien fan communities have bridged this gap with excellent replica fonts and terminal tools.

= Dedicated MU/TH/UR Replica Fonts (TrueType/OpenType)

If you want the exact typographic look seen on the Nostromo's screens, type designers have digitized the stretched, blocked look of the film's terminal interface into standard vector (.ttf/.otf) formats:

- #link("https://shoptly.com/i/3xpa")[MUT/TH/UR Replica Font]: A highly accurate replica designed specifically to mimic the MU-TH-UR 6000 interface. It includes custom character sets and OpenType vector graphics inspired by the Alien franchise's diagnostic icons. [5]
- City Light (Stretched): If you want to build it yourself natively, download a variant of Berthold City Light Regular. In any graphic tool (or terminal CSS config), set the font scaling to 200% width to achieve the exact geometric look of the film. [2, 3]

= True Vector / Stroke Fonts (Tektronix Style)

If you want fonts that behave like a true Tektronix vector terminal—meaning they are made of single vector lines (strokes) rather than filled outlines—you can use Plotutils/Plotfont vectors. [6]

- Designers archiving these vintage computers often extract the original terminal stroke paths. You can download modern conversions like #link("http://adoxa.altervista.org/terminalvector/index.html")[TerminalVector], which map old terminal telemetry fonts into scalable formats. [6, 7, 8]

= The Ultimate Shortcut: Cool-Retro-Term

If your goal is to make a modern computer look like the Nostromo terminal, the easiest path is to use #link("https://github.com/Swordfish90/cool-retro-term")[Cool-Retro-Term], a highly customizable terminal emulator that perfectly mimics cathode-ray tubes (CRTs), phosphor burn, and vector rendering. Within its community profiles, users have combined the stretched font settings and amber glow to flawlessly recreate the Mother command line. [3, 9]

Are you trying to configure a Linux/Mac terminal skin, loading a font into a game engine (like Unity or Unreal), or setting up a Raspberry Pi hardware prop? I can give you the exact configuration or file format you need based on your project.

== References

1. #link("https://en.wikipedia.org/wiki/Tektronix_4010")[https://en.wikipedia.org]
2. #link("https://www.reddit.com/r/LV426/comments/66c589/is_there_a_muthur_6000_font_available/")[https://www.reddit.com]
3. #link("https://github.com/Swordfish90/cool-retro-term/issues/168")[https://github.com]
4. #link("https://www.reddit.com/r/VintageComputers/comments/r06v44/an_old_tektronix_graphics_terminal_i_found_at_my/")[https://www.reddit.com]
5. #link("https://shoptly.com/i/3xpa")[https://shoptly.com]
6. #link("https://gitlab.com/oskay/svg-fonts/-/work_items/6")[https://gitlab.com]
7. #link("https://www.yohng.com/software/terminalvector.html")[https://www.yohng.com]
8. #link("http://adoxa.altervista.org/terminalvector/index.html")[https://adoxa.altervista.org]
9. #link("https://www.reddit.com/r/alienrpg/comments/udigma/anybody_know_a_good_website_to_make_green/")[https://www.reddit.com]

#set document(title: "Tek4010 GTK3 C++ Migration Log")
#set page(margin: 28mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 C++ Migration Log]
]
#align(center)[Updated: 2026-11-09]

= Purpose and scope

This log records the upstream source baseline for the C++ migration and the
upstream changes subsequently incorporated into it. The original C application
remains under `src/`; the GTK3/Cairo C++26 migration is under
`dev/Tek4010/cpp/tek4010-gtk3/`.

= Upstream baseline

#table(
  columns: (1fr, 3fr),
  [Item], [Value],
  [Upstream repository], [#link("https://github.com/rricharz/Tek4010")[rricharz/Tek4010]],
  [Fork repository], [#link("https://github.com/hghpublic/rricharz-Tek4010")[hghpublic/rricharz-Tek4010]],
  [Fork branch used as source], [`master`],
  [Base commit], [`cde8441d969ee5e9bc58301256cd7feb5e274a54`],
  [Commit subject], [Allow to abort GIN mode from host],
  [Commit date], [2026-10-26],
  [Migration target], [`dev/Tek4010/cpp/tek4010-gtk3/`],
)

At migration start, the fork's `master` and the configured `upstream/master`
both resolved to the base commit above. The initial migration scaffold was
introduced on 2026-11-08 by fork commit
`25a84a88cc62bdc3c4665a4d9007d7798d6af253`.

= Upstream update procedure

Treat the recorded base commit as the last upstream source state incorporated
into the C++ migration. When upstream receives new commits:

+ Fetch the upstream remote and inspect commits after the recorded base, for
  example with `git log --oneline cde8441d969ee5e9bc58301256cd7feb5e274a54..upstream/master`.
+ Review the upstream source and behavior changes; port relevant changes into
  `dev/Tek4010/cpp/tek4010-gtk3/` rather than replacing the migrated
  implementation with upstream C files.
+ Build and run the relevant tests after porting.
+ Once a batch is incorporated and validated, append a log entry with the
  upstream commit range, the ported changes, and validation evidence. Then
  advance the recorded base commit to the newest upstream commit fully
  represented in the C++ migration.

Do not advance the baseline merely because `upstream/master` moves. Keep the
current hash as the comparison point until the corresponding changes have been
ported and validated.

= Migration entries

#table(
  columns: (auto, 1fr, 2fr),
  [Date], [Upstream base], [Migration record],
  [2026-11-08], [`cde8441d969ee5e9bc58301256cd7feb5e274a54`],
  [Established the GTK3/Cairo C++26 migration scaffold under the target
  directory while retaining the upstream C implementation.],
  [2026-11-09], [Same baseline],
  [Continued the migration with per-instance core state, renderer-neutral
  operations, GTK/Cairo host separation, and parser/session encapsulation.
  Details and validation history are in
  `dev/copilot/chats/tek4010/tek4010-handoff.md`.],
)

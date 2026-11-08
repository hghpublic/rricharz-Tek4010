meta:
  id: tektronix_plt
  title: Tektronix 4010/4014 PLT byte stream
  file-extension: plt

doc: |
  A captured stream of bytes sent to a Tektronix 4010/4014-compatible
  terminal. This Kaitai Struct schema describes the byte-level container:
  the stream has no file header, record lengths, or in-band end marker.
  End of file terminates a capture, but a capture may end partway through a
  stateful command.

  Byte meanings depend on the terminal's current parser mode. Printable
  bytes can be text in alpha mode, coordinate data in vector modes, or
  commands in other modes. In particular, coordinate data is not a fixed
  record: the parser consumes tagged 5-bit values in a mode-dependent
  sequence. The 4014 extended coordinate interpretation also uses extra
  coordinate bits retained from earlier bytes.

  This schema preserves each input octet and labels known control values;
  it does not attempt to decode state-dependent commands or coordinates.
  The GTK3 emulator interprets only the low seven bits of each input octet.

seq:
  - id: bytes
    type: u1
    enum: control_code
    repeat: eos
    doc: |
      One captured octet. Enum names identify recognized control values,
      not context-independent meanings. Other values remain valid bytes and
      may be text, coordinate components, or mode-specific commands.

enums:
  control_code:
    0x00: nul
    0x05: enquiry
    0x07: bell
    0x08: backspace
    0x09: horizontal_tab
    0x0a: line_feed
    0x0b: vertical_tab
    0x0c: form_feed
    0x0d: carriage_return
    0x0e: shift_out
    0x0f: shift_in
    0x17: etb
    0x1a: substitute
    0x1b: escape
    0x1c: file_separator
    0x1d: group_separator
    0x1e: record_separator
    0x1f: unit_separator

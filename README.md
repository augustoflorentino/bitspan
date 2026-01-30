# bitspan

Reads and writes integer fields of 1 to 64 bits at any bit offset of a byte buffer. It was
written for HID reports and works the same for binary file formats and bitstreams. C99, no
allocation, no state.

[![release](https://img.shields.io/github/v/release/augustoflorentino/bitspan?style=flat-square)](https://github.com/augustoflorentino/bitspan/releases/latest)
[![license](https://img.shields.io/github/license/augustoflorentino/bitspan?style=flat-square)](LICENSE)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/layout-dark.svg">
  <img src="docs/images/layout-light.svg" alt="A six-byte report read with bitf_get_many: two one-bit buttons, a signed 8-bit trigger, a signed 16-bit stick and a 16-bit counter" width="820">
</picture>

That report, read by [`examples/basic.c`](examples/basic.c):

```c
static const unsigned char report[] = {0x05, 0x80, 0xff, 0x7f, 0x34, 0x12};
static const bitf_field_t fields[] = {
    {0, 1, BITF_LE},
    {2, 1, BITF_LE},
    {8, 8, BITF_LE | BITF_SIGNED},
    {16, 16, BITF_LE | BITF_SIGNED},
    {32, 16, BITF_LE},
};
static const char *const names[] = {"button a", "button b", "trigger", "stick x", "counter"};
bitf_value_t values[5];
size_t bad = 0;

if (bitf_get_many(report, sizeof(report), fields, 5, values, &bad) != BITF_OK)
    return 1;
```

```console
$ ./basic
button a 1
button b 1
trigger  -128
stick x  32767
counter  4660
```

## Why not C bit-fields

Their layout is up to the compiler and they know nothing about byte order, so they cannot
describe a report someone else defined. Every parser then grows its own shifts and masks, and
those fail in the same places: a field crossing a byte boundary, sign extension of an odd width,
MSB-first numbering, a width of 64. bitspan does that arithmetic once, checks it against the
buffer length, and is tested and fuzzed on exactly those edges.

## What is in it

- two layouts: `BITF_LE`, least significant bit first (USB/HID, most file formats), and
  `BITF_BE`, most significant bit first (network protocols, bitstreams)
- signed fields of any width with `BITF_SIGNED`
- checked calls that never touch memory outside the buffer, plus inline unchecked readers and
  writers for hot loops that ran one check up front
- lists of fields read or written in one call, validated before anything is written
- `bitf_copy` for runs of any length between two buffers, overlap included, and `bitf_fill` to
  set a run to 0 or 1

## Building

```sh
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
cmake --build build
cmake --install build --prefix /usr/local
```

CMake projects use `find_package(bitspan REQUIRED)` and `bitspan::bitspan`; anything else gets
the flags from `pkg-config --cflags --libs bitspan`.

To vendor it instead, run `scripts/amalgamate.sh` and copy `dist/bitspan/bitspan.h`. One `.c`
file defines `BITF_IMPLEMENTATION` before including it; `BITF_STATIC` keeps the functions
private to that file.

## API notes

Every function is declared in [`include/bitspan/bitspan.h`](include/bitspan/bitspan.h).

- `bit_offset` counts from the start of the buffer. With `BITF_LE`, bit 0 is the least
  significant bit of byte 0; with `BITF_BE`, the most significant one.
- Calls that can fail return `BITF_OK` or one of `BITF_ERR_ARG` (null pointer, width 0 or
  above 64, unknown flag), `BITF_ERR_BOUNDS` (field outside the buffer) and `BITF_ERR_RANGE`
  (value wider than the field); `bitf_strerror` turns them into text.
- A failed call writes nothing. `bitf_set_many` checks every field and value first, and the
  list calls report the first bad field through `out_bad_index`.
- List values are `bitf_value_t`: `.s` for `BITF_SIGNED` fields, `.u` for the others.
- `bitf_get_unchecked`, `bitf_set_unchecked` and the rest of that family trust that
  `bitf_check` or `bitf_fields_check` ran before. The writer keeps only the low `bit_len` bits.
- Names ending in `_` belong to the inline functions and may change in any release.
- There is no state, so calls on different buffers can run in parallel.

## Compatibility

3.1 is the last feature release planned; from here on, fixes. Inside a major version no
function, macro, flag or struct member goes away or changes meaning, and `bitf_field_t` keeps
its layout. Anything due to be removed is marked deprecated in the [CHANGELOG](CHANGELOG.md) one
major version earlier, and an incompatible release bumps both the major version and the soname.

## Working on it

```sh
cmake --preset dev && cmake --build build/dev && ctest --preset dev
cmake --preset fuzz && cmake --build build/fuzz
```

The `dev` preset turns on ASan, UBSan and `-Werror`; the `fuzz` one builds
`build/fuzz/fuzz_roundtrip`. CI runs the `dev` preset, builds with MSVC at `/W4 /WX`, installs
on Linux, macOS and Windows and links a program against the result, and compares the exported
symbols and the ABI with the 3.0 baseline in `abi/`.

Bugs go to the [issues](https://github.com/augustoflorentino/bitspan/issues), anything that
reads or writes out of bounds to [SECURITY.md](SECURITY.md). MIT licensed, see
[LICENSE](LICENSE).

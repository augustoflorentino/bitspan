# Changelog

Each release lists what changed, what updating means for existing code, and every security fix.

## Unreleased

Incompatible with 1.x: the soname is now `libbitspan.so.2`.

- `bitf_fields_check`, `bitf_get_many` and `bitf_set_many` take a last argument,
  `size_t *out_bad_index`, which receives the index of the field that failed. With a long
  list, the error code alone did not say which field was out of the buffer or out of range

### Upgrading

Add `NULL` as the last argument to keep the old behaviour, or pass a `size_t` to learn which
field failed.

## 1.1.2 - 2026-01-25

- `bitf_copy`: copy any number of bits from one buffer to another, at any bit offset on
  either side
- `bitf_copy` within one buffer, with the destination after an overlapping source, no
  longer copies bits it has already overwritten

## 1.0.0 - 2026-01-25

The API is now stable: see Versioning in the README for what that covers.

- the header is checked to build as C++ with `-Wconversion -Wold-style-cast`
- security fixes are promised for 6 months to the previous major version

## 0.3.0 - 2026-01-15

- `BITF_VERSION_MAJOR`, `BITF_VERSION_MINOR`, `BITF_VERSION_PATCH`, `BITF_VERSION` and
  `bitf_version()`, to compare the header with the library linked at run time
- `bitf_set_unchecked`, `bitf_set_le_unchecked`, `bitf_set_be_unchecked`: inline writers
  without checks, the counterpart of the unchecked readers

## 0.1.1 - 2026-01-13

- the single-file form is now written to `dist/bitspan/bitspan.h`
- `bitf_get_unchecked` with a width of 0 and `BITF_SIGNED` no longer shifts out of range

## 0.1.0 - 2026-01-13

First release.

- `bitf_get`, `bitf_get_signed`, `bitf_set`, `bitf_set_signed`: one field of 1 to 64 bits at
  any bit offset, little endian (`BITF_LE`) or big endian (`BITF_BE`), with bounds and range
  checks
- `bitf_check`, `bitf_fields_check`: validate a field or a field list once
- `bitf_get_many`, `bitf_set_many`: batch access to a field list; writes are all or nothing
- `bitf_get_unchecked`, `bitf_get_le_unchecked`, `bitf_get_be_unchecked`: inline readers
  without checks for hot loops

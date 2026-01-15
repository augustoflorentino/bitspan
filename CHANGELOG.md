# Changelog

Each release lists what changed, what updating means for existing code, and every security fix.

## Unreleased

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

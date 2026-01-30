#include "bitspan/bitspan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                    \
        }                                                               \
    } while (0)

#define BUF_LEN 11

static uint64_t rng_state = 0x9e3779b97f4a7c15u;

static uint64_t rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static void fill_random(unsigned char *buf, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        buf[i] = (unsigned char)rng_next();
}

static unsigned ref_bit(const unsigned char *buf, size_t pos, unsigned flags)
{
    unsigned bit = (flags & BITF_BE) ? 7u - (unsigned)(pos % 8u) : (unsigned)(pos % 8u);

    return (buf[pos / 8u] >> bit) & 1u;
}

static void ref_put_bit(unsigned char *buf, size_t pos, unsigned flags, unsigned value)
{
    unsigned bit = (flags & BITF_BE) ? 7u - (unsigned)(pos % 8u) : (unsigned)(pos % 8u);

    buf[pos / 8u] = (unsigned char)((buf[pos / 8u] & ~(1u << bit)) | (value << bit));
}

static uint64_t ref_get(const unsigned char *buf, size_t off, unsigned len, unsigned flags)
{
    uint64_t value = 0;
    unsigned i;

    for (i = 0; i < len; i++) {
        unsigned bit = ref_bit(buf, off + i, flags);

        if (flags & BITF_BE)
            value = (value << 1) | bit;
        else
            value |= (uint64_t)bit << i;
    }
    if ((flags & BITF_SIGNED) && len < 64u && (value >> (len - 1u)) & 1u)
        value |= UINT64_MAX << len;
    return value;
}

static void ref_set(unsigned char *buf, size_t off, unsigned len, unsigned flags, uint64_t value)
{
    unsigned i;

    for (i = 0; i < len; i++) {
        unsigned shift = (flags & BITF_BE) ? len - 1u - i : i;

        ref_put_bit(buf, off + i, flags, (unsigned)(value >> shift) & 1u);
    }
}

static uint64_t truncate_bits(uint64_t value, unsigned len)
{
    return len >= 64u ? value : value & (((uint64_t)1 << len) - 1u);
}

static void test_exhaustive_get_set(void)
{
    static const unsigned orders[] = {BITF_LE, BITF_BE};
    unsigned char buf[BUF_LEN];
    unsigned char want[BUF_LEN];
    unsigned char got[BUF_LEN];
    unsigned o;
    unsigned len;
    size_t off;
    int round;

    for (round = 0; round < 8; round++) {
        fill_random(buf, sizeof(buf));
        for (o = 0; o < 2; o++) {
            for (len = 1; len <= 64u; len++) {
                for (off = 0; off + len <= BUF_LEN * 8u; off++) {
                    unsigned flags = orders[o];
                    uint64_t value = 0;
                    int64_t svalue = 0;
                    uint64_t fresh = truncate_bits(rng_next(), len);

                    CHECK(bitf_get(buf, sizeof(buf), off, len, flags, &value) == BITF_OK);
                    CHECK(value == ref_get(buf, off, len, flags));
                    CHECK(bitf_get_unchecked(buf, off, len, flags) == value);
                    CHECK(bitf_get_signed(buf, sizeof(buf), off, len, flags, &svalue) == BITF_OK);
                    CHECK((uint64_t)svalue == ref_get(buf, off, len, flags | BITF_SIGNED));

                    memcpy(want, buf, sizeof(buf));
                    memcpy(got, buf, sizeof(buf));
                    ref_set(want, off, len, flags, fresh);
                    CHECK(bitf_set(got, sizeof(got), off, len, flags, fresh) == BITF_OK);
                    CHECK(memcmp(want, got, sizeof(got)) == 0);
                }
            }
        }
    }
}

static void test_known_values(void)
{
    static const unsigned char le[] = {0x34, 0x12, 0xff};
    static const unsigned char be[] = {0x12, 0x34, 0x80};
    uint64_t value;
    int64_t svalue;

    CHECK(bitf_get(le, sizeof(le), 0, 16, BITF_LE, &value) == BITF_OK && value == 0x1234);
    CHECK(bitf_get(be, sizeof(be), 0, 16, BITF_BE, &value) == BITF_OK && value == 0x1234);
    CHECK(bitf_get(le, sizeof(le), 4, 8, BITF_LE, &value) == BITF_OK && value == 0x23);
    CHECK(bitf_get(be, sizeof(be), 4, 8, BITF_BE, &value) == BITF_OK && value == 0x23);
    CHECK(bitf_get_signed(le, sizeof(le), 16, 8, BITF_LE, &svalue) == BITF_OK && svalue == -1);
    CHECK(bitf_get_signed(be, sizeof(be), 16, 1, BITF_BE, &svalue) == BITF_OK && svalue == -1);
    CHECK(bitf_get_signed(le, sizeof(le), 0, 16, BITF_LE, &svalue) == BITF_OK && svalue == 0x1234);
}

static void test_signed_range(void)
{
    unsigned char buf[8] = {0};
    int64_t svalue;

    CHECK(bitf_set_signed(buf, sizeof(buf), 3, 4, BITF_LE, -8) == BITF_OK);
    CHECK(bitf_get_signed(buf, sizeof(buf), 3, 4, BITF_LE, &svalue) == BITF_OK && svalue == -8);
    CHECK(bitf_set_signed(buf, sizeof(buf), 3, 4, BITF_LE, 7) == BITF_OK);
    CHECK(bitf_set_signed(buf, sizeof(buf), 3, 4, BITF_LE, 8) == BITF_ERR_RANGE);
    CHECK(bitf_set_signed(buf, sizeof(buf), 3, 4, BITF_LE, -9) == BITF_ERR_RANGE);
    CHECK(bitf_set_signed(buf, sizeof(buf), 0, 64, BITF_BE, INT64_MIN) == BITF_OK);
    CHECK(bitf_get_signed(buf, sizeof(buf), 0, 64, BITF_BE, &svalue) == BITF_OK);
    CHECK(svalue == INT64_MIN);
    CHECK(bitf_set(buf, sizeof(buf), 0, 4, BITF_LE, 16) == BITF_ERR_RANGE);
    CHECK(bitf_set(buf, sizeof(buf), 0, 64, BITF_LE, UINT64_MAX) == BITF_OK);
}

static void test_bounds_and_args(void)
{
    unsigned char buf[4] = {0};
    uint64_t value;

    CHECK(bitf_check(4, 31, 1, BITF_LE) == BITF_OK);
    CHECK(bitf_check(4, 31, 2, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_check(4, 32, 1, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_check(0, 0, 1, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_check(4, SIZE_MAX, 8, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_check(SIZE_MAX, SIZE_MAX - 7u, 8, BITF_LE) == BITF_OK);
    CHECK(bitf_check(SIZE_MAX / 8u, SIZE_MAX - 7u, 64, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_check(4, 0, 0, BITF_LE) == BITF_ERR_ARG);
    CHECK(bitf_check(16, 0, 65, BITF_LE) == BITF_ERR_ARG);
    CHECK(bitf_check(4, 0, 8, 0x10) == BITF_ERR_ARG);
    CHECK(bitf_get(NULL, 4, 0, 8, BITF_LE, &value) == BITF_ERR_ARG);
    CHECK(bitf_get(buf, 4, 0, 8, BITF_LE, NULL) == BITF_ERR_ARG);
    CHECK(bitf_get_signed(buf, 4, 0, 8, BITF_LE, NULL) == BITF_ERR_ARG);
    CHECK(bitf_set(NULL, 4, 0, 8, BITF_LE, 0) == BITF_ERR_ARG);
    CHECK(bitf_set(buf, 4, 30, 8, BITF_LE, 0) == BITF_ERR_BOUNDS);
    CHECK(bitf_strerror(BITF_ERR_RANGE) != NULL);
    CHECK(strcmp(bitf_strerror(12345), "unknown status") == 0);
}

static void test_many(void)
{
    static const bitf_field_t fields[] = {
        {0, 1, BITF_LE},
        {1, 7, BITF_LE},
        {8, 16, BITF_LE | BITF_SIGNED},
        {24, 12, BITF_BE},
        {36, 4, BITF_BE | BITF_SIGNED},
    };
    bitf_value_t values[5];
    unsigned char buf[5] = {0};
    bitf_value_t out[5];
    static const bitf_field_t bad[] = {{0, 8, BITF_LE}, {8, 0, BITF_LE}, {16, 8, 0x10}};
    size_t min_len = 0;
    size_t bad_index = 99;
    size_t i;

    values[0].u = 1;
    values[1].u = 0x55;
    values[2].s = -300;
    values[3].u = 0xabc;
    values[4].s = -2;
    CHECK(bitf_fields_check(fields, 5, &min_len, NULL) == BITF_OK && min_len == 5);
    CHECK(bitf_set_many(buf, sizeof(buf), fields, 5, values, NULL) == BITF_OK);
    CHECK(bitf_get_many(buf, sizeof(buf), fields, 5, out, &bad_index) == BITF_OK);
    CHECK(bad_index == 99);
    for (i = 0; i < 5; i++)
        CHECK(out[i].u == values[i].u);
    CHECK(out[2].s == -300 && out[4].s == -2);
    CHECK(bitf_get_many(buf, 4, fields, 5, out, &bad_index) == BITF_ERR_BOUNDS);
    CHECK(bad_index == 3);
    CHECK(bitf_set_many(buf, 2, fields, 5, values, &bad_index) == BITF_ERR_BOUNDS);
    CHECK(bad_index == 2);
    CHECK(bitf_fields_check(bad, 3, &min_len, &bad_index) == BITF_ERR_ARG && bad_index == 1);
    CHECK(bitf_get_many(buf, sizeof(buf), NULL, 0, NULL, NULL) == BITF_OK);
    CHECK(bitf_fields_check(NULL, 0, &min_len, NULL) == BITF_OK && min_len == 0);
    CHECK(bitf_fields_check(fields, 5, NULL, NULL) == BITF_ERR_ARG);
}

static void test_many_range_is_atomic(void)
{
    static const bitf_field_t fields[] = {{0, 8, BITF_LE}, {8, 4, BITF_LE}};
    bitf_value_t values[2];
    unsigned char buf[2] = {0x11, 0x22};
    size_t bad_index = 0;

    values[0].u = 0xaa;
    values[1].u = 0x1f;
    CHECK(bitf_set_many(buf, sizeof(buf), fields, 2, values, &bad_index) == BITF_ERR_RANGE);
    CHECK(bad_index == 1);
    CHECK(buf[0] == 0x11 && buf[1] == 0x22);
}

static void test_version(void)
{
    CHECK(bitf_version() == BITF_VERSION);
    CHECK(BITF_VERSION == BITF_VERSION_MAJOR * 10000 + BITF_VERSION_MINOR * 100 + BITF_VERSION_PATCH);
}

static void test_unchecked_writes(void)
{
    static const unsigned flag_sets[] = {BITF_LE, BITF_BE};
    unsigned char got[BUF_LEN];
    unsigned char want[BUF_LEN];
    unsigned f;
    unsigned len;
    size_t off;

    for (f = 0; f < 2u; f++)
        for (len = 1; len <= 64u; len++)
            for (off = 0; off + len <= BUF_LEN * 8u; off += 3u) {
                uint64_t value = rng_next();

                fill_random(got, BUF_LEN);
                memcpy(want, got, BUF_LEN);
                bitf_set_unchecked(got, off, len, flag_sets[f], value);
                ref_set(want, off, len, flag_sets[f], truncate_bits(value, len));
                CHECK(memcmp(got, want, BUF_LEN) == 0);
            }
}

static void test_copy(void)
{
    static const unsigned flag_sets[] = {BITF_LE, BITF_BE};
    unsigned char src[BUF_LEN];
    unsigned char got[BUF_LEN];
    unsigned char want[BUF_LEN];
    size_t so;
    size_t dof;
    size_t n;
    size_t i;
    unsigned f;

    for (f = 0; f < 2u; f++)
        for (so = 0; so < 20u; so += 3u)
            for (dof = 0; dof < 20u; dof += 5u)
                for (n = 0; dof + n <= BUF_LEN * 8u && so + n <= BUF_LEN * 8u; n += 7u) {
                    fill_random(src, BUF_LEN);
                    fill_random(got, BUF_LEN);
                    memcpy(want, got, BUF_LEN);
                    for (i = 0; i < n; i++)
                        ref_put_bit(want, dof + i, flag_sets[f], ref_bit(src, so + i, flag_sets[f]));
                    CHECK(bitf_copy(got, BUF_LEN, dof, src, BUF_LEN, so, n, flag_sets[f]) == BITF_OK);
                    CHECK(memcmp(got, want, BUF_LEN) == 0);
                }
    CHECK(bitf_copy(got, BUF_LEN, 1, src, BUF_LEN, 0, BUF_LEN * 8u, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_copy(got, BUF_LEN, 0, src, BUF_LEN, 1, BUF_LEN * 8u, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_copy(got, BUF_LEN, BUF_LEN * 8u, src, BUF_LEN, 0, 0, BITF_LE) == BITF_OK);
    CHECK(bitf_copy(got, BUF_LEN, BUF_LEN * 8u + 8u, src, BUF_LEN, 0, 0, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_copy(got, BUF_LEN, BUF_LEN * 8u + 1u, src, BUF_LEN, 0, 0, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_copy(got, BUF_LEN, 0, src, BUF_LEN, 0, SIZE_MAX, BITF_LE) == BITF_ERR_BOUNDS);
    CHECK(bitf_copy(got, BUF_LEN, 0, src, BUF_LEN, 0, 8, BITF_SIGNED) == BITF_ERR_ARG);
    CHECK(bitf_copy(NULL, 0, 0, src, BUF_LEN, 0, 0, BITF_LE) == BITF_ERR_ARG);
}

static void test_copy_overlap(void)
{
    static const unsigned flag_sets[] = {BITF_LE, BITF_BE};
    unsigned char got[BUF_LEN];
    unsigned char want[BUF_LEN];
    unsigned char orig[BUF_LEN];
    size_t so;
    size_t dof;
    size_t n;
    size_t i;
    unsigned f;

    for (f = 0; f < 2u; f++)
        for (so = 0; so < 30u; so += 7u)
            for (dof = 0; dof < 30u; dof += 3u)
                for (n = 0; dof + n <= BUF_LEN * 8u && so + n <= BUF_LEN * 8u; n += 11u) {
                    fill_random(orig, BUF_LEN);
                    memcpy(got, orig, BUF_LEN);
                    memcpy(want, orig, BUF_LEN);
                    for (i = 0; i < n; i++)
                        ref_put_bit(want, dof + i, flag_sets[f], ref_bit(orig, so + i, flag_sets[f]));
                    CHECK(bitf_copy(got, BUF_LEN, dof, got, BUF_LEN, so, n, flag_sets[f]) == BITF_OK);
                    CHECK(memcmp(got, want, BUF_LEN) == 0);
                }
}

static void test_fill(void)
{
    static const unsigned flag_sets[] = {BITF_LE, BITF_BE};
    unsigned char got[BUF_LEN];
    unsigned char want[BUF_LEN];
    size_t off;
    size_t n;
    size_t i;
    unsigned f;
    int bit;

    for (f = 0; f < 2u; f++)
        for (bit = 0; bit < 2; bit++)
            for (off = 0; off < BUF_LEN * 8u; off++)
                for (n = 0; off + n <= BUF_LEN * 8u; n++) {
                    fill_random(got, BUF_LEN);
                    memcpy(want, got, BUF_LEN);
                    for (i = 0; i < n; i++)
                        ref_put_bit(want, off + i, flag_sets[f], (unsigned)bit);
                    CHECK(bitf_fill(got, BUF_LEN, off, n, flag_sets[f], bit) == BITF_OK);
                    CHECK(memcmp(got, want, BUF_LEN) == 0);
                }
    CHECK(bitf_fill(got, BUF_LEN, 1, BUF_LEN * 8u, BITF_LE, 1) == BITF_ERR_BOUNDS);
    CHECK(bitf_fill(got, BUF_LEN, 0, 1, BITF_SIGNED, 1) == BITF_ERR_ARG);
    CHECK(bitf_fill(NULL, BUF_LEN, 0, 1, BITF_LE, 1) == BITF_ERR_ARG);
}

int main(void)
{
    test_version();
    test_unchecked_writes();
    test_copy();
    test_copy_overlap();
    test_fill();
    test_exhaustive_get_set();
    test_known_values();
    test_signed_range();
    test_bounds_and_args();
    test_many();
    test_many_range_is_atomic();
    return 0;
}

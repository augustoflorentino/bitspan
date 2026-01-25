#ifndef BITSPAN_H
#define BITSPAN_H

#include <stddef.h>
#include <stdint.h>

#if defined(BITF_STATIC) && defined(__GNUC__)
#define BITF_API static __attribute__((unused))
#elif defined(BITF_STATIC)
#define BITF_API static
#elif defined(_WIN32) && defined(BITF_BUILD_SHARED)
#define BITF_API __declspec(dllexport)
#elif defined(_WIN32) && defined(BITF_SHARED)
#define BITF_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(BITF_BUILD_SHARED)
#define BITF_API __attribute__((visibility("default")))
#else
#define BITF_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BITF_VERSION_MAJOR 1
#define BITF_VERSION_MINOR 1
#define BITF_VERSION_PATCH 2
#define BITF_VERSION (BITF_VERSION_MAJOR * 10000 + BITF_VERSION_MINOR * 100 + BITF_VERSION_PATCH)

#define BITF_OK 0
#define BITF_ERR_ARG 1
#define BITF_ERR_BOUNDS 2
#define BITF_ERR_RANGE 3

#define BITF_LE 0x0
#define BITF_BE 0x1
#define BITF_SIGNED 0x2
#define BITF_FLAGS_MASK 0x3

#define BITF_MAX_BITS 64u

typedef struct bitf_field {
    size_t bit_offset;
    unsigned bit_len;
    unsigned flags;
} bitf_field_t;

BITF_API int bitf_get(const unsigned char *data, size_t data_len, size_t bit_offset,
                      unsigned bit_len, unsigned flags, uint64_t *out_value);
BITF_API int bitf_get_signed(const unsigned char *data, size_t data_len, size_t bit_offset,
                             unsigned bit_len, unsigned flags, int64_t *out_value);
BITF_API int bitf_set(unsigned char *data, size_t data_len, size_t bit_offset,
                      unsigned bit_len, unsigned flags, uint64_t value);
BITF_API int bitf_set_signed(unsigned char *data, size_t data_len, size_t bit_offset,
                             unsigned bit_len, unsigned flags, int64_t value);
BITF_API int bitf_check(size_t data_len, size_t bit_offset, unsigned bit_len, unsigned flags);
BITF_API int bitf_fields_check(const bitf_field_t *fields, size_t fields_len,
                               size_t *out_min_data_len);
BITF_API int bitf_get_many(const unsigned char *data, size_t data_len,
                           const bitf_field_t *fields, size_t fields_len, uint64_t *out_values);
BITF_API int bitf_set_many(unsigned char *data, size_t data_len, const bitf_field_t *fields,
                           size_t fields_len, const uint64_t *values);
BITF_API int bitf_copy(unsigned char *dst, size_t dst_len, size_t dst_offset,
                       const unsigned char *src, size_t src_len, size_t src_offset,
                       size_t bit_count, unsigned flags);
BITF_API const char *bitf_strerror(int status);
BITF_API int bitf_version(void);

static inline uint64_t bitf_mask_(unsigned bit_len)
{
    return bit_len >= 64u ? UINT64_MAX : (((uint64_t)1 << bit_len) - 1u);
}

static inline uint64_t bitf_extend_(uint64_t value, unsigned bit_len)
{
    uint64_t sign;

    if (bit_len == 0u || bit_len >= 64u)
        return value;
    sign = (uint64_t)1 << (bit_len - 1u);
    return (value ^ sign) - sign;
}

static inline uint64_t bitf_get_le_unchecked(const unsigned char *data, size_t bit_offset,
                                             unsigned bit_len)
{
    const unsigned char *p = data + bit_offset / 8u;
    unsigned shift = (unsigned)(bit_offset % 8u);
    unsigned total = shift + bit_len;
    uint64_t acc = 0;
    unsigned i;

    for (i = 0; i * 8u < total && i < 8u; i++)
        acc |= (uint64_t)p[i] << (8u * i);
    acc >>= shift;
    if (total > 64u)
        acc |= (uint64_t)p[8] << (64u - shift);
    return acc & bitf_mask_(bit_len);
}

static inline uint64_t bitf_get_be_unchecked(const unsigned char *data, size_t bit_offset,
                                             unsigned bit_len)
{
    const unsigned char *p = data + bit_offset / 8u;
    unsigned shift = (unsigned)(bit_offset % 8u);
    unsigned total = shift + bit_len;
    unsigned nbytes = (total + 7u) / 8u;
    uint64_t acc = 0;
    unsigned i;

    for (i = 0; i < nbytes && i < 8u; i++)
        acc = (acc << 8) | p[i];
    if (nbytes > 8u)
        return ((acc << (8u - (72u - total))) | (p[8] >> (72u - total))) & bitf_mask_(bit_len);
    return (acc >> (nbytes * 8u - total)) & bitf_mask_(bit_len);
}

static inline uint64_t bitf_get_unchecked(const unsigned char *data, size_t bit_offset,
                                          unsigned bit_len, unsigned flags)
{
    uint64_t value = (flags & BITF_BE) ? bitf_get_be_unchecked(data, bit_offset, bit_len)
                                       : bitf_get_le_unchecked(data, bit_offset, bit_len);

    return (flags & BITF_SIGNED) ? bitf_extend_(value, bit_len) : value;
}

static inline void bitf_set_le_unchecked(unsigned char *data, size_t bit_offset, unsigned bit_len,
                                         uint64_t value)
{
    unsigned char *p = data + bit_offset / 8u;
    unsigned pos = (unsigned)(bit_offset % 8u);
    unsigned left = bit_len;

    while (left > 0u) {
        unsigned take = 8u - pos < left ? 8u - pos : left;
        unsigned mask = ((1u << take) - 1u) << pos;

        *p = (unsigned char)((*p & ~mask) | (((unsigned)value << pos) & mask));
        value >>= take;
        left -= take;
        pos = 0;
        p++;
    }
}

static inline void bitf_set_be_unchecked(unsigned char *data, size_t bit_offset, unsigned bit_len,
                                         uint64_t value)
{
    unsigned char *p = data + bit_offset / 8u;
    unsigned pos = (unsigned)(bit_offset % 8u);
    unsigned left = bit_len;

    while (left > 0u) {
        unsigned take = 8u - pos < left ? 8u - pos : left;
        unsigned shift = 8u - pos - take;
        unsigned mask = ((1u << take) - 1u) << shift;
        unsigned chunk = (unsigned)(value >> (left - take)) & ((1u << take) - 1u);

        *p = (unsigned char)((*p & ~mask) | (chunk << shift));
        left -= take;
        pos = 0;
        p++;
    }
}

static inline void bitf_set_unchecked(unsigned char *data, size_t bit_offset, unsigned bit_len,
                                      unsigned flags, uint64_t value)
{
    value &= bitf_mask_(bit_len);
    if (flags & BITF_BE)
        bitf_set_be_unchecked(data, bit_offset, bit_len, value);
    else
        bitf_set_le_unchecked(data, bit_offset, bit_len, value);
}

#ifdef __cplusplus
}
#endif

#endif

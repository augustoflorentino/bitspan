#include "bitspan/bitspan.h"

typedef char bitf_field_layout_check[sizeof(bitf_field_t) == sizeof(size_t) + 2 * sizeof(unsigned) ? 1 : -1];

int bitf_check(size_t data_len, size_t bit_offset, unsigned bit_len, unsigned flags)
{
    size_t first_byte = bit_offset / 8u;
    size_t end_bits = bit_offset % 8u + bit_len;

    if (bit_len == 0u || bit_len > BITF_MAX_BITS || (flags & ~(unsigned)BITF_FLAGS_MASK) != 0u)
        return BITF_ERR_ARG;
    if (first_byte >= data_len || (end_bits + 7u) / 8u > data_len - first_byte)
        return BITF_ERR_BOUNDS;
    return BITF_OK;
}

int bitf_get(const unsigned char *data, size_t data_len, size_t bit_offset,
                      unsigned bit_len, unsigned flags, uint64_t *out_value)
{
    int status;

    if (data == NULL || out_value == NULL)
        return BITF_ERR_ARG;
    status = bitf_check(data_len, bit_offset, bit_len, flags);
    if (status != BITF_OK)
        return status;
    *out_value = bitf_get_unchecked(data, bit_offset, bit_len, flags);
    return BITF_OK;
}

int bitf_get_signed(const unsigned char *data, size_t data_len, size_t bit_offset,
                             unsigned bit_len, unsigned flags, int64_t *out_value)
{
    uint64_t value;
    int status;

    if (out_value == NULL)
        return BITF_ERR_ARG;
    status = bitf_get(data, data_len, bit_offset, bit_len, flags | BITF_SIGNED, &value);
    if (status != BITF_OK)
        return status;
    *out_value = value <= (uint64_t)INT64_MAX ? (int64_t)value
                                              : -(int64_t)(~value) - 1;
    return BITF_OK;
}

static int bitf_fits(uint64_t value, unsigned bit_len, unsigned flags)
{
    uint64_t high;

    if (bit_len >= 64u)
        return 1;
    if (!(flags & BITF_SIGNED))
        return value <= bitf_mask_(bit_len);
    high = value >> (bit_len - 1u);
    return high == 0u || high == UINT64_MAX >> (bit_len - 1u);
}

int bitf_set(unsigned char *data, size_t data_len, size_t bit_offset, unsigned bit_len,
                      unsigned flags, uint64_t value)
{
    int status;

    if (data == NULL)
        return BITF_ERR_ARG;
    status = bitf_check(data_len, bit_offset, bit_len, flags);
    if (status != BITF_OK)
        return status;
    if (!bitf_fits(value, bit_len, flags))
        return BITF_ERR_RANGE;
    bitf_set_unchecked(data, bit_offset, bit_len, flags, value);
    return BITF_OK;
}

int bitf_set_signed(unsigned char *data, size_t data_len, size_t bit_offset,
                             unsigned bit_len, unsigned flags, int64_t value)
{
    return bitf_set(data, data_len, bit_offset, bit_len, flags | BITF_SIGNED, (uint64_t)value);
}

int bitf_fields_check(const bitf_field_t *fields, size_t fields_len,
                               size_t *out_min_data_len)
{
    size_t min_len = 0;
    size_t i;

    if ((fields == NULL && fields_len > 0u) || out_min_data_len == NULL)
        return BITF_ERR_ARG;
    for (i = 0; i < fields_len; i++) {
        const bitf_field_t *f = &fields[i];
        size_t first_byte = f->bit_offset / 8u;
        size_t nbytes = (f->bit_offset % 8u + f->bit_len + 7u) / 8u;

        if (f->bit_len == 0u || f->bit_len > BITF_MAX_BITS
            || (f->flags & ~(unsigned)BITF_FLAGS_MASK) != 0u)
            return BITF_ERR_ARG;
        if (first_byte > SIZE_MAX - nbytes)
            return BITF_ERR_BOUNDS;
        if (first_byte + nbytes > min_len)
            min_len = first_byte + nbytes;
    }
    *out_min_data_len = min_len;
    return BITF_OK;
}

int bitf_get_many(const unsigned char *data, size_t data_len,
                           const bitf_field_t *fields, size_t fields_len, uint64_t *out_values)
{
    size_t min_len;
    size_t i;
    int status;

    if ((data == NULL || out_values == NULL) && fields_len > 0u)
        return BITF_ERR_ARG;
    status = bitf_fields_check(fields, fields_len, &min_len);
    if (status != BITF_OK)
        return status;
    if (min_len > data_len)
        return BITF_ERR_BOUNDS;
    for (i = 0; i < fields_len; i++)
        out_values[i] = bitf_get_unchecked(data, fields[i].bit_offset, fields[i].bit_len,
                                           fields[i].flags);
    return BITF_OK;
}

int bitf_set_many(unsigned char *data, size_t data_len, const bitf_field_t *fields,
                           size_t fields_len, const uint64_t *values)
{
    size_t min_len;
    size_t i;
    int status;

    if ((data == NULL || values == NULL) && fields_len > 0u)
        return BITF_ERR_ARG;
    status = bitf_fields_check(fields, fields_len, &min_len);
    if (status != BITF_OK)
        return status;
    if (min_len > data_len)
        return BITF_ERR_BOUNDS;
    for (i = 0; i < fields_len; i++)
        if (!bitf_fits(values[i], fields[i].bit_len, fields[i].flags))
            return BITF_ERR_RANGE;
    for (i = 0; i < fields_len; i++)
        bitf_set_unchecked(data, fields[i].bit_offset, fields[i].bit_len, fields[i].flags,
                           values[i]);
    return BITF_OK;
}

const char *bitf_strerror(int status)
{
    switch (status) {
    case BITF_OK:
        return "ok";
    case BITF_ERR_ARG:
        return "invalid argument";
    case BITF_ERR_BOUNDS:
        return "field outside the buffer";
    case BITF_ERR_RANGE:
        return "value does not fit the field";
    default:
        return "unknown status";
    }
}

int bitf_version(void)
{
    return BITF_VERSION;
}

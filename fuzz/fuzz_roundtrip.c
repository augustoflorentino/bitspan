#include "bitspan/bitspan.h"

#include <stdlib.h>
#include <string.h>

#define MAX_BUF 64

static void require(int cond)
{
    if (!cond)
        abort();
}

static int bit_inside(size_t pos, size_t off, unsigned len)
{
    return pos >= off && pos - off < len;
}

static unsigned raw_bit(const unsigned char *buf, size_t pos, unsigned flags)
{
    unsigned bit = (flags & BITF_BE) ? 7u - (unsigned)(pos % 8u) : (unsigned)(pos % 8u);

    return (buf[pos / 8u] >> bit) & 1u;
}

static void copy_check(const unsigned char *src, size_t len, size_t src_off, size_t count,
                       unsigned flags)
{
    unsigned char dst[MAX_BUF] = {0};
    size_t dst_off = src_off / 3u;
    size_t i;

    if (bitf_copy(dst, len, dst_off, src, len, src_off, count, flags) != BITF_OK) {
        require(src_off + count > len * 8u || dst_off + count > len * 8u);
        return;
    }
    for (i = 0; i < count; i++)
        require(raw_bit(dst, dst_off + i, flags) == raw_bit(src, src_off + i, flags));
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    unsigned char buf[MAX_BUF];
    unsigned char copy[MAX_BUF];
    unsigned char raw[MAX_BUF];
    size_t buf_len;
    size_t off;
    size_t pos;
    unsigned len;
    unsigned flags;
    uint64_t value;
    uint64_t fresh;
    uint64_t back;
    int status;

    if (size < 4)
        return 0;
    off = (size_t)data[0] | ((size_t)data[1] << 8);
    len = data[2] % 70u;
    flags = data[3] % 8u;
    buf_len = size - 4 < MAX_BUF ? size - 4 : MAX_BUF;
    memcpy(buf, data + 4, buf_len);
    copy_check(buf, buf_len, off, (size_t)data[2] * 3u, flags & BITF_BE);

    status = bitf_get(buf, buf_len, off, len, flags, &value);
    require(status == bitf_check(buf_len, off, len, flags));
    if (status != BITF_OK)
        return 0;
    require(value == bitf_get_unchecked(buf, off, len, flags));

    memcpy(copy, buf, buf_len);
    require(bitf_set(copy, buf_len, off, len, flags, value) == BITF_OK);
    require(memcmp(copy, buf, buf_len) == 0);

    fresh = ~value;
    if (len < 64u)
        fresh &= ((uint64_t)1 << len) - 1u;
    if (bitf_set(copy, buf_len, off, len, flags & ~(unsigned)BITF_SIGNED, fresh) != BITF_OK)
        abort();
    require(bitf_get(copy, buf_len, off, len, flags & ~(unsigned)BITF_SIGNED, &back) == BITF_OK);
    require(back == fresh);
    memcpy(raw, buf, buf_len);
    bitf_set_unchecked(raw, off, len, flags, ~value);
    require(memcmp(raw, copy, buf_len) == 0);
    for (pos = 0; pos < buf_len * 8u; pos++)
        if (!bit_inside(pos, off, len))
            require(raw_bit(copy, pos, flags) == raw_bit(buf, pos, flags));
    return 0;
}

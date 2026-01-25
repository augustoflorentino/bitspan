#include <bitspan/bitspan.h>

#include <stdio.h>

int main(void)
{
    static const unsigned char report[] = {0x05, 0x80, 0xff, 0x7f, 0x34, 0x12};
    static const bitf_field_t fields[] = {
        {0, 1, BITF_LE},
        {2, 1, BITF_LE},
        {8, 8, BITF_LE | BITF_SIGNED},
        {16, 16, BITF_LE | BITF_SIGNED},
        {32, 16, BITF_LE},
    };
    static const char *const names[] = {"button a", "button b", "trigger", "stick x", "counter"};
    uint64_t values[5];
    size_t bad = 0;
    int status;
    int i;

    status = bitf_get_many(report, sizeof(report), fields, 5, values, &bad);
    if (status != BITF_OK) {
        fprintf(stderr, "error in %s: %s\n", names[bad], bitf_strerror(status));
        return 1;
    }
    for (i = 0; i < 5; i++) {
        if (fields[i].flags & BITF_SIGNED)
            printf("%-8s %lld\n", names[i], (long long)(int64_t)values[i]);
        else
            printf("%-8s %llu\n", names[i], (unsigned long long)values[i]);
    }
    return 0;
}

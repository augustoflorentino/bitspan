#define BITF_IMPLEMENTATION
#include "bitspan/bitspan.h"

int main(void)
{
    unsigned char buf[3] = {0};
    uint64_t value = 0;
    int status;

    status = bitf_set(buf, sizeof(buf), 5, 13, BITF_BE, 0x1abc);
    if (status == BITF_OK)
        status = bitf_get(buf, sizeof(buf), 5, 13, BITF_BE, &value);
    if (bitf_strerror(status) == NULL)
        status = BITF_ERR_ARG;
    return status == BITF_OK && value == 0x1abc ? 0 : 1;
}

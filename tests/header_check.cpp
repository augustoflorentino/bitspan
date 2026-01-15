#include "bitspan/bitspan.h"

int header_check_cxx(unsigned char *buf)
{
    bitf_set_unchecked(buf, 3, 20, BITF_LE, 5u);
    return bitf_get_unchecked(buf, 3, 20, BITF_LE) == 5u;
}

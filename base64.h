
#include <stdio.h>

/*
cencode.h - c header for a base64 encoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

typedef enum {
    step_A,
    step_B,
    step_C
} base64_encodestep;

typedef struct
{
    base64_encodestep step;
    char result;
    int stepcount;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate *state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char *plaintext_in, int length_in, char *code_out, base64_encodestate *state_in);

int base64_encode_blockend(char *code_out, base64_encodestate *state_in);

const int CHARS_PER_LINE = 72;

/* custom 64-bit encoding functions to avoid portability issues */
#define webdis_ntohl64(p)                                          \
    ((((uint64_t)((p)[0])) << 0) + (((uint64_t)((p)[1])) << 8) +   \
     (((uint64_t)((p)[2])) << 16) + (((uint64_t)((p)[3])) << 24) + \
     (((uint64_t)((p)[4])) << 32) + (((uint64_t)((p)[5])) << 40) + \
     (((uint64_t)((p)[6])) << 48) + (((uint64_t)((p)[7])) << 56))

#define webdis_htonl64(p)                                                                                             \
    {                                                                                                                 \
        (char)(((p & ((uint64_t)0xff << 0)) >> 0) & 0xff), (char)(((p & ((uint64_t)0xff << 8)) >> 8) & 0xff),         \
            (char)(((p & ((uint64_t)0xff << 16)) >> 16) & 0xff), (char)(((p & ((uint64_t)0xff << 24)) >> 24) & 0xff), \
            (char)(((p & ((uint64_t)0xff << 32)) >> 32) & 0xff), (char)(((p & ((uint64_t)0xff << 40)) >> 40) & 0xff), \
            (char)(((p & ((uint64_t)0xff << 48)) >> 48) & 0xff), (char)(((p & ((uint64_t)0xff << 56)) >> 56) & 0xff)  \
    }

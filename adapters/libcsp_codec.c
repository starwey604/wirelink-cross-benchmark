#include "codec.h"

/* libcsp carries opaque application bytes. This fixed, big-endian layout is
 * the minimum business codec required for the CSP transaction/send APIs. */
static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static uint32_t get32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

int bench_encode(bench_message_kind_t kind, const bench_fields_t *value,
                 uint8_t *out, size_t capacity, size_t *length)
{
    size_t count = kind == BENCH_SAMPLE ? 4 : 3;
    if (kind < BENCH_SETPOINT_REQUEST || kind > BENCH_SAMPLE ||
        !value || !out || !length || capacity < count * 4) return -1;
    for (size_t i = 0; i < count; ++i) put32(out + 4 * i, value->word[i]);
    *length = count * 4;
    return 0;
}

int bench_decode(bench_message_kind_t kind, const uint8_t *input, size_t length,
                 bench_fields_t *out)
{
    size_t count = kind == BENCH_SAMPLE ? 4 : 3;
    if (kind < BENCH_SETPOINT_REQUEST || kind > BENCH_SAMPLE ||
        !input || !out || length != count * 4) return -1;
    for (size_t i = 0; i < count; ++i) out->word[i] = get32(input + 4 * i);
    if (count == 3) out->word[3] = 0;
    return 0;
}

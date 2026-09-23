#ifndef WIRELINK_CROSS_BENCH_CODEC_H
#define WIRELINK_CROSS_BENCH_CODEC_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BENCH_SETPOINT_REQUEST,
    BENCH_SETPOINT_RESPONSE,
    BENCH_SAMPLE
} bench_message_kind_t;

/* The first three words are the RPC request/response fields; Sample uses all
 * four. Field names and business meaning live in workloads/v1.md. */
typedef struct {
    uint32_t word[4];
} bench_fields_t;

/* Returns 0 on success. Output is caller-owned; no adapter may silently
 * truncate a packet. The codec-only adapters use generated code. */
int bench_encode(bench_message_kind_t kind, const bench_fields_t *value,
                 uint8_t *out, size_t capacity, size_t *length);
int bench_decode(bench_message_kind_t kind, const uint8_t *input, size_t length,
                 bench_fields_t *out);

#endif

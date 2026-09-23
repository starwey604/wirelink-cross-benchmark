#include "codec.h"
#include "task_builder.h"
#include "task_verifier.h"
#include <string.h>

int bench_encode(bench_message_kind_t kind, const bench_fields_t *v,
                 uint8_t *out, size_t capacity, size_t *length)
{
    flatcc_builder_t builder;
    void *buffer;
    size_t size;
    int failed = 0;
    if (!v || !out || !length ||
        kind < BENCH_SETPOINT_REQUEST || kind > BENCH_SAMPLE) return -1;
    flatcc_builder_init(&builder);
    switch (kind) {
    case BENCH_SETPOINT_REQUEST:
        failed = !WirelinkBench_SetpointRequest_create_as_root(&builder,
                    v->word[0], v->word[1], v->word[2]);
        break;
    case BENCH_SETPOINT_RESPONSE:
        failed = !WirelinkBench_SetpointResponse_create_as_root(&builder,
                    v->word[0], v->word[1], v->word[2]);
        break;
    case BENCH_SAMPLE:
        failed = !WirelinkBench_Sample_create_as_root(&builder,
                    v->word[0], v->word[1], v->word[2], v->word[3]);
        break;
    }
    if (failed) { flatcc_builder_clear(&builder); return -1; }
    buffer = flatcc_builder_finalize_aligned_buffer(&builder, &size);
    if (!buffer || size > capacity) {
        flatcc_builder_aligned_free(buffer);
        flatcc_builder_clear(&builder);
        return -1;
    }
    memcpy(out, buffer, size);
    *length = size;
    flatcc_builder_aligned_free(buffer);
    flatcc_builder_clear(&builder);
    return 0;
}

int bench_decode(bench_message_kind_t kind, const uint8_t *input, size_t length,
                 bench_fields_t *out)
{
    if (!input || !out ||
        kind < BENCH_SETPOINT_REQUEST || kind > BENCH_SAMPLE) return -1;
    switch (kind) {
    case BENCH_SETPOINT_REQUEST: {
        if (WirelinkBench_SetpointRequest_verify_as_root(input, length)) return -1;
        WirelinkBench_SetpointRequest_table_t m = WirelinkBench_SetpointRequest_as_root(input);
        out->word[0] = WirelinkBench_SetpointRequest_request_id(m);
        out->word[1] = WirelinkBench_SetpointRequest_channel(m);
        out->word[2] = WirelinkBench_SetpointRequest_value(m);
        out->word[3] = 0;
        return 0;
    }
    case BENCH_SETPOINT_RESPONSE: {
        if (WirelinkBench_SetpointResponse_verify_as_root(input, length)) return -1;
        WirelinkBench_SetpointResponse_table_t m = WirelinkBench_SetpointResponse_as_root(input);
        out->word[0] = WirelinkBench_SetpointResponse_request_id(m);
        out->word[1] = WirelinkBench_SetpointResponse_status(m);
        out->word[2] = WirelinkBench_SetpointResponse_applied_value(m);
        out->word[3] = 0;
        return 0;
    }
    case BENCH_SAMPLE: {
        if (WirelinkBench_Sample_verify_as_root(input, length)) return -1;
        WirelinkBench_Sample_table_t m = WirelinkBench_Sample_as_root(input);
        out->word[0] = WirelinkBench_Sample_sequence(m);
        out->word[1] = WirelinkBench_Sample_sample_tick(m);
        out->word[2] = WirelinkBench_Sample_value(m);
        out->word[3] = WirelinkBench_Sample_flags(m);
        return 0;
    }
    }
    return -1;
}

#include "codec.h"
#include "task.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>

static const pb_msgdesc_t *descriptor(bench_message_kind_t kind)
{
    switch (kind) {
    case BENCH_SETPOINT_REQUEST: return wirelink_bench_SetpointRequest_fields;
    case BENCH_SETPOINT_RESPONSE: return wirelink_bench_SetpointResponse_fields;
    case BENCH_SAMPLE: return wirelink_bench_Sample_fields;
    }
    return NULL;
}

int bench_encode(bench_message_kind_t kind, const bench_fields_t *v,
                 uint8_t *out, size_t capacity, size_t *length)
{
    if (!v || !out || !length || !descriptor(kind)) return -1;
    pb_ostream_t stream = pb_ostream_from_buffer(out, capacity);
    bool ok;
    if (kind == BENCH_SETPOINT_REQUEST) {
        wirelink_bench_SetpointRequest m = {v->word[0], v->word[1], v->word[2]};
        ok = pb_encode(&stream, descriptor(kind), &m);
    } else if (kind == BENCH_SETPOINT_RESPONSE) {
        wirelink_bench_SetpointResponse m = {v->word[0], v->word[1], v->word[2]};
        ok = pb_encode(&stream, descriptor(kind), &m);
    } else {
        wirelink_bench_Sample m = {v->word[0], v->word[1], v->word[2], v->word[3]};
        ok = pb_encode(&stream, descriptor(kind), &m);
    }
    if (!ok) return -1;
    *length = stream.bytes_written;
    return 0;
}

int bench_decode(bench_message_kind_t kind, const uint8_t *input, size_t length,
                 bench_fields_t *out)
{
    if (!input || !out || !descriptor(kind)) return -1;
    pb_istream_t stream = pb_istream_from_buffer(input, length);
    bool ok;
    if (kind == BENCH_SETPOINT_REQUEST) {
        wirelink_bench_SetpointRequest m = wirelink_bench_SetpointRequest_init_zero;
        ok = pb_decode(&stream, descriptor(kind), &m);
        if (ok) { out->word[0] = m.request_id; out->word[1] = m.channel; out->word[2] = m.value; }
    } else if (kind == BENCH_SETPOINT_RESPONSE) {
        wirelink_bench_SetpointResponse m = wirelink_bench_SetpointResponse_init_zero;
        ok = pb_decode(&stream, descriptor(kind), &m);
        if (ok) { out->word[0] = m.request_id; out->word[1] = m.status; out->word[2] = m.applied_value; }
    } else {
        wirelink_bench_Sample m = wirelink_bench_Sample_init_zero;
        ok = pb_decode(&stream, descriptor(kind), &m);
        if (ok) { out->word[0] = m.sequence; out->word[1] = m.sample_tick; out->word[2] = m.value; out->word[3] = m.flags; }
    }
    if (!ok) return -1;
    if (kind != BENCH_SAMPLE) out->word[3] = 0;
    return 0;
}

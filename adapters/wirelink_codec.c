#include "codec.h"
#include "task.h"

int bench_encode(bench_message_kind_t kind, const bench_fields_t *v,
                 uint8_t *out, size_t capacity, size_t *length)
{
    if (!v || !out || !length) return -1;
    switch (kind) {
    case BENCH_SETPOINT_REQUEST: {
        setpoint_request_t m;
        setpoint_request_clear(&m);
        m.has_request_id = m.has_channel = m.has_value = true;
        m.request_id = v->word[0]; m.channel = v->word[1]; m.value = v->word[2];
        return setpoint_request_encode(&m, out, capacity, length) == WL_CODEC_OK ? 0 : -1;
    }
    case BENCH_SETPOINT_RESPONSE: {
        setpoint_response_t m;
        setpoint_response_clear(&m);
        m.has_request_id = m.has_status = m.has_applied_value = true;
        m.request_id = v->word[0]; m.status = v->word[1]; m.applied_value = v->word[2];
        return setpoint_response_encode(&m, out, capacity, length) == WL_CODEC_OK ? 0 : -1;
    }
    case BENCH_SAMPLE: {
        sample_t m;
        sample_clear(&m);
        m.has_sequence = m.has_sample_tick = m.has_value = m.has_flags = true;
        m.sequence = v->word[0]; m.sample_tick = v->word[1];
        m.value = v->word[2]; m.flags = v->word[3];
        return sample_encode(&m, out, capacity, length) == WL_CODEC_OK ? 0 : -1;
    }
    }
    return -1;
}

int bench_decode(bench_message_kind_t kind, const uint8_t *input, size_t length,
                 bench_fields_t *out)
{
    if (!input || !out) return -1;
    switch (kind) {
    case BENCH_SETPOINT_REQUEST: {
        setpoint_request_t m;
        if (setpoint_request_decode(input, length, &m) != WL_CODEC_OK) return -1;
        out->word[0] = m.request_id; out->word[1] = m.channel; out->word[2] = m.value;
        out->word[3] = 0;
        return 0;
    }
    case BENCH_SETPOINT_RESPONSE: {
        setpoint_response_t m;
        if (setpoint_response_decode(input, length, &m) != WL_CODEC_OK) return -1;
        out->word[0] = m.request_id; out->word[1] = m.status;
        out->word[2] = m.applied_value; out->word[3] = 0;
        return 0;
    }
    case BENCH_SAMPLE: {
        sample_t m;
        if (sample_decode(input, length, &m) != WL_CODEC_OK) return -1;
        out->word[0] = m.sequence; out->word[1] = m.sample_tick;
        out->word[2] = m.value; out->word[3] = m.flags;
        return 0;
    }
    }
    return -1;
}

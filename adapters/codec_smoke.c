#include "codec.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    const bench_fields_t input[] = {
        {{17, 2, 99999, 0}},
        {{17, 0, 99999, 0}},
        {{8, 123456, 42, 0xa5}}
    };
    uint8_t buffer[256];
    for (int kind = BENCH_SETPOINT_REQUEST; kind <= BENCH_SAMPLE; ++kind) {
        bench_fields_t output = {{0}};
        size_t length = 0;
        size_t count = kind == BENCH_SAMPLE ? 4 : 3;
        if (bench_encode((bench_message_kind_t)kind, &input[kind],
                         buffer, sizeof(buffer), &length) != 0 ||
            length == 0 ||
            bench_decode((bench_message_kind_t)kind, buffer, length, &output) != 0 ||
            memcmp(input[kind].word, output.word, count * sizeof(uint32_t)) != 0) {
            fprintf(stderr, "codec roundtrip failed for kind %d\n", kind);
            return 1;
        }
        printf("kind=%d bytes=%zu\n", kind, length);
    }
    return 0;
}

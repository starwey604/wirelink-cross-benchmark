#include "codec.h"
#include <csp/csp.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum { RPC_PORT = 10, SAMPLE_PORT = 11 };
static uint32_t setpoints[4];
static atomic_uint rpc_seen;
static atomic_uint sample_seen;
static atomic_bool stop_router;

static void rpc_handler(csp_packet_t *packet)
{
    bench_fields_t request, response = {{0}};
    uint8_t encoded[32];
    size_t length;
    if (bench_decode(BENCH_SETPOINT_REQUEST, packet->data, packet->length, &request) == 0) {
        response.word[0] = request.word[0];
        if (request.word[0] == 0 || request.word[1] >= 4 || request.word[2] > 100000) {
            response.word[1] = 1;
        } else {
            setpoints[request.word[1]] = request.word[2];
            response.word[2] = setpoints[request.word[1]];
        }
        if (bench_encode(BENCH_SETPOINT_RESPONSE, &response,
                         encoded, sizeof(encoded), &length) == 0) {
            csp_packet_t *reply = csp_buffer_get(0);
            if (reply) {
                memcpy(reply->data, encoded, length);
                reply->length = (uint16_t)length;
                csp_sendto_reply(packet, reply, CSP_O_SAME);
                atomic_fetch_add(&rpc_seen, 1);
            }
        }
    }
    csp_buffer_free(packet);
}

static void sample_handler(csp_packet_t *packet)
{
    bench_fields_t sample;
    if (bench_decode(BENCH_SAMPLE, packet->data, packet->length, &sample) == 0 &&
        sample.word[0] == 8 && sample.word[1] == 123456 &&
        sample.word[2] == 42 && sample.word[3] == 0xa5) {
        atomic_fetch_add(&sample_seen, 1);
    }
    csp_buffer_free(packet);
}

static void *route_thread(void *unused)
{
    (void)unused;
    while (!atomic_load(&stop_router)) (void)csp_route_work();
    return NULL;
}

int main(void)
{
    pthread_t router;
    csp_init();
    if (csp_bind_callback(rpc_handler, RPC_PORT) != CSP_ERR_NONE ||
        csp_bind_callback(sample_handler, SAMPLE_PORT) != CSP_ERR_NONE ||
        pthread_create(&router, NULL, route_thread, NULL) != 0) return 1;
    csp_conn_t *connection = csp_connect(CSP_PRIO_NORM, 0, RPC_PORT,
                                         1000, CSP_O_CRC32);
    if (!connection) return 1;

    const bench_fields_t requests[] = {{{17, 2, 99999, 0}}, {{18, 9, 7, 0}}};
    for (unsigned i = 0; i < 2; ++i) {
        uint8_t encoded[32], reply_bytes[32];
        size_t length;
        bench_fields_t response;
        if (bench_encode(BENCH_SETPOINT_REQUEST, &requests[i],
                         encoded, sizeof(encoded), &length) != 0) return 1;
        int got = csp_transaction_persistent(connection, 1000,
                                             encoded, (int)length, reply_bytes, 12);
        if (got != 12 || bench_decode(BENCH_SETPOINT_RESPONSE,
                                     reply_bytes, (size_t)got, &response) != 0 ||
            response.word[0] != requests[i].word[0] ||
            response.word[1] != i ||
            response.word[2] != (i == 0 ? 99999U : 0U)) return 1;
    }

    bench_fields_t sample = {{8, 123456, 42, 0xa5}};
    csp_packet_t *packet = csp_buffer_get(0);
    size_t length;
    if (!packet || bench_encode(BENCH_SAMPLE, &sample, packet->data,
                                sizeof(packet->data), &length) != 0) return 1;
    packet->length = (uint16_t)length;
    csp_sendto(CSP_PRIO_NORM, 0, SAMPLE_PORT, 12, CSP_O_CRC32, packet);
    for (unsigned i = 0; i < 100 && atomic_load(&sample_seen) < 1; ++i) {
        struct timespec pause = {0, 1000000};
        nanosleep(&pause, NULL);
    }
    if (atomic_load(&rpc_seen) != 2 || atomic_load(&sample_seen) != 1) return 1;
    if (csp_close(connection) != CSP_ERR_NONE) return 1;
    atomic_store(&stop_router, true);
    pthread_join(router, NULL);
    puts("libcsp: two calls and one sample validated");
    return 0;
}

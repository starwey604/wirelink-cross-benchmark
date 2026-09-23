#include "c_wirelink_cross_benchmark_server.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"
#include "erpc_transport_setup.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t setpoints[4];
static unsigned rpc_seen;
static unsigned sample_seen;

SetpointResponse *setpoint(const SetpointRequest *input)
{
    SetpointResponse *reply = erpc_malloc(sizeof(*reply));
    if (!reply) return NULL;
    reply->request_id = input->request_id;
    if (input->request_id == 0 || input->channel >= 4 || input->value > 100000) {
        reply->status = 1;
        reply->applied_value = 0;
    } else {
        setpoints[input->channel] = input->value;
        reply->status = 0;
        reply->applied_value = setpoints[input->channel];
    }
    ++rpc_seen;
    return reply;
}

void publish_sample(const Sample *sample)
{
    if (sample->sequence == 8 && sample->sample_tick == 123456 &&
        sample->value == 42 && sample->flags == 0xa5) ++sample_seen;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    unsigned long port = strtoul(argv[1], NULL, 10);
    if (port == 0 || port > 65535) return 2;
    erpc_transport_t transport = erpc_transport_tcp_init("127.0.0.1", (uint16_t)port, true);
    if (!transport) return 1;
    erpc_mbf_t mbf = erpc_mbf_static_init();
    erpc_server_t server = erpc_server_init(transport, mbf);
    erpc_service_t service = create_TaskService_service();
    if (!mbf || !server || !service) return 1;
    erpc_add_service_to_server(server, service);
    while (rpc_seen < 2 || sample_seen < 1) {
        if (erpc_server_poll(server) != kErpcStatus_Success) return 1;
    }
    destroy_TaskService_service(service);
    erpc_server_deinit(server);
    erpc_mbf_static_deinit(mbf);
    erpc_transport_tcp_close(transport);
    erpc_transport_tcp_deinit(transport);
    puts("eRPC server: two calls and one sample validated");
    return 0;
}

#include "c_wirelink_cross_benchmark_client.h"
#include "erpc_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_transport_setup.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    unsigned long port = strtoul(argv[1], NULL, 10);
    if (port == 0 || port > 65535) return 2;
    erpc_transport_t transport = erpc_transport_tcp_init("127.0.0.1", (uint16_t)port, false);
    if (!transport) return 1;
    erpc_mbf_t mbf = erpc_mbf_static_init();
    erpc_client_t client = erpc_client_init(transport, mbf);
    if (!mbf || !client) return 1;
    initTaskService_client(client);

    SetpointRequest requests[] = {{17, 2, 99999}, {18, 9, 7}};
    for (unsigned i = 0; i < 2; ++i) {
        SetpointResponse *reply = setpoint(&requests[i]);
        if (!reply || reply->request_id != requests[i].request_id ||
            reply->status != i ||
            reply->applied_value != (i == 0 ? 99999U : 0U)) return 1;
        erpc_free(reply);
    }
    Sample sample = {8, 123456, 42, 0xa5};
    publish_sample(&sample);
    deinitTaskService_client();
    erpc_client_deinit(client);
    erpc_mbf_static_deinit(mbf);
    erpc_transport_tcp_close(transport);
    erpc_transport_tcp_deinit(transport);
    puts("eRPC client: responses validated, sample sent");
    return 0;
}

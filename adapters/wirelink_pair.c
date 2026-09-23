#include "task_endpoint.h"
#include <wirelink/loopback.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint64_t session;
    wl_time_ms_t now;
    uint32_t setpoints[4];
    unsigned completed;
    setpoint_response_value_t last_response;
    wl_rpc_completion_t last_completion;
} pair_state_t;

static wl_time_ms_t clock_now(void *user_data)
{
    return ((pair_state_t *)user_data)->now;
}

static wl_err_t next_session(void *user_data, uint64_t *out)
{
    *out = ++((pair_state_t *)user_data)->session;
    return WL_OK;
}

static int32_t on_setpoint(void *user_data,
                           const setpoint_request_value_t *request,
                           setpoint_response_value_t *response)
{
    pair_state_t *state = user_data;
    response->has_request_id = response->has_status = response->has_applied_value = true;
    response->request_id = request->request_id;
    if (request->request_id == 0 || request->channel >= 4 || request->value > 100000) {
        response->status = 1;
        response->applied_value = 0;
    } else {
        state->setpoints[request->channel] = request->value;
        response->status = 0;
        response->applied_value = state->setpoints[request->channel];
    }
    return 0;
}

static void on_completion(void *user_data, const wl_rpc_completion_t *result,
                          const setpoint_response_value_t *response)
{
    pair_state_t *state = user_data;
    state->last_completion = *result;
    if (response) state->last_response = *response;
    ++state->completed;
}

static int drive(task_endpoint_t *client, task_endpoint_t *server,
                 pair_state_t *state, unsigned target)
{
    for (unsigned pass = 0; pass < 128 && state->completed < target; ++pass) {
        ++state->now;
        if (task_endpoint_step(client) != WL_OK ||
            task_endpoint_step(server) != WL_OK) return -1;
    }
    return state->completed == target ? 0 : -1;
}

int main(void)
{
    pair_state_t state = {0};
    task_endpoint_t client = {0}, server = {0};
    wl_loopback_t cable = {0};
    wl_environment_t environment = {{clock_now, &state}, {next_session, &state}};
    task_endpoint_config_t config;
    if (task_endpoint_init(&client, environment) != WL_OK ||
        task_endpoint_config_defaults(&config, environment) != WL_OK) return 1;
    config.on_setpoint = on_setpoint;
    config.user_data = &state;
    if (task_endpoint_init_config(&server, &config) != WL_OK ||
        wl_loopback_connect(&cable, task_endpoint_handle(&client),
                            task_endpoint_handle(&server)) != WL_OK) return 1;

    const uint32_t requests[2][3] = {{17, 2, 99999}, {18, 9, 7}};
    for (unsigned i = 0; i < 2; ++i) {
        setpoint_request_value_t request = {0};
        request.has_request_id = request.has_channel = request.has_value = true;
        request.request_id = requests[i][0];
        request.channel = requests[i][1];
        request.value = requests[i][2];
        if (task_endpoint_setpoint_async(&client, &request, 1000,
                                         on_completion, &state, NULL) != WL_OK ||
            drive(&client, &server, &state, i + 1) != 0 ||
            state.last_completion.status != WL_RPC_SUCCESS ||
            state.last_response.request_id != request.request_id ||
            state.last_response.status != i ||
            state.last_response.applied_value != (i == 0 ? 99999U : 0U)) return 1;
    }

    sample_t sample;
    sample_clear(&sample);
    sample.has_sequence = sample.has_sample_tick = sample.has_value = sample.has_flags = true;
    sample.sequence = 8; sample.sample_tick = 123456;
    sample.value = 42; sample.flags = 0xa5;
    if (task_endpoint_send_sample(&client, &sample).domain != TASK_SEND_OK) return 1;
    sample_t received = {0};
    int found = 0;
    for (unsigned pass = 0; pass < 128 && !found; ++pass) {
        ++state.now;
        if (task_endpoint_step(&client) != WL_OK ||
            task_endpoint_step(&server) != WL_OK) return 1;
        found = task_endpoint_read_sample(&server, &received) == WL_OK;
    }
    if (!found || received.sequence != sample.sequence ||
        received.sample_tick != sample.sample_tick ||
        received.value != sample.value || received.flags != sample.flags) return 1;
    if (task_endpoint_close(&client) != WL_OK ||
        task_endpoint_close(&server) != WL_OK) return 1;
    puts("Wirelink: two calls and one sample validated");
    return 0;
}

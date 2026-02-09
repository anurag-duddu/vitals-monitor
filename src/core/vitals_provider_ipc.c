/**
 * @file vitals_provider_ipc.c
 * @brief IPC implementation of vitals provider for target builds
 *
 * This implementation subscribes to nanomsg IPC sockets to receive
 * vital signs and waveform data from sensor-service.
 *
 * Compile this file when building for target (VITALS_PROVIDER_IPC defined).
 *
 * TODO (Remote Team):
 *   - Implement nanomsg socket setup
 *   - Implement message parsing
 *   - Test with sensor-service
 */

#include "vitals_provider.h"
#include "../common/ipc/ipc_messages.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifdef USE_NANOMSG
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#endif

/* ── State ─────────────────────────────────────────────────── */

static bool g_initialized = false;
static bool g_running = false;

static vitals_callback_t   g_vitals_cb = NULL;
static void               *g_vitals_user_data = NULL;
static waveform_callback_t g_waveform_cb = NULL;
static void               *g_waveform_user_data = NULL;

static vitals_data_t g_current_vitals[2];
static vitals_history_t g_history[2];

#ifdef USE_NANOMSG
static int g_vitals_socket = -1;
static int g_waveform_socket = -1;
static pthread_t g_vitals_thread;
static pthread_t g_waveform_thread;
static volatile bool g_threads_running = false;
#endif

/* ── Forward declarations ──────────────────────────────────── */

#ifdef USE_NANOMSG
static void *vitals_receiver_thread(void *arg);
static void *waveform_receiver_thread(void *arg);
static void process_vitals_message(const ipc_msg_vitals_t *msg);
static void process_waveform_message(const ipc_msg_waveform_t *msg);
#endif

/* ── Public API ────────────────────────────────────────────── */

int vitals_provider_init(void) {
    if (g_initialized) {
        return 0;
    }

    memset(g_current_vitals, 0, sizeof(g_current_vitals));
    memset(g_history, 0, sizeof(g_history));

#ifdef USE_NANOMSG
    /* Create nanomsg subscriber sockets */
    g_vitals_socket = nn_socket(AF_SP, NN_SUB);
    if (g_vitals_socket < 0) {
        fprintf(stderr, "[vitals_provider] Failed to create vitals socket\n");
        return -1;
    }

    g_waveform_socket = nn_socket(AF_SP, NN_SUB);
    if (g_waveform_socket < 0) {
        nn_close(g_vitals_socket);
        fprintf(stderr, "[vitals_provider] Failed to create waveform socket\n");
        return -1;
    }

    /* Subscribe to all messages */
    nn_setsockopt(g_vitals_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    nn_setsockopt(g_waveform_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);

    /* Connect to publisher sockets */
    if (nn_connect(g_vitals_socket, IPC_SOCKET_VITALS) < 0) {
        fprintf(stderr, "[vitals_provider] Failed to connect to %s\n", IPC_SOCKET_VITALS);
        nn_close(g_vitals_socket);
        nn_close(g_waveform_socket);
        return -1;
    }

    if (nn_connect(g_waveform_socket, IPC_SOCKET_WAVEFORMS) < 0) {
        fprintf(stderr, "[vitals_provider] Failed to connect to %s\n", IPC_SOCKET_WAVEFORMS);
        nn_close(g_vitals_socket);
        nn_close(g_waveform_socket);
        return -1;
    }
#else
    fprintf(stderr, "[vitals_provider] IPC mode requires USE_NANOMSG to be defined\n");
    fprintf(stderr, "[vitals_provider] This is a stub - remote team needs to complete\n");
#endif

    g_initialized = true;
    printf("[vitals_provider] Initialized (type=ipc)\n");
    return 0;
}

int vitals_provider_start(uint32_t vitals_interval_ms) {
    (void)vitals_interval_ms;  /* Interval is controlled by sensor-service */

    if (!g_initialized) {
        return -1;
    }
    if (g_running) {
        return 0;
    }

#ifdef USE_NANOMSG
    g_threads_running = true;

    if (pthread_create(&g_vitals_thread, NULL, vitals_receiver_thread, NULL) != 0) {
        fprintf(stderr, "[vitals_provider] Failed to create vitals thread\n");
        return -1;
    }

    if (pthread_create(&g_waveform_thread, NULL, waveform_receiver_thread, NULL) != 0) {
        fprintf(stderr, "[vitals_provider] Failed to create waveform thread\n");
        g_threads_running = false;
        pthread_join(g_vitals_thread, NULL);
        return -1;
    }
#else
    fprintf(stderr, "[vitals_provider] IPC start is a stub - no data will be received\n");
#endif

    g_running = true;
    printf("[vitals_provider] Started (IPC mode)\n");
    return 0;
}

void vitals_provider_stop(void) {
    if (!g_running) {
        return;
    }

#ifdef USE_NANOMSG
    g_threads_running = false;

    /* Close sockets to unblock recv() calls */
    if (g_vitals_socket >= 0) {
        nn_close(g_vitals_socket);
        g_vitals_socket = -1;
    }
    if (g_waveform_socket >= 0) {
        nn_close(g_waveform_socket);
        g_waveform_socket = -1;
    }

    pthread_join(g_vitals_thread, NULL);
    pthread_join(g_waveform_thread, NULL);
#endif

    g_running = false;
    printf("[vitals_provider] Stopped\n");
}

void vitals_provider_deinit(void) {
    vitals_provider_stop();
    g_initialized = false;
    g_vitals_cb = NULL;
    g_waveform_cb = NULL;
    printf("[vitals_provider] Deinitialized\n");
}

void vitals_provider_set_vitals_callback(vitals_callback_t callback, void *user_data) {
    g_vitals_cb = callback;
    g_vitals_user_data = user_data;
}

void vitals_provider_set_waveform_callback(waveform_callback_t callback, void *user_data) {
    g_waveform_cb = callback;
    g_waveform_user_data = user_data;
}

const vitals_data_t *vitals_provider_get_current(uint8_t slot) {
    if (slot > 1) {
        return NULL;
    }
    return &g_current_vitals[slot];
}

bool vitals_provider_is_running(void) {
    return g_running;
}

const char *vitals_provider_get_type(void) {
    return "ipc";
}

const vitals_history_t *vitals_provider_get_history(uint8_t slot) {
    if (slot > 1) {
        return NULL;
    }
    return &g_history[slot];
}

/* ── IPC Receiver Threads ──────────────────────────────────── */

#ifdef USE_NANOMSG

static void *vitals_receiver_thread(void *arg) {
    (void)arg;
    char buf[512];

    printf("[vitals_provider] Vitals receiver thread started\n");

    while (g_threads_running) {
        int bytes = nn_recv(g_vitals_socket, buf, sizeof(buf), 0);
        if (bytes < 0) {
            if (g_threads_running) {
                fprintf(stderr, "[vitals_provider] Vitals recv error\n");
            }
            break;
        }

        if (bytes >= (int)sizeof(ipc_msg_header_t)) {
            ipc_msg_header_t *header = (ipc_msg_header_t *)buf;
            if (header->msg_type == IPC_MSG_VITALS &&
                bytes >= (int)sizeof(ipc_msg_vitals_t)) {
                process_vitals_message((ipc_msg_vitals_t *)buf);
            }
        }
    }

    printf("[vitals_provider] Vitals receiver thread exiting\n");
    return NULL;
}

static void *waveform_receiver_thread(void *arg) {
    (void)arg;
    char buf[1024];

    printf("[vitals_provider] Waveform receiver thread started\n");

    while (g_threads_running) {
        int bytes = nn_recv(g_waveform_socket, buf, sizeof(buf), 0);
        if (bytes < 0) {
            if (g_threads_running) {
                fprintf(stderr, "[vitals_provider] Waveform recv error\n");
            }
            break;
        }

        if (bytes >= (int)sizeof(ipc_msg_header_t)) {
            ipc_msg_header_t *header = (ipc_msg_header_t *)buf;
            if (header->msg_type == IPC_MSG_WAVEFORM &&
                bytes >= (int)sizeof(ipc_msg_waveform_t)) {
                process_waveform_message((ipc_msg_waveform_t *)buf);
            }
        }
    }

    printf("[vitals_provider] Waveform receiver thread exiting\n");
    return NULL;
}

static void process_vitals_message(const ipc_msg_vitals_t *msg) {
    uint8_t slot = msg->patient_slot;
    if (slot > 1) {
        return;
    }

    /* Convert IPC message to vitals_data_t */
    vitals_data_t *v = &g_current_vitals[slot];
    v->hr = (msg->hr >= 0) ? msg->hr : 0;
    v->spo2 = (msg->spo2 >= 0) ? msg->spo2 : 0;
    v->rr = (msg->rr >= 0) ? msg->rr : 0;
    v->temp = (msg->temp_x10 >= 0) ? (float)msg->temp_x10 / 10.0f : 0.0f;
    v->nibp_sys = (msg->nibp_sys >= 0) ? msg->nibp_sys : 0;
    v->nibp_dia = (msg->nibp_dia >= 0) ? msg->nibp_dia : 0;
    v->nibp_map = (msg->nibp_map >= 0) ? msg->nibp_map : 0;
    v->nibp_fresh = msg->nibp_fresh;
    v->timestamp_ms = msg->timestamp_ms;
    v->patient_slot = slot;
    v->hr_quality = msg->hr_quality;
    v->spo2_quality = msg->spo2_quality;
    v->ecg_lead_off = msg->ecg_lead_off;

    /* Update history */
    vitals_history_t *h = &g_history[slot];
    int idx = h->write_idx;
    h->hr[idx] = v->hr;
    h->spo2[idx] = v->spo2;
    h->rr[idx] = v->rr;
    h->nibp_sys[idx] = v->nibp_sys;
    h->nibp_dia[idx] = v->nibp_dia;
    h->timestamps[idx] = v->timestamp_ms;
    h->write_idx = (idx + 1) % VITALS_HISTORY_LEN;
    if (h->count < VITALS_HISTORY_LEN) {
        h->count++;
    }

    /* Notify callback */
    if (g_vitals_cb) {
        g_vitals_cb(v, g_vitals_user_data);
    }
}

static void process_waveform_message(const ipc_msg_waveform_t *msg) {
    if (!g_waveform_cb) {
        return;
    }

    waveform_packet_t packet;
    packet.type = (waveform_type_t)msg->waveform_type;
    packet.patient_slot = msg->patient_slot;
    packet.sample_rate_hz = msg->sample_rate_hz;
    packet.sample_count = msg->sample_count;
    packet.timestamp_ms = msg->timestamp_ms;

    if (packet.sample_count > WAVEFORM_SAMPLES_PER_PACKET) {
        packet.sample_count = WAVEFORM_SAMPLES_PER_PACKET;
    }

    memcpy(packet.samples, msg->samples, packet.sample_count * sizeof(int16_t));

    g_waveform_cb(&packet, g_waveform_user_data);
}

#endif /* USE_NANOMSG */

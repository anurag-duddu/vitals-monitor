/**
 * @file ipc_transport.c
 * @brief IPC transport layer — nanomsg pub/sub wrapper
 *
 * Build modes:
 *   - SIMULATOR_BUILD: Stub implementation (no nanomsg dependency).
 *     Publishers accept and discard messages. Subscribers can receive
 *     data via ipc_sub_inject() for testing.
 *   - Target build: Real nanomsg nn_socket/nn_bind/nn_connect with
 *     NN_PUB/NN_SUB protocol, configurable timeouts, and topic
 *     subscription via nn_setsockopt(NN_SUB_SUBSCRIBE).
 *
 * All state is stored in caller-provided structs (static allocation).
 */

#include "ipc_transport.h"
#include "ipc_messages.h"
#include <stdio.h>
#include <string.h>

#ifndef SIMULATOR_BUILD
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#endif

/* ── Module tag for printf logging ───────────────────────── */

#define TAG "[ipc_transport] "

/* ── Module state ────────────────────────────────────────── */

static bool transport_initialized = false;

/* ── Transport lifecycle ─────────────────────────────────── */

ipc_error_t ipc_transport_init(void) {
    if (transport_initialized) {
        printf(TAG "Already initialized\n");
        return IPC_OK;
    }

    transport_initialized = true;

#ifdef SIMULATOR_BUILD
    printf(TAG "Initialized (simulator stub — no nanomsg)\n");
#else
    printf(TAG "Initialized (nanomsg transport)\n");
#endif

    return IPC_OK;
}

void ipc_transport_close(void) {
    if (!transport_initialized) return;

    transport_initialized = false;
    printf(TAG "Closed\n");
}

bool ipc_transport_is_initialized(void) {
    return transport_initialized;
}

/* ============================================================
 *  SIMULATOR BUILD — Stub Implementation
 * ============================================================ */

#ifdef SIMULATOR_BUILD

/* ── Publisher (simulator stub) ──────────────────────────── */

ipc_error_t ipc_pub_create(ipc_publisher_t *pub, const char *endpoint) {
    if (!pub || !endpoint) return IPC_ERR_PARAM;
    if (!transport_initialized) return IPC_ERR_INIT;

    memset(pub, 0, sizeof(*pub));
    pub->socket_fd = -1;   /* No real socket in simulator */
    strncpy(pub->endpoint, endpoint, IPC_ENDPOINT_MAX - 1);
    pub->endpoint[IPC_ENDPOINT_MAX - 1] = '\0';
    pub->active = true;
    pub->msgs_sent = 0;
    pub->bytes_sent = 0;

    printf(TAG "Publisher created (stub): %s\n", pub->endpoint);
    return IPC_OK;
}

ipc_error_t ipc_pub_send(ipc_publisher_t *pub, const void *data, size_t len) {
    if (!pub || !data) return IPC_ERR_PARAM;
    if (!pub->active) return IPC_ERR_CLOSED;
    if (!transport_initialized) return IPC_ERR_INIT;

    /* Stub: accept message but do nothing with it */
    pub->msgs_sent++;
    pub->bytes_sent += (uint32_t)len;

    /* Extract message type for debug logging */
    if (len >= sizeof(ipc_msg_header_t)) {
        const ipc_msg_header_t *hdr = (const ipc_msg_header_t *)data;
        printf(TAG "PUB send (stub): type=%s len=%zu on %s\n",
               ipc_msg_type_name((ipc_msg_type_t)hdr->msg_type),
               len, pub->endpoint);
    }

    return IPC_OK;
}

void ipc_pub_close(ipc_publisher_t *pub) {
    if (!pub || !pub->active) return;

    printf(TAG "Publisher closed (stub): %s (sent %u msgs, %u bytes)\n",
           pub->endpoint, pub->msgs_sent, pub->bytes_sent);

    pub->active = false;
    pub->socket_fd = -1;
}

/* ── Subscriber (simulator stub) ─────────────────────────── */

ipc_error_t ipc_sub_create(ipc_subscriber_t *sub, const char *endpoint,
                            int timeout_ms, ipc_recv_callback_t callback,
                            void *user_data) {
    if (!sub || !endpoint) return IPC_ERR_PARAM;
    if (!transport_initialized) return IPC_ERR_INIT;

    memset(sub, 0, sizeof(*sub));
    sub->socket_fd = -1;   /* No real socket in simulator */
    strncpy(sub->endpoint, endpoint, IPC_ENDPOINT_MAX - 1);
    sub->endpoint[IPC_ENDPOINT_MAX - 1] = '\0';
    sub->active = true;
    sub->timeout_ms = timeout_ms > 0 ? timeout_ms : IPC_RECV_TIMEOUT_MS;
    sub->msgs_received = 0;
    sub->bytes_received = 0;
    sub->callback = callback;
    sub->user_data = user_data;

    printf(TAG "Subscriber created (stub): %s timeout=%dms\n",
           sub->endpoint, sub->timeout_ms);
    return IPC_OK;
}

ipc_error_t ipc_sub_recv(ipc_subscriber_t *sub, size_t *out_len) {
    if (!sub) return IPC_ERR_PARAM;
    if (!sub->active) return IPC_ERR_CLOSED;
    if (!transport_initialized) return IPC_ERR_INIT;

    /* Stub: no real socket, always timeout (no data available) */
    if (out_len) *out_len = 0;
    return IPC_ERR_TIMEOUT;
}

void ipc_sub_close(ipc_subscriber_t *sub) {
    if (!sub || !sub->active) return;

    printf(TAG "Subscriber closed (stub): %s (recv %u msgs, %u bytes)\n",
           sub->endpoint, sub->msgs_received, sub->bytes_received);

    sub->active = false;
    sub->socket_fd = -1;
    sub->callback = NULL;
    sub->user_data = NULL;
}

/* ── Test injection (simulator only) ─────────────────────── */

ipc_error_t ipc_sub_inject(ipc_subscriber_t *sub, const void *data, size_t len) {
    if (!sub || !data) return IPC_ERR_PARAM;
    if (!sub->active) return IPC_ERR_CLOSED;
    if (len > IPC_RECV_BUF_MAX) return IPC_ERR_PARAM;

    /* Copy into receive buffer */
    memcpy(sub->recv_buf, data, len);
    sub->msgs_received++;
    sub->bytes_received += (uint32_t)len;

    /* If callback registered, dispatch immediately */
    if (sub->callback && len >= sizeof(ipc_msg_header_t)) {
        const ipc_msg_header_t *hdr = (const ipc_msg_header_t *)data;
        sub->callback(hdr->msg_type, data, len, sub->user_data);
    }

    printf(TAG "SUB inject: %zu bytes into %s\n", len, sub->endpoint);
    return IPC_OK;
}

/* ============================================================
 *  TARGET BUILD — Real nanomsg Implementation
 * ============================================================ */

#else /* !SIMULATOR_BUILD */

/* ── Publisher (nanomsg) ─────────────────────────────────── */

ipc_error_t ipc_pub_create(ipc_publisher_t *pub, const char *endpoint) {
    if (!pub || !endpoint) return IPC_ERR_PARAM;
    if (!transport_initialized) return IPC_ERR_INIT;

    memset(pub, 0, sizeof(*pub));
    strncpy(pub->endpoint, endpoint, IPC_ENDPOINT_MAX - 1);
    pub->endpoint[IPC_ENDPOINT_MAX - 1] = '\0';

    /* Create PUB socket */
    pub->socket_fd = nn_socket(AF_SP, NN_PUB);
    if (pub->socket_fd < 0) {
        printf(TAG "Failed to create PUB socket: %s\n", nn_strerror(nn_errno()));
        return IPC_ERR_SOCKET;
    }

    /* Bind to endpoint */
    int rv = nn_bind(pub->socket_fd, endpoint);
    if (rv < 0) {
        printf(TAG "Failed to bind PUB to %s: %s\n", endpoint, nn_strerror(nn_errno()));
        nn_close(pub->socket_fd);
        pub->socket_fd = -1;
        return IPC_ERR_BIND;
    }

    pub->active = true;
    pub->msgs_sent = 0;
    pub->bytes_sent = 0;

    printf(TAG "Publisher created: %s (fd=%d)\n", pub->endpoint, pub->socket_fd);
    return IPC_OK;
}

ipc_error_t ipc_pub_send(ipc_publisher_t *pub, const void *data, size_t len) {
    if (!pub || !data) return IPC_ERR_PARAM;
    if (!pub->active) return IPC_ERR_CLOSED;
    if (!transport_initialized) return IPC_ERR_INIT;

    int bytes = nn_send(pub->socket_fd, data, len, 0);
    if (bytes < 0) {
        printf(TAG "PUB send failed on %s: %s\n",
               pub->endpoint, nn_strerror(nn_errno()));
        return IPC_ERR_SEND;
    }

    pub->msgs_sent++;
    pub->bytes_sent += (uint32_t)bytes;
    return IPC_OK;
}

void ipc_pub_close(ipc_publisher_t *pub) {
    if (!pub || !pub->active) return;

    printf(TAG "Publisher closing: %s (sent %u msgs, %u bytes)\n",
           pub->endpoint, pub->msgs_sent, pub->bytes_sent);

    nn_close(pub->socket_fd);
    pub->socket_fd = -1;
    pub->active = false;
}

/* ── Subscriber (nanomsg) ────────────────────────────────── */

ipc_error_t ipc_sub_create(ipc_subscriber_t *sub, const char *endpoint,
                            int timeout_ms, ipc_recv_callback_t callback,
                            void *user_data) {
    if (!sub || !endpoint) return IPC_ERR_PARAM;
    if (!transport_initialized) return IPC_ERR_INIT;

    memset(sub, 0, sizeof(*sub));
    strncpy(sub->endpoint, endpoint, IPC_ENDPOINT_MAX - 1);
    sub->endpoint[IPC_ENDPOINT_MAX - 1] = '\0';
    sub->timeout_ms = timeout_ms > 0 ? timeout_ms : IPC_RECV_TIMEOUT_MS;
    sub->callback = callback;
    sub->user_data = user_data;

    /* Create SUB socket */
    sub->socket_fd = nn_socket(AF_SP, NN_SUB);
    if (sub->socket_fd < 0) {
        printf(TAG "Failed to create SUB socket: %s\n", nn_strerror(nn_errno()));
        return IPC_ERR_SOCKET;
    }

    /* Subscribe to all messages (empty topic = receive everything) */
    int rv = nn_setsockopt(sub->socket_fd, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    if (rv < 0) {
        printf(TAG "Failed to set SUB subscribe: %s\n", nn_strerror(nn_errno()));
        nn_close(sub->socket_fd);
        sub->socket_fd = -1;
        return IPC_ERR_SOCKET;
    }

    /* Set receive timeout */
    rv = nn_setsockopt(sub->socket_fd, NN_SOL_SOCKET, NN_RCVTIMEO,
                       &sub->timeout_ms, sizeof(sub->timeout_ms));
    if (rv < 0) {
        printf(TAG "Warning: failed to set recv timeout: %s\n",
               nn_strerror(nn_errno()));
        /* Non-fatal, continue */
    }

    /* Connect to publisher endpoint */
    rv = nn_connect(sub->socket_fd, endpoint);
    if (rv < 0) {
        printf(TAG "Failed to connect SUB to %s: %s\n",
               endpoint, nn_strerror(nn_errno()));
        nn_close(sub->socket_fd);
        sub->socket_fd = -1;
        return IPC_ERR_CONNECT;
    }

    sub->active = true;
    sub->msgs_received = 0;
    sub->bytes_received = 0;

    printf(TAG "Subscriber created: %s (fd=%d, timeout=%dms)\n",
           sub->endpoint, sub->socket_fd, sub->timeout_ms);
    return IPC_OK;
}

ipc_error_t ipc_sub_recv(ipc_subscriber_t *sub, size_t *out_len) {
    if (!sub) return IPC_ERR_PARAM;
    if (!sub->active) return IPC_ERR_CLOSED;
    if (!transport_initialized) return IPC_ERR_INIT;

    int bytes = nn_recv(sub->socket_fd, sub->recv_buf,
                        IPC_RECV_BUF_MAX, 0);

    if (bytes < 0) {
        int err = nn_errno();
        if (err == ETIMEDOUT || err == EAGAIN) {
            if (out_len) *out_len = 0;
            return IPC_ERR_TIMEOUT;
        }
        printf(TAG "SUB recv failed on %s: %s\n",
               sub->endpoint, nn_strerror(err));
        if (out_len) *out_len = 0;
        return IPC_ERR_RECV;
    }

    sub->msgs_received++;
    sub->bytes_received += (uint32_t)bytes;
    if (out_len) *out_len = (size_t)bytes;

    /* Dispatch to callback if registered */
    if (sub->callback && (size_t)bytes >= sizeof(ipc_msg_header_t)) {
        const ipc_msg_header_t *hdr = (const ipc_msg_header_t *)sub->recv_buf;
        sub->callback(hdr->msg_type, sub->recv_buf, (size_t)bytes, sub->user_data);
    }

    return IPC_OK;
}

void ipc_sub_close(ipc_subscriber_t *sub) {
    if (!sub || !sub->active) return;

    printf(TAG "Subscriber closing: %s (recv %u msgs, %u bytes)\n",
           sub->endpoint, sub->msgs_received, sub->bytes_received);

    nn_close(sub->socket_fd);
    sub->socket_fd = -1;
    sub->active = false;
    sub->callback = NULL;
    sub->user_data = NULL;
}

#endif /* SIMULATOR_BUILD */

/* ── Debug / statistics ──────────────────────────────────── */

const char *ipc_error_str(ipc_error_t err) {
    switch (err) {
        case IPC_OK:            return "OK";
        case IPC_ERR_INIT:      return "Transport not initialized";
        case IPC_ERR_SOCKET:    return "Socket creation failed";
        case IPC_ERR_BIND:      return "Bind failed";
        case IPC_ERR_CONNECT:   return "Connect failed";
        case IPC_ERR_SEND:      return "Send failed";
        case IPC_ERR_RECV:      return "Receive failed";
        case IPC_ERR_TIMEOUT:   return "Receive timed out";
        case IPC_ERR_PARAM:     return "Invalid parameter";
        case IPC_ERR_FULL:      return "No free slots";
        case IPC_ERR_CLOSED:    return "Socket closed";
        default:                return "Unknown error";
    }
}

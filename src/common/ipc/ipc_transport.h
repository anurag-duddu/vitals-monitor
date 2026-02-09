/**
 * @file ipc_transport.h
 * @brief IPC transport layer for nanomsg pub/sub messaging
 *
 * Provides a thin wrapper around nanomsg PUB/SUB sockets for
 * inter-process communication between services:
 *   - sensor-service (publisher) -> ui-app, alarm-service (subscribers)
 *   - alarm-service (publisher)  -> ui-app (subscriber)
 *   - ui-app (publisher)         -> alarm-service (control commands)
 *
 * In the simulator build (SIMULATOR_BUILD), all operations are stubbed:
 *   - Publishers accept messages but discard them (no-op send)
 *   - Subscribers can receive data via ipc_sub_inject() for testing
 *
 * In the target build, real nanomsg nn_socket/nn_bind/nn_connect
 * calls are used with the IPC_SOCKET_* endpoints from ipc_messages.h.
 *
 * Thread safety:
 *   - Each publisher/subscriber instance is NOT thread-safe
 *   - Use one instance per thread, or protect with external mutex
 *   - ipc_sub_recv() supports configurable timeout for polling
 *
 * Static allocation: All state is stored in caller-provided structs.
 * No dynamic memory allocation in the transport layer.
 */

#ifndef IPC_TRANSPORT_H
#define IPC_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ─────────────────────────────────────────────── */

#define IPC_ENDPOINT_MAX        128     /* Max endpoint URL length */
#define IPC_RECV_BUF_MAX        2048    /* Max receive buffer size */
#define IPC_RECV_TIMEOUT_MS     100     /* Default receive timeout (ms) */
#define IPC_MAX_PUBLISHERS      4       /* Max simultaneous publishers */
#define IPC_MAX_SUBSCRIBERS     8       /* Max simultaneous subscribers */

/* ── Error codes ───────────────────────────────────────────── */

typedef enum {
    IPC_OK              =  0,   /* Success */
    IPC_ERR_INIT        = -1,   /* Transport not initialized */
    IPC_ERR_SOCKET      = -2,   /* Socket creation failed */
    IPC_ERR_BIND        = -3,   /* Bind failed (publisher) */
    IPC_ERR_CONNECT     = -4,   /* Connect failed (subscriber) */
    IPC_ERR_SEND        = -5,   /* Send failed */
    IPC_ERR_RECV        = -6,   /* Receive failed */
    IPC_ERR_TIMEOUT     = -7,   /* Receive timed out (no data) */
    IPC_ERR_PARAM       = -8,   /* Invalid parameter */
    IPC_ERR_FULL        = -9,   /* No free slots (max publishers/subscribers) */
    IPC_ERR_CLOSED      = -10,  /* Socket already closed */
} ipc_error_t;

/* ── Publisher ─────────────────────────────────────────────── */

typedef struct {
    int     socket_fd;                      /* nanomsg socket fd (-1 = closed) */
    char    endpoint[IPC_ENDPOINT_MAX];     /* Bound endpoint URL */
    bool    active;                         /* True if successfully bound */
    uint32_t msgs_sent;                     /* Cumulative messages sent */
    uint32_t bytes_sent;                    /* Cumulative bytes sent */
} ipc_publisher_t;

/* ── Subscriber ────────────────────────────────────────────── */

/**
 * Callback invoked when a message is received.
 * @param msg_type  Message type byte (ipc_msg_type_t)
 * @param data      Pointer to raw message payload (including header)
 * @param len       Total message length in bytes
 * @param user_data Opaque pointer from ipc_sub_create()
 */
typedef void (*ipc_recv_callback_t)(uint8_t msg_type, const void *data,
                                     size_t len, void *user_data);

typedef struct {
    int     socket_fd;                      /* nanomsg socket fd (-1 = closed) */
    char    endpoint[IPC_ENDPOINT_MAX];     /* Connected endpoint URL */
    bool    active;                         /* True if successfully connected */
    int     timeout_ms;                     /* Receive timeout in ms */
    uint32_t msgs_received;                 /* Cumulative messages received */
    uint32_t bytes_received;                /* Cumulative bytes received */
    uint8_t  recv_buf[IPC_RECV_BUF_MAX];   /* Static receive buffer */

    /* Optional callback for async dispatch */
    ipc_recv_callback_t callback;
    void               *user_data;
} ipc_subscriber_t;

/* ── Transport lifecycle ───────────────────────────────────── */

/**
 * Initialize the IPC transport layer.
 * Must be called before creating any publishers or subscribers.
 * @return IPC_OK on success, negative error code on failure.
 */
ipc_error_t ipc_transport_init(void);

/**
 * Shut down the IPC transport layer.
 * Closes all active publishers and subscribers.
 */
void ipc_transport_close(void);

/**
 * Check if the transport layer is initialized.
 * @return true if initialized.
 */
bool ipc_transport_is_initialized(void);

/* ── Publisher API ─────────────────────────────────────────── */

/**
 * Create a publisher bound to the given endpoint.
 * @param pub       Publisher struct to initialize (caller-allocated).
 * @param endpoint  nanomsg endpoint URL (e.g. IPC_SOCKET_VITALS).
 * @return IPC_OK on success, negative error code on failure.
 */
ipc_error_t ipc_pub_create(ipc_publisher_t *pub, const char *endpoint);

/**
 * Send a message on the publisher socket.
 * @param pub   Active publisher.
 * @param data  Message bytes (must include ipc_msg_header_t).
 * @param len   Total message length.
 * @return IPC_OK on success, negative error code on failure.
 */
ipc_error_t ipc_pub_send(ipc_publisher_t *pub, const void *data, size_t len);

/**
 * Close a publisher and release its nanomsg socket.
 * @param pub  Publisher to close. Safe to call on already-closed publisher.
 */
void ipc_pub_close(ipc_publisher_t *pub);

/* ── Subscriber API ────────────────────────────────────────── */

/**
 * Create a subscriber connected to the given endpoint.
 * @param sub        Subscriber struct to initialize (caller-allocated).
 * @param endpoint   nanomsg endpoint URL (e.g. IPC_SOCKET_VITALS).
 * @param timeout_ms Receive timeout in milliseconds (0 = non-blocking).
 * @param callback   Optional message callback (NULL for polling mode).
 * @param user_data  Opaque pointer passed to callback.
 * @return IPC_OK on success, negative error code on failure.
 */
ipc_error_t ipc_sub_create(ipc_subscriber_t *sub, const char *endpoint,
                            int timeout_ms, ipc_recv_callback_t callback,
                            void *user_data);

/**
 * Receive a message from the subscriber socket (blocking with timeout).
 *
 * If a callback was registered, it is invoked with the received message.
 * Otherwise, the message is stored in sub->recv_buf and the caller can
 * inspect it directly.
 *
 * @param sub       Active subscriber.
 * @param out_len   If non-NULL, receives the number of bytes read.
 * @return IPC_OK if a message was received, IPC_ERR_TIMEOUT if none
 *         available, or negative error code on failure.
 */
ipc_error_t ipc_sub_recv(ipc_subscriber_t *sub, size_t *out_len);

/**
 * Close a subscriber and release its nanomsg socket.
 * @param sub  Subscriber to close. Safe to call on already-closed subscriber.
 */
void ipc_sub_close(ipc_subscriber_t *sub);

/* ── Simulator test helpers ────────────────────────────────── */

#ifdef SIMULATOR_BUILD

/**
 * Inject a message into a subscriber for testing.
 * The message is delivered as if it arrived on the nanomsg socket.
 * If a callback is registered, it is invoked immediately.
 *
 * @param sub   Subscriber to inject into.
 * @param data  Message bytes (must include ipc_msg_header_t).
 * @param len   Total message length.
 * @return IPC_OK on success, IPC_ERR_PARAM if len > IPC_RECV_BUF_MAX.
 */
ipc_error_t ipc_sub_inject(ipc_subscriber_t *sub, const void *data, size_t len);

#endif /* SIMULATOR_BUILD */

/* ── Debug / statistics ────────────────────────────────────── */

/**
 * Get human-readable string for an IPC error code.
 * @param err  Error code.
 * @return Static string describing the error.
 */
const char *ipc_error_str(ipc_error_t err);

#ifdef __cplusplus
}
#endif

#endif /* IPC_TRANSPORT_H */

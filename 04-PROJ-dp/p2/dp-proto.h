/*
 * dp-proto.h - dp (Drexel Protocol), Part 2: streams and head-of-line blocking
 *
 * Part 2 extends dp from Part 1 with multiple independent streams per
 * connection, a sliding window with selective retransmission, and per-stream
 * reassembly. TCP delivers one ordered byte stream, so one lost packet stalls
 * everything behind it (head-of-line blocking). QUIC, a real transport that
 * runs over UDP and underlies HTTP/3, avoids this with independent streams;
 * dp borrows that idea. dp is not QUIC.
 *
 * Provided file: do not modify.
 */
#ifndef DP_PROTO_H
#define DP_PROTO_H

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

#define DP_MAGIC 0x4450
#define DP_MAX_PAYLOAD 480
#define DP_DEFAULT_PORT 9000
#define DP_DEFAULT_TIMEOUT_MS 250
#define DP_DEFAULT_MAX_RETRIES 20
#define DP_DEFAULT_WINDOW 8
#define DP_MAX_WINDOW 64
#define DP_MAX_STREAMS 8
#define DP_MAX_STREAM_BYTES (64U * 1024U * 1024U)
#define DP_MAX_FORCED_DROPS 32
#define DP_FLAG_FIN 0x01
#define DP_ERR_NOT_IMPLEMENTED -99

/* After answering CLOSE, the receiver lingers this many timeouts of silence
 * so it can re-answer a retransmitted CLOSE if its CLOSE_ACK was lost. */
#define DP_CLOSE_LINGER_TIMEOUTS 8

typedef enum {
    DP_CONNECT = 1,
    DP_CONNECT_ACK = 2,
    DP_DATA = 3,
    DP_ACK = 4,
    DP_CLOSE = 5,
    DP_CLOSE_ACK = 6
} dp_type_t;

/* Same 28-byte header as Part 1. Two fields changed meaning:
 *   message_offset -> stream_offset: position of this payload in its stream
 *   message_length -> unused: the FIN flag alone marks the end of a stream
 * ACK packets echo the acknowledged DATA packet's number, stream, and offset. */
typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t type;
    uint8_t flags;
    uint32_t connection_id;
    uint32_t packet_number;   /* connection-scoped; a retransmission gets a NEW number */
    uint32_t stream_id;       /* 0 .. DP_MAX_STREAMS-1 */
    uint32_t stream_offset;   /* byte offset within the stream (multiple of DP_MAX_PAYLOAD) */
    uint32_t unused;          /* Part 1 message_length; always 0 in Part 2 */
    uint16_t payload_length;
    uint16_t reserved;
    uint8_t payload[DP_MAX_PAYLOAD];
} dp_packet_t;

/* One piece of stream data. This is what gets retransmitted: the same
 * (stream, offset, length) can travel in several packets with different
 * packet numbers. */
typedef struct {
    uint32_t stream_id;
    uint32_t offset;
    uint16_t length;
    uint8_t fin;
    uint8_t transmissions;    /* how many times it has been sent so far */
} dp_chunk_t;

/* A DATA packet that has been sent and not yet acknowledged. */
typedef struct {
    int in_use;
    uint32_t packet_number;
    dp_chunk_t chunk;
    long long deadline_ms;    /* monotonic time at which it times out */
} dp_inflight_t;

/* Sender-side state for one stream. The application's buffer is not copied;
 * it must stay valid until dp_flush() returns. */
typedef struct {
    int active;
    const uint8_t *data;
    size_t length;
    size_t next_offset;       /* next byte not yet packetized */
    int fin_queued;           /* the FIN chunk has been packetized */
} dp_send_stream_t;

/* Receiver-side state for one stream. Data may arrive in any order; it is
 * delivered to the application only in order. */
typedef struct {
    int active;
    uint8_t *buf;
    size_t cap;
    uint8_t *have;            /* have[i] != 0 when chunk i (offset i*480) arrived */
    size_t have_cap;
    size_t max_end;           /* highest byte position received so far */
    size_t contiguous;        /* bytes 0..contiguous-1 have all arrived */
    size_t delivered;         /* bytes already returned by dp_stream_read() */
    int fin_seen;
    size_t final_size;        /* valid once fin_seen */
    int fin_delivered;
} dp_recv_stream_t;

typedef struct {
    int fd;
    struct sockaddr_in peer;
    socklen_t peer_len;
    uint32_t connection_id;
    uint32_t next_packet_number;   /* next packet number this endpoint sends */
    int timeout_ms;
    int max_retries;
    int window;                    /* max DATA packets in flight */
    double loss_probability;
    unsigned max_delay_ms;
    unsigned random_seed;
    int log_network;
    uint32_t forced_drops[DP_MAX_FORCED_DROPS];
    int forced_drop_count;
    const char *role;              /* Output prefix: dp-client or dp-server. */

    /* Sender state */
    dp_send_stream_t send[DP_MAX_STREAMS];
    int send_rr;                   /* round-robin position for new data */
    dp_inflight_t inflight[DP_MAX_WINDOW];
    int inflight_count;
    dp_chunk_t retransmit[DP_MAX_WINDOW];
    int retransmit_count;

    /* Receiver state */
    dp_recv_stream_t recv[DP_MAX_STREAMS];
    int recv_rr;
    int peer_closed;
} dp_connection_t;

typedef struct {
    unsigned long attempted;
    unsigned long dropped;
    unsigned long delivered;
    unsigned long delayed;
    unsigned long long delay_ms_total;
    unsigned long retransmits;     /* DATA chunks sent more than once */
    unsigned long duplicates;      /* DATA chunks received more than once */
} dp_net_stats_t;

int dp_client_init(dp_connection_t *conn, const char *host, uint16_t port);
int dp_server_init(dp_connection_t *conn, uint16_t port);
int dp_accept(dp_connection_t *conn);
int dp_connect(dp_connection_t *conn);

/* Sender: register each stream's data, then dp_flush() sends all streams
 * through the window and returns once every byte is acknowledged. */
int dp_stream_write(dp_connection_t *conn, uint32_t stream_id, const void *buffer, size_t len);
int dp_flush(dp_connection_t *conn);

/* Receiver: returns the next newly in-order bytes of any stream.
 * Returns 0 with an event, 1 when the peer has closed and all data has been
 * returned, negative on error. *data stays valid until the next call. */
int dp_stream_read(dp_connection_t *conn, uint32_t *stream_id,
                   const uint8_t **data, size_t *len, int *fin);

int dp_close(dp_connection_t *conn);
int dp_wait_for_close(dp_connection_t *conn);
void dp_free(dp_connection_t *conn);

void dp_set_network_simulation(dp_connection_t *conn,
                               double loss_probability,
                               unsigned max_delay_ms,
                               unsigned random_seed,
                               int log_network);
int dp_add_forced_drop(dp_connection_t *conn, uint32_t packet_number);
void dp_set_window(dp_connection_t *conn, int window);
void dp_get_network_stats(const dp_connection_t *conn, dp_net_stats_t *stats);

#endif

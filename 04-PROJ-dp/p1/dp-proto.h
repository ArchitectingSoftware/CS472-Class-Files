/*
 * dp-proto.h - dp (Drexel Protocol), Part 1: reliable delivery over UDP
 *
 * dp is a small transport protocol built on top of UDP for CS472. Part 1
 * gives dp what UDP lacks for reliable delivery: connections, packet numbers,
 * ACKs, timeouts and retransmission, duplicate detection, fragmentation, and
 * FIN. These are the same problems TCP solves. Some design ideas come from
 * QUIC, a real transport that also runs over UDP; dp is not QUIC.
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
#define DP_DEFAULT_SIZE_KB 1024
#define DP_DEFAULT_TIMEOUT_MS 250
#define DP_DEFAULT_MAX_RETRIES 20
#define DP_FLAG_FIN 0x01
#define DP_ERR_NOT_IMPLEMENTED -99

/* After answering CLOSE, the receiver lingers this many timeouts of silence
 * so it can re-answer a retransmitted CLOSE if its CLOSE_ACK was lost. */
#define DP_CLOSE_LINGER_TIMEOUTS 8
#define DP_MAX_FORCED_DROPS 32

typedef enum {
    DP_CONNECT = 1,
    DP_CONNECT_ACK = 2,
    DP_DATA = 3,
    DP_ACK = 4,
    DP_CLOSE = 5,
    DP_CLOSE_ACK = 6
} dp_type_t;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t type;
    uint8_t flags;
    uint32_t connection_id;
    uint32_t packet_number;
    uint32_t stream_id;       /* Part 1: always 0. Revisited in Part 2. */
    uint32_t message_offset;
    uint32_t message_length;
    uint16_t payload_length;
    uint16_t reserved;
    uint8_t payload[DP_MAX_PAYLOAD];
} dp_packet_t;

typedef struct {
    int fd;
    struct sockaddr_in peer;
    socklen_t peer_len;
    uint32_t connection_id;
    uint32_t next_packet_number;   /* next packet number this endpoint sends */
    uint32_t recv_next_packet;     /* next DATA packet number this endpoint expects */
    int timeout_ms;
    int max_retries;
    double loss_probability;
    unsigned max_delay_ms;
    unsigned random_seed;
    int log_network;
    const char *role;          /* Output prefix: dp-client or dp-server. */
    /* -D: drop the FIRST transmission of these DATA (client) or ACK (server)
     * packet numbers. Each entry fires once, since Part 1 retransmits with
     * the same packet number. */
    uint32_t forced_drops[DP_MAX_FORCED_DROPS];
    uint8_t forced_used[DP_MAX_FORCED_DROPS];
    int forced_drop_count;
} dp_connection_t;

typedef struct {
    unsigned long attempted;
    unsigned long dropped;
    unsigned long delivered;
    unsigned long delayed;
    unsigned long long delay_ms_total;
} dp_net_stats_t;

int dp_client_init(dp_connection_t *conn, const char *host, uint16_t port);
int dp_server_init(dp_connection_t *conn, uint16_t port);
int dp_accept(dp_connection_t *conn);

int dp_connect(dp_connection_t *conn);
int dp_send(dp_connection_t *conn, const void *buffer, size_t len);
int dp_recv(dp_connection_t *conn, void *buffer, size_t capacity, size_t *out_len);
int dp_close(dp_connection_t *conn);
int dp_wait_for_close(dp_connection_t *conn);

void dp_set_network_simulation(dp_connection_t *conn,
                               double loss_probability,
                               unsigned max_delay_ms,
                               unsigned random_seed,
                               int log_network);
void dp_get_network_stats(const dp_connection_t *conn, dp_net_stats_t *stats);
int dp_add_forced_drop(dp_connection_t *conn, uint32_t packet_number);

#endif

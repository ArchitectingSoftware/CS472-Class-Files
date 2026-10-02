/*
 * dp-proto.c - dp (Drexel Protocol), Part 1: reliable delivery over UDP
 *
 * The dp transport: connection setup and close, stop-and-wait reliability,
 * fragmentation and reassembly, and the network simulator.
 *
 * Student TODOs: dp_connect(), send_data_packet(), dp_recv(), dp_close().
 * Every other function is provided and must not be modified.
 */
#define _POSIX_C_SOURCE 200809L
#include "dp-proto.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DP_HEADER_SIZE (sizeof(dp_packet_t) - DP_MAX_PAYLOAD)
/* Kept outside the public structure so the student-facing connection API stays compact. */
typedef struct dp_private_state {
    dp_connection_t *conn;
    dp_net_stats_t stats;
    int connected;
    struct dp_private_state *next;
} dp_private_state_t;

static dp_private_state_t *g_states;

static dp_private_state_t *state_for(dp_connection_t *conn) {
    for (dp_private_state_t *s = g_states; s; s = s->next) {
        if (s->conn == conn) return s;
    }

    dp_private_state_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->conn = conn;
    s->next = g_states;
    g_states = s;
    return s;
}

static void packet_host_to_net(dp_packet_t *p) {
    p->magic = htons(p->magic);
    p->connection_id = htonl(p->connection_id);
    p->packet_number = htonl(p->packet_number);
    p->stream_id = htonl(p->stream_id);
    p->message_offset = htonl(p->message_offset);
    p->message_length = htonl(p->message_length);
    p->payload_length = htons(p->payload_length);
    p->reserved = htons(p->reserved);
}

static void packet_net_to_host(dp_packet_t *p) {
    p->magic = ntohs(p->magic);
    p->connection_id = ntohl(p->connection_id);
    p->packet_number = ntohl(p->packet_number);
    p->stream_id = ntohl(p->stream_id);
    p->message_offset = ntohl(p->message_offset);
    p->message_length = ntohl(p->message_length);
    p->payload_length = ntohs(p->payload_length);
    p->reserved = ntohs(p->reserved);
}

static int packet_valid(const dp_packet_t *p, size_t received) {
    if (received < DP_HEADER_SIZE) return 0;
    if (p->magic != DP_MAGIC) return 0;
    if (p->payload_length > DP_MAX_PAYLOAD) return 0;
    if ((p->flags & ~DP_FLAG_FIN) != 0) return 0;
    if (received < DP_HEADER_SIZE + p->payload_length) return 0;
    return 1;
}

static unsigned random_delay(unsigned max_delay_ms) {
    if (max_delay_ms == 0) return 0;
    return (unsigned)(rand() % (max_delay_ms + 1U));
}

static const char *packet_type_name(uint8_t type) {
    switch (type) {
        case DP_DATA: return "DATA";
        case DP_ACK: return "ACK";
        case DP_CONNECT: return "CONNECT";
        case DP_CONNECT_ACK: return "CONN_ACK";
        case DP_CLOSE: return "CLOSE";
        case DP_CLOSE_ACK: return "CLOSE_ACK";
        default: return "UNKNOWN";
    }
}

static long long log_timestamp_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) return 0;
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000L;
}

static void log_net_event(const dp_connection_t *conn,
                          const char *direction,
                          const dp_packet_t *packet,
                          const char *detail) {
    if (!conn || !conn->log_network) return;
    long long timestamp_us = log_timestamp_us();
    fprintf(stderr, "%s: NET: %lld.%06lld %s %-11s pkt=%u",
            conn->role ? conn->role : "dp-proto",
            timestamp_us / 1000000LL,
            timestamp_us % 1000000LL,
            direction,
            packet_type_name(packet->type),
            packet->packet_number);
    if (detail && *detail) fprintf(stderr, " %s", detail);
    fputc('\n', stderr);
}


/* ---------------------------------------------------------------------------
 * Timing helpers (provided).
 *
 * SO_RCVTIMEO restarts every time recvfrom() is called. If we simply called
 * recv_packet(conn, ..., timeout_ms) in a loop, every unrelated packet (for
 * example a stale, delayed ACK) would restart the timer and the wait could
 * last forever. recv_until() waits against a fixed deadline instead.
 * ------------------------------------------------------------------------- */
static long long monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) return 0;
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000L;
}

static long long deadline_after(int timeout_ms) {
    return monotonic_ms() + timeout_ms;
}

static int net_simulate(dp_connection_t *conn, const dp_packet_t *packet, size_t bytes) {
    dp_private_state_t *state = state_for(conn);
    if (!state) return -1;

    state->stats.attempted++;

    /* Forced drop (-D): first transmission of a listed DATA or ACK number. */
    int forced = 0;
    if (packet->type == DP_DATA || packet->type == DP_ACK) {
        for (int i = 0; i < conn->forced_drop_count; ++i) {
            if (!conn->forced_used[i] && conn->forced_drops[i] == packet->packet_number) {
                conn->forced_used[i] = 1;
                forced = 1;
                break;
            }
        }
    }

    double r = (double)rand() / ((double)RAND_MAX + 1.0);
    if (forced || r < conn->loss_probability) {
        state->stats.dropped++;
        char detail[64];
        snprintf(detail, sizeof(detail), "bytes=%zu DROP%s", bytes, forced ? " (forced)" : "");
        log_net_event(conn, "OUT", packet, detail);
        /* A drop is silent. The caller sees the same result as a successful
         * send and must discover the loss the way a real sender does: no
         * response arrives before the timeout. */
        return 0;
    }

    unsigned delay = random_delay(conn->max_delay_ms);
    if (delay) {
        state->stats.delayed++;
        state->stats.delay_ms_total += delay;
        char detail[64];
        snprintf(detail, sizeof(detail), "bytes=%zu delay=%ums", bytes, delay);
        log_net_event(conn, "OUT", packet, detail);
        struct timespec ts;
        ts.tv_sec = delay / 1000U;
        ts.tv_nsec = (long)(delay % 1000U) * 1000000L;
        nanosleep(&ts, NULL);
    } else {
        char detail[64];
        snprintf(detail, sizeof(detail), "bytes=%zu", bytes);
        log_net_event(conn, "OUT", packet, detail);
    }

    dp_packet_t wire = *packet;
    packet_host_to_net(&wire);
    ssize_t sent = sendto(conn->fd, &wire, bytes, 0,
                          (struct sockaddr *)&conn->peer, conn->peer_len);
    if (sent < 0) return -1;
    state->stats.delivered++;
    return 0;
}

/* Receive one packet. timeout_ms < 0 blocks forever; timeout_ms == 0 returns
 * immediately as a timeout. Returns bytes received (> 2), 0 on timeout,
 * 2 for a malformed packet or one from the wrong peer, -1 on socket error. */
static int recv_packet(dp_connection_t *conn, dp_packet_t *packet, int timeout_ms) {
    if (timeout_ms == 0) return 0;

    struct timeval tv = {0, 0};
    if (timeout_ms > 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
    }
    if (setsockopt(conn->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) return -1;

    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    ssize_t n = recvfrom(conn->fd, packet, sizeof(*packet), 0,
                         (struct sockaddr *)&from, &from_len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
        return -1;
    }

    packet_net_to_host(packet);
    if (!packet_valid(packet, (size_t)n)) return 2;

    /* Server learns its peer during connection establishment; both sides then
     * ignore datagrams from any other UDP endpoint. */
    if (conn->peer_len != 0 &&
        (from.sin_addr.s_addr != conn->peer.sin_addr.s_addr ||
         from.sin_port != conn->peer.sin_port)) {
        return 2;
    }

    if (conn->log_network) {
        char detail[64];
        snprintf(detail, sizeof(detail), "bytes=%zd", n);
        log_net_event(conn, "IN ", packet, detail);
    }
    return (int)n;
}

/* Wait for one valid packet from the peer until deadline_ms (a monotonic_ms()
 * value). Malformed/foreign packets are skipped without extending the wait.
 * Returns > 0 when a packet was received, 0 when the deadline passed, -1 on
 * socket error. */
static int recv_until(dp_connection_t *conn, dp_packet_t *packet, long long deadline_ms) {
    for (;;) {
        long long remaining = deadline_ms - monotonic_ms();
        if (remaining <= 0) return 0;
        int rc = recv_packet(conn, packet, (int)remaining);
        if (rc == 2) continue;
        return rc;
    }
}

static void init_connection(dp_connection_t *conn) {
    memset(conn, 0, sizeof(*conn));
    conn->fd = -1;
    conn->timeout_ms = DP_DEFAULT_TIMEOUT_MS;
    conn->max_retries = DP_DEFAULT_MAX_RETRIES;
    conn->loss_probability = 0.0;
    conn->max_delay_ms = 0;
    conn->random_seed = (unsigned)time(NULL) ^ (unsigned)getpid();
    srand(conn->random_seed);
}

static int make_udp_socket(void) {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

int dp_client_init(dp_connection_t *conn, const char *host, uint16_t port) {
    if (!conn || !host) return -1;
    init_connection(conn);
    conn->role = "dp-client";
    conn->fd = make_udp_socket();
    if (conn->fd < 0) return -1;

    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%u", port);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int rc = getaddrinfo(host, portstr, &hints, &res);
    if (rc != 0) {
        close(conn->fd);
        conn->fd = -1;
        return -1;
    }

    memcpy(&conn->peer, res->ai_addr, sizeof(conn->peer));
    conn->peer_len = sizeof(conn->peer);
    freeaddrinfo(res);
    if (!state_for(conn)) {
        close(conn->fd);
        conn->fd = -1;
        return -1;
    }
    return 0;
}

int dp_server_init(dp_connection_t *conn, uint16_t port) {
    if (!conn) return -1;
    init_connection(conn);
    conn->role = "dp-server";
    conn->fd = make_udp_socket();
    if (conn->fd < 0) return -1;

    int one = 1;
    setsockopt(conn->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (bind(conn->fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        close(conn->fd);
        conn->fd = -1;
        return -1;
    }
    conn->peer_len = 0;
    return state_for(conn) ? 0 : -1;
}

/* Build and send a header-only control packet (provided). */
static int send_control(dp_connection_t *conn, dp_type_t type, uint32_t packet_number) {
    dp_packet_t p = {0};
    p.magic = DP_MAGIC;
    p.type = (uint8_t)type;
    p.connection_id = conn->connection_id;
    p.packet_number = packet_number;
    p.stream_id = 0;
    p.payload_length = 0;
    return net_simulate(conn, &p, DP_HEADER_SIZE);
}

static int send_ack(dp_connection_t *conn, uint32_t packet_number) {
    return send_control(conn, DP_ACK, packet_number);
}

static int send_connect_ack(dp_connection_t *conn) {
    return send_control(conn, DP_CONNECT_ACK, 0);
}

static int send_close_ack(dp_connection_t *conn, uint32_t packet_number) {
    return send_control(conn, DP_CLOSE_ACK, packet_number);
}

int dp_accept(dp_connection_t *conn) {
    if (!conn || conn->fd < 0) return -1;

    for (;;) {
        dp_packet_t packet = {0};
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        ssize_t n = recvfrom(conn->fd, &packet, sizeof(packet), 0,
                             (struct sockaddr *)&from, &from_len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        packet_net_to_host(&packet);
        if (!packet_valid(&packet, (size_t)n) || packet.type != DP_CONNECT) continue;

        conn->peer = from;
        conn->peer_len = from_len;
        conn->connection_id = packet.connection_id;
        conn->next_packet_number = 1;
        conn->recv_next_packet = 1;

        if (conn->log_network) {
            char detail[64];
            snprintf(detail, sizeof(detail), "bytes=%zd", n);
            log_net_event(conn, "IN ", &packet, detail);
        }

        /* The server cannot know whether this CONNECT_ACK arrives. If it is
         * lost, the client retransmits CONNECT and dp_recv() answers again. */
        if (send_connect_ack(conn) < 0) return -1;
        return 0;
    }
}

int dp_connect(dp_connection_t *conn) {
    /* TODO 1: Establish the logical connection reliably.
     *
     *   - If conn->connection_id is 0, choose a random non-zero ID.
     *   - Send a DP_CONNECT packet with packet number 0 and that ID.
     *   - Wait for a DP_CONNECT_ACK with the same connection ID using
     *     recv_until() and a deadline from deadline_after(conn->timeout_ms).
     *   - On timeout, retransmit the SAME CONNECT (same ID, same packet
     *     number) and log it with log_net_event(conn, "TIMEOUT", ...).
     *     Give up with -2 after conn->max_retries retransmissions.
     *   - On success set next_packet_number and recv_next_packet to 1.
     */
    fprintf(stderr, "%s: dp_connect: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

static int send_data_packet(dp_connection_t *conn, const dp_packet_t *packet) {
    /* TODO 2: Send one DATA packet using stop-and-wait reliability.
     *
     *   - Send the packet through net_simulate().
     *   - Wait for an ACK with the same connection ID and packet number.
     *     Use recv_until() with ONE deadline per transmission so that
     *     unrelated packets (for example a delayed duplicate ACK for an
     *     earlier packet) are ignored without restarting the timer.
     *   - On timeout, log a TIMEOUT event and retransmit the exact same PDU.
     *     Only timeouts count as retries. Return -2 after
     *     conn->max_retries retransmissions.
     *   - Return 0 when the matching ACK arrives, -1 on socket error.
     *
     * A timeout does NOT prove that the DATA packet was lost: the DATA may
     * have arrived and its ACK may have been lost or delayed.
     */
    (void)packet;
    fprintf(stderr, "%s: send_data_packet: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

int dp_send(dp_connection_t *conn, const void *buffer, size_t len) {
    if (!conn || conn->fd < 0 || !buffer) return -1;
    if (len > UINT32_MAX) return -1;

    const uint8_t *src = buffer;
    size_t offset = 0;
    do {
        size_t remaining = len - offset;
        size_t chunk = remaining > DP_MAX_PAYLOAD ? DP_MAX_PAYLOAD : remaining;
        dp_packet_t packet = {0};
        packet.magic = DP_MAGIC;
        packet.type = DP_DATA;
        packet.connection_id = conn->connection_id;
        packet.packet_number = conn->next_packet_number++;
        packet.stream_id = 0;
        packet.message_offset = (uint32_t)offset;
        packet.message_length = (uint32_t)len;
        packet.payload_length = (uint16_t)chunk;
        if (offset + chunk == len) packet.flags |= DP_FLAG_FIN;
        if (chunk) memcpy(packet.payload, src + offset, chunk);

        int rc = send_data_packet(conn, &packet);
        if (rc < 0) return rc;
        offset += chunk;
    } while (offset < len);
    return 0;
}

/* Returns 0 with a complete message, 1 if the peer closed the connection
 * first, -1 on socket error, -3 if the message exceeds capacity, and -4 on a
 * protocol violation. */
int dp_recv(dp_connection_t *conn, void *buffer, size_t capacity, size_t *out_len) {
    /* TODO 3: Receive and reassemble exactly one logical message.
     *
     * Block with recv_packet(conn, &packet, -1) and ignore packets whose
     * connection ID does not match. Then handle each packet type:
     *
     *   DP_CONNECT  Our CONNECT_ACK was lost and the client retried.
     *               Answer again with send_connect_ack().
     *   DP_CLOSE    Answer with send_close_ack() and return 1.
     *   DP_DATA     Compare packet_number with conn->recv_next_packet:
     *     - smaller: a duplicate you already accepted (its ACK was lost).
     *                Log it with log_net_event(conn, "DUP", &packet,
     *                "already delivered, re-ACK"), then re-ACK it with
     *                send_ack(). Do NOT copy it again.
     *     - larger:  cannot happen with a correct stop-and-wait sender.
     *                Ignore it and do NOT ACK it. An ACK tells the sender
     *                the data was accepted.
     *     - equal:   the packet you are waiting for. Check that it is on
     *                stream 0, that message_offset equals the bytes received
     *                so far, and that message_length is consistent and fits
     *                in capacity. Copy the payload, advance
     *                conn->recv_next_packet, and ACK it.
     *
     * The message is complete when received == message_length; the final
     * DATA packet must carry DP_FLAG_FIN. Set *out_len and return 0.
     *
     * Return -3 if the message will not fit in capacity and -4 for a
     * protocol violation (wrong stream, bad offset/length, missing or early
     * FIN). Return -1 on socket error.
     */
    (void)buffer;
    (void)capacity;
    (void)out_len;
    fprintf(stderr, "%s: dp_recv: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

/* Called by the receiver after dp_recv() completes (provided). Keeps answering
 * duplicates until the peer has closed and the line has been quiet for
 * DP_CLOSE_LINGER_TIMEOUTS timeouts. Returns 0 after a CLOSE was answered,
 * -2 if no CLOSE ever arrived, -1 on socket error. */
int dp_wait_for_close(dp_connection_t *conn) {
    if (!conn || conn->fd < 0) return -1;

    int saw_close = 0;
    int linger_ms = conn->timeout_ms * DP_CLOSE_LINGER_TIMEOUTS;

    for (;;) {
        dp_packet_t packet = {0};
        int rc = recv_until(conn, &packet, deadline_after(linger_ms));
        if (rc < 0) return -1;
        if (rc == 0) return saw_close ? 0 : -2;
        if (packet.connection_id != conn->connection_id) continue;

        if (packet.type == DP_DATA &&
            packet.packet_number < conn->recv_next_packet) {
            /* The final ACK from dp_recv() was lost: re-ACK, do not deliver. */
            log_net_event(conn, "DUP", &packet, "already delivered, re-ACK");
            if (send_ack(conn, packet.packet_number) < 0) return -1;
        } else if (packet.type == DP_CLOSE) {
            /* Re-answer every CLOSE: an earlier CLOSE_ACK may have been lost. */
            if (send_close_ack(conn, packet.packet_number) < 0) return -1;
            saw_close = 1;
        }
    }
}

int dp_close(dp_connection_t *conn) {
    /* TODO 4: Gracefully close the connection.
     *
     * Send DP_CLOSE using the next packet number and wait for a matching
     * DP_CLOSE_ACK (same connection ID and packet number). Use the same
     * deadline/timeout/retry pattern as send_data_packet(). If the CLOSE is
     * dropped or its CLOSE_ACK is lost, retransmit the same CLOSE packet.
     * Return 0 on success, -2 after conn->max_retries retransmissions.
     */
    fprintf(stderr, "%s: dp_close: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

void dp_set_network_simulation(dp_connection_t *conn,
                               double loss_probability,
                               unsigned max_delay_ms,
                               unsigned random_seed,
                               int log_network) {
    if (!conn) return;
    if (loss_probability < 0.0) loss_probability = 0.0;
    if (loss_probability > 1.0) loss_probability = 1.0;
    conn->loss_probability = loss_probability;
    conn->max_delay_ms = max_delay_ms;
    conn->random_seed = random_seed;
    conn->log_network = log_network;
    srand(random_seed);
}

void dp_get_network_stats(const dp_connection_t *conn, dp_net_stats_t *stats) {
    if (!conn || !stats) return;
    dp_private_state_t *s = state_for((dp_connection_t *)conn);
    if (!s) {
        memset(stats, 0, sizeof(*stats));
        return;
    }
    *stats = s->stats;
}

int dp_add_forced_drop(dp_connection_t *conn, uint32_t packet_number) {
    if (!conn || conn->forced_drop_count >= DP_MAX_FORCED_DROPS) return -1;
    conn->forced_drops[conn->forced_drop_count] = packet_number;
    conn->forced_used[conn->forced_drop_count] = 0;
    conn->forced_drop_count++;
    return 0;
}

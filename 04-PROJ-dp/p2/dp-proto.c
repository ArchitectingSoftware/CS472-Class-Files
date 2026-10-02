/*
 * dp-proto.c - dp (Drexel Protocol), Part 2: streams and head-of-line blocking
 *
 * The dp transport: connection setup and close, the sliding-window sender,
 * the per-stream receiver, and the network simulator.
 *
 * Student TODOs: transmit_chunk(), handle_ack(), check_timeouts(), dp_flush(),
 * accept_data(), advance_contiguous(), next_delivery().
 * Every other function is provided and must not be modified.
 */
#define _POSIX_C_SOURCE 200809L
#include "dp-proto.h"

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
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
#define DP_DELAY_QUEUE_MAX 256
#define DP_FOREVER LLONG_MAX

/* ===========================================================================
 * Private state (provided): statistics and the simulator's delay queue.
 * ======================================================================== */
typedef struct {
    dp_packet_t packet;       /* host byte order */
    size_t bytes;
    long long release_ms;
    unsigned delay_ms;
} dp_delayed_t;

typedef struct dp_private_state {
    dp_connection_t *conn;
    dp_net_stats_t stats;
    dp_delayed_t queue[DP_DELAY_QUEUE_MAX];
    int queue_count;
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

/* ===========================================================================
 * Wire format helpers (provided).
 * ======================================================================== */
static void packet_host_to_net(dp_packet_t *p) {
    p->magic = htons(p->magic);
    p->connection_id = htonl(p->connection_id);
    p->packet_number = htonl(p->packet_number);
    p->stream_id = htonl(p->stream_id);
    p->stream_offset = htonl(p->stream_offset);
    p->unused = htonl(p->unused);
    p->payload_length = htons(p->payload_length);
    p->reserved = htons(p->reserved);
}

static void packet_net_to_host(dp_packet_t *p) {
    p->magic = ntohs(p->magic);
    p->connection_id = ntohl(p->connection_id);
    p->packet_number = ntohl(p->packet_number);
    p->stream_id = ntohl(p->stream_id);
    p->stream_offset = ntohl(p->stream_offset);
    p->unused = ntohl(p->unused);
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

/* ===========================================================================
 * Time and logging helpers (provided).
 * ======================================================================== */
static long long monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) return 0;
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000L;
}

static long long deadline_after(int timeout_ms) {
    return monotonic_ms() + timeout_ms;
}

static long long log_timestamp_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) return 0;
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000L;
}

/* NET: <time> <dir> <type> pkt=N [sid=S off=O] [FIN] [detail] */
static void log_net_event(const dp_connection_t *conn,
                          const char *direction,
                          const dp_packet_t *packet,
                          const char *detail) {
    if (!conn || !conn->log_network) return;
    long long timestamp_us = log_timestamp_us();
    fprintf(stderr, "%s: NET: %lld.%06lld %s %-9s pkt=%u",
            conn->role ? conn->role : "dp-proto",
            timestamp_us / 1000000LL,
            timestamp_us % 1000000LL,
            direction,
            packet_type_name(packet->type),
            packet->packet_number);
    if (packet->type == DP_DATA || packet->type == DP_ACK)
        fprintf(stderr, " sid=%u off=%u", packet->stream_id, packet->stream_offset);
    if (packet->type == DP_DATA && (packet->flags & DP_FLAG_FIN))
        fprintf(stderr, " FIN");
    if (detail && *detail) fprintf(stderr, " %s", detail);
    fputc('\n', stderr);
}

/* ===========================================================================
 * Network simulator (provided in Part 2).
 *
 * Outbound packets may be dropped (randomly with -l, or deterministically
 * with -D by packet number) or delayed (0..-d ms). Delayed packets wait in a
 * queue and are released by net_pump(), which recv_until() calls while it
 * waits. Because each packet gets its own random delay, packets can arrive
 * out of order, and the endpoint keeps receiving while packets are delayed.
 * ======================================================================== */
static unsigned random_delay(unsigned max_delay_ms) {
    if (max_delay_ms == 0) return 0;
    return (unsigned)(rand() % (max_delay_ms + 1U));
}

static int wire_send(dp_connection_t *conn, const dp_packet_t *packet,
                     size_t bytes, unsigned delay_ms) {
    dp_private_state_t *state = state_for(conn);
    if (!state) return -1;

    char detail[64];
    if (delay_ms)
        snprintf(detail, sizeof(detail), "bytes=%zu delayed=%ums", bytes, delay_ms);
    else
        snprintf(detail, sizeof(detail), "bytes=%zu", bytes);
    log_net_event(conn, "OUT", packet, detail);

    dp_packet_t wire = *packet;
    packet_host_to_net(&wire);
    ssize_t sent = sendto(conn->fd, &wire, bytes, 0,
                          (struct sockaddr *)&conn->peer, conn->peer_len);
    if (sent < 0) return -1;
    state->stats.delivered++;
    return 0;
}

static int is_forced_drop(const dp_connection_t *conn, const dp_packet_t *packet) {
    if (packet->type != DP_DATA && packet->type != DP_ACK) return 0;
    for (int i = 0; i < conn->forced_drop_count; ++i)
        if (conn->forced_drops[i] == packet->packet_number) return 1;
    return 0;
}

/* A drop is silent: it returns 0 exactly like a send. */
static int net_simulate(dp_connection_t *conn, const dp_packet_t *packet, size_t bytes) {
    dp_private_state_t *state = state_for(conn);
    if (!state) return -1;
    state->stats.attempted++;

    int forced = is_forced_drop(conn, packet);
    double r = (double)rand() / ((double)RAND_MAX + 1.0);
    if (forced || r < conn->loss_probability) {
        state->stats.dropped++;
        char detail[64];
        snprintf(detail, sizeof(detail), "bytes=%zu DROP%s", bytes, forced ? " (forced)" : "");
        log_net_event(conn, "OUT", packet, detail);
        return 0;
    }

    unsigned delay = random_delay(conn->max_delay_ms);
    if (delay == 0 || state->queue_count == DP_DELAY_QUEUE_MAX)
        return wire_send(conn, packet, bytes, 0);

    state->stats.delayed++;
    state->stats.delay_ms_total += delay;
    dp_delayed_t *d = &state->queue[state->queue_count++];
    d->packet = *packet;
    d->bytes = bytes;
    d->release_ms = monotonic_ms() + delay;
    d->delay_ms = delay;
    return 0;
}

/* Send every delayed packet whose release time has arrived. */
static int net_pump(dp_connection_t *conn) {
    dp_private_state_t *state = state_for(conn);
    if (!state) return -1;
    long long now = monotonic_ms();
    int kept = 0;
    for (int i = 0; i < state->queue_count; ++i) {
        dp_delayed_t *d = &state->queue[i];
        if (d->release_ms <= now) {
            if (wire_send(conn, &d->packet, d->bytes, d->delay_ms) < 0) return -1;
        } else {
            state->queue[kept++] = *d;
        }
    }
    state->queue_count = kept;
    return 0;
}

static long long net_next_release(dp_connection_t *conn) {
    dp_private_state_t *state = state_for(conn);
    long long next = DP_FOREVER;
    if (!state) return next;
    for (int i = 0; i < state->queue_count; ++i)
        if (state->queue[i].release_ms < next) next = state->queue[i].release_ms;
    return next;
}

/* Wait until every delayed packet has been sent (used before exiting). */
static int net_drain(dp_connection_t *conn) {
    for (;;) {
        if (net_pump(conn) < 0) return -1;
        long long next = net_next_release(conn);
        if (next == DP_FOREVER) return 0;
        long long wait = next - monotonic_ms();
        if (wait > 0) {
            struct timespec ts = { (time_t)(wait / 1000), (long)(wait % 1000) * 1000000L };
            nanosleep(&ts, NULL);
        }
    }
}

/* ===========================================================================
 * Receiving (provided).
 * ======================================================================== */

/* Receive one packet. Returns bytes (> 2), 0 on timeout, 2 for a malformed
 * packet or one from the wrong peer, -1 on socket error. */
static int recv_packet(dp_connection_t *conn, dp_packet_t *packet, int timeout_ms) {
    if (timeout_ms <= 0) return 0;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
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

/* Wait for one valid packet until deadline_ms (monotonic), releasing delayed
 * outbound packets while waiting. Returns > 0 with a packet, 0 when the
 * deadline passed, -1 on error. Pass DP_FOREVER to wait indefinitely. */
static int recv_until(dp_connection_t *conn, dp_packet_t *packet, long long deadline_ms) {
    for (;;) {
        if (net_pump(conn) < 0) return -1;
        long long now = monotonic_ms();
        if (now >= deadline_ms) return 0;

        long long wake = deadline_ms;
        long long release = net_next_release(conn);
        if (release < wake) wake = release;
        long long wait = wake - now;
        if (wait < 1) wait = 1;
        if (wait > 1000) wait = 1000;

        int rc = recv_packet(conn, packet, (int)wait);
        if (rc == 0 || rc == 2) continue;
        return rc;
    }
}

/* ===========================================================================
 * Setup (provided).
 * ======================================================================== */
static void init_connection(dp_connection_t *conn) {
    memset(conn, 0, sizeof(*conn));
    conn->fd = -1;
    conn->timeout_ms = DP_DEFAULT_TIMEOUT_MS;
    conn->max_retries = DP_DEFAULT_MAX_RETRIES;
    conn->window = DP_DEFAULT_WINDOW;
    conn->random_seed = (unsigned)time(NULL) ^ (unsigned)getpid();
    srand(conn->random_seed);
}

int dp_client_init(dp_connection_t *conn, const char *host, uint16_t port) {
    if (!conn || !host) return -1;
    init_connection(conn);
    conn->role = "dp-client";
    conn->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (conn->fd < 0) return -1;

    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%u", port);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, portstr, &hints, &res) != 0) {
        close(conn->fd);
        conn->fd = -1;
        return -1;
    }
    memcpy(&conn->peer, res->ai_addr, sizeof(conn->peer));
    conn->peer_len = sizeof(conn->peer);
    freeaddrinfo(res);
    return state_for(conn) ? 0 : -1;
}

int dp_server_init(dp_connection_t *conn, uint16_t port) {
    if (!conn) return -1;
    init_connection(conn);
    conn->role = "dp-server";
    conn->fd = socket(AF_INET, SOCK_DGRAM, 0);
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

/* ===========================================================================
 * Control packets and handshake (provided; unchanged in spirit from Part 1).
 * ======================================================================== */
static int send_control(dp_connection_t *conn, dp_type_t type, uint32_t packet_number) {
    dp_packet_t p = {0};
    p.magic = DP_MAGIC;
    p.type = (uint8_t)type;
    p.connection_id = conn->connection_id;
    p.packet_number = packet_number;
    return net_simulate(conn, &p, DP_HEADER_SIZE);
}

/* ACK a DATA packet: echo its packet number, stream, and offset. */
static int send_ack(dp_connection_t *conn, const dp_packet_t *data) {
    dp_packet_t p = {0};
    p.magic = DP_MAGIC;
    p.type = DP_ACK;
    p.connection_id = conn->connection_id;
    p.packet_number = data->packet_number;
    p.stream_id = data->stream_id;
    p.stream_offset = data->stream_offset;
    return net_simulate(conn, &p, DP_HEADER_SIZE);
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

        if (conn->log_network) {
            char detail[64];
            snprintf(detail, sizeof(detail), "bytes=%zd", n);
            log_net_event(conn, "IN ", &packet, detail);
        }
        /* If this CONNECT_ACK is lost, the client retries and
         * dp_stream_read() answers again. */
        if (send_connect_ack(conn) < 0) return -1;
        return 0;
    }
}

int dp_connect(dp_connection_t *conn) {
    if (!conn || conn->fd < 0) return -1;
    if (conn->connection_id == 0) {
        conn->connection_id = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
        if (conn->connection_id == 0) conn->connection_id = 1;
    }

    dp_packet_t packet = {0};
    packet.magic = DP_MAGIC;
    packet.type = DP_CONNECT;
    packet.connection_id = conn->connection_id;

    for (int attempt = 0; attempt <= conn->max_retries; ++attempt) {
        if (attempt > 0) log_net_event(conn, "TIMEOUT", &packet, "retransmitting");
        if (net_simulate(conn, &packet, DP_HEADER_SIZE) < 0) return -1;

        long long deadline = deadline_after(conn->timeout_ms);
        for (;;) {
            dp_packet_t response = {0};
            int rc = recv_until(conn, &response, deadline);
            if (rc == 0) break;
            if (rc < 0) return -1;
            if (response.type == DP_CONNECT_ACK &&
                response.connection_id == conn->connection_id) {
                conn->next_packet_number = 1;
                return 0;
            }
        }
    }
    return -2;
}

/* ===========================================================================
 * Sender: streams, window, retransmission.
 * ======================================================================== */
int dp_stream_write(dp_connection_t *conn, uint32_t stream_id, const void *buffer, size_t len) {
    if (!conn || stream_id >= DP_MAX_STREAMS) return -1;
    if (!buffer && len != 0) return -1;
    if (len > DP_MAX_STREAM_BYTES) return -1;
    dp_send_stream_t *s = &conn->send[stream_id];
    if (s->active) return -1;   /* one write per stream */
    s->active = 1;
    s->data = buffer;
    s->length = len;
    s->next_offset = 0;
    s->fin_queued = 0;
    return 0;
}

/* Take the next chunk of new data, visiting streams round-robin so that
 * streams share the window fairly. Returns 1 with a chunk, 0 if none left. */
static int next_new_chunk(dp_connection_t *conn, dp_chunk_t *c) {
    for (int i = 0; i < DP_MAX_STREAMS; ++i) {
        int sid = (conn->send_rr + i) % DP_MAX_STREAMS;
        dp_send_stream_t *s = &conn->send[sid];
        if (!s->active || s->fin_queued) continue;

        size_t remaining = s->length - s->next_offset;
        size_t n = remaining > DP_MAX_PAYLOAD ? DP_MAX_PAYLOAD : remaining;
        c->stream_id = (uint32_t)sid;
        c->offset = (uint32_t)s->next_offset;
        c->length = (uint16_t)n;
        c->fin = (s->next_offset + n == s->length);
        c->transmissions = 0;
        s->next_offset += n;
        if (c->fin) s->fin_queued = 1;
        conn->send_rr = (sid + 1) % DP_MAX_STREAMS;
        return 1;
    }
    return 0;
}

/* Lost data goes first, then new data. */
static int next_chunk(dp_connection_t *conn, dp_chunk_t *c) {
    if (conn->retransmit_count > 0) {
        *c = conn->retransmit[0];
        memmove(&conn->retransmit[0], &conn->retransmit[1],
                (size_t)(conn->retransmit_count - 1) * sizeof(dp_chunk_t));
        conn->retransmit_count--;
        return 1;
    }
    return next_new_chunk(conn, c);
}

/* Send a chunk in a DATA packet with a NEW packet number and track it. */
static int transmit_chunk(dp_connection_t *conn, const dp_chunk_t *c) {
    /* TODO 1: Send one chunk in a DATA packet and track it in the window.
     *
     *   - If c->transmissions > conn->max_retries, give up: return -2.
     *   - Find a free slot in conn->inflight[] (in_use == 0).
     *   - Build a DP_DATA packet: magic, type, connection ID, a NEW packet
     *     number from conn->next_packet_number++, the chunk's stream ID and
     *     offset as stream_offset, payload_length = c->length, DP_FLAG_FIN if
     *     c->fin. Copy the payload from conn->send[c->stream_id].data + offset.
     *     A retransmission gets a new packet number too; only the stream ID
     *     and offset stay the same.
     *   - Fill the slot: in_use, packet_number, chunk (with transmissions
     *     incremented), deadline_ms = deadline_after(conn->timeout_ms).
     *     Increment conn->inflight_count.
     *   - If this chunk was sent before (c->transmissions > 0), increment the
     *     retransmits statistic: state_for(conn)->stats.retransmits++.
     *   - Send it with net_simulate(). Return 0, or -1 on socket error.
     */
    (void)c;
    fprintf(stderr, "%s: transmit_chunk: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

static void handle_ack(dp_connection_t *conn, const dp_packet_t *ack) {
    /* TODO 2: Process one ACK.
     *
     * Find the in-flight slot whose packet_number equals ack->packet_number,
     * mark it free, and decrement conn->inflight_count.
     *
     * If no slot matches, the ACK is for a packet number you already gave up
     * on (timed out and retransmitted under a new number). Ignore it. Think
     * about why this can never be confused with the ACK for the new copy.
     */
    (void)conn;
    (void)ack;
}

/* Any in-flight packet past its deadline is declared lost: forget its packet
 * number and queue its chunk for retransmission under a new number. */
static void check_timeouts(dp_connection_t *conn) {
    /* TODO 3: Detect lost packets.
     *
     * For every in-flight slot whose deadline_ms has passed (compare with
     * monotonic_ms()):
     *   - log it: build a dp_packet_t with type DP_DATA, the slot's packet
     *     number, stream ID, offset (and FIN flag), then call
     *     log_net_event(conn, "TIMEOUT", &shown, "retransmit with new pkt");
     *   - append the slot's chunk to conn->retransmit[] (retransmit_count++);
     *   - free the slot and decrement conn->inflight_count.
     *
     * Only the lost packet is retransmitted (selective retransmission), not
     * everything after it.
     */
    (void)conn;
}

static long long earliest_deadline(const dp_connection_t *conn) {
    long long d = DP_FOREVER;
    for (int i = 0; i < DP_MAX_WINDOW; ++i)
        if (conn->inflight[i].in_use && conn->inflight[i].deadline_ms < d)
            d = conn->inflight[i].deadline_ms;
    return d;
}

int dp_flush(dp_connection_t *conn) {
    /* TODO 4: The sliding-window send loop.
     *
     * Repeat:
     *   1. Fill the window: while conn->inflight_count < conn->window, get
     *      the next chunk with next_chunk() (retransmissions first, then new
     *      data round-robin across streams) and send it with
     *      transmit_chunk(). Stop filling when next_chunk() returns 0.
     *      Return any negative value from transmit_chunk().
     *   2. If nothing is in flight, every byte has been acknowledged:
     *      return 0.
     *   3. Wait for a packet until earliest_deadline(conn) with recv_until().
     *      Return -1 on error. If an ACK for this connection arrived, pass it
     *      to handle_ack().
     *   4. Call check_timeouts() whether or not a packet arrived.
     *
     * Compare with Part 1's stop-and-wait send_data_packet(): the structure
     * is similar, but now up to conn->window packets are outstanding.
     */
    (void)next_chunk;
    (void)earliest_deadline;
    (void)transmit_chunk;
    (void)handle_ack;
    (void)check_timeouts;
    fprintf(stderr, "%s: dp_flush: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

/* ===========================================================================
 * Receiver: per-stream reassembly and in-order delivery.
 * ======================================================================== */
static int ensure_capacity(dp_recv_stream_t *rs, size_t need) {
    if (need > rs->cap) {
        size_t cap = rs->cap ? rs->cap : 4096;
        while (cap < need) cap *= 2;
        uint8_t *nb = realloc(rs->buf, cap);
        if (!nb) return -1;
        rs->buf = nb;
        rs->cap = cap;
    }
    size_t chunks = (rs->cap + DP_MAX_PAYLOAD - 1) / DP_MAX_PAYLOAD;
    if (chunks > rs->have_cap) {
        uint8_t *nh = realloc(rs->have, chunks);
        if (!nh) return -1;
        memset(nh + rs->have_cap, 0, chunks - rs->have_cap);
        rs->have = nh;
        rs->have_cap = chunks;
    }
    return 0;
}

static void advance_contiguous(dp_recv_stream_t *rs) {
    /* TODO 6a: Advance the stream's in-order point.
     *
     * rs->contiguous is the number of bytes from the start of the stream
     * that have ALL arrived. Starting at chunk rs->contiguous / DP_MAX_PAYLOAD,
     * keep advancing while the chunk at rs->contiguous has arrived
     * (rs->have[idx]). Each chunk adds DP_MAX_PAYLOAD bytes, except the last
     * chunk of the stream, which adds final_size - contiguous. Never advance
     * past final_size once FIN has been seen, and never index past
     * rs->have_cap.
     *
     * This is where head-of-line blocking happens: a single missing chunk
     * stops this stream's in-order point, no matter how much data after it
     * has already arrived.
     */
    (void)rs;
}

/* Store one DATA packet. Returns 0 if it should be ACKed (new or duplicate),
 * -4 for a protocol violation, -1 on allocation failure. */
static int accept_data(dp_connection_t *conn, const dp_packet_t *p) {
    /* TODO 5: Store one received DATA packet in its stream.
     *
     * Validate (return -4 for any violation):
     *   - stream_id < DP_MAX_STREAMS
     *   - stream_offset is a multiple of DP_MAX_PAYLOAD
     *   - a non-FIN packet carries exactly DP_MAX_PAYLOAD bytes
     *   - end = stream_offset + payload_length <= DP_MAX_STREAM_BYTES
     *   - if FIN was already seen, end <= final_size
     *
     * Let rs = &conn->recv[p->stream_id]; set rs->active = 1.
     *
     * FIN: if this packet has DP_FLAG_FIN, it defines the stream's final
     * size (end). A second FIN must agree, and the final size cannot be
     * smaller than data already received (rs->max_end). Set rs->fin_seen and
     * rs->final_size.
     *
     * Data (payload_length > 0):
     *   - ensure_capacity(rs, end); return -1 on failure.
     *   - idx = stream_offset / DP_MAX_PAYLOAD. If rs->have[idx] is already
     *     set, this is a DUPLICATE: increment the duplicates statistic, log
     *     it with log_net_event(conn, "DUP", p, "already have this data"),
     *     and return 0 (the caller still ACKs it). Duplicates are detected by
     *     stream offset, not packet number. Why?
     *   - Otherwise copy the payload to rs->buf + stream_offset, set
     *     rs->have[idx], and update rs->max_end.
     *
     * Finally call advance_contiguous(rs) and return 0.
     */
    (void)p;
    (void)ensure_capacity;
    (void)advance_contiguous;
    fprintf(stderr, "%s: accept_data: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

/* Find a stream with newly in-order bytes (or a just-completed stream) and
 * describe them to the caller. Returns 1 with an event, 0 if nothing is ready.
 * Streams are checked round-robin starting at conn->recv_rr. */
static int next_delivery(dp_connection_t *conn, uint32_t *stream_id,
                         const uint8_t **data, size_t *len, int *fin) {
    /* TODO 6b: Hand newly in-order data to the application.
     *
     * Check streams round-robin starting at conn->recv_rr. For the first
     * active stream that has new in-order bytes (contiguous > delivered) or
     * has just completed (FIN seen, contiguous == final_size, and
     * fin_delivered not yet set):
     *   - *stream_id = that stream; *data = rs->buf + rs->delivered (NULL if
     *     buf is NULL); *len = contiguous - delivered; *fin = completed?
     *   - set delivered = contiguous; set fin_delivered if completed;
     *   - conn->recv_rr = next stream after this one;
     *   - return 1.
     * Return 0 if no stream has anything new.
     */
    (void)stream_id;
    (void)data;
    (void)len;
    (void)fin;
    fprintf(stderr, "%s: next_delivery: NOT IMPLEMENTED\n",
            conn && conn->role ? conn->role : "dp-proto");
    return DP_ERR_NOT_IMPLEMENTED;
}

int dp_stream_read(dp_connection_t *conn, uint32_t *stream_id,
                   const uint8_t **data, size_t *len, int *fin) {
    if (!conn || conn->fd < 0 || !stream_id || !data || !len || !fin) return -1;

    for (;;) {
        /* Hand the application any stream with new in-order bytes. */
        int ev = next_delivery(conn, stream_id, data, len, fin);
        if (ev < 0) return ev;
        if (ev > 0) return 0;
        if (conn->peer_closed) return 1;

        dp_packet_t p = {0};
        int rc = recv_until(conn, &p, DP_FOREVER);
        if (rc < 0) return -1;
        if (rc == 0 || p.connection_id != conn->connection_id) continue;

        if (p.type == DP_CONNECT) {
            if (send_connect_ack(conn) < 0) return -1;
        } else if (p.type == DP_CLOSE) {
            if (send_close_ack(conn, p.packet_number) < 0) return -1;
            conn->peer_closed = 1;
        } else if (p.type == DP_DATA) {
            int ar = accept_data(conn, &p);
            if (ar < 0) return ar;
            if (send_ack(conn, &p) < 0) return -1;
        }
    }
}

/* ===========================================================================
 * Close (provided).
 * ======================================================================== */
int dp_wait_for_close(dp_connection_t *conn) {
    if (!conn || conn->fd < 0) return -1;
    int saw_close = conn->peer_closed;
    int linger_ms = conn->timeout_ms * DP_CLOSE_LINGER_TIMEOUTS;

    for (;;) {
        dp_packet_t packet = {0};
        int rc = recv_until(conn, &packet, deadline_after(linger_ms));
        if (rc < 0) return -1;
        if (rc == 0) {
            if (net_drain(conn) < 0) return -1;
            return saw_close ? 0 : -2;
        }
        if (packet.connection_id != conn->connection_id) continue;

        if (packet.type == DP_DATA) {
            /* A late retransmission: everything was already delivered. */
            if (accept_data(conn, &packet) < 0) return -4;
            if (send_ack(conn, &packet) < 0) return -1;
        } else if (packet.type == DP_CLOSE) {
            if (send_close_ack(conn, packet.packet_number) < 0) return -1;
            saw_close = 1;
        }
    }
}

int dp_close(dp_connection_t *conn) {
    if (!conn || conn->fd < 0) return -1;

    for (int attempt = 0; attempt <= conn->max_retries; ++attempt) {
        dp_packet_t packet = {0};
        packet.magic = DP_MAGIC;
        packet.type = DP_CLOSE;
        packet.connection_id = conn->connection_id;
        packet.packet_number = conn->next_packet_number++;   /* new number each try */
        if (attempt > 0) log_net_event(conn, "TIMEOUT", &packet, "retransmitting");
        if (net_simulate(conn, &packet, DP_HEADER_SIZE) < 0) return -1;

        long long deadline = deadline_after(conn->timeout_ms);
        for (;;) {
            dp_packet_t response = {0};
            int rc = recv_until(conn, &response, deadline);
            if (rc == 0) break;
            if (rc < 0) return -1;
            if (response.type == DP_CLOSE_ACK &&
                response.connection_id == conn->connection_id) {
                return net_drain(conn) < 0 ? -1 : 0;
            }
        }
    }
    return -2;
}

void dp_free(dp_connection_t *conn) {
    if (!conn) return;
    for (int i = 0; i < DP_MAX_STREAMS; ++i) {
        free(conn->recv[i].buf);
        free(conn->recv[i].have);
        conn->recv[i].buf = NULL;
        conn->recv[i].have = NULL;
    }
}

/* ===========================================================================
 * Configuration and statistics (provided).
 * ======================================================================== */
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

int dp_add_forced_drop(dp_connection_t *conn, uint32_t packet_number) {
    if (!conn || conn->forced_drop_count >= DP_MAX_FORCED_DROPS) return -1;
    conn->forced_drops[conn->forced_drop_count++] = packet_number;
    return 0;
}

void dp_set_window(dp_connection_t *conn, int window) {
    if (!conn) return;
    if (window < 1) window = 1;
    if (window > DP_MAX_WINDOW) window = DP_MAX_WINDOW;
    conn->window = window;
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

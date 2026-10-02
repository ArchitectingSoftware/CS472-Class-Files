/*
 * dp-server.c - dp (Drexel Protocol), Part 2 server program
 *
 * Accepts one dp connection, reassembles each file, records when each file
 * completes, and verifies every byte. Provided file: do not modify.
 */
#define _POSIX_C_SOURCE 200809L
#include "dp-proto.h"
#include "dp-app.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *prog) {
    printf("dp-server: Usage: %s [options]\n", prog);
    printf("dp-server:   -p, --port PORT        listen port (default: %u)\n", DP_DEFAULT_PORT);
    printf("dp-server:   -l, --loss P           outbound loss probability 0.0-1.0 (default: 0)\n");
    printf("dp-server:   -d, --delay MS         outbound random delay 0..MS (default: 0)\n");
    printf("dp-server:   -D, --drop LIST        always drop outbound ACKs for these packet numbers\n");
    printf("dp-server:   -s, --seed N           random seed (default: generated)\n");
    printf("dp-server:   -v, --verbose          log network events\n");
    printf("dp-server:       --help             show this help\n");
}

static int parse_u32(const char *s, unsigned *out) {
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 10);
    if (errno || !end || *end || v > 0xffffffffUL) return -1;
    *out = (unsigned)v;
    return 0;
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

typedef struct {
    int seen;
    unsigned stream;
    uint8_t *buf;
    size_t len, cap;
    int complete;
    double complete_ms;
} app_file_t;

static app_file_t g_files[APP_MAX_FILES + 1];   /* index = file id */
static double g_t0;
static int g_verify_ok = 1;

static int file_append(unsigned id, unsigned stream, const uint8_t *data, size_t n) {
    if (id < 1 || id > APP_MAX_FILES) return -1;
    app_file_t *f = &g_files[id];
    if (f->complete) return -1;
    f->seen = 1;
    f->stream = stream;
    if (f->len + n > f->cap) {
        size_t cap = f->cap ? f->cap : 4096;
        while (cap < f->len + n) cap *= 2;
        uint8_t *nb = realloc(f->buf, cap);
        if (!nb) return -1;
        f->buf = nb;
        f->cap = cap;
    }
    if (n) memcpy(f->buf + f->len, data, n);
    f->len += n;
    return 0;
}

static void file_complete(unsigned id) {
    app_file_t *f = &g_files[id];
    f->complete = 1;
    f->complete_ms = now_ms() - g_t0;
    int ok = 1;
    for (size_t j = 0; j < f->len; ++j) {
        if (f->buf[j] != app_pattern(id, j)) { ok = 0; break; }
    }
    if (!ok) g_verify_ok = 0;
    printf("dp-server: RESULT file=%u stream=%u bytes=%zu complete_ms=%.1f verify=%s\n",
           id, f->stream, f->len, f->complete_ms, ok ? "PASS" : "FAIL");
}

/* Single mode: parse application frames out of the in-order stream 0 bytes.
 * Frames can be split across reads, so partial frames are buffered. */
static uint8_t g_pending[2 * 480];
static size_t g_pending_len;

static int deframe(const uint8_t *data, size_t n) {
    while (n > 0) {
        size_t take = sizeof(g_pending) - g_pending_len;
        if (take > n) take = n;
        memcpy(g_pending + g_pending_len, data, take);
        g_pending_len += take;
        data += take;
        n -= take;

        for (;;) {
            if (g_pending_len < APP_FRAME_HEADER) break;
            uint16_t id, flags;
            uint32_t len;
            memcpy(&id, g_pending, 2);
            memcpy(&flags, g_pending + 2, 2);
            memcpy(&len, g_pending + 4, 4);
            id = ntohs(id);
            flags = ntohs(flags);
            len = ntohl(len);
            if (len > APP_FRAME_DATA) return -1;
            if (g_pending_len < APP_FRAME_HEADER + len) break;
            if (file_append(id, 0, g_pending + APP_FRAME_HEADER, len) < 0) return -1;
            if (flags & APP_FLAG_LAST) file_complete(id);
            size_t used = APP_FRAME_HEADER + len;
            memmove(g_pending, g_pending + used, g_pending_len - used);
            g_pending_len -= used;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    unsigned port = DP_DEFAULT_PORT;
    double loss = 0.0;
    unsigned delay = 0;
    const char *drops_arg = NULL;
    unsigned seed = 0;
    int seed_set = 0;
    int verbose = 0;

    static const struct option opts[] = {
        {"port", required_argument, 0, 'p'},
        {"loss", required_argument, 0, 'l'},
        {"delay", required_argument, 0, 'd'},
        {"drop", required_argument, 0, 'D'},
        {"seed", required_argument, 0, 's'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 1000},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "p:l:d:D:s:v", opts, NULL)) != -1) {
        switch (c) {
            case 'p': if (parse_u32(optarg, &port) || port > 65535) { fprintf(stderr, "dp-server: bad port\n"); return 1; } break;
            case 'l': loss = strtod(optarg, NULL); break;
            case 'd': if (parse_u32(optarg, &delay)) { fprintf(stderr, "dp-server: bad delay\n"); return 1; } break;
            case 'D': drops_arg = optarg; break;
            case 's': if (parse_u32(optarg, &seed)) { fprintf(stderr, "dp-server: bad seed\n"); return 1; } seed_set = 1; break;
            case 'v': verbose = 1; break;
            case 1000: usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }
    if (loss < 0.0 || loss > 1.0) { fprintf(stderr, "dp-server: loss must be between 0 and 1\n"); return 1; }

    dp_connection_t conn;
    if (dp_server_init(&conn, (uint16_t)port) < 0) {
        fprintf(stderr, "dp-server: failed to initialize server\n");
        return 1;
    }
    if (!seed_set) seed = conn.random_seed;
    dp_set_network_simulation(&conn, loss, delay, seed, verbose);
    if (drops_arg) {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s", drops_arg);
        for (char *tok = strtok(tmp, ","); tok; tok = strtok(NULL, ",")) {
            unsigned v;
            if (parse_u32(tok, &v)) { fprintf(stderr, "dp-server: bad -D list\n"); return 1; }
            dp_add_forced_drop(&conn, v);
        }
    }

    printf("dp-server: listening on UDP %u\n", port);
    if (dp_accept(&conn) < 0) {
        fprintf(stderr, "dp-server: accept failed\n");
        return 1;
    }
    g_t0 = now_ms();
    printf("dp-server: accepted cid=%u\n", conn.connection_id);

    int saw_single = 0, saw_multi = 0, ok = 1, peer_closed = 0;
    for (;;) {
        uint32_t sid;
        const uint8_t *data;
        size_t n;
        int fin;
        int rc = dp_stream_read(&conn, &sid, &data, &n, &fin);
        if (rc == 1) { peer_closed = 1; break; }
        if (rc < 0) {
            fprintf(stderr, "dp-server: receive failed (%d)\n", rc);
            ok = 0;
            break;
        }
        if (sid == 0) {
            saw_single = 1;
            if (deframe(data, n) < 0) {
                fprintf(stderr, "dp-server: bad application frame on stream 0\n");
                ok = 0;
                break;
            }
        } else {
            saw_multi = 1;
            if (file_append(sid, sid, data, n) < 0) {
                fprintf(stderr, "dp-server: bad data on stream %u\n", sid);
                ok = 0;
                break;
            }
            if (fin) file_complete(sid);
        }
    }

    size_t total = 0;
    int nfiles = 0;
    for (unsigned id = 1; id <= APP_MAX_FILES; ++id) {
        if (!g_files[id].seen) continue;
        nfiles++;
        total += g_files[id].len;
        if (!g_files[id].complete) {
            fprintf(stderr, "dp-server: file %u incomplete\n", id);
            ok = 0;
        }
    }
    if (nfiles == 0) ok = 0;

    printf("dp-server: mode=%s files=%d\n",
           saw_single && saw_multi ? "mixed" : saw_single ? "single" : "multi", nfiles);
    printf("dp-server: received %zu bytes\n", total);
    if (ok && g_verify_ok) {
        printf("dp-server: verify: PASS\n");
    } else {
        fprintf(stderr, "dp-server: verify: FAIL\n");
        ok = 0;
    }

    /* Linger only after a normal close; after a receive error there is no
     * reason to keep the server alive. */
    int close_rc = peer_closed ? dp_wait_for_close(&conn) : -1;
    if (close_rc == 0) {
        printf("dp-server: close: PASS\n");
    } else {
        fprintf(stderr, "dp-server: close: FAIL (%d)\n", close_rc);
        ok = 0;
    }

    dp_net_stats_t st;
    dp_get_network_stats(&conn, &st);
    printf("dp-server: network: attempted=%lu dropped=%lu delivered=%lu delayed=%lu avg-delay=%.2fms duplicates=%lu\n",
           st.attempted, st.dropped, st.delivered, st.delayed,
           st.delayed ? (double)st.delay_ms_total / (double)st.delayed : 0.0, st.duplicates);

    close(conn.fd);
    dp_free(&conn);
    for (unsigned id = 1; id <= APP_MAX_FILES; ++id) free(g_files[id].buf);
    return ok ? 0 : 1;
}

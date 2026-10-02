/*
 * dp-client.c - dp (Drexel Protocol), Part 2 client program
 *
 * Sends several files over one dp connection, either one stream per file
 * (multi mode) or multiplexed on stream 0 (single mode).
 * Provided file: do not modify.
 */
#define _POSIX_C_SOURCE 200809L
#include "dp-proto.h"
#include "dp-app.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *prog) {
    printf("dp-client: Usage: %s [options]\n", prog);
    printf("dp-client:   -h, --host HOST        server hostname/IP (default: localhost)\n");
    printf("dp-client:   -p, --port PORT        server UDP port (default: %u)\n", DP_DEFAULT_PORT);
    printf("dp-client:   -k, --kb LIST          file sizes in KB, comma separated (default: %s)\n", APP_DEFAULT_SIZES);
    printf("dp-client:   -m, --mode MODE        multi (one stream per file) or single (all files on stream 0)\n");
    printf("dp-client:   -w, --window N         max DATA packets in flight (default: %d)\n", DP_DEFAULT_WINDOW);
    printf("dp-client:   -l, --loss P           outbound loss probability 0.0-1.0 (default: 0)\n");
    printf("dp-client:   -d, --delay MS         outbound random delay 0..MS (default: 0)\n");
    printf("dp-client:   -D, --drop LIST        always drop these outbound DATA packet numbers\n");
    printf("dp-client:   -s, --seed N           random seed (default: generated)\n");
    printf("dp-client:   -v, --verbose          log network events\n");
    printf("dp-client:       --help             show this help\n");
}

static int parse_u32(const char *s, unsigned *out) {
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 10);
    if (errno || !end || *end || v > 0xffffffffUL) return -1;
    *out = (unsigned)v;
    return 0;
}

/* Parse "a,b,c" into values. Returns count or -1. */
static int parse_list(const char *s, unsigned *out, int max) {
    char tmp[256];
    if (strlen(s) >= sizeof(tmp)) return -1;
    strcpy(tmp, s);
    int n = 0;
    for (char *tok = strtok(tmp, ","); tok; tok = strtok(NULL, ",")) {
        if (n == max || parse_u32(tok, &out[n])) return -1;
        n++;
    }
    return n;
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* Single mode: interleave all files round-robin as frames on one buffer. */
static uint8_t *build_single_stream(uint8_t **files, const size_t *sizes, int nfiles, size_t *out_len) {
    size_t total = 0;
    for (int i = 0; i < nfiles; ++i) {
        size_t frames = sizes[i] ? (sizes[i] + APP_FRAME_DATA - 1) / APP_FRAME_DATA : 1;
        total += frames * APP_FRAME_HEADER + sizes[i];
    }
    uint8_t *buf = malloc(total ? total : 1);
    if (!buf) return NULL;

    size_t pos = 0;
    size_t sent[APP_MAX_FILES] = {0};
    int done[APP_MAX_FILES] = {0};
    int remaining = nfiles;
    while (remaining > 0) {
        for (int i = 0; i < nfiles; ++i) {
            if (done[i]) continue;
            size_t left = sizes[i] - sent[i];
            size_t n = left > APP_FRAME_DATA ? APP_FRAME_DATA : left;
            uint16_t flags = (sent[i] + n == sizes[i]) ? APP_FLAG_LAST : 0;
            uint16_t id = htons((uint16_t)(i + 1));
            uint16_t fl = htons(flags);
            uint32_t ln = htonl((uint32_t)n);
            memcpy(buf + pos, &id, 2);
            memcpy(buf + pos + 2, &fl, 2);
            memcpy(buf + pos + 4, &ln, 4);
            memcpy(buf + pos + APP_FRAME_HEADER, files[i] + sent[i], n);
            pos += APP_FRAME_HEADER + n;
            sent[i] += n;
            if (flags) { done[i] = 1; remaining--; }
        }
    }
    *out_len = pos;
    return buf;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *host = "localhost";
    unsigned port = DP_DEFAULT_PORT;
    const char *sizes_arg = APP_DEFAULT_SIZES;
    const char *mode = "multi";
    unsigned window = DP_DEFAULT_WINDOW;
    double loss = 0.0;
    unsigned delay = 0;
    const char *drops_arg = NULL;
    unsigned seed = 0;
    int seed_set = 0;
    int verbose = 0;

    static const struct option opts[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"kb", required_argument, 0, 'k'},
        {"mode", required_argument, 0, 'm'},
        {"window", required_argument, 0, 'w'},
        {"loss", required_argument, 0, 'l'},
        {"delay", required_argument, 0, 'd'},
        {"drop", required_argument, 0, 'D'},
        {"seed", required_argument, 0, 's'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 1000},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "h:p:k:m:w:l:d:D:s:v", opts, NULL)) != -1) {
        switch (c) {
            case 'h': host = optarg; break;
            case 'p': if (parse_u32(optarg, &port) || port > 65535) { fprintf(stderr, "dp-client: bad port\n"); return 1; } break;
            case 'k': sizes_arg = optarg; break;
            case 'm': mode = optarg; break;
            case 'w': if (parse_u32(optarg, &window) || window < 1 || window > DP_MAX_WINDOW) { fprintf(stderr, "dp-client: window must be 1-%d\n", DP_MAX_WINDOW); return 1; } break;
            case 'l': loss = strtod(optarg, NULL); break;
            case 'd': if (parse_u32(optarg, &delay)) { fprintf(stderr, "dp-client: bad delay\n"); return 1; } break;
            case 'D': drops_arg = optarg; break;
            case 's': if (parse_u32(optarg, &seed)) { fprintf(stderr, "dp-client: bad seed\n"); return 1; } seed_set = 1; break;
            case 'v': verbose = 1; break;
            case 1000: usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }

    int single = strcmp(mode, "single") == 0;
    if (!single && strcmp(mode, "multi") != 0) { fprintf(stderr, "dp-client: mode must be multi or single\n"); return 1; }
    if (loss < 0.0 || loss > 1.0) { fprintf(stderr, "dp-client: loss must be between 0 and 1\n"); return 1; }

    unsigned kb[APP_MAX_FILES];
    int nfiles = parse_list(sizes_arg, kb, APP_MAX_FILES);
    if (nfiles < 1) { fprintf(stderr, "dp-client: -k needs 1-%d sizes\n", APP_MAX_FILES); return 1; }

    uint8_t *files[APP_MAX_FILES];
    size_t sizes[APP_MAX_FILES];
    size_t total = 0;
    for (int i = 0; i < nfiles; ++i) {
        sizes[i] = (size_t)kb[i] * 1024U;
        files[i] = malloc(sizes[i] ? sizes[i] : 1);
        if (!files[i]) { perror("dp-client: malloc"); return 1; }
        for (size_t j = 0; j < sizes[i]; ++j) files[i][j] = app_pattern((unsigned)(i + 1), j);
        total += sizes[i];
    }

    dp_connection_t conn;
    if (dp_client_init(&conn, host, (uint16_t)port) < 0) {
        fprintf(stderr, "dp-client: failed to initialize client\n");
        return 1;
    }
    if (!seed_set) seed = conn.random_seed;
    dp_set_network_simulation(&conn, loss, delay, seed, verbose);
    dp_set_window(&conn, (int)window);
    if (drops_arg) {
        unsigned drops[DP_MAX_FORCED_DROPS];
        int nd = parse_list(drops_arg, drops, DP_MAX_FORCED_DROPS);
        if (nd < 0) { fprintf(stderr, "dp-client: bad -D list\n"); return 1; }
        for (int i = 0; i < nd; ++i) dp_add_forced_drop(&conn, drops[i]);
    }

    printf("dp-client: mode=%s files=%d bytes=%zu window=%u\n", single ? "single" : "multi", nfiles, total, window);

    if (dp_connect(&conn) < 0) {
        fprintf(stderr, "dp-client: connect failed\n");
        return 1;
    }
    printf("dp-client: connected cid=%u\n", conn.connection_id);

    uint8_t *single_buf = NULL;
    if (single) {
        size_t len = 0;
        single_buf = build_single_stream(files, sizes, nfiles, &len);
        if (!single_buf || dp_stream_write(&conn, 0, single_buf, len) < 0) {
            fprintf(stderr, "dp-client: stream write failed\n");
            return 1;
        }
    } else {
        for (int i = 0; i < nfiles; ++i) {
            if (dp_stream_write(&conn, (uint32_t)(i + 1), files[i], sizes[i]) < 0) {
                fprintf(stderr, "dp-client: stream write failed\n");
                return 1;
            }
        }
    }

    double t0 = now_ms();
    int rc = dp_flush(&conn);
    if (rc < 0) {
        fprintf(stderr, "dp-client: send failed (%d)\n", rc);
        return 1;
    }
    printf("dp-client: sent %zu bytes, all acknowledged in %.1f ms\n", total, now_ms() - t0);

    if (dp_close(&conn) < 0) {
        fprintf(stderr, "dp-client: close failed\n");
        return 1;
    }
    printf("dp-client: close: PASS\n");

    dp_net_stats_t st;
    dp_get_network_stats(&conn, &st);
    printf("dp-client: network: attempted=%lu dropped=%lu delivered=%lu delayed=%lu avg-delay=%.2fms retransmits=%lu\n",
           st.attempted, st.dropped, st.delivered, st.delayed,
           st.delayed ? (double)st.delay_ms_total / (double)st.delayed : 0.0, st.retransmits);

    close(conn.fd);
    dp_free(&conn);
    free(single_buf);
    for (int i = 0; i < nfiles; ++i) free(files[i]);
    return 0;
}

/*
 * dp-client.c - dp (Drexel Protocol), Part 1 client program
 *
 * Connects to dp-server, sends one message of a known byte pattern over dp,
 * and closes. Provided file: do not modify.
 */
#define _POSIX_C_SOURCE 200809L
#include "dp-proto.h"

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

static void usage(const char *prog) {
    printf("dp-client: Usage: %s [options]\n", prog);
    printf("dp-client:   -h, --host HOST        server hostname/IP (default: localhost)\n");
    printf("dp-client:   -p, --port PORT        server UDP port (default: %u)\n", DP_DEFAULT_PORT);
    printf("dp-client:   -k, --kb KB            transfer size in KB (default: %u)\n", DP_DEFAULT_SIZE_KB);
    printf("dp-client:   -l, --loss P           outbound loss probability 0.0-1.0 (default: 0)\n");
    printf("dp-client:   -d, --delay MS         outbound random delay 0..MS (default: 0)\n");
    printf("dp-client:   -D, --drop LIST        drop the first transmission of these outbound DATA packet numbers\n");
    printf("dp-client:   -s, --seed N           random seed (default: generated)\n");
    printf("dp-client:   -v, --verbose          log network simulation\n");
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

/* Parse "a,b,c" and register each packet number as a forced drop. */
static int add_drops(dp_connection_t *conn, const char *list) {
    char tmp[256];
    if (strlen(list) >= sizeof(tmp)) return -1;
    strcpy(tmp, list);
    for (char *tok = strtok(tmp, ","); tok; tok = strtok(NULL, ",")) {
        unsigned v;
        if (parse_u32(tok, &v) || dp_add_forced_drop(conn, v) < 0) return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    /* Keep status output visible when stdout is redirected to a log file. */
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *host = "localhost";
    unsigned port = DP_DEFAULT_PORT;
    unsigned kb = DP_DEFAULT_SIZE_KB;
    double loss = 0.0;
    unsigned delay = 0;
    unsigned seed = 0;
    int seed_set = 0;
    int verbose = 0;
    const char *drops = NULL;

    static const struct option opts[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"kb", required_argument, 0, 'k'},
        {"loss", required_argument, 0, 'l'},
        {"delay", required_argument, 0, 'd'},
        {"drop", required_argument, 0, 'D'},
        {"seed", required_argument, 0, 's'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 1000},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "h:p:k:l:d:D:s:v", opts, NULL)) != -1) {
        switch (c) {
            case 'h': host = optarg; break;
            case 'p': if (parse_u32(optarg, &port) || port > 65535) { fprintf(stderr, "dp-client: bad port\n"); return 1; } break;
            case 'k': if (parse_u32(optarg, &kb)) { fprintf(stderr, "dp-client: bad size\n"); return 1; } break;
            case 'l': loss = strtod(optarg, NULL); break;
            case 'd': if (parse_u32(optarg, &delay)) { fprintf(stderr, "dp-client: bad delay\n"); return 1; } break;
            case 'D': drops = optarg; break;
            case 's': if (parse_u32(optarg, &seed)) { fprintf(stderr, "dp-client: bad seed\n"); return 1; } seed_set = 1; break;
            case 'v': verbose = 1; break;
            case 1000: usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }

    if (loss < 0.0 || loss > 1.0) {
        fprintf(stderr, "dp-client: loss must be between 0 and 1\n");
        return 1;
    }

    size_t bytes = (size_t)kb * 1024U;
    uint8_t *send_buf = malloc(bytes ? bytes : 1);
    if (!send_buf) {
        perror("dp-client: malloc");
        return 1;
    }

    for (size_t i = 0; i < bytes; ++i)
        send_buf[i] = (uint8_t)((i * 31U + 17U) & 0xffU);

    dp_connection_t conn;
    if (dp_client_init(&conn, host, (uint16_t)port) < 0) {
        fprintf(stderr, "dp-client: failed to initialize client\n");
        free(send_buf);
        return 1;
    }

    if (!seed_set) seed = conn.random_seed;
    dp_set_network_simulation(&conn, loss, delay, seed, verbose);
    if (drops && add_drops(&conn, drops) < 0) {
        fprintf(stderr, "dp-client: bad -D list\n");
        return 1;
    }

    if (verbose)
        printf("dp-client: network logging enabled (-v).\n");

    if (dp_connect(&conn) < 0) {
        fprintf(stderr, "dp-client: connect failed\n");
        close(conn.fd);
        free(send_buf);
        return 1;
    }
    printf("dp-client: connected cid=%u\n", conn.connection_id);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    if (dp_send(&conn, send_buf, bytes) < 0) {
        fprintf(stderr, "dp-client: send failed\n");
        close(conn.fd);
        free(send_buf);
        return 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("dp-client: sent %zu bytes in %zu DATA packets, %.1f ms\n", bytes,
           bytes ? (bytes + DP_MAX_PAYLOAD - 1) / DP_MAX_PAYLOAD : 1, elapsed_ms);

    if (dp_close(&conn) < 0) {
        fprintf(stderr, "dp-client: close failed\n");
        close(conn.fd);
        free(send_buf);
        return 1;
    }
    printf("dp-client: close: PASS\n");

    dp_net_stats_t stats;
    dp_get_network_stats(&conn, &stats);
    printf("dp-client: network: attempted=%lu dropped=%lu delivered=%lu delayed=%lu avg-delay=%.2fms\n",
           stats.attempted, stats.dropped, stats.delivered, stats.delayed,
           stats.delayed ? (double)stats.delay_ms_total / (double)stats.delayed : 0.0);

    close(conn.fd);
    free(send_buf);
    return 0;
}

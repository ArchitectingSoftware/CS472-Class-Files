/*
 * dp-server.c - dp (Drexel Protocol), Part 1 server program
 *
 * Accepts one dp connection, receives one message, verifies every byte, and
 * waits for the client to close. Provided file: do not modify.
 */
#include "dp-proto.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage(const char *prog) {
    printf("dp-server: Usage: %s [options]\n", prog);
    printf("dp-server:   -p, --port PORT        listen port (default: %u)\n", DP_DEFAULT_PORT);
    printf("dp-server:   -l, --loss P           outbound loss probability 0.0-1.0 (default: 0)\n");
    printf("dp-server:   -d, --delay MS         outbound random delay 0..MS (default: 0)\n");
    printf("dp-server:   -D, --drop LIST        drop the first transmission of these outbound ACK packet numbers\n");
    printf("dp-server:   -s, --seed N           random seed (default: generated)\n");
    printf("dp-server:   -v, --verbose          log network simulation\n");
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
    unsigned port = DP_DEFAULT_PORT;
    double loss = 0.0;
    unsigned delay = 0;
    unsigned seed = 0;
    int seed_set = 0;
    int verbose = 0;
    const char *drops = NULL;

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
            case 'D': drops = optarg; break;
            case 's': if (parse_u32(optarg, &seed)) { fprintf(stderr, "dp-server: bad seed\n"); return 1; } seed_set = 1; break;
            case 'v': verbose = 1; break;
            case 1000: usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }

    if (loss < 0.0 || loss > 1.0) {
        fprintf(stderr, "dp-server: loss must be between 0 and 1\n");
        return 1;
    }

    dp_connection_t conn;
    if (dp_server_init(&conn, (uint16_t)port) < 0) {
        fprintf(stderr, "dp-server: failed to initialize server\n");
        return 1;
    }

    if (!seed_set) seed = conn.random_seed;
    dp_set_network_simulation(&conn, loss, delay, seed, verbose);
    if (drops && add_drops(&conn, drops) < 0) {
        fprintf(stderr, "dp-server: bad -D list\n");
        return 1;
    }

    if (verbose)
        printf("dp-server: network logging enabled (-v).\n");

    printf("dp-server: listening on UDP %u\n", port);
    if (dp_accept(&conn) < 0) {
        fprintf(stderr, "dp-server: accept failed\n");
        close(conn.fd);
        return 1;
    }
    printf("dp-server: accepted cid=%u\n", conn.connection_id);

    size_t capacity = 16 * 1024 * 1024;
    uint8_t *buffer = malloc(capacity);
    if (!buffer) {
        perror("dp-server: malloc");
        close(conn.fd);
        return 1;
    }

    size_t received = 0;
    int rc = dp_recv(&conn, buffer, capacity, &received);
    if (rc != 0) {
        if (rc == 1)
            fprintf(stderr, "dp-server: peer closed before a complete message arrived\n");
        else
            fprintf(stderr, "dp-server: receive failed (%d)\n", rc);
        free(buffer);
        close(conn.fd);
        return 1;
    }
    printf("dp-server: received %zu bytes\n", received);

    /* The client sends a deterministic pattern, so the server can check that
     * every byte arrived exactly once and in the right place. */
    int ok = 1;
    for (size_t i = 0; i < received; ++i) {
        if (buffer[i] != (uint8_t)((i * 31U + 17U) & 0xffU)) {
            fprintf(stderr, "dp-server: verify: FAIL (first mismatch at byte %zu)\n", i);
            ok = 0;
            break;
        }
    }
    if (ok) printf("dp-server: verify: PASS\n");

    int close_rc = dp_wait_for_close(&conn);
    if (close_rc == 0) {
        printf("dp-server: close: PASS\n");
    } else {
        fprintf(stderr, "dp-server: close: FAIL (%d)\n", close_rc);
        ok = 0;
    }

    dp_net_stats_t stats;
    dp_get_network_stats(&conn, &stats);
    printf("dp-server: network: attempted=%lu dropped=%lu delivered=%lu delayed=%lu avg-delay=%.2fms\n",
           stats.attempted, stats.dropped, stats.delivered, stats.delayed,
           stats.delayed ? (double)stats.delay_ms_total / (double)stats.delayed : 0.0);

    free(buffer);
    close(conn.fd);
    return ok ? 0 : 1;
}

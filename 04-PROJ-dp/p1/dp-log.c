/*
 * dp-log.c - merge dp-client and dp-server logs into one timeline
 *
 * Part of the dp (Drexel Protocol) project. Provided tool: do not modify.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define INITIAL_CAPACITY 256

struct log_event {
    long long timestamp_us;
    char *line;
};

struct log_events {
    struct log_event *items;
    size_t count;
    size_t capacity;
};

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s client.log server.log\n", prog);
}

static long long parse_timestamp_us(const char *line) {
    const char *p = strstr(line, " NET: ");
    if (!p) return -1;
    p += 6;

    char *end = NULL;
    errno = 0;
    long long seconds = strtoll(p, &end, 10);
    if (errno || end == p || *end != '.') return -1;

    const char *frac = end + 1;
    long long micros = 0;
    int digits = 0;
    while (*frac >= '0' && *frac <= '9' && digits < 6) {
        micros = micros * 10 + (*frac - '0');
        ++frac;
        ++digits;
    }
    while (digits < 6) {
        micros *= 10;
        ++digits;
    }
    if (*frac != ' ') return -1;
    return seconds * 1000000LL + micros;
}

static int add_event(struct log_events *events, const char *line) {
    long long ts = parse_timestamp_us(line);
    if (ts < 0) return 0;

    if (events->count == events->capacity) {
        size_t new_capacity = events->capacity ? events->capacity * 2 : INITIAL_CAPACITY;
        struct log_event *new_items = realloc(events->items,
                                               new_capacity * sizeof(*new_items));
        if (!new_items) return -1;
        events->items = new_items;
        events->capacity = new_capacity;
    }

    events->items[events->count].timestamp_us = ts;
    events->items[events->count].line = strdup(line);
    if (!events->items[events->count].line) return -1;
    events->count++;
    return 1;
}

static int read_log(const char *filename, struct log_events *events) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "dp-log: cannot open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, fp)) != -1) {
        (void)length;
        int rc = add_event(events, line);
        if (rc < 0) {
            free(line);
            fclose(fp);
            return -1;
        }
    }

    free(line);
    fclose(fp);
    return 0;
}

static int compare_events(const void *a, const void *b) {
    const struct log_event *ea = a;
    const struct log_event *eb = b;
    if (ea->timestamp_us < eb->timestamp_us) return -1;
    if (ea->timestamp_us > eb->timestamp_us) return 1;
    return strcmp(ea->line, eb->line);
}

static void print_relative_timestamp(long long timestamp_us, long long first_us) {
    long long delta = timestamp_us - first_us;
    long long milliseconds = delta / 1000;
    long long micros = llabs(delta % 1000);
    printf("%8lld.%03lld ms  ", milliseconds, micros);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    struct log_events events = {0};
    if (read_log(argv[1], &events) < 0 || read_log(argv[2], &events) < 0) {
        for (size_t i = 0; i < events.count; ++i) free(events.items[i].line);
        free(events.items);
        return 1;
    }

    qsort(events.items, events.count, sizeof(events.items[0]), compare_events);

    if (events.count == 0) {
        fprintf(stderr, "dp-log: no NET events found in the supplied logs\n");
        free(events.items);
        return 1;
    }

    const long long first_us = events.items[0].timestamp_us;
    for (size_t i = 0; i < events.count; ++i) {
        print_relative_timestamp(events.items[i].timestamp_us, first_us);

        const char *net = strstr(events.items[i].line, " NET: ");
        if (net) {
            /* Drop the absolute timestamp so the merged view is compact. */
            const char *body = net + 6;
            const char *after_timestamp = strchr(body, ' ');
            if (after_timestamp) {
                printf("%s: NET: %s", strncmp(events.items[i].line, "dp-client:", 10) == 0 ? "dp-client" : "dp-server", after_timestamp + 1);
            } else {
                fputs(events.items[i].line, stdout);
            }
        } else {
            fputs(events.items[i].line, stdout);
        }
    }

    for (size_t i = 0; i < events.count; ++i) free(events.items[i].line);
    free(events.items);
    return 0;
}

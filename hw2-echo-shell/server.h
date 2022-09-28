#pragma once

typedef struct course_item_t {
    char *id;
    char *description;
} course_item_t;

static void start_server();
static void process_requests(int listen_socket);
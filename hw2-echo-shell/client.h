#pragma once

#include "cs472-proto.h"

static int initParams(int argc, char *argv[], char **p);
static void start_client(cs472_proto_header_t *header, uint8_t *packet_buff);
static void init_header(cs472_proto_header_t *packet, int req_type, char *req_data);

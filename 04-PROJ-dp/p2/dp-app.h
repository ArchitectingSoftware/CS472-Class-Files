/*
 * dp-app.h - dp (Drexel Protocol), Part 2 application layer
 * Provided file: do not modify.
 */
#ifndef DP_APP_H
#define DP_APP_H

/* Application layer shared by dp-client and dp-server (provided).
 *
 * The client sends several "files". Two modes:
 *
 *   multi   File i travels on its own transport stream (stream i, i >= 1).
 *           The transport delivers each stream in order, independently.
 *
 *   single  All files are multiplexed by the APPLICATION onto transport
 *           stream 0 as a sequence of frames. Stream 0 is delivered in order
 *           as one byte stream, like HTTP/2 over a single TCP connection.
 *
 * In both modes the files are interleaved in the same round-robin pattern,
 * one chunk per packet, so the only difference is where in-order delivery
 * is enforced.
 */

#include <stdint.h>
#include <stddef.h>

#define APP_MAX_FILES 7                 /* streams 1..7 in multi mode */
#define APP_FRAME_HEADER 8              /* file_id(2) flags(2) length(4) */
#define APP_FRAME_DATA (480 - APP_FRAME_HEADER)
#define APP_FLAG_LAST 0x0001            /* last frame of this file */
#define APP_DEFAULT_SIZES "128,32,8"   /* KB */

/* Deterministic content: byte j of file i. */
static inline uint8_t app_pattern(unsigned file_id, size_t j) {
    return (uint8_t)((j * 31U + 17U + file_id * 101U) & 0xffU);
}

#endif

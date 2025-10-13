/**
 * =============================================================================
 * CRYPTO-SERVER.H - Server Header File
 * =============================================================================
 * 
 * This header defines constants and function prototypes for the crypto
 * server implementation.
 * =============================================================================
 */

#ifndef __CRYPTO_SERVER_H__
#define __CRYPTO_SERVER_H__

#include "protocol.h"
#include "crypto-lib.h"
#include <stdio.h>

/* =============================================================================
 * SERVER CONFIGURATION
 * =============================================================================
 */

/**
 * BACKLOG - Maximum number of pending connections
 * 
 * Used with listen() to specify how many connection requests can be queued
 * before the server accepts them. A value of 5 is typical for simple servers.
 */
#define BACKLOG 5


/* =============================================================================
 * SERVER RETURN CODES
 * =============================================================================
 * These are used to communicate status between your server functions.
 * Return these from your client service function to indicate what happened.
 */

/**
 * RC_CLIENT_EXITED - Client disconnected normally
 * 
 * Return this when:
 * - recv() returns 0 (client closed connection)
 * - Client sends MSG_CMD_CLIENT_STOP
 * - Any other normal client disconnect
 * 
 * The server loop should close the client socket and accept the next connection.
 */
#define RC_CLIENT_EXITED            1

/**
 * RC_CLIENT_REQ_SERVER_EXIT - Client requested server shutdown
 * 
 * Return this when:
 * - Client sends MSG_CMD_SERVER_STOP (typically with '=' command)
 * 
 * The server loop should close all sockets and exit gracefully.
 */
#define RC_CLIENT_REQ_SERVER_EXIT   2


/* =============================================================================
 * FUNCTION PROTOTYPES
 * =============================================================================
 */

/**
 * start_server() - Main server entry point
 * 
 * STUDENT TODO: Implement this function
 * 
 * This function should:
 * 1. Create a TCP socket using socket()
 * 2. Set SO_REUSEADDR option using setsockopt()
 * 3. Configure server address using struct sockaddr_in
 *    - If addr is "0.0.0.0", use INADDR_ANY
 *    - Otherwise use inet_pton() to convert address
 * 4. Bind socket to address using bind()
 * 5. Start listening using listen() with BACKLOG
 * 6. Call your server loop function (you'll create this)
 * 7. Close the socket when server exits
 * 
 * Parameters:
 *   addr - IP address to bind to (e.g., "0.0.0.0" or "127.0.0.1")
 *   port - Port number to listen on (e.g., 1234)
 * 
 * This function is called from main() in crypto-echo.c
 * 
 * HELPFUL HINTS:
 * - SO_REUSEADDR lets you restart the server quickly during development
 * - Use perror() to print helpful error messages for socket operations
 * - The listen() backlog determines how many connections can wait
 * - Don't forget to close the socket before returning!
 */
void start_server(const char* addr, int port);

/**
 * ADDITIONAL FUNCTIONS YOU MAY WANT TO CREATE:
 * 
 * You'll likely want to create additional helper functions, such as:
 * 
 * int server_loop(int server_socket, const char* addr, int port);
 *   - Accepts connections in a loop
 *   - Calls client service function for each connection
 *   - Handles return codes (client exit vs server shutdown)
 * 
 * int service_client_loop(int client_socket);
 *   - Handles communication with one client
 *   - Receives requests, builds responses, sends replies
 *   - Returns RC_CLIENT_EXITED or RC_CLIENT_REQ_SERVER_EXIT
 * 
 * int build_response(crypto_msg_t *request, crypto_msg_t *response,
 *                    crypto_key_t *client_key, crypto_key_t *server_key);
 *   - Builds response PDU based on request type
 *   - Handles key exchange, echoing, encryption
 *   - Returns total PDU size
 * 
 * You can add prototypes for these functions here if you create them,
 * or keep them as static functions in your .c file.
 */

#endif // __CRYPTO_SERVER_H__

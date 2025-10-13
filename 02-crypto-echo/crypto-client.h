/**
 * =============================================================================
 * CRYPTO-CLIENT.H - Client Header File
 * =============================================================================
 * 
 * This header defines the data structures and function prototypes for the
 * crypto client implementation.
 * =============================================================================
 */

#ifndef __CRYPTO_CLIENT_H__
#define __CRYPTO_CLIENT_H__

#include "protocol.h"
#include "crypto-lib.h"
#include <stdio.h>

/* =============================================================================
 * COMMAND STRUCTURE
 * =============================================================================
 */

/**
 * msg_cmd_t - Represents a parsed user command
 * 
 * This structure is populated by get_command() and contains:
 *   - cmd_id: The message type (MSG_DATA, MSG_ENCRYPTED_DATA, etc.)
 *   - cmd_line: Pointer to the message text (or NULL for commands without data)
 * 
 * USAGE:
 *   msg_cmd_t command;
 *   if (get_command(buffer, size, &command) == CMD_EXECUTE) {
 *       // Use command.cmd_id to determine message type
 *       // Use command.cmd_line to get message text (may be NULL)
 *   }
 * 
 * NOTE: cmd_line points into the input buffer, don't free it separately
 */
typedef struct msg_cmd {
    int cmd_id;      // Message type constant (from protocol.h)
    char *cmd_line;  // Pointer to message text (NULL if no data)
} msg_cmd_t;

/**
 * Command execution status codes returned by get_command()
 */
#define CMD_EXECUTE 0   // Command should be sent to server
#define CMD_NO_EXEC 1   // Command was handled locally (like help), don't send


/* =============================================================================
 * FUNCTION PROTOTYPES
 * =============================================================================
 */

/**
 * start_client() - Main client entry point
 * 
 * STUDENT TODO: Implement this function
 * 
 * This function should:
 * 1. Create a TCP socket using socket()
 * 2. Configure server address using struct sockaddr_in
 * 3. Connect to server using connect()
 * 4. Call your communication loop function
 * 5. Close the socket when done
 * 
 * Parameters:
 *   addr - Server IP address string (e.g., "127.0.0.1")
 *   port - Server port number (e.g., 1234)
 * 
 * This function is called from main() in crypto-echo.c
 */
void start_client(const char* addr, int port);

/**
 * get_command() - Parse user input into a command structure
 * 
 * PROVIDED: This function is fully implemented for you in crypto-client.c
 * 
 * Reads a line from stdin and interprets special command characters:
 *   Regular text  -> MSG_DATA
 *   !<text>       -> MSG_ENCRYPTED_DATA
 *   #             -> MSG_KEY_EXCHANGE
 *   -             -> MSG_CMD_CLIENT_STOP
 *   =             -> MSG_CMD_SERVER_STOP
 *   ?             -> Help (returns CMD_NO_EXEC)
 * 
 * Parameters:
 *   cmd_buff    - Buffer to store user input
 *   cmd_buff_sz - Size of cmd_buff
 *   msg_cmd     - Output parameter: populated with parsed command
 * 
 * Returns:
 *   CMD_EXECUTE - Command should be sent to server
 *   CMD_NO_EXEC - Command was handled locally (don't send)
 * 
 * IMPORTANT: The msg_cmd->cmd_line pointer points into cmd_buff,
 * so cmd_buff must remain valid while using msg_cmd.
 */
int get_command(char *cmd_buff, size_t cmd_buff_sz, msg_cmd_t *msg_cmd);

#endif // __CRYPTO_CLIENT_H__

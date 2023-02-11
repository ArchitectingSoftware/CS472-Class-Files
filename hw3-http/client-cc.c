#include "http.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define  BUFF_SZ 1024

char recv_buff[BUFF_SZ];

char *generate_cc_request(const char *host, int port, const char *path){
	static char req[512] = {0};
	int offset = 0;
	
    //note that all paths should start with "/" when passed in
	offset += sprintf((char *)(req + offset),"GET %s HTTP/1.1\r\n", path);
	offset += sprintf((char *)(req + offset),"Host: %s\r\n", host);
	offset += sprintf((char *)(req + offset),"Connection: Close\r\n");
	offset += sprintf((char *)(req + offset),"\r\n");

	printf("DEBUG: %s", req);
	return req;
}


void print_usage(char *exe_name){
    fprintf(stderr, "Usage: %s <hostname> <port> <path...>\n", exe_name);
    fprintf(stderr, "Using default host %s, port %d  and path [\\]\n", DEFAULT_HOST, DEFAULT_PORT); 
}

int process_request(const char *host, uint16_t port, char *resource){
    int sock;
    int total_bytes;

    sock = socket_connect(host, port);
    if(sock < 0) return sock;

    //---------------------------------------------------------------------------------
    //TODO:   Implement Send/Receive loop for Connection:Closed
    //
    // 1. Generate the request - see the helper generate_cc_request
    // 2. Send the request to the HTTP server, make sure the send size
    //    matches the length of the generated request from generate_cc_request().
    // 3. Loop and receive the response data from the server.  You must
    //    loop, and you must save the data received inside of recv_buff.
    // 4. Each interation through the loop print out the data you receive.
    //    Note, the data will not be null terminated so be careful that
    //    you use the size of the data returned to control how the data 
    //    is printed.  Here is a format string that can help you out.
    //  
    //        printf("%.*s", bytes_recvd, recv_buff);
    //
    // 5. This function should return the total number of bytes received
    //    from the server, so why you are looping around, make sure to
    //    accumulate all of the data received and return this value. 
    //---------------------------------------------------------------------------------

    close(sock);
    return total_bytes;
}

int main(int argc, char *argv[]){
    int sock;

    const char *host = DEFAULT_HOST;
    uint16_t   port = DEFAULT_PORT;
    char       *resource = DEFAULT_PATH;
    int        remaining_args = 0;

    // Command line argument processing should be all setup, you should not need
    // to modify this code
    if(argc < 4){
        print_usage(argv[0]);
        //process the default request
        process_request(host, port, resource);
	} else {
        host = argv[1];
        port = atoi(argv[2]);
        resource = argv[3];
        if (port == 0) {
            fprintf(stderr, "NOTE: <port> must be an integer, using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
        fprintf(stdout, "Running with host = %s, port = %d\n", host, port);
        remaining_args = argc-3;
        for(int i = 0; i < remaining_args; i++){
            resource = argv[3+i];
            fprintf(stdout, "\n\nProcessing request for %s\n\n", resource);
            process_request(host, port, resource);
        }
    }
}
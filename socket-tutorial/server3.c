/*
 * THE MOST BASIC SOCKET SERVER
 *
 */
 
#include "server3.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>

#define BUFF_SZ 512

#define PORT_NUM    1090




/*
 * This function processes individual requests in threads
 */
void *connection_handler(void *socket_handle){
    int sock = *((int *)socket_handle);
    int ret = 0;

    // some thread local buffers for the messages - this has to be local to the thread
    uint8_t send_buffer[BUFF_SZ] = {0};
    uint8_t recv_buffer[BUFF_SZ] = {0};

    printf("\t\tHello from socket handler thread\n");
    ret = recv(sock, recv_buffer, sizeof(recv_buffer),0);
        if (ret == -1) {
            perror("read error");
            exit(EXIT_FAILURE);
        }
        if (*recv_buffer == 'A')
            sleep(0);
        else
            sleep(15);
 
        int buff_len = snprintf((char *)send_buffer, 
            sizeof(send_buffer), "THANK YOU -> %s", recv_buffer);

        //now string out buffer has the length
        send (sock, send_buffer, buff_len, 0);
        close(sock);

        return 0;
}


/*
 *  This function accepts a socket and processes requests from clients
 *  the server runs until stopped manually with a CTRL+C
 */
static void process_requests(int listen_socket){
    int data_socket;
    int ret;

    //again, not the best approach, need ctrl-c to exit
    while(1){
        //Do some cleaning
        

        //Establish a connection
        data_socket = accept(listen_socket, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("\t RECEIVED REQ...\n");
        pthread_t thread_id;
        if(pthread_create( &thread_id, NULL, connection_handler, (void*)&data_socket) < 0) {
            perror("could not create thread");
            exit(EXIT_FAILURE);
        }
    }
}

/*
 *  This function starts the server, basically creating the socket
 *  it will listen on INADDR_ANY which is basically all local
 *  interfaces, eg., 0.0.0.0
 */
static void start_server(){
    int listen_socket;
    int ret;
    
    struct sockaddr_in addr;

    /* Create local socket. */
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Bind socket to socket name. */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT_NUM);

    ret = bind(listen_socket, (const struct sockaddr *) &addr,
               sizeof(struct sockaddr_in));
    if (ret == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    /*
     * Prepare for accepting connections. The backlog size is set
     * to 20. So while one request is being processed other requests
     * can be waiting.
     */
    ret = listen(listen_socket, 20);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //Now process requests, this will never return so its bad coding
    //but ok for purposes of demo
    process_requests(listen_socket);

    close(listen_socket);
}

int main(int argc, char *argv[])
{
    printf("STARTING SERVER - CTRL+C to EXIT \n");
    start_server();
}
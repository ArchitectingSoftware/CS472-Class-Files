#include "server.h"
#include "cs472-proto.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>

#define BUFF_SZ 512
static uint8_t send_buffer[BUFF_SZ];
static uint8_t recv_buffer[BUFF_SZ];

/*
 *  A very simple database structure for this assignment, yes, i 
 *  know its hard coded, but its not the main part of this
 *  assignment so it was easist.  The format is the first
 *  field is a key, the second is a value.  In java we 
 *  would be simulating a hashmap. See the server.h file
 *  for the definition of course_item_t 
 */
static course_item_t course_db[] = {
    {"cs472", "CS472: Welcome to computer networks"},
    {"cs281", "CS281: Hello from computer architecture"},
    {"cs575", "CS575: Software Design is fun"},
    {"cs577", "CS577: Software architecture is important"}
};

/*
 *  Helper, given a course_id, returns the item from the course_db[]
 *  array that matches, if no match, returns a default, notice
 *  the static course_item_t for the default 
 */
course_item_t * lookup_course_by_id(char *course_id) {
    static course_item_t NOT_FOUND_COURSE = {"NONE", "Requested Course Not Found"};

    int count = sizeof(course_db) / sizeof(course_db[0]);
    for (int i = 0; i < count; i++){
        if (strcasecmp(course_db[i].id, course_id) == 0)
            return &course_db[i];
    }
    return &NOT_FOUND_COURSE;
}

/*
 *  This function accepts a socket and processes requests from clients
 *  the server runs until stopped manually with a CTRL+C
 */
static void process_requests(int listen_socket){
    cs472_proto_header_t header;
    char msg_in_buffer[MAX_MSG_SIZE];
    char msg_out_buffer[MAX_MSG_BUFFER];
    int data_socket;
    course_item_t *details;
    int ret;

    //again, not the best approach, need ctrl-c to exit
    while(1){
        //Do some cleaning to prepare for the next request given
        //we are looping, lets not get the last request baggage
        //inside of this request
        memset(&header,0,sizeof(cs472_proto_header_t));
        memset(msg_out_buffer,0,sizeof(msg_out_buffer));
        memset(msg_in_buffer,0,sizeof(msg_in_buffer));

        //Establish a connection
        data_socket = accept(listen_socket, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("\t RECEIVED REQ...\n");

        /*
         * TODO:  Handle the rest of the loop, basically you need to:
         *
         *      call recv() to get the request from the client
         * 
         *      Here is some helper code after you receive data from the client.  This
         *      helps get setup to actually process the client request
         * 
         *      cs472_proto_header_t *pcktPointer =  (cs472_proto_header_t *)recv_buffer;
         *      uint8_t *msgPointer = NULL;
         *      uint8_t msgLen = 0;
         *      process_recv_packet(pcktPointer, recv_buffer, &msgPointer, &msgLen);
         * 
         */

        //TODO:  DELETE THESE VARIABLES BELOW...
        //SEE THE COMMENT ABOVE, THESE VARIABLES ARE JUST PUT IN HERE FOR NOW TO MAKE SURE
        //THE STUB COMPILES
        cs472_proto_header_t *pcktPointer;
        uint8_t *msgPointer = NULL;
        uint8_t msgLen = 0;

        //Now lets setup to process the request and send a reply, create a copy of the header
        //also switch header direction
        memcpy(&header, pcktPointer, sizeof(cs472_proto_header_t)); //start building rsp header
        header.dir = DIR_RECV;
        switch(header.cmd){
            case CMD_CLASS_INFO:
                // sprintf(msg_out_buffer, class_msg, header.course);
                details = lookup_course_by_id(header.course);
                prepare_req_packet(&header,(uint8_t *)details->description, 
                    strlen(details->description), send_buffer, sizeof(send_buffer));
                break;
            case CMD_PING_PONG:
                strcpy(msg_out_buffer,"PONG: ");
                memcpy(msg_out_buffer + strlen(msg_out_buffer), msgPointer, msgLen);
                prepare_req_packet(&header,(uint8_t *)msg_out_buffer, 
                    strlen(msg_out_buffer), send_buffer, sizeof(send_buffer));
                break;
            default:
                perror("invalid command");
                close(data_socket);
                continue;
        }

        /*
         * TODO:  Now that we have processed things, send the response back to the 
         *        client - hint - its in the send_buffer. also dont forget to close
         *        the data_socket for the next request.
         */       
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

    unlink(SOCKET_NAME);

    /* Create socket. */
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
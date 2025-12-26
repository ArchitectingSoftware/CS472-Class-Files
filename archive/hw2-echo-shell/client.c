
#include "client.h"
#include "cs472-proto.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <getopt.h>

#define BUFF_SZ 512
static uint8_t send_buffer[BUFF_SZ];
static uint8_t recv_buffer[BUFF_SZ];

/*
 *  Helper function that processes the command line arguements.  Highlights
 *  how to use a very useful utility called getopt, where you pass it a
 *  format string and it does all of the hard work for you.  The arg
 *  string basically states this program accepts a -p or -c flag, the
 *  -p flag is for a "pong message", in other words the server echos
 *  back what the client sends, and a -c message, the -c option takes
 *  a course id, and the server looks up the course id and responds
 *  with an appropriate message. 
 */
static int initParams(int argc, char *argv[], char **p){
    int option;

    //setup defaults if no arguements are passed
    static char cmdBuffer[16] = "CS472"; 
    int         cmdType = CMD_CLASS_INFO;

    //
    // usage client [-p "ping pong message"] | [-c COURSEID]
    //
    while ((option = getopt(argc, argv, ":p:c:")) != -1){
        switch(option) {
            case 'p':
                strncpy(cmdBuffer, optarg, sizeof(cmdBuffer));
                cmdType = CMD_PING_PONG;
                break;
            case 'c':
                strncpy(cmdBuffer, optarg, sizeof(cmdBuffer));
                cmdType = CMD_CLASS_INFO;
                break;
            case ':':
                perror ("Option missing value");
                exit(-1);
            default:
            case '?':
                perror ("Unknown option");
                exit(-1);
        }
    }
    *p = cmdBuffer;
    return cmdType;
}

/*
 *  This function helps to initialize the packet header we will be sending
 *  to the server from the client.  It takes a pointer to a header structure
 *  that gets initiized as part of this function call.  The other args are:
 * 
 *      req_cmd:  The command we are doing - CMD_CLASS_INFO or CMD_PING_PONG
 *      req_data: This is the ping message in the case of CMD_PING_PONG, and
 *                is the courseID (e.g., cs472) in the case of CMD_CLASS_INFO
 */
static void init_header(cs472_proto_header_t *header, int req_cmd, char *reqData){
    memset(header, 0, sizeof(cs472_proto_header_t));

    header->proto = PROTO_CS_FUN;
    header->cmd = req_cmd;

    //TODO: Setup other header fields, eg., header->ver, header->dir, header->atm, header->ay

    //switch based on the command
    switch(req_cmd){
        case CMD_PING_PONG:
            strncpy(header->course, "NONE", sizeof(header->course));
            //length will be the header plus the size of the message
            //adding one more to get the null set to the server
            header->len = sizeof(cs472_proto_header_t) + strlen(reqData) + 1; 
            break;
        case CMD_CLASS_INFO:
            strncpy(header->course, reqData, sizeof(header->course));
            header->len = sizeof(cs472_proto_header_t);
            break;
    }
}

/*
 *  This function "starts the client".  It takes a header structure
 *  and a properly formed packet.  The packet is basically the
 *  header with a message at the end of it in the case of the
 *  command being CMD_PING_PONG.  The length in the header field
 *  indicates the total size to send to the server, thus it will
 *  be sizeof(cs472_proto_header_t) in the case that the cmd is
 *  CMD_COURSE_INFO, or it will be 
 *  sizeof(cs472_proto_header_t) + strlen(MESSAGE) + 1 in the case
 *  we are doing a ping to the server.  We add the extra 1 to send
 *  over the null terminator for the string
 */
static void start_client(cs472_proto_header_t *header, uint8_t *packet){
    struct sockaddr_in addr;
    int data_socket;
    int ret;

    /* Create local socket. */

    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /*
     * For portability clear the whole structure, since some
     * implementations have additional (nonstandard) fields in
     * the structure.
     */

    memset(&addr, 0, sizeof(struct sockaddr_in));

    /* Connect socket to socket address */
    /* note that i am sending to localhost, you will need a server IP 
     * if you are sending over a real network - try it out
     */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT_NUM);

    /*
     * TODO:  The next things you need to do is to handle the cleint
     * socket to send things to the server, basically make the following
     * calls:
     * 
     *      connect()
     *      send() - recall that the formatted packet is passed in
     *      recv() - get the response back from the server
     */

    //Now process what the server sent, here is some helper code
    cs472_proto_header_t *pcktPointer =  (cs472_proto_header_t *)recv_buffer;
    uint8_t *msgPointer = NULL;
    uint8_t msgLen = 0;

    process_recv_packet(pcktPointer, recv_buffer, &msgPointer, &msgLen);
    
    print_proto_header(pcktPointer);
    printf("RECV FROM SERVER -> %s\n",msgPointer);

    close(data_socket);

}


int main(int argc, char *argv[])
{
    cs472_proto_header_t header;
    int  cmd = 0;
    char *cmdData = NULL;

    //Process the parameters and init the header - look at the helpers
    //in the cs472-pproto.c file
    cmd = initParams(argc, argv, &cmdData);
    init_header(&header, cmd, cmdData);

    //Prepare the request packet based on the type of the command
    switch(cmd){
        case CMD_CLASS_INFO:
            prepare_req_packet(&header, 0, 0, send_buffer, sizeof(send_buffer));
            break;
        case CMD_PING_PONG:
            //add the +1 to send the null byte this way we can treat as a 
            //c string
            prepare_req_packet(&header,(uint8_t *)cmdData, strlen(cmdData)+1, 
                send_buffer, sizeof(send_buffer));
            break;
        default:
            perror("usage requires zero or one parameter");
            exit(EXIT_FAILURE);
    }

    //start the client
    start_client(&header, send_buffer);
}
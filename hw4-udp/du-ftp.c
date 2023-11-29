#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>

#include "du-ftp.h"
#include "du-proto.h"


#define BUFF_SZ 512
static char sbuffer[BUFF_SZ];
static char rbuffer[BUFF_SZ];
static char full_file_path[FNAME_SZ];

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
static int initParams(int argc, char *argv[], prog_config *cfg){
    int option;
    //setup defaults if no arguements are passed
    static char cmdBuffer[64] = {0};

    //setup defaults if no arguements are passed
    cfg->prog_mode = PROG_MD_CLI;
    cfg->port_number = DEF_PORT_NO;
    strcpy(cfg->file_name, PROG_DEF_FNAME);
    strcpy(cfg->svr_ip_addr, PROG_DEF_SVR_ADDR);
    
    while ((option = getopt(argc, argv, ":p:f:a:csh")) != -1){
        switch(option) {
            case 'p':
                strncpy(cmdBuffer, optarg, sizeof(cmdBuffer));
                cfg->port_number = atoi(cmdBuffer);
                break;
            case 'f':
                strncpy(cfg->file_name, optarg, sizeof(cfg->file_name));
                break;
            case 'a':
                strncpy(cfg->svr_ip_addr, optarg, sizeof(cfg->svr_ip_addr));
                break;
            case 'c':
                cfg->prog_mode = PROG_MD_CLI;
                break;
            case 's':
                cfg->prog_mode = PROG_MD_SVR;
                break;
            case 'h':
                printf("USAGE: %s [-p port] [-f fname] [-a svr_addr] [-s] [-c] [-h]\n", argv[0]);
                printf("WHERE:\n\t[-c] runs in client mode, [-s] runs in server mode; DEFAULT= client_mode\n");
                printf("\t[-a svr_addr] specifies the servers IP address as a string; DEFAULT = %s\n", cfg->svr_ip_addr);
                printf("\t[-p portnum] specifies the port number; DEFAULT = %d\n", cfg->port_number);
                printf("\t[-f fname] specifies the filename to send or recv; DEFAULT = %s\n", cfg->file_name);
                printf("\t[-p] displays what you are looking at now - the help\n\n");
                exit(0);
            case ':':
                perror ("Option missing value");
                exit(-1);
            default:
            case '?':
                perror ("Unknown option");
                exit(-1);
        }
    }
    return cfg->prog_mode;
}

int server_loop(dp_connp dpc, void *sBuff, void *rBuff, int sbuff_sz, int rbuff_sz){
    int rcvSz;

    FILE *f = fopen(full_file_path, "wb+");
    if(f == NULL){
        printf("ERROR:  Cannot open file %s\n", full_file_path);
        exit(-1);
    }
    if (dpc->isConnected == false){
        perror("Expecting the protocol to be in connect state, but its not");
        exit(-1);
    }
    //Loop until a disconnect is received, or error hapens
    while(1) {

        //receive request from client
        rcvSz = dprecv(dpc, rBuff, rbuff_sz);
        if (rcvSz == DP_CONNECTION_CLOSED){
            fclose(f);
            printf("Client closed connection\n");
            return DP_CONNECTION_CLOSED;
        }
        fwrite(rBuff, 1, rcvSz, f);
        rcvSz = rcvSz > 50 ? 50 : rcvSz;    //Just print the first 50 characters max

        printf("========================> \n%.*s\n========================> \n", 
            rcvSz, (char *)rBuff);
    }

}



void start_client(dp_connp dpc){
    static char sBuff[500];

    if(!dpc->isConnected) {
        printf("Client not connected\n");
        return;
    }


    FILE *f = fopen(full_file_path, "rb");
    if(f == NULL){
        printf("ERROR:  Cannot open file %s\n", full_file_path);
        exit(-1);
    }
    if (dpc->isConnected == false){
        perror("Expecting the protocol to be in connect state, but its not");
        exit(-1);
    }

    int bytes = 0;

    while ((bytes = fread(sBuff, 1, sizeof(sBuff), f )) > 0)
        dpsend(dpc, sBuff, bytes);

    fclose(f);
    dpdisconnect(dpc);
}

void start_server(dp_connp dpc){
    server_loop(dpc, sbuffer, rbuffer, sizeof(sbuffer), sizeof(rbuffer));
}


int main(int argc, char *argv[])
{
    prog_config cfg;
    int cmd;
    dp_connp dpc;
    int rc;


    //Process the parameters and init the header - look at the helpers
    //in the cs472-pproto.c file
    cmd = initParams(argc, argv, &cfg);

    printf("MODE %d\n", cfg.prog_mode);
    printf("PORT %d\n", cfg.port_number);
    printf("FILE NAME: %s\n", cfg.file_name);

    switch(cmd){
        case PROG_MD_CLI:
            //by default client will look for files in the ./outfile directory
            snprintf(full_file_path, sizeof(full_file_path), "./outfile/%s", cfg.file_name);
            dpc = dpClientInit(cfg.svr_ip_addr,cfg.port_number);
            rc = dpconnect(dpc);
            if (rc < 0) {
                perror("Error establishing connection");
                exit(-1);
            }

            start_client(dpc);
            exit(0);
            break;

        case PROG_MD_SVR:
            //by default server will look for files in the ./infile directory
            snprintf(full_file_path, sizeof(full_file_path), "./infile/%s", cfg.file_name);
            dpc = dpServerInit(cfg.port_number);
            rc = dplisten(dpc);
            if (rc < 0) {
                perror("Error establishing connection");
                exit(-1);
            }

            start_server(dpc);
            break;
        default:
            printf("ERROR: Unknown Program Mode.  Mode set is %d\n", cmd);
            break;
    }
}

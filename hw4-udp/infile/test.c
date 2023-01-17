#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/un.h>

#include "du-proto.h"

static char _dpBuffer[DP_MAX_FRAME_SZ];
static int  _debugMode = 1;

static dp_connp dpinit(){
    dp_connp dpsession = malloc(sizeof(dp_connection));
    bzero(dpsession, sizeof(dp_connection));
    dpsession->outSockAddr.isAddrInit = false;
    dpsession->inSockAddr.isAddrInit = false;
    dpsession->outSockAddr.len = sizeof(struct sockaddr_in);
    dpsession->inSockAddr.len = sizeof(struct sockaddr_in);
    dpsession->seqNum = 0;
    dpsession->isConnected = false;
    dpsession->dbgMode = true;
    return dpsession;
}

void dpclose(dp_connp dpsession) {
    free(dpsession);
}

//Opening Up server Side
//Server will set inSockAddress, needs outSockAddress on Connection
dp_connp dpServerInit(int port) {
    struct sockaddr_in *servaddr;
    int *sock;
    int rc;

    dp_connp dpc = dpinit();
    if (dpc == NULL) {
        perror("drexel protocol create failure"); 
        return NULL;
    }

    sock = &(dpc->udp_sock);
    servaddr = &(dpc->inSockAddr.addr);
        

    // Creating socket file descriptor 
    if ( (*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        return NULL;
    } 

    // Filling server information 
    servaddr->sin_family    = AF_INET; // IPv4 
    servaddr->sin_addr.s_addr = INADDR_ANY; 
    servaddr->sin_port = htons(port); 

    // Set socket options so that we dont have to wait for ports held by OS
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        close(*sock);
        return NULL;
    }
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        close(*sock);
        return NULL;
    }
    if ( (rc = bind(*sock, (const struct sockaddr *)servaddr,  
            dpc->inSockAddr.len)) < 0 ) 
    { 
        perror("bind failed"); 
        close (*sock);
        return NULL;
    } 

    dpc->inSockAddr.isAddrInit = true;
    dpc->outSockAddr.len = sizeof(struct sockaddr_in);
    return dpc;
}

//The client uses the same address for both in and out
dp_connp dpClientInit(char *addr, int port) {
    struct sockaddr_in *servaddr;
    int *sock;

    dp_connp dpc = dpinit();
    if (dpc == NULL) {
        perror("drexel protocol create failure"); 
        return NULL;
    }

    sock = &(dpc->udp_sock);
    servaddr = &(dpc->outSockAddr.addr);

    // Creating socket file descriptor 
    if ( (*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        return NULL;
    } 

    // Filling server information 
    servaddr->sin_family = AF_INET; 
    servaddr->sin_port = htons(port); 
    servaddr->sin_addr.s_addr = inet_addr(addr);
    dpc->outSockAddr.len = sizeof(struct sockaddr_in); 
    dpc->outSockAddr.isAddrInit = true;

    // The inbound address is the same as the outbound address
    memcpy(&dpc->inSockAddr, &dpc->outSockAddr, sizeof(dpc->outSockAddr));

    return dpc;
}

int dprecv(dp_connp dp, void *buff, int buff_sz){
    int bytesIn = 0;

    bytesIn = dprecvraw(dp, _dpBuffer, DP_MAX_FRAME_SZ);

    //check for some sort of error and just return it
    if (bytesIn < sizeof(dp_pdu))
        return bytesIn;

    dp_pdu inPdu;
    memcpy(&inPdu, _dpBuffer, sizeof(dp_pdu));
    if (inPdu.dgram_sz > buff_sz)
        return DP_BUFF_UNDERSIZED;

    //Copy buffer back
    memcpy(buff, (_dpBuffer+sizeof(dp_pdu)), inPdu.dgram_sz);
    
    
    //UDPATE SEQ NUMBER AND PREPARE ACK
    if(inPdu.dgram_sz == 0)
        dp->seqNum ++;
    else
        dp->seqNum += inPdu.dgram_sz;

    dp_pdu outPdu;
    outPdu.proto_ver = DP_PROTO_VER_1;
    outPdu.dgram_sz = 0;
    outPdu.seqnum = dp->seqNum;

    int actSndSz = 0;
    switch(inPdu.mtype){
        case DP_MT_SND:
            outPdu.mtype = DP_MT_SNDACK;
            actSndSz = dpsendraw(dp, &outPdu, sizeof(dp_pdu));
            if (actSndSz != sizeof(dp_pdu))
                return DP_ERROR_PROTOCOL;
            break;
        case DP_MT_CLOSE:
            outPdu.mtype = DP_MT_CLOSEACK;
            actSndSz = dpsendraw(dp, &outPdu, sizeof(dp_pdu));
            if (actSndSz != sizeof(dp_pdu))
                return DP_ERROR_PROTOCOL;
            dpclose(dp);
            return DP_CONNECTION_CLOSED;
        default:
        {
            printf("ERROR: Unexpected or bad mtype in header %d\n", inPdu.mtype);
            return DP_ERROR_PROTOCOL;
        }
    }

    return inPdu.dgram_sz;
}


static int dprecvraw(dp_connp dp, void *buff, int buff_sz){
    //struct sockaddr_in servaddr;
    //int sockfd;
    int bytes = 0;

    if(!dp->inSockAddr.isAddrInit) {
        perror("dprecv:dp connection not setup properly - cli struct not init");
        return -1;
    }

    bytes = recvfrom(dp->udp_sock, (char *)buff, buff_sz,  
                MSG_WAITALL, ( struct sockaddr *) &(dp->outSockAddr.addr), 
                &(dp->outSockAddr.len)); 

    if (bytes < 0) {
        perror("dprecv:received from error");
        return -1;
    }
    dp->outSockAddr.isAddrInit = true;
    if (bytes > sizeof(dp_pdu)){
        char * payload = (char *)buff + sizeof(dp_pdu);
        printf("DATA : %s\n", payload); 
    }

    dp_pdu *inPdu = buff;
    print_in_pdu(inPdu);


    return bytes;
}

//used to send data with the CORRECT PDU already on the start of the buffer
static int dpsendraw(dp_connp dp, void *sbuff, int sbuff_sz){
    int bytesOut = 0;

    if(!dp->outSockAddr.isAddrInit) {
        perror("dpsendraw:dp connection not setup properly");
        return -1;
    }

    dp_pdu *outPdu = sbuff;
    bytesOut = sendto(dp->udp_sock, (const char *)sbuff, sbuff_sz, 
        0, (const struct sockaddr *) &(dp->outSockAddr.addr), 
            dp->outSockAddr.len); 

    
    print_out_pdu(outPdu);

    return bytesOut;
}

int dpsend(dp_connp dp, void *sbuff, int sbuff_sz){
    int bytesOut = 0;

    if(!dp->outSockAddr.isAddrInit) {
        perror("dpsend:dp connection not setup properly");
        return DP_ERROR_GENERAL;
    }

    if(sbuff_sz > DP_MAX_BUFF_SZ)
        return DP_ERROR_GENERAL;

    //Build the PDU and out buffer
    dp_pdu *outPdu = _dpBuffer;
    int    sndSz = sbuff_sz;
    outPdu->proto_ver = DP_PROTO_VER_1;
    outPdu->mtype = DP_MT_SND;
    outPdu->dgram_sz = sndSz;
    outPdu->seqnum = dp->seqNum;

    memcpy((_dpBuffer + sizeof(dp_pdu)), sbuff, sndSz);

    int totalSendSz = outPdu->dgram_sz + sizeof(dp_pdu);
    bytesOut = dpsendraw(dp, _dpBuffer, totalSendSz);

    if(bytesOut != totalSendSz){
        printf("Warning send %d, but expected %d!\n", bytesOut, totalSendSz);
    }

    //update seq number after send
    if(outPdu->dgram_sz == 0)
        dp->seqNum++;
    else
        dp->seqNum += outPdu->dgram_sz;

    //need to get an ack
    dp_pdu inPdu = {0};
    int bytesIn = dprecvraw(dp, &inPdu, sizeof(dp_pdu));
    if (inPdu.mtype != DP_MT_SNDACK){
        printf("Expected SND/ACK but got a different mtype %d\n", inPdu.mtype);
    }
    print_in_pdu(&inPdu);

    return bytesOut - sizeof(dp_pdu);

/*
    bytesOut = sendto(dp->udp_sock, (const char *)sbuff, sbuff_sz, 
        0, (const struct sockaddr *) &(dp->outSockAddr.addr), 
            dp->outSockAddr.len); 

    
    print_out_pdu(outPdu);

    return bytesOut;
*/
}


int dplisten(dp_connp dp) {
    int sndSz, rcvSz;

    if(!dp->inSockAddr.isAddrInit) {
        perror("dplisten:dp connection not setup properly - cli struct not init");
        return DP_ERROR_GENERAL;
    }

    dp_pdu pdu = {0};

    printf("Waiting for a connection...\n");
    rcvSz = dprecvraw(dp, &pdu, sizeof(pdu));
    if (rcvSz != sizeof(pdu)) {
        perror("dplisten:The wrong number of bytes were received");
        return DP_ERROR_GENERAL;
    }

    pdu.mtype = DP_MT_CNTACK;
    dp->seqNum = pdu.seqnum + 1;
    pdu.seqnum = dp->seqNum;
    
    sndSz = dpsendraw(dp, &pdu, sizeof(pdu));
    
    if (sndSz != sizeof(pdu)) {
        perror("dplisten:The wrong number of bytes were sent");
        return DP_ERROR_GENERAL;
    }
    dp->isConnected = true; 
    //For non data transmissions, ACK of just control data increase seq # by one
    printf("Connection established OK!\n");

    return true;
}

int dpconnect(dp_connp dp) {

    int sndSz, rcvSz;

    if(!dp->outSockAddr.isAddrInit) {
        perror("dpconnect:dp connection not setup properly - svr struct not init");
        return DP_ERROR_GENERAL;
    }

    dp_pdu pdu = {0};
    pdu.mtype = DP_MT_CONNECT;
    pdu.seqnum = dp->seqNum;
    pdu.dgram_sz = 0;

    sndSz = dpsendraw(dp, &pdu, sizeof(pdu));
    if (sndSz != sizeof(dp_pdu)) {
        perror("dpconnect:Wrong about of connection data sent");
        return -1;
    }
    
    rcvSz = dprecvraw(dp, &pdu, sizeof(pdu));
    if (rcvSz != sizeof(dp_pdu)) {
        perror("dpconnect:Wrong about of connection data received");
        return -1;
    }
    if (pdu.mtype != DP_MT_CNTACK) {
        perror("dpconnect:Expected CNTACT Message but didnt get it");
        return -1;
    }

    //For non data transmissions, ACK of just control data increase seq # by one
    dp->seqNum++;
    dp->isConnected = true;
    printf("Connection established OK!\n");

    return true;
}
/**** END *****/
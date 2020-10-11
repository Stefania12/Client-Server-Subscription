#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>

#ifndef INCLUDES_H
#define INCLUDES_H
#define MAX_CLIENTS 50

#pragma pack (push, 1)
struct notification
{
    unsigned char topic[50];
    unsigned char data_type;
    unsigned int ip;
    unsigned short port;
    int len;
    unsigned char payload[1500];
};


#define APPROVAL_REQUEST (char) 0
#define UPDATE (char) 1


struct client_command
{
    char id[10];
    char cmd_type;
    char command[11];
    char topic[50];
    unsigned char SF;
};
#pragma pack(pop)


#endif
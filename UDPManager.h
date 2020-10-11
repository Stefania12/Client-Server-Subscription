#include <iostream>
#include "includes.h"
#include "NotificationValidityChecker.h"

#ifndef UDPMANAGER_H
#define UDPMANAGER_H

class UDPManager
{
    public:
    static UDPManager* get_instance();
    // Initializes socket.
    bool init(int port);
    ~UDPManager();
    // Receives a notification and checks its validity.
    bool receive_data(notification *notif);
    // Returns UDP socket.
    int get_server_sockfd();

    private:
    static UDPManager* instance;
    int server_sockfd;
    UDPManager();
};

#endif
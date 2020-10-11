#include "UDPManager.h"

UDPManager* UDPManager::instance = nullptr;

UDPManager::UDPManager()
{
}

UDPManager::~UDPManager()
{
    close(server_sockfd);
}

UDPManager* UDPManager::get_instance()
{
    if (!instance)
        instance = new UDPManager();
    return instance;
}

bool UDPManager::init(int port)
{
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
        return false;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    int ret;
    if ((ret = bind(server_sockfd,
            (const sockaddr*) &serv_addr, sizeof(serv_addr))) != 0)
        return false;

    return true;
}


bool UDPManager::receive_data(notification *notif)
{
    sockaddr_in client_addr;
    int client_len = sizeof(sockaddr_in);

    char buf[1600];
    memset(buf, 0, 1600);
    int n;
    // Receive a notification.
    if ((n = recvfrom(server_sockfd, buf, sizeof(buf), 0,
            (sockaddr*)&client_addr, (socklen_t*)&client_len)) < 0)
    {
        perror("An error occurred while receiving notifications");
        return false;
    }
    
    // Get topic and data type.
    memcpy((char*)notif, buf, 51);
    // Get ip.
    notif->ip = client_addr.sin_addr.s_addr;
    // Get port.
    memcpy((char*)&(notif->port), (char*)&(client_addr.sin_port),
            sizeof(unsigned short));

    // Get length of payload.
    notif->len = n - 51;
    // Get payload.
    memcpy(notif->payload, buf+51, n-51);

    // Check validity of notification.

    if (!NotificationValidityChecker::is_topic_valid(notif))
        return false;

    if (!NotificationValidityChecker::is_data_type_valid(notif))
        return false;

    if (!NotificationValidityChecker::is_payload_valid(notif))
        return false;

    return true;
}

int UDPManager::get_server_sockfd()
{
    return server_sockfd;
}
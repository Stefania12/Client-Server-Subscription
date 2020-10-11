#include <iostream>
#include "includes.h"
#include <set>
#include <map>
#include <queue>

#ifndef TCPMANAGER_H
#define TCPMANAGER_H

// Manager the flow of TCP messages.
class TCPManager
{
    public:
    ~TCPManager();
    static TCPManager* get_instance();
    // Initializes socket.
    bool init(int port);
    // Returns listening fd.
    int get_listen_fd();
    // Returns fd of the new connection, or a negative value upon error.
    int manage_new_connection();
    // Receives commands from client and processes them.
    bool receive_client_commands(int fd);
    // Manages a notification, forwarding it to subscribers or storing
    // it if subscriber is offline.
    void manage_notification(notification notif, fd_set *read_fds);

    private:
    static TCPManager* instance;
    // Listening fd.
    int listen_fd;
    // Holds ids of online subscribers.
    std::set<std::string> online_subscribers;

    // Holds subscriber fd-id pairs.
    std::map<int, std::string> subscribers_by_fd;
    // Holds subscriber id-fd pairs.
    std::map<std::string, int> subscribers_by_id;
    // Holds information on fds.
    std::map<int, sockaddr_in> subscriber_fd_info;

    // Holds (last unfinished command, length) for each fd.
    std::map<int, std::pair<client_command, int>> last_unfinished;
    // Holds a queue of pending notifications for each id.
    std::map<std::string, std::queue<notification>> pending_notificatons;

    // Holds subscribers id and sf for each topic.
    std::map<std::string, std::map<std::string, int>> subscriptions;
    TCPManager();
    // Closes connection of an id.
    void close_subscriber(std::string id);
    // Closes connection of a fd.
    void close_fd(int fd);
    // Manages commands: accepts/denies new id, (un)subscribes clients
    // to topics.
    bool manage_command(client_command &command, int fd);
    // Subscribes a client to a topic or overwrites the old subscription.
    void subscribe(std::string id, std::string topic, unsigned char SF);
    // Unsubscribes a client from a topic.
    void unsubscribe(std::string id, std::string topic, unsigned char SF);
    // Sends notification to a client.
    bool send_notification(std::string id, notification notif);
    // Sends pending notifications to a client.
    bool send_pending_notifications(int fd);
    // Sends accept/deny message to a client.
    bool send_approval_reply(int fd, bool accepted);
};

#endif

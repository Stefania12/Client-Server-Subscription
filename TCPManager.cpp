#include "TCPManager.h"

TCPManager* TCPManager::instance = nullptr;

TCPManager::TCPManager()
{
}

TCPManager::~TCPManager()
{
    for (std::string id : online_subscribers)
        close(subscribers_by_id[id]);
    close(listen_fd);
}

TCPManager* TCPManager::get_instance()
{
    if (!instance)
        instance = new TCPManager();
    return instance;
}

bool TCPManager::init(int port)
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        return false;

    int flag = 1;
    // Disable Neagle.
    if (setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY,
                    (char*)&flag, sizeof(int)) < 0)
        return -1;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        return false;

    if (listen(listen_fd, MAX_CLIENTS) < 0)
        return false;

    return true;
}

int TCPManager::get_listen_fd()
{
    return listen_fd;
}

void TCPManager::close_subscriber(std::string id)
{
    printf("Client %s disconnected.\n", id.c_str());
    close(subscribers_by_id[id]);
    online_subscribers.erase(id);
    subscribers_by_fd.erase(subscribers_by_id[id]);
    subscribers_by_id.erase(id);
}

void TCPManager::close_fd(int fd)
{
    close(fd);
    if (subscribers_by_fd.find(fd) != subscribers_by_fd.end())
    {
        printf("Client %s disconnected.\n", subscribers_by_fd[fd].c_str());
        online_subscribers.erase(subscribers_by_fd[fd]);
        subscribers_by_id.erase(subscribers_by_fd[fd]);
        subscribers_by_fd.erase(fd);
    }
}

int TCPManager::manage_new_connection()
{
    int new_sock_fd;
    socklen_t sock_len = sizeof(sockaddr_in);
    sockaddr_in client_addr;
    new_sock_fd = accept(listen_fd, (sockaddr*)&client_addr, &sock_len);
    subscriber_fd_info[new_sock_fd] = client_addr;
    return new_sock_fd;
}

bool TCPManager::receive_client_commands(int fd)
{
    auto node = last_unfinished.find(fd);
    client_command last_command;
    int last_command_len;

    // Check for a previous unfinished command.
    if (node == last_unfinished.end())
    {
        memset((char*)&last_command, 0, sizeof(last_command));
        last_command_len = 0;
    }
    else
    {
        last_command = last_unfinished[fd].first;
        last_command_len = last_unfinished[fd].second;
        last_unfinished.erase(fd);
    }
    
    int n;
    unsigned char buf[1500];
    memset(buf, 0, 1500);

    // Receive some data.
    if ((n = recv(fd, buf, 1500, 0)) < 0)
    {
        perror("An error occurred while receiving a command from client");
        close_fd(fd);
        return false;
    }
    // Client closed connection.
    if (n == 0)
    {
        close_fd(fd);
        return false;
    }

    int offset = 0;
    // Add to the unfinished command as much as possible.
    if (n < (int)sizeof(client_command) - last_command_len)
    {
        memcpy(((char*)&last_command) + last_command_len, buf, n);
        last_unfinished[fd] =
            std::pair<client_command, int>(last_command, last_command_len+n);
        return true;
    }

    // Finish last command and process it.
    memcpy(((char*)&last_command) + last_command_len, buf,
            sizeof(client_command)-last_command_len);

    if (!manage_command(last_command, fd))
        return false;
    offset = sizeof(client_command)-last_command_len;

    // Read full commands and process them.
    while (offset + (int)sizeof(client_command) <= n)
    {
        memcpy((char*)&last_command, buf+offset, sizeof(client_command));
        if (!manage_command(last_command, fd))
            return false;
        offset += sizeof(client_command);
    }

    // Add the beginning of a new unfinished command.
    if (offset < n)
    {
        memcpy((char*)&last_command, buf+offset, n-offset);
        last_command_len = n-offset;
        last_unfinished[fd] =
            std::pair<client_command, int>(last_command, last_command_len);
    }

    return true;
}

bool TCPManager::manage_command(client_command &command, int fd)
{
    sockaddr_in client_addr = subscriber_fd_info[fd];

    if (command.cmd_type == APPROVAL_REQUEST)
    {
        std::string id;
        char buf[100];
        memset(buf, 0, 100);
        memcpy(buf, &command.id, sizeof(command.id));
        id.assign(buf, buf+strlen(buf));

        // There is already a client with the same id, connection refused.
        if (online_subscribers.find(id) != online_subscribers.end())
        {
            send_approval_reply(fd, false);
            close_fd(fd);
            return false;
        }
        else
        {
            // Add new id-fd links.
            online_subscribers.insert(id);
            subscribers_by_fd.insert(std::pair<int, std::string>(fd, id));
            subscribers_by_id.insert(std::pair<std::string, int>(id, fd));

            char* ip = (char*)&(client_addr.sin_addr.s_addr);

            printf("New client %s connected from %d.%d.%d.%d:%d.\n",
                    id.c_str(), ip[0], ip[1], ip[2], ip[3],
                    ntohs(client_addr.sin_port));

            if (!send_approval_reply(fd, true))
            {
                close_fd(fd);
                return false;
            }

            if (!send_pending_notifications(fd))
            {
                close_subscriber(id);
                return false;
            }
        }
    }
    else
    {
        if (command.cmd_type == UPDATE)
        {
            std::string topic, cmd;
            char buf[100];
            memset(buf, 0, 100);
            memcpy(buf, command.topic, sizeof(command.topic));
            topic.assign(buf, buf+strlen(buf));

            memset(buf, 0, 100);
            memcpy(buf, command.command, sizeof(command.command));
            cmd.assign(buf, buf+strlen(buf));

            if (cmd.compare("subscribe") == 0)
            {
                subscribe(subscribers_by_fd[fd], topic, command.SF);
            }
            else
            {
                if (cmd.compare("unsubscribe") == 0)
                    unsubscribe(subscribers_by_fd[fd], topic, command.SF);
            }
        }
    }
    return true;
}

void TCPManager::subscribe(std::string id, std::string topic, unsigned char SF)
{
    if (subscriptions.find(topic) == subscriptions.end())
        subscriptions[topic] = std::map<std::string, int>();
    if (subscriptions[topic].find(id) != subscriptions[topic].end())
        subscriptions[topic].erase(id);
    subscriptions[topic].insert(std::pair<std::string, int>(id, (int)SF));
}

void TCPManager::unsubscribe(std::string id, std::string topic,
                            unsigned char SF)
{
    if (subscriptions.find(topic) != subscriptions.end())
        subscriptions[topic].erase(id);
}


void TCPManager::manage_notification(notification notif, fd_set *read_fds)
{
    char buf[1600];
    memset(buf, 0, sizeof(buf));

    std::string topic;
    memcpy(buf, notif.topic, sizeof(notif.topic));
    topic.assign(buf, buf+strlen(buf));

    std::map<std::string, int>::iterator it;
    for (it = subscriptions[topic].begin(); it != subscriptions[topic].end();
        it++)
    {
        std::string id = it->first;
        int sf = it->second;

        if (online_subscribers.find(id) != online_subscribers.end())
        {
            if (!send_notification(id, notif))
            {
                FD_CLR(subscribers_by_id[id], read_fds);
                close_subscriber(id);
            }
        }
        else
        {
            if (sf == 1)
            {
                if (pending_notificatons.find(id) == pending_notificatons.end())
                    pending_notificatons[id] = std::queue<notification>();
                pending_notificatons[id].push(notif);
            }
        }
    }
}

bool TCPManager::send_notification(std::string id, notification notif)
{
    int len = sizeof(notification) - sizeof(notif.payload) + notif.len;
    if (send(subscribers_by_id[id], &notif, len, 0) < 0)
    {
        perror("An error occurred while sending notification to client");
        return false;
    }
    return true;
}

bool TCPManager::send_pending_notifications(int fd)
{
    std::string id = subscribers_by_fd[fd];

    if (pending_notificatons.find(id) != pending_notificatons.end())
    {
        while (pending_notificatons[id].size() > 0)
        {
            notification notif = pending_notificatons[id].front();
            if (!send_notification(id, notif))
                return false;
            pending_notificatons[id].pop();
        }
    }
    return true;
}

bool TCPManager::send_approval_reply(int fd, bool accepted)
{
    notification notif;
    memset((char*)&notif, 0, sizeof(notification));
    notif.len = sizeof(notif.payload);
    if (accepted)
        notif.data_type = 1;
    else
        notif.data_type = 0;    

    if (send(fd, (char*)&notif, sizeof(notification), 0) < 0)
    {
        perror("An error occurred while sending approval of connection");
        return false;
    }
    return true;
}
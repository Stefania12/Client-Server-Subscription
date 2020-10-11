#include <iostream>
#include "includes.h"
#include <algorithm>
#include "NotificationValidityChecker.h"
#include "NotificationInterpreter.h"

// Computes a client command based on the input and returns true if valid.
bool make_client_command(client_command* command, std::string &input, char id[])
{
    std::string cmd, topic, sf;

    int space_idx = input.find(" ");
    if (space_idx < 0)
    {
        fprintf(stderr,
            "Invalid number of parameters, try subscribe topic sf_val.\n");
        return false;
    }
    cmd = input.substr(0, space_idx);
    input = input.substr(space_idx+1);

    // Check validity of command type and spaces after.
    if (cmd.compare("subscribe") == 0)
    {
        if (std::count(input.begin(), input.end(), ' ') != 1)
        {
            fprintf(stderr,
                "Invalid number of parameters, try subscribe topic sf_val.\n");
            return false;
        }
    }
    else
    {
        if (cmd.compare("unsubscribe") == 0)
        {
            if (std::count(input.begin(), input.end(), ' ') != 0
                || input.size() == 0)
            {
                fprintf(stderr,
                    "Invalid number of parameters, try unsubscribe topic.\n");
                return false;
            }
        }
        else
        {
            fprintf(stderr, "Invalid command, try subscribe or unsubscribe.\n");
            return false;
        }
    }

    space_idx = input.find(" ");
    topic = input.substr(0, space_idx);
    input = input.substr(space_idx+1);

    if (topic.size() > 50)
    {
        fprintf(stderr, "Topic too long, maximum characters number: 50.\n");
        return false;
    }

    if (topic.size() == 0)
    {
        fprintf(stderr, "Topic unspecified.\n");
    }

    // Check sf value only for subscribe.
    if (cmd.compare("subscribe") == 0)
    {
        sf = input;
        if (sf.size() != 1 || (sf.c_str()[0] > '1' || sf.c_str()[0] < '0'))
        {
            fprintf(stderr, "Invalid SF value, try 0 or 1.\n");
            return false;
        }
    }
    // Compute command.
    memset(command, 0, sizeof(*command));
    memcpy(command->id, id, strlen(id));
    command->cmd_type = UPDATE;
    memcpy(command->command, cmd.c_str(), cmd.size());
    memcpy(command->topic, topic.c_str(), topic.size());
    command->SF = sf.c_str()[0] - '0';
    return true;
}

// Opens a connection to the server and returns sockfd,
// or a negative number on error.
int open_connection(int port, char* serv_ip_addr, char my_id[])
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
        return -1;

    int flag = 1;
    // Disable Neagle.
    if (setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY,
                    (char*)&flag, sizeof(int)) < 0)
        return -1;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((unsigned short)port);
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip_addr);

    if (connect(sock_fd, (const sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        return -1;

    // Compute and send an approval request message.
    client_command command;
    memset((char*)&command, 0, sizeof(command));
    memcpy(command.id, my_id, strlen(my_id));
    command.cmd_type = APPROVAL_REQUEST;

    if (send(sock_fd, &command, sizeof(command), 0) < 0)
        return -1;

    return sock_fd;
}

// Verifies if server accepted client.
bool verify_approval_reply(notification notif)
{
    return notif.data_type == 1;
}

// Receives notifications from server and dislpays them.
// Returns true if no error occurred.
bool receive_notifications(int fd, notification *last_notif, int &last_len)
{
    // Offset to the beginning of payload.
    int len_off = sizeof(notification) - sizeof(last_notif->payload);

    int n, offset = 0;
    char buf[1600];
    memset(buf, 0, sizeof(buf));
    // Receive some notifications.
    if ((n = recv(fd, buf, sizeof(buf), 0)) < 0)
    {
        perror("An error occured while receiving data");
        return false;
    }

    if (n == 0)
    {
        return false;
    }

    // Check if there is an unfinished notification.
    if (last_len > 0)
    {
        // Add as much as possible till the beginning of payload.
        if (last_len < len_off)
        {
            if (n < len_off - last_len)
            {
                memcpy(((char*)last_notif)+last_len, buf+offset, n);
                last_len += n;
                return true; 
            }
            else
            {
                memcpy(((char*)last_notif)+last_len,
                        buf+offset, len_off-last_len);
                offset += len_off - last_len;
                last_len = len_off;
            }
            
        }
        int already_in_payload = last_len - len_off;

        // Add as much as possible in payload.
        if (offset < n)
        {
            if (n-offset < last_notif->len - already_in_payload)
            {
                memcpy(((char*)last_notif)+last_len, buf+offset, n-offset);
                last_len += n-offset;
                return true;
            }
            else
            {
                // Finish the previous notification.
                memcpy(((char*)last_notif)+last_len, buf+offset,
                        last_notif->len - already_in_payload);
                offset += last_notif->len - already_in_payload;
                last_len = 0;
                // Check nature of notification. Close if connection refused.
                // Display notification if there is the case.
                if (last_notif->topic[0] != 0)
                {
                    printf("%s\n",
                    NotificationInterpreter::notification_to_string(*last_notif).c_str());
                }
                else
                {
                    if (!verify_approval_reply(*last_notif))
                    {
                        fprintf(stderr, "ID already in use, try something else.\n");
                        return false;
                    }
                }
                
            }
        }
        else
        {
            return true;
        }
    }

    // Read data till payload and the rest if possible.
    while (offset+len_off <= n)
    {
        memcpy(((char*)last_notif), buf+offset, len_off);
        last_len = len_off;
        offset += len_off;

        if (offset+last_notif->len <= n)
        {
            memcpy(((char*)last_notif)+len_off, buf+offset, last_notif->len);
            offset += last_notif->len;
            last_len = 0;
            // Manage nature of notification as above.
            if (last_notif->topic[0] != 0)
            {
                printf("%s\n",
                NotificationInterpreter::notification_to_string(*last_notif).c_str());
            }
            else
            {
                if (!verify_approval_reply(*last_notif))
                {
                    fprintf(stderr, "ID already in use, try something else.\n");
                    return false;
                }
            }
        }
        else
        {
            memcpy(((char*)last_notif)+len_off, buf+offset, n-offset);
            last_len += n-offset;
            return true;
        }
    }

    // Add last part of the buffer to the last notification.
    if (offset < n)
    {
        memcpy(((char*)last_notif), buf+offset, n-offset);
        last_len = n-offset;
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr,
        "Wrong number of arguments, use ./subscriber ID server_ip server_port\n");
        return -1;
    }

    if (strlen(argv[1]) > 10)
    {
        fprintf(stderr,
            "ID too long, maximum number of characters allowed: 10.\n");
        return 0;
    }

    char my_id[15];
    strcpy(my_id, argv[1]);
    
    int sock_fd = open_connection(atoi(argv[3]), argv[2], my_id);
    if (sock_fd < 0)
    {
        perror("Socket initialization failed");
        return 0;
    }
    fd_set read_fds, tmp_fds;
    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
    
    FD_SET(0, &read_fds);
    FD_SET(sock_fd, &read_fds);
    

    notification last_notif;
    int last_len = 0;

    while (true)
    {
        tmp_fds = read_fds;
        if (select(sock_fd+1, &tmp_fds, NULL, NULL, NULL) < 0)
        {
            perror("An error occurred while receiving data");
            return 0;
        }

        // Check input from stdin.
        if (FD_ISSET(0, &tmp_fds))
        {
            std::string buf;
            std::getline(std::cin, buf);

            if (buf.compare("exit") == 0)
            {
                close(sock_fd);
                return 0;
            }

            client_command command;
            if (make_client_command(&command, buf, my_id))
            {
                if (send(sock_fd, &command, sizeof(command), 0) <= 0)
                {
                    perror("An error occurred while sending command to server");
                    close(sock_fd);
                    return 0;
                }

                // Compute displayable message.
                std::string cmd, topic;
                char buf[100];
                memset(buf, 0, 100);
                memcpy(buf, command.command, sizeof(command.command));
                cmd.assign(buf, buf+strlen(buf));
                cmd.append("d");

                memset(buf, 0, 100);
                memcpy(buf, command.topic, sizeof(command.topic));
                topic.assign(buf, buf+strlen(buf));

                printf("%s %s\n", cmd.c_str(), topic.c_str());
            }
        }
        // Check notification from server.
        if (FD_ISSET(sock_fd, &tmp_fds))
        {
            if (!receive_notifications(sock_fd, &last_notif, last_len))
            {
                close(sock_fd);
                return 0;
            }
        }
    }

    return 0;
}
#include "includes.h"
#include <iostream>

#include "UDPManager.h"
#include "TCPManager.h"

void close_everything()
{
    delete UDPManager::get_instance();
    delete TCPManager::get_instance();
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr,
                "Wrong number of arguments, use ./server server_port\n");
        return 0;
    }

    int port = atoi(argv[1]);

    if (!UDPManager::get_instance()->init(port))
    {
        perror("UDP socket initialization failed");
        close_everything();
        return 0;
    }

    if (!TCPManager::get_instance()->init(port))
    {
        perror("TCP socket initialization failed");
        close_everything();
        return 0;
    }

    fd_set read_fds, tmp_fds;
    int fd_max = std::max(UDPManager::get_instance()->get_server_sockfd(),
                            TCPManager::get_instance()->get_listen_fd());

    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
    
    FD_SET(0, &read_fds);
    FD_SET(UDPManager::get_instance()->get_server_sockfd(), &read_fds);
    FD_SET(TCPManager::get_instance()->get_listen_fd(), &read_fds);

    int old_fd_max;
    while (true)
    {
        old_fd_max = fd_max;
        tmp_fds = read_fds; 
		if (select(fd_max + 1, &tmp_fds, NULL, NULL, NULL) < 0)
        {
            perror("An error occurred while receiving data");
            close_everything();
            return 0;
        }

        for (int i = 0; i <= old_fd_max; i++)
        {
            if (FD_ISSET(i, &tmp_fds))
            {
                // Command from stdin.
                if (i == 0)
                {
                    std::string buf;
                    std::getline(std::cin, buf);
                    if (buf.compare("exit") == 0)
                    {
                        close_everything();
                        return 0;
                    }
                    else
                    {
                        fprintf(stderr,
                        "Invalid command, use exit to close the server.\n");
                        continue;
                    }
                }

                // Received a notification on the UDP socket.
                if (i == UDPManager::get_instance()->get_server_sockfd())
                {
                    notification notif;
                    // If it is valid, notify TCP clients and remove those
                    // with errors.
                    if (UDPManager::get_instance()->receive_data(&notif))
                        TCPManager::get_instance()->manage_notification(notif,
                                                                    &read_fds);
                    continue;
                }

                // Received a new connection on the TCP listening socket.
                if (i == TCPManager::get_instance()->get_listen_fd())
                {
                    int new_sock_fd;
                    // Check value of socket and display existing error.
                    if ((new_sock_fd = 
                        TCPManager::get_instance()->manage_new_connection()) < 0)
                    {
                        perror("An error occured while accepting a new connection");
                    }
                    else
                    {
                        // Add socket to the fd_set.
                        FD_SET(new_sock_fd, &read_fds);
                        // Update max fd.
                        if (new_sock_fd > fd_max)
                        { 
                            fd_max = new_sock_fd;
                        }
                    }
                }
                else
                {
                    // Received a TCP client command.
                    // If any error occured, the socket is closed and removed from the fd_set.
                    if (!TCPManager::get_instance()->receive_client_commands(i))
                    {
                        FD_CLR(i, &read_fds);
                    }
                }
                
            }
        }
    }

    return 0;
}
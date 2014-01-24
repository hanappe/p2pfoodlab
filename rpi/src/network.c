/* 

    P2P Food Lab Sensorbox

    Copyright (C) 2013  Sony Computer Science Laboratory Paris
    Author: Peter Hanappe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "log_message.h"
#include "network.h"

static void _close(int sock)
{
        shutdown(sock, SHUT_RDWR);
        close(sock);
}

int network_connected()
{
        int _socket = -1;
        int port = 80;
        const char* host = "opensensordata.net";
        struct sockaddr_in sock_addr;
        struct sockaddr_in serv_addr;
        struct in_addr addr;
        struct hostent *hostptr;

        /* Call DNS */
        hostptr = gethostbyname(host);
        if (!hostptr) {
                return 0;
        }
        addr = *(struct in_addr *) hostptr->h_addr_list[0];
  
        /* Establish a connection */        
        _socket = socket(AF_INET, SOCK_STREAM, 0);
        if (_socket == -1) {
                log_err("Network: Failed to create the socket");
                return -1;
        }

        /* Bind the socket */        
        memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_port = 0;
        
        if (bind(_socket, (const struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1)  {
                _close(_socket);
                log_err("Network: Failed to bind the socket");
                return -1;
        }

        /* Connect the socket */        
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr = addr;
        serv_addr.sin_port = htons(port);
        
        if (connect(_socket, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
                _close(_socket);
                return 0;
        }
        
        _close(_socket);        

        return 1;
}

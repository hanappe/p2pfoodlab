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
#define _POSIX_SOURCE
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <curl/curl.h>
#include "log_message.h"
#include "network.h"
#include "system.h"

#if !defined(NI_MAXHOST)
#define NI_MAXHOST 1025
#endif

static void _close(int sock)
{
        shutdown(sock, SHUT_RDWR);
        close(sock);
}

/*
  Returns: 1: connected; 0: not connected; -1: error 
 */
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

int network_ifaddr(const char* iface, char *addr, int len)
{
        struct ifaddrs *ifaddr, *ifa;
        int s;
        char host[NI_MAXHOST];

        if (getifaddrs(&ifaddr) == -1) {
                log_err("Network: getifaddrs failed: %s", strerror(errno));
                return -1;
        }

        /* Walk through linked list, maintaining head pointer so we
           can free list later */

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                        continue;

                if (ifa->ifa_addr->sa_family != AF_INET)
                        continue;

                if (strcmp(ifa->ifa_name, iface) == 0) {

                        s = getnameinfo(ifa->ifa_addr,
                                        sizeof(struct sockaddr_in),
                                        host, NI_MAXHOST, NULL, 
                                        0, NI_NUMERICHOST);
                        if (s != 0) {
                                log_err("Network: getnameinfo failed: %s", gai_strerror(s));
                                freeifaddrs(ifaddr);
                                return -1;
                        }

                        strncpy(addr, host, len);
                        addr[len - 1] = 0;
                        
                        freeifaddrs(ifaddr);
                        return 0;
                }
        }

        freeifaddrs(ifaddr);
        return -1;
}

int network_ifchange(const char* name, const char* cmd)
{
        log_info("Network: Running %s %s", cmd, name);
        char* const argv[] = { "/usr/bin/sudo", (char*) cmd, (char*) name, NULL};
        return system_run(argv, 180);
}

int network_ifup(const char* name)
{
        log_info("Network: Bringing up %s", name);
        int r = network_ifchange(name, "/sbin/ifup");
        // give the network layer some time to settle.
        sleep(20);
        return r;
}

int network_ifdown(const char* name)
{
        log_info("Network: Shutting down %s", name);
        return network_ifchange(name, "/sbin/ifdown");
}

int network_gogo(const char* iface)
{
        char addr[1024];

        if (network_ifaddr(iface, addr, 1024) == 0) {
                log_info("Network: %s is up, address %s", iface, addr);
                return 0;
        }

        for (int i = 0; i < 5; i++) {
                log_info("Network: Bringing up interface %s (attempt %d/5)", 
                         iface, i+1);

                int ret = network_ifup(iface);
                if (ret != 0) {
                        log_info("Network: ifup %s failed", iface);
                        continue;
                }
                
                // wait until the interface has an IP address
                for (int j = 0; j < 5; j++) {
                        char addr[1024];
                        ret = network_ifaddr(iface, addr, 1024);
                        if (ret == 0) {
                                log_info("Network: %s is up, address %s", iface, addr);
                                break;
                        } else
                                sleep(10);
                }

                // check that we have network access to be be sure
                if (network_connected()) {
                        log_info("Network: Connected");
                        return 0;
                }

                log_info("Network: interface %s up, but failed to establish a network connection", iface);

                if (network_ifdown(iface) != 0)
                        log_info("Network: ifdown %s failed", iface);
        }
        return -1;
}

int network_byebye(const char* iface)
{
        return network_ifdown(iface);
}

int network_http_get(const char* url,
                     const char* filename)
{
        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Network: Failed to open the file '%s'.", filename);
                return -1;
        }

        CURL* curl = curl_easy_init();
        if (curl == NULL) {
                log_err("Network: Failed to initialise curl.");
                fclose(fp);
                return -1;
        }
        
        char errmsg[CURL_ERROR_SIZE];

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5 minutes
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errmsg);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
                log_err("Network: Failed to get the data: %s", errmsg);
                fclose(fp);
                curl_easy_cleanup(curl);
                curl = NULL;
                return res;
        }
        fclose(fp);

        long response_code = -1;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (res != CURLE_OK) {
                log_err("Network: Failed to obtain curl's response code.");
                curl_easy_cleanup(curl);
                curl = NULL;
                return -1;
        }
        if (response_code != 200) {
                log_err("Network: HTTP GET request failed (HTTP code %d).", response_code);
                curl_easy_cleanup(curl);
                curl = NULL;
                return -1;
        }
        log_debug("Network: HTTP GET request successful (HTTP code %d).", response_code);
                
        curl_easy_cleanup(curl);
        curl = NULL;

        return 0;
}


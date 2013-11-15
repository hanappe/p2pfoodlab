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
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include "logMessage.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

static char* pidFile = "/var/run/p2pfoodlab-daemon.pid";
static char* logFile = "/var/log/p2pfoodlab.log";
static int port = 10080;
static int nodaemon = 0;
static int serverSocket = -1;

static int openServerSocket(int port) 
{
        struct sockaddr_in sockaddr;
        struct in_addr addr;
        int serverSocket = -1;

        int r = inet_aton("127.0.0.1", &addr);
        if (r == 0) {
                logMessage("daemon", LOG_ERROR, "Failed to convert '127.0.0.1' to an IP address...?!\n");
                return -1;
        }
        //addr.s_addr = htonl(INADDR_ANY);

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
                logMessage("daemon", LOG_ERROR, "Failed to create server socket\n");
                return -1;
        }

        memset((char *)&sockaddr, 0, sizeof(struct sockaddr_in));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr = addr;
        sockaddr.sin_port = htons(port);

        if (bind(serverSocket, (const struct sockaddr *) &sockaddr, 
                 sizeof(struct sockaddr_in)) == -1) {
                close(serverSocket);
                serverSocket = -1;
                logMessage("daemon", LOG_ERROR, "Failed to bind server socket\n");
                return -1;
        }
        
        if (listen(serverSocket, 10) == -1) {
                close(serverSocket);
                serverSocket = -1;
                logMessage("daemon", LOG_ERROR, "Failed to bind server socket\n");
                return -1;
        }

        return serverSocket;
}

static void closeServerSocket(int serverSocket) 
{
        if (serverSocket != -1) {
                close(serverSocket);
        }
}

static int serverSocketAccept(int serverSocket) 
{
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);

        if (serverSocket == -1) {
                logMessage("daemon", LOG_ERROR, "Invalid server socket\n");
                return -1;
        }
  
        int clientSocket = accept(serverSocket, (struct sockaddr*) &addr, &addrlen);
  
        if (clientSocket == -1) {
                logMessage("daemon", LOG_ERROR, "Accept failed\n");
                return -1;
        } 

        int flag = 1; 
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

        return clientSocket;
}

static int clientWrite(int clientSocket, char* buffer, int len) 
{
        return send(clientSocket, buffer, len, MSG_NOSIGNAL);
}

static int clientPrint(int clientSocket, char* s) 
{
        int len = strlen(s);
        return clientWrite(clientSocket, s, len);
}

static int clientPrintf(int client, const char* format, ...) 
{
        char buffer[1024];
        va_list ap;

        va_start(ap, format);
        vsnprintf(buffer, 1023, format, ap);
        va_end(ap);
        buffer[1023] = 0;

        return clientPrint(client, buffer);
}

static void closeClient(int clientSocket) 
{
        if (clientSocket != -1) {
                close(clientSocket);
        }
}

typedef struct _output_t {
        char* buf;
        int size;
        int count;
} output_t;

void output_clear(output_t* output)
{
        if (output->buf) 
                free(output->buf);
        output->buf = NULL;
        output->size = 0;
        output->count = 0;
}

int output_append(output_t* output, char c)
{
        if (output->count >= output->size) {
                int newsize = 1024 + 2 * output->size;
                output->buf = realloc(output->buf, newsize);
                if (output->buf == NULL) {
                        logMessage("daemon", LOG_ERROR, "Out of memory\n");
                        output_clear(output);
                        return -1;
                }
                output->size = newsize;
        }
        output->buf[output->count++] = c;
        return 0;
}

int execute(output_t* output, const char* cmdline)
{
        logMessage("daemon", LOG_INFO, "Executing: '%s'\n", cmdline);

        FILE * pipe = popen(cmdline, "r");
        if (pipe == NULL) {
                logMessage("daemon", LOG_WARNING, "Command failed: '%s'\n", cmdline);
                return -1;
        }
        
        while (1) {
                char c;
                size_t read = fread(&c, 1, 1, pipe);
                if (read == 0) {
                        int err = ferror(pipe);
                        int r = 0;
                        if (err != 0) {
                                r = -1;
                                logMessage("daemon", LOG_ERROR, 
                                           "Command failed: ferror(%d)\n", err);
                        }
                        pclose(pipe);
                        return r;
                }
                if (output_append(output, c) != 0) {
                        pclose(pipe);
                        return -1;
                }
        }
        return 0;
}

int writePidFile(const char* filename) 
{
	pid_t pid = getpid();
        FILE* fp = fopen(filename, "w");
        if (fp) {
                fprintf(fp, "%d\n", pid);
                fclose(fp);
                return 0;
        } else {
                logMessage("daemon", LOG_ERROR, "Failed to create the PID file '%s'\n", filename);
        }
        return -1;
}

int removePidFile(const char* filename) 
{
        return unlink(filename);
}

int daemonise() 
{
	switch (fork()) {
        case 0:  
		break;
        case -1: 
		logMessage("daemon", LOG_ERROR, "Failed to fork\n");		
		return -1;
        default: 
		_exit(0);
	}
	
	if (setsid() < 0) {
		logMessage("daemon", LOG_ERROR, "setsid() failed\n");		
		return -1;
	}

	for (int i = 0; i < 3; ++i) 
		close(i);

	int fd;

	if ((fd = open("/dev/null", O_RDONLY)) == -1) {
		logMessage("daemon", LOG_ERROR, "Failed to open /dev/null\n");		
		return -1;
	}

	if ((fd = open("/dev/null", O_WRONLY | O_TRUNC, 0644)) == -1) {
		logMessage("daemon", LOG_ERROR, "Failed to open /dev/null\n");		
		return -1;
	}

	if ((fd = open("/dev/null", O_WRONLY | O_TRUNC, 0644)) == -1) {
		logMessage("daemon", LOG_ERROR, "Failed to open /dev/null\n");		
		return -1;
	}

	return 0;
}

void sighandler(int signum)
{
	if ((signum == SIGINT) || (signum == SIGHUP) || (signum == SIGTERM)) {
		logMessage("daemon", LOG_INFO, "Caught signal. Shutting down.");
		signal(SIGINT, sighandler);
		signal(SIGHUP, sighandler);
		signal(SIGTERM, sighandler);
		closeServerSocket(serverSocket);
                serverSocket = -1;
	}
}

int signalisation() 
{
	// FIXME: use sigaction(2)
	signal(SIGINT, sighandler);
	signal(SIGHUP, sighandler);
	signal(SIGTERM, sighandler);
	return 0;
}

static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-P | --pidfile       PID file\n"
                 "-L | --logfile       Log file\n"
                 "-p | --port          Port number\n"
                 "-n | --nodaemon      Don't put the applciation in the background\n"
                 "",
                 argv[0]);
}

static void printVersion(FILE* fp)
{
        fprintf (fp,
                 "P2P Food Lab Sensorbox\n"
                 "  Update daemon\n"
                 "  Version %d.%d.%d\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

void parseArguments(int argc, char **argv)
{
        static const char short_options [] = "hvnP:L:p:";

        static const struct option
                long_options [] = {
                { "help",       no_argument, NULL, 'h' },
                { "version",    no_argument, NULL, 'v' },
                { "pidfile",    required_argument, NULL, 'P' },
                { "logfile",    required_argument, NULL, 'L' },
                { "port",       required_argument, NULL, 'p' },
                { "nodaemon",   no_argument, NULL, 'n' },
                { 0, 0, 0, 0 }
        };

        for (;;) {
                int index, c = 0;
                
                c = getopt_long(argc, argv, short_options, long_options, &index);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);
                case 'v':
                        printVersion(stdout);
                        exit(EXIT_SUCCESS);
                case 'P':
                        pidFile = optarg;
                        break;
                case 'L':
                        logFile = optarg;
                        break;
                case 'p':
                        port = atoi(optarg);
                        break;
                case 'n':
                        nodaemon = 1;
                        break;

                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }
}

const char* findCommand(const char* request)
{
        typedef struct _command_t {
                char* header;
                char* cmdline;
        } command_t;

        static command_t commands[] = {
                { "/update/network/wifi", "/var/p2pfoodlab/bin/update-network wifi 2>&1" },
                { "/update/network/wired", "/var/p2pfoodlab/bin/update-network wired 2>&1" },
                { "/update/network/gsm", "/var/p2pfoodlab/bin/update-network gsm 2>&1" },
                { "/update/ssh", "/var/p2pfoodlab/bin/update-ssh 2>&1" },
                { "/update/crontab", "/var/p2pfoodlab/bin/update-crontab 2>&1" },
                { "/test/camera", "/var/p2pfoodlab/bin/test-camera 2>&1" },
                { "/update/sensors", "/var/p2pfoodlab/bin/arduino enable-sensors 2>&1" },
                { "/update/version", "/var/p2pfoodlab/bin/update-version 2>&1" },
                { "/update/poweroff", "/var/p2pfoodlab/bin/update-poweroff 2>&1" },
                { NULL, NULL }
        };

        for (int i = 0; commands[i].header != NULL; i++) {
                if (strcmp(request, commands[i].header) == 0) 
                        return commands[i].cmdline;
                
        }

        return NULL;
}

enum {
        HTTP_UNKNOWN,
        HTTP_GET
};

#define REQ_MAX_ARGS 16

typedef struct _request_t {
        int method;
        char* path;
        int num_args;
        char* names[REQ_MAX_ARGS];
        char* values[REQ_MAX_ARGS];
} request_t;

typedef struct _response_t {
        int status;
        char* body;
        int len;
} response_t;

int parseRequest(int client, request_t* req, response_t* resp)
{
        enum {
                REQ_METHOD,
                REQ_PATH,
                REQ_HTTPVERSION,
                REQ_ARGS_NAME,
                REQ_ARGS_VALUE,
                REQ_REQLINE_LF,
                REQ_HEADER_NAME,
                REQ_HEADER_VALUE,
                REQ_HEADER_LF,
                REQ_HEADERS_END_LF,
                REQ_HEADERS_END,
        };

#define BUFLEN 512
        int state = REQ_METHOD;
        char buffer[BUFLEN];
        int count = 0;
        char c;
        int r;

        while (1) {
                
                if (state == REQ_HEADERS_END)
                        return 0;

                r = read(client, &c, 1);
                if ((r == -1) || (r == 0)) {
                        logMessage("daemon", LOG_WARNING, "Failed to parse the request\n");
                        return -1;
                }
                
                switch (state) {

                case REQ_METHOD: 
                        if (c == ' ') {
                                buffer[count] = 0;
                                if (strcmp(buffer, "GET") == 0)
                                        req->method = HTTP_GET;
                                else
                                        req->method = HTTP_UNKNOWN;
                                count = 0;
                                state = REQ_PATH;

                        } else if ((c == '\r') || (c == '\n')) {
                                logMessage("daemon", LOG_WARNING, "Bad request: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                                
                        } else if (count < BUFLEN-1) {
                                buffer[count++] = (char) c;

                        } else {
                                buffer[BUFLEN-1] = 0;
                                logMessage("daemon", LOG_WARNING, "Request line too long: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        }
                        break;

                case REQ_PATH: 
                        if (c == ' ') {
                                buffer[count++] = 0;
                                req->path = malloc(count);
                                memcpy(req->path, buffer, count);
                                count = 0;
                                state = REQ_HTTPVERSION;

                        } else if (c == '?') {
                                buffer[count++] = 0;
                                req->path = malloc(count);
                                memcpy(req->path, buffer, count);
                                count = 0;
                                state = REQ_ARGS_NAME;

                        } else if ((c == '\r') || (c == '\n')) {
                                logMessage("daemon", LOG_WARNING, "Bad request: %s\n", buffer);
                                resp->status = 400;
                                return -1;

                        } else if (count < BUFLEN-1) {
                                buffer[count++] = (char) c;
                        } else {
                                buffer[BUFLEN-1] = 0;
                                logMessage("daemon", LOG_WARNING, "Requested path too long: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        }
                        break;

                case REQ_ARGS_NAME: 
                        if (c == ' ') {
                                buffer[count++] = 0;
                                if (req->num_args >= REQ_MAX_ARGS) {
                                        logMessage("daemon", LOG_WARNING, "Too many arguments: %d\n", REQ_MAX_ARGS);
                                        resp->status = 400;
                                        return -1;
                                }
                                req->names[req->num_args++] = strdup(buffer);
                                count = 0;
                                state = REQ_HTTPVERSION;
                                
                        } else if (c == '=') {
                                buffer[count++] = 0;
                                if (req->num_args >= REQ_MAX_ARGS) {
                                        logMessage("daemon", LOG_WARNING, "Too many arguments: %d\n", REQ_MAX_ARGS);
                                        resp->status = 400;
                                        return -1;
                                }
                                req->names[req->num_args] = strdup(buffer);
                                count = 0;
                                state = REQ_ARGS_VALUE;

                        } else if (c == '&') {
                                buffer[count++] = 0;
                                if (req->num_args >= REQ_MAX_ARGS) {
                                        logMessage("daemon", LOG_WARNING, "Too many arguments: %d\n", REQ_MAX_ARGS);
                                        resp->status = 400;
                                        return -1;
                                }
                                req->names[req->num_args++] = strdup(buffer);
                                count = 0;
                                state = REQ_ARGS_NAME;

                        } else if ((c == '\r') || (c == '\n')) {
                                logMessage("daemon", LOG_WARNING, "Bad request: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        } else if (count < BUFLEN-1) {
                                buffer[count++] = (char) c;
                        } else {
                                buffer[BUFLEN-1] = 0;
                                logMessage("daemon", LOG_WARNING, "Requested path too long: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        }
                        break;

                case REQ_ARGS_VALUE: 
                        if (c == ' ') {
                                buffer[count++] = 0;
                                req->values[req->num_args++] = strdup(buffer);
                                count = 0;
                                state = REQ_HTTPVERSION;
                                
                        } else if (c == '&') {
                                buffer[count++] = 0;
                                req->values[req->num_args++] = strdup(buffer);
                                count = 0;
                                state = REQ_ARGS_NAME;

                        } else if ((c == '\r') || (c == '\n')) {
                                logMessage("daemon", LOG_WARNING, "Bad request: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        } else if (count < BUFLEN-1) {
                                buffer[count++] = (char) c;
                        } else {
                                buffer[BUFLEN-1] = 0;
                                logMessage("daemon", LOG_WARNING, "Requested path too long: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        }
                        break;

                case REQ_HTTPVERSION: 
                        if (c == '\n') {
                                buffer[count] = 0;
                                // check HTTP version? Not needed in this app...
                                count = 0;
                                state = REQ_HEADER_NAME;
                        } else if (c == '\r') {
                                buffer[count] = 0;
                                // check HTTP version? Not needed in this app...
                                count = 0;
                                state = REQ_REQLINE_LF;
                        } else if (strchr("HTTP/1.0", c) == NULL) {
                                logMessage("daemon", LOG_WARNING, "Invalid HTTP version string\n");
                                resp->status = 400;
                                return -1;
                        } else {
                                buffer[count++] = (char) c;
                        }
                        break;

                case REQ_REQLINE_LF: 
                        if (c != '\n') {
                                logMessage("daemon", LOG_WARNING, "Invalid HTTP version string\n");
                                resp->status = 400;
                                return -1;
                        } else {
                                count = 0;
                                state = REQ_HEADER_NAME;
                        }
                        break;

                case REQ_HEADER_NAME: 
                        if ((c == '\n') && (count == 0)) {
                                state = REQ_HEADERS_END;
                                
                        } else if ((c == '\r') && (count == 0)) {
                                state = REQ_HEADERS_END_LF;
                                
                        } else if (c == ':') {
                                buffer[count++] = 0;
                                //logMessage("daemon", LOG_INFO, "Header '%s'\n", buffer);
                                count = 0;
                                state = REQ_HEADER_VALUE;
                        } else if (count < BUFLEN-1) {
                                buffer[count++] = (char) c;
                        } else {
                                buffer[BUFLEN-1] = 0;
                                logMessage("daemon", LOG_WARNING, "Header name too long: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        }
                        break;

                case REQ_HEADER_VALUE: 
                        if (c == '\r') {
                                buffer[count++] = 0;
                                //logMessage("daemon", LOG_, "Header value: %s\n", buffer);
                                count = 0;
                                state = REQ_HEADER_LF;
                        } else if (c == '\n') {
                                buffer[count++] = 0;
                                //logMessage("daemon", LOG_, "Header value: %s\n", buffer);
                                count = 0;
                                state = REQ_HEADER_NAME;
                        } else if (count < BUFLEN-1) {
                                buffer[count++] = (char) c;
                        } else {
                                buffer[BUFLEN-1] = 0;
                                logMessage("daemon", LOG_WARNING, "Header value too long: %s\n", buffer);
                                resp->status = 400;
                                return -1;
                        }
                        break;
 
                case REQ_HEADER_LF: 
                        if (c != '\n') {
                                logMessage("daemon", LOG_WARNING, "Invalid HTTP header\n");
                                resp->status = 400;
                                return -1;
                        } else {
                                count = 0;
                                state = REQ_HEADER_NAME;
                        }
                        break;

                case REQ_HEADERS_END_LF: 
                        if (c != '\n') {
                                logMessage("daemon", LOG_WARNING, "Invalid HTTP header\n");
                                resp->status = 400;
                                return -1;
                        } else {
                                count = 0;
                                state = REQ_HEADERS_END;
                        }
                        break;
                }
       }
}

int main(int argc, char **argv)
{
        int err;

        parseArguments(argc, argv);

        if (nodaemon == 0) {
                err = daemonise();
                if (err != 0) exit(1);

                err = writePidFile(pidFile);
                if (err != 0) exit(1);
        }

        err = signalisation();
        if (err != 0) exit(1);

        serverSocket = openServerSocket(port);
        if (serverSocket == -1) exit(1);

        request_t req;
        response_t resp;
        output_t output;

        memset(&output, 0, sizeof(output_t));
                
        while (serverSocket != -1) {

                memset(&req, 0, sizeof(request_t));
                memset(&resp, 0, sizeof(response_t));
        
                int client = serverSocketAccept(serverSocket);
                if (client == -1)
                        continue;

                
                int r = parseRequest(client, &req, &resp);
                if (r != 0) {
                        clientPrintf(client, "HTTP/1.1 %03d\r\n", resp.status);
                        closeClient(client);
                        continue;
                }

                if (1) {
                        printf("path: %s\n", req.path);
                        for (int i =0; i < req.num_args; i++)
                                printf("arg[%d]: %s = %s\n", i, req.names[i], req.values[i]);
                }

                const char* cmdline = findCommand(req.path);

                if (cmdline == NULL) {
                        logMessage("daemon", LOG_WARNING, "Invalid path: '%s'\n", req.path);
                        clientPrint(client, "HTTP/1.1 404\r\n");
                        closeClient(client);
                        continue;
                }

                if (execute(&output, cmdline) != 0) {
                        clientPrint(client, "HTTP/1.1 500\r\n"); // Internal server error
                        closeClient(client);
                        continue;
                }

                if ((output.count > 8) && (strncmp(output.buf, "HTTP/1.1", 8) == 0)) {
                        clientWrite(client, output.buf, output.count);
                } else {
                        clientPrintf(client, "HTTP/1.1 200\r\nContent-Length: %d\r\n\r\n", 
                                     output.count);
                        clientWrite(client, output.buf, output.count);
                }

                closeClient(client);
                
                output_clear(&output);
                if (req.path) free(req.path);
        }

        removePidFile(pidFile); 

        return 0;
}

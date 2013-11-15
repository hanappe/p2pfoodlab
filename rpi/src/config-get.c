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
#include <stdarg.h>
#include "json.h"
#include "logMessage.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

static char* configFile = "/var/p2pfoodlab/etc/config.json";

static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options] variable-name\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-c | --config        Configuration file\n"
                 "",
                 argv[0]);
}

static void printVersion(FILE* fp)
{
        fprintf (fp,
                 "P2P Food Lab Sensorbox\n"
                 "  config-get\n"
                 "  Version %d.%d.%d\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

void parseArguments(int argc, char **argv)
{
        static const char short_options [] = "hvc:";

        static const struct option
                long_options [] = {
                { "help",        no_argument, NULL, 'h' },
                { "version",     no_argument, NULL, 'v' },
                { "config",      required_argument, NULL, 'c' },
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
                case 'c':
                        configFile = optarg;
                        break;
                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }
}

int main(int argc, char **argv)
{
        char buffer[512];

        //setLogFile(stderr);

        parseArguments(argc, argv);

        const char* path = argv[optind];
        if (optind >= argc) {
                logMessage("config-get", LOG_ERROR, "missing path\n"); 
                exit(1);
        }

        json_object_t config = json_load(configFile, buffer, 512);
        if (json_isnull(config)) {
                logMessage("config-get", LOG_ERROR, "%s\n", buffer); 
                exit(1);
        } 

        int len = strlen(path);
        int n = 0;
        json_object_t obj = config;

        for (int i = 0; i <= len; i++) {
                if (n >= 512) {
                        logMessage("config-get", LOG_ERROR, "Variable name too long\n"); 
                        exit(1);                                
                }
                if ((path[i] == '.') || (i == len)) {
                        buffer[n] = 0;
                        if (n == 0) {
                                logMessage("config-get", LOG_ERROR, "Empty variable name\n"); 
                                exit(1);                                
                        }
                        if (!json_isobject(obj)) {
                                logMessage("config-get", LOG_ERROR, "Looking for variable '%s' in a non-object\n", buffer); 
                                exit(1);
                        }
                        obj = json_object_get(obj, buffer);
                        if (json_isnull(obj)) {
                                logMessage("config-get", LOG_ERROR, "Could not find the variable '%s'\n", buffer); 
                                exit(1);
                        }

                        if (i == len) {
                                if (json_isstring(obj)) {
                                        fprintf(stdout, "%s", json_string_value(obj));
                                } else {
                                        json_tofilep(obj, 0, stdout);
                                }
                                exit(0);
                        }

                        n = 0;
                } else {
                        buffer[n++] = path[i];
                }
        }

        return 0;
}

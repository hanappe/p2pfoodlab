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
#include <unistd.h>
#include <stdarg.h>
#include <getopt.h>
#include <dirent.h> 
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "log_message.h"
#include "sensorbox.h"

static char* _home_dir = "/var/p2pfoodlab";
static char* _log_file = "/var/p2pfoodlab/log.txt";
static int _test = 0;

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-d | --d             Directory (default: /var/p2pfoodlab))\n"
                 "-l | --log           Log file ('-' for stderr, default: /var/p2pfoodlab/log.txt)\n"
                 "-t | --test          Test run\n"
                 "-D | --debug         Print debug message\n"
                  "",
                 argv[0]);
}

static void print_version(FILE* fp)
{
        fprintf (fp,
                 "P2P Food Lab Sensorbox\n"
                 "  update\n"
                 "  Version %d.%d.%d\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

static void parse_arguments(int argc, char **argv)
{
        static const char short_options [] = "hvtDd:l:";

        static const struct option
                long_options [] = {
                { "help",        no_argument, NULL, 'h' },
                { "version",     no_argument, NULL, 'v' },
                { "directory",   required_argument, NULL, 'd' },
                { "log",         required_argument, NULL, 'l' },
                { "test",        no_argument, NULL, 't' },
                { "debug",       no_argument, NULL, 'D' },
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
                        print_version(stdout);
                        exit(EXIT_SUCCESS);
                case 't':
                        _test = 1;
                        break;
                case 'D':
                        log_set_level(LOG_DEBUG);
                        break;
                case 'd':
                        _home_dir = optarg;
                        break;
                case 'l':
                        _log_file = optarg;
                        break;
                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }
}

static void init_log(const char* file)
{
        FILE* log = NULL;
        if (strcmp(file, "-") == 0) {
                log = stderr;
        } else {
                log = fopen(file, "a");
        }
        if (log == NULL) {
                fprintf(stderr, "Failed to open the log file ('%s')", file);
                log_set_filep(stderr);
        } else {
                log_set_filep(log);
        }
}

int main(int argc, char **argv)
{
        parse_arguments(argc, argv);

        init_log(_log_file);

        sensorbox_t* box = new_sensorbox(_home_dir);
        if (box == NULL) 
                exit(1);

        sensorbox_handle_events(box);
        sensorbox_upload_data(box);
        sensorbox_upload_photos(box);
        sensorbox_poweroff_maybe(box);

        delete_sensorbox(box);

        return 0;
}
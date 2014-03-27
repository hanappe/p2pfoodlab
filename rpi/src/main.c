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
#include "clock.h"
#include "sensorbox.h"

static char* _home_dir = "/var/p2pfoodlab";
static char* _log_file = "/var/p2pfoodlab/log.txt";
static char* _output_file = NULL;
static int _test = 0;

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options] [command]\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-d | --d             Directory (default: /var/p2pfoodlab))\n"
                 "-l | --log           Log file ('-' for stderr, default: /var/p2pfoodlab/log.txt)\n"
                 "-t | --test          Test run\n"
                 "-o | --output-file   Where to write the data/image to ['-' for console]\n"
                 "-D | --debug         Print debug message\n"
                 "Commands:\n"
                 "  update             Parse the config, create the OpenSensorData definitions if needed,\n"
                 "                     update sensors and/or camera if necessary,\n"
                 "                     upload data and photos [default]\n"
                 "  store-data         Store the latest sensor values\n"
                 "  upload-data        Upload the datapoints\n"
                 "  camera             Grab a photo\n"
                 "  upload-photos      Upload the photos on the disk\n"
                 "  get-time           Get the current time on the arduino\n"
                 "  set-time           Set the time on the arduino\n"
                 "  list-events        Set the time on the arduino\n"
                 "  ifup               Bring the network inerface up\n"
                 "  ifdown             Bring the network inerface down\n"
                 "  osd                Create the OpenSensorData definitions\n"
                 "  measure            Measure and print sensor values\n"
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
        static const char short_options [] = "hvtDd:l:o:";

        static const struct option
                long_options [] = {
                { "help",        no_argument, NULL, 'h' },
                { "version",     no_argument, NULL, 'v' },
                { "directory",   required_argument, NULL, 'd' },
                { "log",         required_argument, NULL, 'l' },
                { "test",        no_argument, NULL, 't' },
                { "debug",       no_argument, NULL, 'D' },
                { "output-file", required_argument, NULL, 'o' },
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
                case 'o':
                        _output_file = optarg;
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
        char* command = "update";

        parse_arguments(argc, argv);

        if (optind < argc) 
                command = argv[optind++];

        init_log(_log_file);

        sensorbox_t* box = new_sensorbox(_home_dir);
        if (box == NULL) 
                exit(1);

        if (sensorbox_lock(box) != 0) {
                log_warn("Another process is already active. Exiting.");
                delete_sensorbox(box);
                exit(1);
        } 

        if (_test) 
                sensorbox_test_run(box);

        if (strcmp(command, "update") == 0) {
                
                /* Check whether the time needs to be set. */
                time_t t;
                int r = sensorbox_get_time(box, &t);
                if (r != 0) {
                        log_warn("Failed to get Arduino's current time.");
                } else if (t < 1395332000L) {
                        /* The time on the Arduino has not been
                           set. Run NTP and pass the correct date to
                           Arduino. */
                        if (sensorbox_run_ntp(box) == 0)
                                sensorbox_set_time(box, time(NULL)); 
                }

                /* Check whether the definitions of the datastream are
                   up-to-date. */
                if (sensorbox_check_osd_definitions(box) != 0) {
                        if (sensorbox_bring_network_up(box) == 0)
                                sensorbox_create_osd_definitions(box);
                }

                /* Check whether the active sensors on the Arduino are
                   up-to-date. */
                sensorbox_check_sensors(box);

                /* Handle all data and camera events. */
                sensorbox_handle_events(box);

                /* Upload what ever data we have. */
                sensorbox_upload_data(box);
                sensorbox_upload_photos(box);

                /* Bring the network down, if not needed (particularly
                   GSM). */
                sensorbox_bring_network_down_maybe(box);

                /* Power of the RPi if in energy saving mode. */
                sensorbox_poweroff_maybe(box);

                //clock_update(box);

        } else if (strcmp(command, "upload-data") == 0) {
                sensorbox_upload_data(box);

        } else if (strcmp(command, "upload-photos") == 0) {
                sensorbox_upload_photos(box);

        } else if (strcmp(command, "list-events") == 0) {
                sensorbox_print_events(box);

        } else if (strcmp(command, "grab-image") == 0) {
                sensorbox_grab_image(box, _output_file);

        } else if (strcmp(command, "camera") == 0) {
                time_t t;
                if (sensorbox_get_time(box, &t) == 0)
                        sensorbox_update_camera(box, t);

        } else if (strcmp(command, "store-data") == 0) {
                sensorbox_store_sensor_data(box, _output_file);

        } else if (strcmp(command, "update-clock") == 0) {
                clock_update(box);

        } else if (strcmp(command, "set-time") == 0) {
                //clock_set(box);
                time_t m = time(NULL);
                sensorbox_set_time(box, m); 

        } else if (strcmp(command, "get-time") == 0) {
                time_t m;
                int err = sensorbox_get_time(box, &m); 
                if (!err) 
                        printf("%lu\n", (unsigned long) m);
                
        } else if (strcmp(command, "ifup") == 0) {
                sensorbox_bring_network_up(box);

        } else if (strcmp(command, "ifdown") == 0) {
                sensorbox_bring_network_down(box);

        } else if (strcmp(command, "osd") == 0) {
                sensorbox_create_osd_definitions(box);

        } else if (strcmp(command, "measure") == 0) {
                sensorbox_measure(box);

        /* } else if (strcmp(command, "status") == 0) { */
        /*         status_t status; */
        /*         sensorbox_get_status(box, &status); */

        } else if (strcmp(command, "config-set") == 0) {
                const char* s = sensorbox_config_getstr(box, "general.name");
                printf("%s\n", s);

        } else {
                usage(stderr, argc, argv);
        }

        sensorbox_unlock(box);
        delete_sensorbox(box);

        exit(0);
}

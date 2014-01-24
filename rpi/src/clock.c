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
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "arduino.h"
#include "log_message.h"
#include "sensorbox.h"
#include "network.h"
#include "clock.h"

static int clock_get_millis(sensorbox_t* box, unsigned long* m)
{
        unsigned long millis, test;

        if (sensorbox_millis(box, &millis) != 0)
                return -1;

        for (int i = 0; i < 3; i++) {
                if (sensorbox_millis(box, &test) != 0)
                        return -1;
                if (test < millis)
                        return -1;
                if (test > millis + 10000)
                        return -1;
        }

        *m = millis;
        return 0;
}

int clock_update(sensorbox_t* box)
{
        unsigned long millis;
        char filename[512];
        struct timespec tp;
        time_t epoch;
        FILE* fp;
        int err;

        if (clock_get_millis(box, &millis) != 0)
                return -1;

        snprintf(filename, 511, "%s/etc/arduino.millis", sensorbox_getdir(box));
        filename[511] = 0;

        fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Clock: Failed to open file (%s)", filename);
                return -1;
        }
        fprintf(fp, "%lu\n", millis);
        fclose(fp);

        err = clock_gettime(CLOCK_REALTIME, &tp);
        if (err != 0) {
                log_err("Clock: Failed to get the current time");
                return -1;
        }
        epoch = tp.tv_sec;

        snprintf(filename, 511, "%s/etc/system.epoch", sensorbox_getdir(box));
        filename[511] = 0;

        fp = fopen(filename, "w");
        if (fp == NULL) {
                log_err("Clock: Failed to open file (%s)", filename);
                return -1;
        }
        fprintf(fp, "%lu\n", epoch);
        fclose(fp);

        return 0;
}

int clock_set(sensorbox_t* box)
{
        unsigned long cur_millis;
        unsigned long saved_millis;
        time_t cur_epoch;
        time_t saved_epoch;
        time_t epoch;
        unsigned long cur_seconds;
        unsigned long saved_seconds;
        struct timespec tp;
        char filename[512];
        FILE* fp;
        int n;
        int err;

        log_info("Clock: Setting system time.");

        if (network_connected() == 1) {
                log_info("Clock: Connected to network. Let ntpd do its job.");
                return 0;
        }

        /* Get saved millis */
        snprintf(filename, 511, "%s/etc/arduino.millis", sensorbox_getdir(box));
        filename[511] = 0;

        fp = fopen(filename, "r");
        if (fp == NULL) {
                log_err("Clock: Failed to open file (%s)", filename);
                return -1;
        }
        n = fscanf(fp, "%lu\n", &saved_millis);
        if (n != 1) {
                log_err("Clock: Failed to read the saved millis values (%s)", filename);
                return -1;
        }
        fclose(fp);

        /* Get the corresponding saved epoch */
        snprintf(filename, 511, "%s/etc/system.epoch", sensorbox_getdir(box));
        filename[511] = 0;

        fp = fopen(filename, "r");
        if (fp == NULL) {
                log_err("Clock: Failed to open file (%s)", filename);
                return -1;
        }
        n = fscanf(fp, "%lu\n", &saved_epoch);
        if (n != 1) {
                log_err("Clock: Failed to read the saved epoch values (%s)", filename);
                return -1;
        }
        fclose(fp);


        /* Get current millis */
        if (clock_get_millis(box, &cur_millis) != 0)
                return -1;

        /* Estimate current time */
        cur_seconds = (cur_millis + 500) / 1000;
        saved_seconds = (saved_millis + 500) / 1000;

        /* Arduino's clock may have wrapped around. */
        if (cur_seconds < saved_seconds)
                cur_seconds += 4294967; // 2^32 milliseconds = 4294967.296 seconds

        epoch = saved_epoch + (cur_seconds - saved_seconds); 


        /* Get current epoch */
        err = clock_gettime(CLOCK_REALTIME, &tp);
        if (err != 0) {
                log_err("Clock: Failed to get the current time");
                return -1;
        }
        cur_epoch = tp.tv_sec;

        /* Don't change the time is the difference is less than 30 seconds */
        if ((epoch >= cur_epoch) && (epoch - cur_epoch < 30)) {
                log_err("Clock: Not changing current time: difference is %lu seconds", epoch - cur_epoch);
                return 0;
        }
        if ((epoch < cur_epoch) && (cur_epoch - epoch  < 30)) {
                log_err("Clock: Not changing current time: difference is %lu seconds", cur_epoch - epoch);
                return 0;
        }

        log_info("Clock: Saved Arduino time:    %lu sec", saved_seconds);
        log_info("Clock: Current Arduino time:  %lu sec", cur_seconds);
        log_info("Clock: Saved system time:     %lu sec", saved_epoch);
        log_info("Clock: Current system time:   %lu sec", cur_epoch);
        log_info("Clock: Estimated system time: %lu sec", epoch);

        /* Update system time */
        tp.tv_sec = epoch;
        tp.tv_nsec = 0;

        err = clock_settime(CLOCK_REALTIME, &tp);
        if (err != 0) {
                log_err("Clock: Failed to set time: %s", strerror(errno));
                return err;
        }

        return 0;
}



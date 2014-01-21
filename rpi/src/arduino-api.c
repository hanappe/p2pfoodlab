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
#include <time.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <getopt.h>
#include "json.h"
#include "config.h"
#include "log_message.h"
#include "opensensordata.h"
#include "arduino-api.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define CMD_SET_POWEROFF 0x00
#define CMD_SET_SENSORS 0x10
#define CMD_SUSPEND 0x20
#define CMD_RESUME 0x30
#define CMD_GET_SENSORS 0x40
#define CMD_GET_MILLIS 0x50
#define CMD_GET_FRAMES 0x60
#define CMD_TRANSFER 0x70
#define CMD_PUMP 0x80
#define CMD_GET_POWEROFF 0x90

static int bus = 1;
static int address = 0x04;

static int arduino_connect(int bus, int address)
{
        int arduino = -1;
        char *fileName = (bus == 0)? "/dev/i2c-0" : "/dev/i2c-1";
 
        log_info("Connecting to Arduino"); 

        if ((arduino = open(fileName, O_RDWR)) < 0) {
                log_err("Failed to open the I2C device"); 
                return -1;
        }

        if (ioctl(arduino, I2C_SLAVE, address) < 0) {
                log_err("Unable to get bus access to talk to slave");
                return -1;
        }

        usleep(10000);
        return arduino;
}

static void arduino_disconnect(int arduino)
{
        log_info("Disconnecting from Arduino"); 
        if (arduino > 0)
                close(arduino);
}

static int arduino_suspend_(int arduino)
{
        log_info("Sending start transfer"); 

        if (i2c_smbus_write_byte(arduino, CMD_SUSPEND) == -1) {
                log_err("Failed to send the 'start transfer' command");
                return -1;
        }
        usleep(10000);
        return 0;
}

static int arduino_resume_(int arduino, int reset)
{
        if (reset) 
                log_info("Clearing the memory and resuming measurements"); 
        else
                log_info("Resuming measurements"); 

        unsigned char c = CMD_RESUME;
        if (reset) c |= 0x01;

        if (i2c_smbus_write_byte(arduino, c) == -1) {
                log_err("Failed to send the 'transfer done' command. This is not good... You may have to restart the device.");
                return -1;
        }
        usleep(10000);
        return 0;
}

static int arduino_read_float(int arduino, float* value)
{

        /* Read the four bytes of the floating point value. We do four
           separate reads of one byte because this is the only call
           that seems to work successfully with the Arduino. */
        unsigned char c[4];
        for (int i = 0; i < 4; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
		usleep(10000);
        }

        unsigned int r = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | (c[3] << 0);
        *value = *((float*) &r);

        return 0;
}

static int arduino_set_poweroff_(int arduino, int minutes)
{
        unsigned char c[3];

        c[0] = CMD_SET_POWEROFF;
        c[1] = (minutes & 0x0000ff00) >> 8;
        c[2] = (minutes & 0x000000ff);

        int n = write(arduino, c, 3);
        if (n != 3) {
                log_err("Failed to write the data"); 
                return -1;
        }
        usleep(10000);

        return 0;
}

static int arduino_get_poweroff(int arduino, unsigned long* off, unsigned long* on)
{
        unsigned char c[4];

        c[0] = CMD_GET_POWEROFF;

        int n = write(arduino, c, 1);
        if (n != 1) {
                log_err("Failed to write the data"); 
                return -1;
        }
        usleep(10000);

        for (int i = 0; i < 4; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        log_err("Failed to read the 'poweroff' value");
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        *off = (unsigned long) (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | (c[3] << 0);

        for (int i = 0; i < 4; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        log_err("Failed to read the 'wakeup' value");
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        *on = (unsigned long) (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | (c[3] << 0);

        return 0;
}

static int arduino_pump_(int arduino, int seconds)
{
        unsigned char c;
        int err;

        log_info("Turning pump on"); 
        c = CMD_PUMP | 0x01;

        for (int attempt = 0; attempt < 4; attempt++) {
                err = i2c_smbus_write_byte(arduino, c);
                if (err == 0) break;
                log_err("Failed to send the 'pump on' command.");
                usleep(2000);
        }
        if (err != 0) return -1;

        sleep(seconds);

        log_info("Turning pump off"); 
        c = CMD_PUMP;

        for (int attempt = 0; attempt < 20; attempt++) {
                err = i2c_smbus_write_byte(arduino, c);
                if (err == 0) break;
                log_err("Failed to send the 'pump off' command. "
                        "This is not good... You may have to restart the device.");
                usleep(2000);
        }
        if (err != 0) return -1;

        usleep(10000);

        return 0;
}

static int arduino_millis_(int arduino, unsigned long* m)
{
        log_info("Getting the current time on the Arduino"); 

        if (i2c_smbus_write_byte(arduino, CMD_GET_MILLIS) == -1) {
                log_err("Failed to send the 'get millis' command");
                return -1;
        }
        usleep(10000);

        unsigned char c[4];
        for (int i = 0; i < 4; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        log_err("Failed to read the 'millis' value");
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        *m = (unsigned long) (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | (c[3] << 0);

        return 0;
}

static int arduino_get_frames(int arduino)
{
        log_info("Getting the number of data frames on the Arduino"); 

        if (i2c_smbus_write_byte(arduino, CMD_GET_FRAMES) == -1) {
                log_err("Failed to send the 'get count' command");
                return -1;
        }
        usleep(10000);

        unsigned char c[2];
        for (int i = 0; i < 2; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        log_err("Failed to read the 'frames' value");
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        return (int) (c[0] << 8) | (c[1] << 0);
}

static int arduino_get_sensors_(int arduino, unsigned char* enabled, unsigned char* period)
{
        log_info("Getting enabled sensors and update period"); 

        if (i2c_smbus_write_byte(arduino, CMD_GET_SENSORS) == -1) {
                log_err("Failed to send the 'get sensors' command");
                return -1;
        }
        usleep(10000);

        int v = i2c_smbus_read_byte(arduino);
        if (v == -1) {
                log_err("Failed to read the 'get sensors' value");
                return -1;
        }
        unsigned char _enabled = (v & 0x000000ff);
        *enabled = _enabled;
        usleep(10000);

        v = i2c_smbus_read_byte(arduino);
        if (v == -1) {
                log_err("Failed to read the 'get sensors' value");
                return -1;
        }
        unsigned char _period = (v & 0x000000ff);
        *period = _period;
        usleep(10000);

        log_info("Enabled sensors: %x; Period: %d\n", (int) _enabled, (int) _period); 

        return 0;
}

static int arduino_set_sensors_(int arduino, unsigned char enabled, unsigned char period)
{
        unsigned char c[3];

        c[0] = CMD_SET_SENSORS;
        c[1] = enabled;
        c[2] = period;

        int n = write(arduino, c, 3);
        if (n != 3) {
                log_err("Failed to write the 'set sensors' command"); 
                return -1;
        }
        usleep(10000);

        return 0;
}

static int download_data(int arduino, 
                         time_t delta, 
                         int* datastreams, 
                         int numDatastreams, 
                         int numFrames,
                         const char* filename)
{
        int err = -1;

        FILE* fp = fopen(filename, "a");
        if (fp == NULL) {
                log_err("Failed to open the output file: %s\n", filename); 
                return -1;
        }

        if (i2c_smbus_write_byte(arduino, CMD_TRANSFER) == -1) {
                log_err("Failed to send the 'transfer' command");
                goto error_recovery;
        }
        usleep(10000);

        float value;
                
        for (int frame = 0; frame < numFrames; frame++) {
                
                if (arduino_read_float(arduino, &value) != 0) 
                        goto error_recovery;

                time_t timestamp = (time_t) value + delta;
                struct tm r;
                char time[256];

                localtime_r(&timestamp, &r);
                snprintf(time, 256, "%04d-%02d-%02dT%02d:%02d:%02d",
                         1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, r.tm_hour, r.tm_min, r.tm_sec);
                
                for (int i = 0; i < numDatastreams; i++) {
                        if (arduino_read_float(arduino, &value) != 0) 
                                goto error_recovery;
                        if (datastreams[i] > 0)
                                fprintf(fp, "%d,%s,%f\n", datastreams[i], time, value);
                }
        }

        err = 0;

 error_recovery:
        fclose(fp);
        return err;
}

int arduino_store_data(int* datastreams,
                       int numDatastreams,
                       const char* outputFile)
{
        int arduino =  -1; 
        int err = -1; 

        arduino = arduino_connect(bus, address);
        if (arduino < 0) {
                return -1;
        }

        if (arduino_suspend_(arduino) != 0) 
                goto error_recovery;

        time_t now = time(NULL);
        unsigned long millis;
        if (arduino_millis_(arduino, &millis) != 0) {
                arduino_resume_(arduino, 0);
                goto error_recovery;
        }
        log_info("Current time on the arduino: %u msec\n", millis); 

        time_t delta = now - ((millis + 500) / 1000);

        unsigned char enabled;
        unsigned char period;

        int v = arduino_get_sensors_(arduino, &enabled, &period);
        if (v == -1) {
                arduino_resume_(arduino, 0);
                goto error_recovery;
        }

        int numFrames = arduino_get_frames(arduino);
        if (numFrames == -1) {
                arduino_resume_(arduino, 0);
                goto error_recovery;
        }
        log_info("Found %d measurement frames\n", numFrames); 
        
        for (int attempt = 0; attempt < 4; attempt++) {
                err = download_data(arduino, 
                                    delta, 
                                    datastreams, 
                                    numDatastreams, 
                                    numFrames,
                                    outputFile);
                        if (err == 0) break;
        }

        if (err != 0) {
                arduino_resume_(arduino, 0);
                goto error_recovery;
        }

        if (arduino_resume_(arduino, 1) != 0) 
                goto error_recovery;

        log_info("Download successful"); 
        err = 0; 

 error_recovery:

        arduino_disconnect(arduino);
        return err;
}

int arduino_set_sensors(unsigned char enabled, unsigned char period)
{
        int arduino =  -1; 
        int err = -1;

        for (int attempt = 0; attempt < 5; attempt++) {
                arduino = arduino_connect(bus, address);
                if (arduino < 0) continue;

                if (arduino_set_sensors_(arduino, enabled, period) != 0) {
                        arduino_disconnect(arduino);
                        continue;
                }

                arduino_disconnect(arduino);
                err = 0;
                break;
        }

        return err;
}

int arduino_get_sensors(unsigned char* enabled, unsigned char* period)
{
        int arduino =  -1; 
        int err = -1;

        for (int attempt = 0; attempt < 5; attempt++) {
                arduino = arduino_connect(bus, address);
                if (arduino < 0) continue;

                if (arduino_get_sensors_(arduino, enabled, period) != 0) {
                        arduino_disconnect(arduino);
                        continue;
                }

                arduino_disconnect(arduino);
                err = 0;
                break;
        }

        return err;
}

int arduino_set_poweroff(int minutes)
{
        int arduino =  -1; 
        int err = -1;
                
        for (int attempt = 0; attempt < 5; attempt++) {
                
                arduino = arduino_connect(bus, address);
                if (arduino < 0) {
                        continue;
                }

                if (arduino_set_poweroff_(arduino, minutes) != 0) {
                        arduino_disconnect(arduino);
                        continue;
                }

                unsigned long millis, off, on;
                if (arduino_millis_(arduino, &millis) != 0) {
                        arduino_disconnect(arduino);
                        continue;
                }
                if (arduino_get_poweroff(arduino, &off, &on) != 0) {
                        arduino_disconnect(arduino);
                        continue;
                }

                log_info("Arduino: Current time: %u min\n", millis / 60000);
                log_info("Arduino: Power off at: %u min\n", off); 
                log_info("Arduino: Wake up at: %u min\n", on); 
                
                arduino_disconnect(arduino);
                err = 0;
                break;
        }

        return err;
}

int arduino_pump(int seconds)
{
        int arduino =  -1; 

        arduino = arduino_connect(bus, address);
        if (arduino < 0) {
                return -1;
        }
        
        if (arduino_pump_(arduino, seconds) != 0) {
                arduino_disconnect(arduino);
                return -1;
        }
                
        arduino_disconnect(arduino);

        return 0;
}

int arduino_millis(unsigned long* m)
{
        int arduino =  -1; 
        int err = -1;
                
        for (int attempt = 0; attempt < 5; attempt++) {
                
                arduino = arduino_connect(bus, address);
                if (arduino < 0) {
                        return -1;
                }

                if (arduino_millis_(arduino, m) != 0) {
                        arduino_disconnect(arduino);
                        return -1;
                }

                arduino_disconnect(arduino);
                err = 0;
                break;
        }

        return err;
}


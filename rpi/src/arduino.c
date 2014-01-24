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
#include "arduino.h"

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

typedef struct _acmd_t acmd_t;

struct _acmd_t {
        char* name;
        int sn;
        unsigned char s[8];
        int rn;
        unsigned char r[8];
};

struct _arduino_t {
        int bus;
        int address;
        int fd;
};

static int arduino_connect(arduino_t* arduino);

arduino_t* new_arduino(int bus, int address)
{
        arduino_t* arduino = (arduino_t*) malloc(sizeof(arduino_t));
        if (arduino == NULL) { 
                log_err("Arduino: out of memory");
                return NULL;
        }
        memset(arduino, 0, sizeof(arduino_t));

        arduino->bus = bus;
        arduino->address = address;
        arduino->fd = -1;

        return arduino;
}

int delete_arduino(arduino_t* arduino)
{
        free(arduino);
        return 0;
}

static int arduino_connect(arduino_t* arduino)
{
        char *fileName = (arduino->bus == 0)? "/dev/i2c-0" : "/dev/i2c-1";
 
        log_debug("Arduino: Connecting"); 

        if ((arduino->fd = open(fileName, O_RDWR)) < 0) {
                log_err("Arduino: Failed to open the I2C device"); 
                arduino->fd = -1;
                return -1;
        }
        
        if (ioctl(arduino->fd, I2C_SLAVE, arduino->address) < 0) {
                log_err("Arduino: Unable to get bus access to talk to slave");
                close(arduino->fd);
                arduino->fd = -1;
                return -1;
        }

        usleep(10000);
        return 0;
}

static int arduino_disconnect(arduino_t* arduino)
{
        log_debug("Arduino: Disconnecting"); 
        if (arduino->fd >= 0)
                close(arduino->fd);
        arduino->fd = -1;
        return 0;
}

static int arduino_send_(arduino_t* arduino, acmd_t* cmd)
{
        int sent = write(arduino->fd, cmd->s, cmd->sn);
        if (sent != cmd->sn) {
                log_err("Arduino: %s: Failed to write the data", cmd->name); 
                return -1;
        }
        usleep(10000);

        for (int i = 0; i < cmd->rn; i++) {
                int v = i2c_smbus_read_byte(arduino->fd);
                log_debug("Arduino: Read[%d]=%d", i, v);
                if (v == -1) {
                        log_err("Arduino: %s: Failed to read the return value", cmd->name);
                        return -1;
                }
                cmd->r[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        return 0;
}

static int arduino_repeat_send_(arduino_t* arduino, acmd_t* cmd, int attempts)
{
        int err = -1;
        for (int attempt = 0; attempt < attempts; attempt++) {
                err = arduino_send_(arduino, cmd);
                if (err == 0) 
                        break;
        }
        return err;
}

static int arduino_suspend_(arduino_t* arduino)
{
        log_debug("Arduino: Sending start transfer"); 
        acmd_t cmd = { "suspend", 1, { CMD_SUSPEND }, 0, {} };
        return arduino_repeat_send_(arduino, &cmd, 5);
}

static int arduino_resume_(arduino_t* arduino, int reset)
{
        if (reset) 
                log_info("Arduino: Clearing the memory and resuming measurements"); 
        else
                log_info("Arduino: Resuming measurements"); 

        unsigned char c = CMD_RESUME;
        if (reset) c |= 0x01;

        acmd_t cmd = { "resume", 1, { c }, 0, {} };
        return arduino_repeat_send_(arduino, &cmd, 10);
}

static int arduino_set_poweroff_(arduino_t* arduino, int minutes)
{
        log_info("Arduino: Request poweroff, wakeup in %d minutes", minutes); 

        acmd_t cmd = { "set_poweroff", 
                       3, { CMD_SET_POWEROFF, (minutes & 0x0000ff00) >> 8, (minutes & 0x000000ff) }, 
                       0, {} };

        return arduino_repeat_send_(arduino, &cmd, 5);
}

static int arduino_get_poweroff_(arduino_t* arduino, unsigned long* off, unsigned long* on)
{
        acmd_t cmd = { "get_poweroff", 
                       1, { CMD_GET_POWEROFF }, 
                       8, {} };

        int err = arduino_repeat_send_(arduino, &cmd, 5);
        if (err != 0) 
                return err;

        *off = (unsigned long) (cmd.r[0] << 24) | (cmd.r[1] << 16) | (cmd.r[2] << 8) | (cmd.r[3] << 0);
        *on = (unsigned long) (cmd.r[4] << 24) | (cmd.r[5] << 16) | (cmd.r[6] << 8) | (cmd.r[7] << 0);

        return 0;
}

static int arduino_pump_(arduino_t* arduino, int seconds)
{
        int err;
        acmd_t on_cmd = { "pump_on", 1, { CMD_PUMP | 0x01 }, 0, {} };
        acmd_t off_cmd = { "pump_off", 1, { CMD_PUMP }, 0, {} };

        log_info("Arduino: Turning pump on"); 
        err = arduino_repeat_send_(arduino, &on_cmd, 5);
        if (err != 0) return err;

        sleep(seconds);

        log_info("Arduino: Turning pump off"); 
        err = arduino_repeat_send_(arduino, &off_cmd, 5);
        if (err != 0) {
                log_err("Arduino: Failed to send the 'pump off' command. "
                        "This is not good... You may have to restart the device.");
        }

        return err;
}

static int arduino_millis_(arduino_t* arduino, unsigned long* m)
{
        log_info("Arduino: Getting the current time on the Arduino"); 

        acmd_t cmd = { "millis", 
                       1, { CMD_GET_MILLIS }, 
                       4, {} };

        int err = arduino_repeat_send_(arduino, &cmd, 5);
        if (err != 0) 
                return err;

        *m = (unsigned long) (cmd.r[0] << 24) | (cmd.r[1] << 16) | (cmd.r[2] << 8) | (cmd.r[3] << 0);

        return err;
}

static int arduino_get_frames(arduino_t* arduino)
{
        log_info("Arduino: Getting the number of data frames on the Arduino"); 

        acmd_t cmd = { "get_frames", 
                       1, { CMD_GET_FRAMES }, 
                       2, {} };

        int err = arduino_repeat_send_(arduino, &cmd, 5);
        if (err != 0) 
                return -1;

        return (int) (cmd.r[0] << 8) | (cmd.r[1] << 0);
}

static int arduino_get_sensors_(arduino_t* arduino, unsigned char* enabled, unsigned char* period)
{
        log_info("Arduino: Getting enabled sensors and update period"); 

        acmd_t cmd = { "get_sensors", 
                       1, { CMD_GET_SENSORS }, 
                       2, {} };

        int err = arduino_repeat_send_(arduino, &cmd, 5);
        if (err != 0) 
                return -1;

        *enabled = cmd.r[0];
        *period = cmd.r[1];

        log_info("Arduino: Enabled sensors: 0x%02x, period: %d", (int) cmd.r[0], (int) cmd.r[1]); 

        return 0;
}

static int arduino_set_sensors_(arduino_t* arduino, unsigned char enabled, unsigned char period)
{
        log_info("Arduino: Configuring sensors and update period"); 

        acmd_t cmd = { "set_sensors", 
                       3, { CMD_SET_SENSORS, enabled, period }, 
                       0, {} };

        return arduino_repeat_send_(arduino, &cmd, 5);
}

static int arduino_read_float(arduino_t* arduino, float* value)
{

        /* Read the four bytes of the floating point value. We do four
           separate reads of one byte because this is the only call
           that seems to work successfully with the Arduino. */
        unsigned char c[4];
        for (int i = 0; i < 4; i++) {
                int v = i2c_smbus_read_byte(arduino->fd);
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

static int arduino_download_data(arduino_t* arduino, 
                                 time_t delta, 
                                 int* datastreams, 
                                 int num_datastreams, 
                                 int nframes,
                                 const char* filename)
{
        int err = -1;

        FILE* fp = fopen(filename, "a");
        if (fp == NULL) {
                log_err("Arduino: Failed to open the output file: %s", filename); 
                return -1;
        }

        if (i2c_smbus_write_byte(arduino->fd, CMD_TRANSFER) == -1) {
                log_err("Arduino: Failed to send the 'transfer' command");
                goto error_recovery;
        }
        usleep(10000);

        for (int frame = 0; frame < nframes; frame++) {
                
                float value;
        
                if (arduino_read_float(arduino, &value) != 0) 
                        goto error_recovery;

                time_t timestamp = (time_t) value + delta;
                struct tm r;
                char time[256];

                localtime_r(&timestamp, &r);
                snprintf(time, 256, "%04d-%02d-%02dT%02d:%02d:%02d",
                         1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, r.tm_hour, r.tm_min, r.tm_sec);
                
                for (int i = 0; i < num_datastreams; i++) {
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

int arduino_store_data(arduino_t* arduino,
                       int* datastreams,
                       int num_datastreams,
                       const char* filename)
{
        int err = -1; 
        time_t now = time(NULL);
        unsigned long millis;
        unsigned char enabled;
        unsigned char period;
        time_t delta;

        err = arduino_connect(arduino);
        if (err != 0) 
                return err;

        err = arduino_suspend_(arduino);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }
        
        err =  arduino_millis_(arduino, &millis);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }
        log_info("Arduino: Current time on the arduino: %u msec", millis); 
        delta = now - ((millis + 500) / 1000);
        
        err = arduino_get_sensors_(arduino, &enabled, &period);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        int nframes = arduino_get_frames(arduino);
        if (nframes == -1) {
                arduino_disconnect(arduino);
                return err;
        }
        log_info("Arduino: Found %d measurement frames", nframes); 

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_download_data(arduino, 
                                            delta, 
                                            datastreams, 
                                            num_datastreams, 
                                            nframes,
                                            filename);
                if (err == 0) 
                        continue;
        }

        if (err == 0) {
                log_info("Arduino: Download successful"); 
                err = arduino_resume_(arduino, 1);
        } else {
                log_info("Arduino: Download failed"); 
                arduino_resume_(arduino, 0);
        }

        arduino_disconnect(arduino);
        return err;
}

int arduino_set_sensors(arduino_t* arduino, unsigned char enabled, unsigned char period)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return err;

        err = arduino_set_sensors_(arduino, enabled, period);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        return arduino_disconnect(arduino);
}

int arduino_get_sensors(arduino_t* arduino, unsigned char* enabled, unsigned char* period)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_sensors_(arduino, enabled, period);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        return arduino_disconnect(arduino);
}

int arduino_set_poweroff(arduino_t* arduino, int minutes)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_set_poweroff_(arduino, minutes);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        return arduino_disconnect(arduino);
}

int arduino_get_poweroff(arduino_t* arduino, unsigned long* off, unsigned long* on)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_poweroff_(arduino, off, on);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }                

        return arduino_disconnect(arduino);
}

int arduino_pump(arduino_t* arduino, int seconds)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_pump_(arduino, seconds);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        return arduino_disconnect(arduino);
}

int arduino_millis(arduino_t* arduino, unsigned long* m)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_millis_(arduino, m);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        return arduino_disconnect(arduino);
}


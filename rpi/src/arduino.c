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
/*
    This file uses code from rtc-ds1374.c (Linux kernel).
    The original rtc-ds1374.c license can be found below. 
*/
/*
 * RTC client/driver for the Maxim/Dallas DS1374 Real-Time Clock over I2C
 *
 * Based on code by Randy Vinson <rvinson@mvista.com>,
 * which was based on the m41t00.c by Mark Greer <mgreer@mvista.com>.
 *
 * Copyright (C) 2006-2007 Freescale Semiconductor
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
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

#define DS1374_REG_TOD0		0x00 /* Time of Day */
#define DS1374_REG_TOD1		0x01
#define DS1374_REG_TOD2		0x02
#define DS1374_REG_TOD3		0x03
#define DS1374_REG_WDALM0	0x04 /* Watchdog/Alarm */
#define DS1374_REG_WDALM1	0x05
#define DS1374_REG_WDALM2	0x06
#define DS1374_REG_CR		0x07 /* Control */
#define DS1374_REG_CR_AIE	0x01 /* Alarm Int. Enable */
#define DS1374_REG_CR_WDALM	0x20 /* 1=Watchdog, 0=Alarm */
#define DS1374_REG_CR_WACE	0x40 /* WD/Alarm counter enable */
#define DS1374_REG_SR		0x08 /* Status */
#define DS1374_REG_SR_OSF	0x80 /* Oscillator Stop Flag */
#define DS1374_REG_SR_AF	0x01 /* Alarm Flag */
#define DS1374_REG_TCR		0x09 /* Trickle Charge */

#define CMD_POWEROFF            0x0a
#define CMD_SENSORS             0x0b
#define CMD_STATE               0x0c
#define CMD_FRAMES              0x0d
#define CMD_READ                0x0e
#define CMD_PUMP                0x0f
#define CMD_PERIOD              0x10
#define CMD_START               0x11

#define STATE_MEASURING         1
#define STATE_RESETSTACK        2
#define STATE_SUSPEND           3

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

        if (arduino->fd != -1) {
                log_debug("Arduino: Connection already established");
                return 0;
        }

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

static int arduino_read(arduino_t* arduino, unsigned long *value,
                        int reg, int nbytes)
{
	unsigned char buf[4];
	int ret;
	int i;

	if (nbytes > 4) {
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(arduino->fd, reg, nbytes, buf);

	if (ret < 0) {
                log_err("Arduino: failed to read the data");
		return ret;
	}
        if (ret < nbytes) {
                log_err("Arduino: failed to read the data, got %d/%d bytes",
                        ret, nbytes);
		return -1;
        }

        *value = 0;
	for (i = 0; i < nbytes; i++)
		*value = (*value << 8) | buf[i];

        usleep(10000);

	return 0;
}

static int arduino_write(arduino_t* arduino, 
                         unsigned long value,
                         int reg, int nbytes)
{
	unsigned char buf[4];
	int i;
        int ret;

	if (nbytes > 4) {
		return -1;
	}

	for (i = nbytes - 1; i >= 0; i--) {
		buf[i] = value & 0xff;
		value >>= 8;
	}

	ret = i2c_smbus_write_i2c_block_data(arduino->fd, reg, nbytes, buf);
        if (ret != 0) {
                log_err("Arduino: failed to write the data");
        }

        usleep(10000);

        return ret;
}

static int arduino_set_time_(arduino_t* arduino, time_t time)
{
        log_info("Arduino: Setting time to %ul", (unsigned int) time); 
	unsigned long itime;
        itime = (unsigned long) time;
	return arduino_write(arduino, itime, DS1374_REG_TOD0, 4);
}

static int arduino_get_time_(arduino_t* arduino, time_t *time)
{
	unsigned long itime;
	int ret;

	ret = arduino_read(arduino, &itime, DS1374_REG_TOD0, 4);
	if (!ret)
		*time = (time_t) itime;

	return ret;
}

static int arduino_set_poweroff_(arduino_t* arduino, int minutes)
{
        log_info("Arduino: Request poweroff, wake up in %d minutes", minutes); 
	unsigned long value;

        value = (unsigned long) minutes;
	return arduino_write(arduino, value, CMD_POWEROFF, 2);
}

static int arduino_get_poweroff_(arduino_t* arduino, int* minutes)
{
	unsigned long value;
	int ret;

	ret = arduino_read(arduino, &value, CMD_POWEROFF, 2);
	if (!ret)
		*minutes = (int) value;

	return ret;
}

static int arduino_set_state_(arduino_t* arduino, int state)
{
        log_info("Arduino: Set state to %d", state); 
	unsigned long value = (unsigned long) state;
        int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_write(arduino, value, CMD_STATE, state);
                if (!err)
                        break;
        }
        return err;
}

static int arduino_get_state_(arduino_t* arduino, int* state)
{
	unsigned long value;
	int ret;

	ret = arduino_read(arduino, &value, CMD_STATE, 1);
	if (!ret)
		*state = (int) value;

	return ret;
}

static int arduino_get_frames_(arduino_t* arduino, int* frames)
{
        log_debug("Arduino: Getting the number of data frames on the Arduino"); 
	unsigned long value;
	int ret;

        for (int attempt = 0; attempt < 5; attempt++) {
                ret = arduino_read(arduino, &value, CMD_FRAMES, 2);
                if (!ret) {
                        *frames = (int) value;
                        break;
                }
        }

	return ret;
}

static int arduino_pump_(arduino_t* arduino, int seconds)
{
        /* int err; */
        /* acmd_t on_cmd = { "pump_on", 1, { CMD_PUMP | 0x01 }, 0, {} }; */
        /* acmd_t off_cmd = { "pump_off", 1, { CMD_PUMP }, 0, {} }; */

        /* log_info("Arduino: Turning pump on");  */
        /* err = arduino_repeat_send_(arduino, &on_cmd, 5); */
        /* if (err != 0) return err; */

        /* sleep(seconds); */

        /* log_info("Arduino: Turning pump off");  */
        /* err = arduino_repeat_send_(arduino, &off_cmd, 5); */
        /* if (err != 0) { */
        /*         log_err("Arduino: Failed to send the 'pump off' command. " */
        /*                 "This is not good... You may have to restart the device."); */
        /* } */

        /* return err; */
        return -1;
}

static int arduino_get_sensors_(arduino_t* arduino, 
                                unsigned char* sensors)
{
        log_debug("Arduino: Getting enabled sensors"); 
        unsigned long value;
        int ret = arduino_read(arduino, &value, CMD_SENSORS, 1);
        if (!ret) {
                *sensors = (unsigned char) value;
                log_info("Arduino: Enabled sensors: 0x%02x", (int) value); 
        }

        return ret;
}

static int arduino_set_sensors_(arduino_t* arduino, 
                                unsigned char sensors)
{
        log_info("Arduino: Configuring sensors and update period"); 
        unsigned long value = sensors;
        return arduino_write(arduino, value, CMD_SENSORS, 1);
}

static int arduino_get_period_(arduino_t* arduino, 
                                unsigned char* period)
{
        log_info("Arduino: Getting period"); 

        unsigned long value;
        int ret = arduino_read(arduino, &value, CMD_PERIOD, 1);
        if (ret) {
                *period = (unsigned char) value;
                log_info("Arduino: period: 0x%02x", (int) value); 
        }
        return ret;
}

static int arduino_set_period_(arduino_t* arduino, 
                                unsigned char period)
{
        log_info("Arduino: Configuring period and update period"); 
        unsigned long value = period;
        return arduino_write(arduino, value, CMD_PERIOD, 1);
}

static int arduino_read_timestamp(arduino_t* arduino, time_t* timestamp)
{
        log_debug("Arduino: Read timestamp"); 
        unsigned long v;
        int ret = arduino_read(arduino, &v, CMD_READ, 4);
        if (!ret) 
                *timestamp = (time_t) v;
        return ret;
}

static int arduino_read_float(arduino_t* arduino, float* value)
{
        log_debug("Arduino: Read float value"); 
        unsigned long v;
        int ret = arduino_read(arduino, &v, CMD_READ, 4);
        if (!ret) 
                *value = *((float*) &v);
        return ret;
}

static int arduino_start_transfer(arduino_t* arduino)
{
        if (i2c_smbus_write_byte(arduino->fd, CMD_START) == -1) {
                log_err("Arduino: Failed to send the 'start transfer' command");
                return -1;
        }
        return 0;
}

int arduino_read_data(arduino_t* arduino,
                      int* datastreams,
                      int num_datastreams,
                      arduino_data_callback_t callback,
                      void* ptr)
{
        int err = -1; 
        int nframes; 

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_set_state_(arduino, STATE_SUSPEND);
        if (err != 0) {
                arduino_disconnect(arduino);
                return -1;
        }
        
        err = arduino_get_frames_(arduino, &nframes);
        if (err != 0) {
                arduino_disconnect(arduino);
                return -1;
        }
        log_info("Arduino: Found %d measurement frames", nframes); 

        for (int attempt = 0; attempt < 5; attempt++) {

                log_info("Arduino: Download attempt %d", attempt + 1); 

                if (arduino_start_transfer(arduino) != 0) {
                        //arduino_disconnect(arduino);
                        continue;
                }

                for (int frame = 0; frame < nframes; frame++) {
                
                        time_t timestamp;
                        float value;
        
                        err = arduino_read_timestamp(arduino, &timestamp);
                        if (err != 0) {
                                arduino_set_state_(arduino, STATE_MEASURING);
                                //arduino_disconnect(arduino);
                                continue;
                        }

                        for (int i = 0; i < num_datastreams; i++) {
                                err = arduino_read_float(arduino, &value);
                                if (err != 0) {
                                        arduino_set_state_(arduino, STATE_MEASURING);
                                        //arduino_disconnect(arduino);
                                        continue;
                                }
                        
                                callback(ptr, datastreams[i], timestamp, value);
                        }
                }

                if (err == 0)
                        break;
        }

        if (err == 0) {
                log_info("Arduino: Download successful"); 
                err = arduino_set_state_(arduino, STATE_RESETSTACK);
        } else {
                log_info("Arduino: Download failed"); 
                err = arduino_set_state_(arduino, STATE_MEASURING);
                arduino_disconnect(arduino);
        }

        return err;
}

int arduino_set_sensors(arduino_t* arduino, unsigned char sensors)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return err;

        err = arduino_set_sensors_(arduino, sensors);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_get_sensors(arduino_t* arduino, unsigned char* sensors)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_sensors_(arduino, sensors);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_set_period(arduino_t* arduino, unsigned char period)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return err;

        err = arduino_set_period_(arduino, period);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_get_period(arduino_t* arduino, unsigned char* period)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_period_(arduino, period);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        //return arduino_disconnect(arduino);
        return 0;
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

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_get_poweroff(arduino_t* arduino, int* minutes)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_poweroff_(arduino, minutes);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }                

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_get_frames(arduino_t* arduino, int* frames)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_frames_(arduino, frames);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }

        //return arduino_disconnect(arduino);
        return 0;
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

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_set_time(arduino_t* arduino, time_t time)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_set_time_(arduino, time);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }                

        //return arduino_disconnect(arduino);
        return 0;
}

int arduino_get_time(arduino_t* arduino, time_t *time)
{
        int err;

        err = arduino_connect(arduino);
        if (err != 0) 
                return -1;

        err = arduino_get_time_(arduino, time);
        if (err != 0) {
                arduino_disconnect(arduino);
                return err;
        }                

        //return arduino_disconnect(arduino);
        return 0;
}

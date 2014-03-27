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
//#include "arduino-serial-lib.h"
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
#define CMD_CHECKSUM            0x12
#define CMD_OFFSET              0x13
#define CMD_MEASURENOW          0x14
#define CMD_MEASUREMENT0        0x15
#define CMD_GETMEASUREMENT      0x16
#define CMD_DEBUG               0xff

#define STATE_MEASURING         0
#define STATE_RESETSTACK        1
#define STATE_SUSPEND           2

#define DEBUG_STACK             (1 << 0)

#define STACK_SIZE 1024

/* typedef union _stack_entry_t { */
/*         unsigned long i; */
/*         float f; */
/* } stack_entry_t; */

typedef short sensor_value_t; 

typedef struct _stack_t {
        int frames;
        int framesize;
        unsigned char checksum;
        unsigned long offset;
        sensor_value_t values[STACK_SIZE];
} stack_t;

struct _arduino_t {
        int bus;
        int address;
        int fd;
        //        int serial;
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
        /* arduino->serial = serialport_init("/dev/ttyAMA0", 9600); */
        /* if (arduino->serial == -1) {  */
        /*         log_err("Arduino: failed to open the serial connection"); */
        /* } */

        return arduino;
}

int delete_arduino(arduino_t* arduino)
{
        /* if (arduino->serial != -1) {  */
        /*         serialport_close(arduino->serial); */
        /* } */
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

/* static int arduino_read_serial(arduino_t* arduino, */
/*                                int reg,  */
/*                                int nbytes,  */
/*                                char* buf) */
/* { */
/*         int r = serialport_writebyte(arduino->serial, (uint8_t) reg); */
/*         if (r != 0)  */
/*                 return -1; */
/*         r = serialport_flush(arduino->serial); */
/*         return serialport_read_until(arduino->serial, buf, 0, nbytes, 1000); */
/* } */

/* static int arduino_read_value_serial(arduino_t* arduino,  */
/*                                      unsigned long *value, */
/*                                      int reg, int nbytes) */
/* { */
/*         char buf[64]; */
/*         int r = arduino_read_serial(arduino, reg, 64, buf);  */
/*         if (r == 0) { */
/*                 unsigned long v = atol(buf); */
/*                 *value = v; */
/*                 return 0; */
/*         } */
/*         return r; */
/* } */

static int arduino_read(arduino_t* arduino,
                        int reg, 
                        int nbytes, 
                        unsigned char* buf)
{
	int n;

	n = i2c_smbus_read_i2c_block_data(arduino->fd, reg, nbytes, buf);

	if (n < 0) {
                log_err("Arduino: Failed to read the data");
		return n;
	}
        if (n < nbytes) {
                log_err("Arduino: Failed to read the data, got %d/%d bytes",
                        n, nbytes);
		return -1;
        }

        usleep(10000);

	return 0;
}

static int arduino_read_value(arduino_t* arduino, 
                              unsigned long *value,
                              int reg, int nbytes)
{
	unsigned char buf[4];
	int i, n;

	if (nbytes > 4) {
		return -1;
	}

	n = i2c_smbus_read_i2c_block_data(arduino->fd, reg, nbytes, buf);

	if (n < 0) {
                log_err("Arduino: Failed to read the data");
		return n;
	}
        if (n < nbytes) {
                log_err("Arduino: Failed to read the data, got %d/%d bytes",
                        n, nbytes);
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
        int err;

	if (nbytes > 4) {
		return -1;
	}

	for (i = nbytes - 1; i >= 0; i--) {
		buf[i] = value & 0xff;
		value >>= 8;
	}

	err = i2c_smbus_write_i2c_block_data(arduino->fd, reg, nbytes, buf);
        if (err != 0) {
                log_err("Arduino: Failed to write the data");
        }

        usleep(10000);

        return err;
}

static int arduino_set_time_(arduino_t* arduino, time_t time)
{
        log_info("Arduino: Setting time to %lu", (unsigned int) time); 
	unsigned long itime;
        itime = (unsigned long) time;
	return arduino_write(arduino, itime, DS1374_REG_TOD0, 4);
}

static int arduino_get_time_(arduino_t* arduino, time_t *time)
{
	unsigned long itime;
	int err;

	err = arduino_read_value(arduino, &itime, DS1374_REG_TOD0, 4);
	if (err == 0)
		*time = (time_t) itime;

	return err;
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
	int err;

	err = arduino_read_value(arduino, &value, CMD_POWEROFF, 2);
	if (err == 0)
		*minutes = (int) value;

	return err;
}

static int arduino_set_state_(arduino_t* arduino, int state)
{
        log_info("Arduino: Set state to %d", state); 
	unsigned long value = (unsigned long) state;
        int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_write(arduino, value, CMD_STATE, 1);
                if (err == 0)
                        break;
        }
        return err;
}

static int arduino_measure_(arduino_t* arduino)
{
        log_info("Arduino: Sending measure request"); 
        int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_write(arduino, 0, CMD_MEASURENOW, 0);
                if (err == 0)
                        break;
        }
        return err;
}

static int arduino_measurement0_(arduino_t* arduino)
{
        log_info("Arduino: Sending measurement 0 request"); 
        int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_write(arduino, 0, CMD_MEASUREMENT0, 0);
                if (err == 0)
                        break;
        }
        return err;
}

#if 0
static int arduino_get_state_(arduino_t* arduino, int* state)
{
	unsigned long value;
	int err;

	err = arduino_read_value(arduino, &value, CMD_STATE, 1);
	if (err == 0)
		*state = (int) value;

	return err;
}
#endif

static int arduino_get_frames_(arduino_t* arduino, int* frames)
{
        log_debug("Arduino: Getting the number of data frames on the Arduino"); 
	unsigned long value;
	int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_read_value(arduino, &value, CMD_FRAMES, 2);
                if (err == 0) {
                        *frames = (int) value;
                        break;
                }
        }

	return err;
}

static int arduino_get_checksum_(arduino_t* arduino, unsigned char* checksum)
{
        log_debug("Arduino: Getting the checksum on the Arduino"); 
	unsigned long value;
	int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_read_value(arduino, &value, CMD_CHECKSUM, 1);
                if (err == 0) {
                        *checksum = (unsigned char) (value & 0xff);
                        break;
                }
        }

	return err;
}

static int arduino_get_offset_(arduino_t* arduino, unsigned long* offset)
{
        log_debug("Arduino: Getting the time offset on the Arduino"); 
	unsigned long value;
	int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_read_value(arduino, &value, CMD_OFFSET, 4);
                if (err == 0) {
                        *offset = value;
                        break;
                }
        }

	return err;
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
        int err = arduino_read_value(arduino, &value, CMD_SENSORS, 1);
        if (err == 0) {
                *sensors = (unsigned char) value;
                log_info("Arduino: Enabled sensors: 0x%02x", (int) value); 
        }

        return err;
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
        int err;

        for (int attempt = 0; attempt < 5; attempt++) {
                err = arduino_read_value(arduino, &value, CMD_PERIOD, 1);
                if (err == 0) {
                        *period = (unsigned char) value;
                        log_info("Arduino: period: 0x%02x", (int) value); 
                        break;
                }
        }
        return err;
}

static int arduino_set_period_(arduino_t* arduino, 
                                unsigned char period)
{
        log_info("Arduino: Configuring period and update period"); 
        unsigned long value = period;
        return arduino_write(arduino, value, CMD_PERIOD, 1);
}

static int arduino_start_transfer(arduino_t* arduino)
{
        if (i2c_smbus_write_byte(arduino->fd, CMD_START) == -1) {
                log_err("Arduino: Failed to send the 'start transfer' command");
                return -1;
        }
        return 0;
}

// CRC-8 - based on the CRC8 formulas by Dallas/Maxim
// code released under the therms of the GNU GPL 3.0 license
// http://www.leonardomiliani.com/2013/un-semplice-crc8-per-arduino/?lang=en
static unsigned char crc8(unsigned char crc, const unsigned char *data, unsigned char len) 
{
        while (len--) {
                unsigned char extract = *data++;
                for (unsigned char i = 8; i; i--) {
                        unsigned char sum = (crc ^ extract) & 0x01;
                        crc >>= 1;
                        if (sum) {
                                crc ^= 0x8C;
                        }
                        extract >>= 1;
                }
        }
        return crc;
}

static int arduino_copy_stack_(arduino_t* arduino,
                               stack_t* stack)
{
        unsigned char* ptr = (unsigned char*) &stack->values[0];
        int index = 0;
        int len = stack->frames * stack->framesize * sizeof(sensor_value_t);
        int err;

        if (arduino_start_transfer(arduino) != 0)
                return -1;

        while (index <= len) {
                err = arduino_read(arduino, CMD_READ, sizeof(sensor_value_t), ptr + index);
                if (err != 0)
                        return -1;
                index += sizeof(sensor_value_t);
        }

        /* while (index <= len) { */
        /*         err = arduino_read_serial(arduino, SCMD_READ, 4, (char*) ptr + index); */
        /*         if (err != 0) */
        /*                 return -1; */
        /*         index += 4; */
        /* } */

        {
                unsigned char* ptr = (unsigned char*) &stack->values[0];
                //int index = 0;
                int len = stack->frames * stack->framesize * sizeof(sensor_value_t);

                printf("len=%d\n", len);

                for (int i = 0; i < len; i++) {
                        printf("%02x", ptr[i]);
                        if ((i % 4) == 3)
                                printf("\n");
                        else
                                printf(" ");
                }
                if (((len - 1) % 4) != 3)
                        printf("\n");
        }

        return 0;
}

static datapoint_t* arduino_convert_stack_(arduino_t* arduino,
                                           stack_t* stack,
                                           int* datastreams,
                                           float* factors,
                                           int* num_points)
{
        datapoint_t* datapoints = NULL;
        int num_streams = stack->framesize - 1;
        int size = stack->frames * num_streams * sizeof(datapoint_t);

        datapoints = (datapoint_t*) malloc(size);
        if (datapoints == NULL) {
                log_info("Arduino: Out of memory"); 
                return NULL;
        }
        memset(datapoints, 0, size);

        int index = 0;
        int datapoint = 0;

        for (int i = 0; i < stack->frames; i++) {
                
                time_t timestamp = (time_t) (stack->offset + stack->values[index++] * 60);

                for (int j = 0; j < num_streams; j++) {
                        float v = (float) stack->values[index++];                        
                        datapoints[datapoint].datastream = datastreams[j];
                        datapoints[datapoint].timestamp = timestamp;
                        datapoints[datapoint].value = factors[j] * v;
                        datapoint++;
                }
        }

        *num_points = datapoint;

        return datapoints;
}

datapoint_t* arduino_read_data(arduino_t* arduino, int* num_points)
{
        int err = -1; 
        unsigned char sensors; 
        int datastreams[32];
        int num_streams = 0;
        datapoint_t* datapoints = NULL;
        float factors[32];

        *num_points = 0;

        err = arduino_connect(arduino);
        if (err != 0) 
                goto error_recovery;

        err = arduino_get_sensors_(arduino, &sensors);
        if (err != 0)
                goto error_recovery;

        if (sensors & SENSOR_TRH) {
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_T;
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_RH;
        }
        if (sensors & SENSOR_TRHX) {
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_TX;
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_RHX;
        }
        if (sensors & SENSOR_LUM) {
                factors[num_streams] = 1.0f;
                datastreams[num_streams++] = DATASTREAM_LUM;
        }
        if (sensors & SENSOR_USBBAT) {
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_USBBAT;
        }
        if (sensors & SENSOR_SOIL) {
                factors[num_streams] = 1.0f;
                datastreams[num_streams++] = DATASTREAM_SOIL;
        }

        stack_t stack;
        stack.framesize = num_streams + 1;


        err = arduino_set_state_(arduino, STATE_SUSPEND);
        if (err != 0) 
                goto error_recovery;
        
        err = arduino_get_frames_(arduino, &stack.frames);
        if (err != 0)
                goto error_recovery;

        log_info("Arduino: Found %d measurement frames", stack.frames); 

        if (stack.frames == 0) {
                err = arduino_set_state_(arduino, STATE_MEASURING);
                goto clean_exit;
        }

        err = arduino_get_checksum_(arduino, &stack.checksum);
        if (err != 0)
                goto error_recovery;

        log_info("Arduino: Checksum Arduino 0x%02x", stack.checksum); 

        err = arduino_get_offset_(arduino, &stack.offset);
        if (err != 0)
                goto error_recovery;

        log_info("Arduino: Time offset Arduino %lu", (unsigned int) stack.offset); 


        for (int attempt = 0; attempt < 5; attempt++) {

                log_info("Arduino: Download attempt %d", attempt + 1); 

                err = arduino_copy_stack_(arduino, &stack);
                if (err != 0)
                        continue;

                unsigned char* ptr = (unsigned char*) &stack.values[0];
                int len = stack.frames * stack.framesize * sizeof(sensor_value_t);
                unsigned char checksum = crc8(0, ptr, len);
                
                log_info("Arduino: Checksum Linux 0x%02x", checksum); 

                //
                /* for (int i = 0; i < len; i++) { */
                /*         fprintf(stderr, "%02x", ptr[i]); */
                /*         if ((i % 4) == 3) */
                /*                 fprintf(stderr, "\n"); */
                /*         else  */
                /*                 fprintf(stderr, " "); */
                /* } */
                //

                if (checksum != stack.checksum)
                        err = -1;

                if (err == 0)
                        break;
        }
        
        if (err == 0)
                datapoints = arduino_convert_stack_(arduino, &stack, 
                                                    datastreams,
                                                    factors,
                                                    num_points);

 clean_exit:
 error_recovery:

        if (err == 0) {
                log_info("Arduino: Download successful"); 
                err = arduino_set_state_(arduino, STATE_RESETSTACK);
        } else {
                log_info("Arduino: Download failed"); 
                err = arduino_set_state_(arduino, STATE_MEASURING);
                arduino_disconnect(arduino);
        }

        return datapoints;
}

datapoint_t* arduino_measure(arduino_t* arduino, int* num_points)
{
        int err = -1; 
        unsigned char sensors; 
        int datastreams[32];
        int num_streams = 0;
        datapoint_t* datapoints = NULL;
        float factors[32];

        *num_points = 0;

        err = arduino_connect(arduino);
        if (err != 0) 
                goto error_recovery;

        err = arduino_get_sensors_(arduino, &sensors);
        if (err != 0)
                goto error_recovery;

        if (sensors & SENSOR_TRH) {
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_T;
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_RH;
        }
        if (sensors & SENSOR_TRHX) {
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_TX;
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_RHX;
        }
        if (sensors & SENSOR_LUM) {
                factors[num_streams] = 1.0f;
                datastreams[num_streams++] = DATASTREAM_LUM;
        }
        if (sensors & SENSOR_USBBAT) {
                factors[num_streams] = 0.01f;
                datastreams[num_streams++] = DATASTREAM_USBBAT;
        }
        if (sensors & SENSOR_SOIL) {
                factors[num_streams] = 1.0f;
                datastreams[num_streams++] = DATASTREAM_SOIL;
        }

        err = arduino_set_state_(arduino, STATE_SUSPEND);
        if (err != 0) 
                goto error_recovery;

        err = arduino_measure_(arduino);
        if (err != 0) 
                goto error_recovery;

        sleep(60); // Sleep for one minute.

        stack_t stack;
        stack.framesize = num_streams + 1;
        stack.values[0] = 0;
        stack.frames = 1;

        for (int attempt = 0; attempt < 5; attempt++) {

                log_info("Arduino: Download attempt %d", attempt + 1); 

                err = arduino_measurement0_(arduino);
                if (err != 0) 
                        goto error_recovery;

                for (int i = 0; i < num_streams; i++) {
                        unsigned long value;
                        err = arduino_read_value(arduino, &value, 
                                                 CMD_GETMEASUREMENT, 
                                                 sizeof(sensor_value_t));
                        
                        if (err != 0)
                                break;

                        stack.values[i+1] = (sensor_value_t) value;
                }
                if (err == 0)
                        break;
        }
        
        if (err == 0)
                datapoints = arduino_convert_stack_(arduino, &stack, 
                                                    datastreams,
                                                    factors,
                                                    num_points);
 error_recovery:

        if (err == 0) {
                log_info("Arduino: Download successful"); 
                err = arduino_set_state_(arduino, STATE_MEASURING);
        } else {
                log_info("Arduino: Download failed"); 
                err = arduino_set_state_(arduino, STATE_MEASURING);
                arduino_disconnect(arduino);
        }

        return datapoints;
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

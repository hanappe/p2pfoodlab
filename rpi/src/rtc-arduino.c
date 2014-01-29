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
/*
 * It would be more efficient to use i2c msgs/i2c_transfer directly but, as
 * recommened in .../Documentation/i2c/writing-clients section
 * "Sending and receiving", using SMBus level communication is preferred.
 */
/* 

    Adapted from rtc-ds1374.c (Linux kernel) for testing puposes.
    (Peter Hanappe, Sony Computer Science Laboratory Paris, 2014)

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

static int arduino_read_rtc(int client, unsigned long *time,
			   int reg, int nbytes)
{
	unsigned char buf[4];
	int ret;
	int i;

	if (nbytes > 4) {
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, reg, nbytes, buf);

	if (ret < 0)
		return ret;
	if (ret < nbytes)
		return -EIO;

	for (i = nbytes - 1, *time = 0; i >= 0; i--)
		*time = (*time << 8) | buf[i];

	return 0;
}

static int arduino_write_rtc(int client, unsigned long time,
                             int reg, int nbytes)
{
	unsigned char buf[4];
	int i;

	if (nbytes > 4) {
		return -1;
	}

	for (i = 0; i < nbytes; i++) {
		buf[i] = time & 0xff;
		time >>= 8;
	}

	return i2c_smbus_write_i2c_block_data(client, reg, nbytes, buf);
}

static int arduino_read_time(int client, time_t *time)
{
	unsigned long itime;
	int ret;

	ret = arduino_read_rtc(client, &itime, DS1374_REG_TOD0, 4);
	if (!ret)
		*time = (time_t) itime;

	return ret;
}

static int arduino_set_time(int client, time_t time)
{
	unsigned long itime;

        itime = (unsigned long) time;
	return arduino_write_rtc(client, itime, DS1374_REG_TOD0, 4);
}

int main(int argc, char** argv)
{
        int client = -1;
        int ret;

        if ((client = open("/dev/i2c-1", O_RDWR)) < 0) {
                fprintf(stderr, "Failed to open the I2C device"); 
                return 1;
        }

        if (ioctl(client, I2C_SLAVE, 0x04) < 0) {
                fprintf(stderr, "Unable to get bus access to talk to slave");
                close(client);
                return 1;
        }

        time_t time;

        ret = arduino_read_time(client, &time);
        if (!ret) {
		fprintf(stdout, "time: %lu (0x%04x)", 
                        time, (unsigned int) time);
        }
        
	return ret;
}


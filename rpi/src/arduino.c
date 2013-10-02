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
#include "logMessage.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define RHT03_1_FLAG (1 << 0)
#define RHT03_2_FLAG (1 << 1)
#define SHT15_1_FLAG (1 << 2)
#define SHT15_2_FLAG (1 << 3)
#define LUMINOSITY_FLAG (1 << 4)
#define SOIL_FLAG (1 << 5)

#define CMD_POWEROFF 0x00
#define CMD_SET_SENSORS 0x10
#define CMD_SUSPEND 0x20
#define CMD_RESUME 0x30
#define CMD_GET_SENSORS 0x40
#define CMD_GET_MILLIS 0x50
#define CMD_GET_FRAMES 0x60
#define CMD_TRANSFER 0x70
#define CMD_PUMP 0x80

static const char* homeDir = "/var/p2pfoodlab";
static const char* configFile = "/var/p2pfoodlab/etc/config.json";
static const char* outputFile = "/var/p2pfoodlab/datapoints.csv";
static int bus = 1;
static int address = 0x04;

int arduino_connect(int bus, int address)
{
        int arduino = -1;
        char *fileName = (bus == 0)? "/dev/i2c-0" : "/dev/i2c-1";
 
        logMessage("arduino", LOG_INFO, "Connecting to Arduino\n"); 

        if ((arduino = open(fileName, O_RDWR)) < 0) {
                logMessage("arduino", LOG_ERROR, "Failed to open the I2C device\n"); 
                return -1;
        }

        if (ioctl(arduino, I2C_SLAVE, address) < 0) {
                logMessage("arduino", LOG_ERROR, "Unable to get bus access to talk to slave\n");
                return -1;
        }

        usleep(10000);
        return arduino;
}

void arduino_disconnect(int arduino)
{
        logMessage("arduino", LOG_INFO, "Disconnecting from Arduino\n"); 
        if (arduino > 0)
                close(arduino);
}

int arduino_suspend(int arduino)
{
        logMessage("arduino", LOG_INFO, "Sending start transfer\n"); 

        if (i2c_smbus_write_byte(arduino, CMD_SUSPEND) == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to send the 'start transfer' command\n");
                return -1;
        }
        usleep(10000);
        return 0;
}

int arduino_resume(int arduino, int reset)
{
        if (reset) 
                logMessage("arduino", LOG_INFO, "Clearing the memory and resuming measurements\n"); 
        else
                logMessage("arduino", LOG_INFO, "Resuming measurements\n"); 

        unsigned char c = CMD_RESUME;
        if (reset) c |= 0x01;

        if (i2c_smbus_write_byte(arduino, c) == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to send the 'transfer done' command. This is not good... You may have to restart the device.\n");
                return -1;
        }
        usleep(10000);
        return 0;
}

int arduino_read_float(int arduino, float* value)
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

int arduino_poweroff(int arduino, int minutes)
{
        unsigned char c[3];

        c[0] = CMD_POWEROFF;
        c[1] = (minutes & 0x0000ff00) >> 8;
        c[2] = (minutes & 0x000000ff);

        int n = write(arduino, c, 3);
        if (n != 3) {
                logMessage("arduino", LOG_ERROR, "Failed to write the data\n"); 
                return -1;
        }
        usleep(10000);

        return 0;
}

int arduino_pump(int arduino, int seconds)
{
        unsigned char c;
        int err;

        logMessage("arduino", LOG_INFO, "Turning pump on\n"); 
        c = CMD_PUMP | 0x01;

        for (int attempt = 0; attempt < 4; attempt++) {
                err = i2c_smbus_write_byte(arduino, c);
                if (err == 0) break;
                logMessage("arduino", LOG_ERROR, "Failed to send the 'pump on' command.\n");
                usleep(2000);
        }
        if (err != 0) return -1;

        sleep(seconds);

        logMessage("arduino", LOG_INFO, "Turning pump off\n"); 
        c = CMD_PUMP;

        for (int attempt = 0; attempt < 20; attempt++) {
                err = i2c_smbus_write_byte(arduino, c);
                if (err == 0) break;
                logMessage("arduino", LOG_ERROR, 
                           "Failed to send the 'pump off' command. "
                           "This is not good... You may have to restart the device.\n");
                usleep(2000);
        }
        if (err != 0) return -1;

        usleep(10000);

        return 0;
}

int arduino_millis(int arduino, unsigned long* m)
{
        logMessage("arduino", LOG_INFO, "Getting the current time on the Arduino\n"); 

        if (i2c_smbus_write_byte(arduino, CMD_GET_MILLIS) == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to send the 'get millis' command\n");
                return -1;
        }
        usleep(10000);

        unsigned char c[4];
        for (int i = 0; i < 4; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        logMessage("arduino", LOG_ERROR, "Failed to read the 'millis' value\n");
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        *m = (unsigned long) (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | (c[3] << 0);

        return 0;
}

int arduino_get_frames(int arduino)
{
        logMessage("arduino", LOG_INFO, "Getting the number of data frames on the Arduino\n"); 

        if (i2c_smbus_write_byte(arduino, CMD_GET_FRAMES) == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to send the 'get count' command\n");
                return -1;
        }
        usleep(10000);

        unsigned char c[2];
        for (int i = 0; i < 2; i++) {
                int v = i2c_smbus_read_byte(arduino);
                if (v == -1) {
                        logMessage("arduino", LOG_ERROR, "Failed to read the 'frames' value\n");
                        return -1;
                }
                c[i] = (unsigned char) (v & 0xff);
                usleep(10000);
        }
        return (int) (c[0] << 8) | (c[1] << 0);
}

int arduino_get_sensors(int arduino, unsigned char* enabled, unsigned char* period)
{
        logMessage("arduino", LOG_INFO, "Getting enabled sensors and update period\n"); 

        if (i2c_smbus_write_byte(arduino, CMD_GET_SENSORS) == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to send the 'get sensors' command\n");
                return -1;
        }
        usleep(10000);

        int v = i2c_smbus_read_byte(arduino);
        if (v == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to read the 'get sensors' value\n");
                return -1;
        }
        unsigned char _enabled = (v & 0x000000ff);
        *enabled = _enabled;
        usleep(10000);

        v = i2c_smbus_read_byte(arduino);
        if (v == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to read the 'get sensors' value\n");
                return -1;
        }
        unsigned char _period = (v & 0x000000ff);
        *period = _period;
        usleep(10000);

        logMessage("arduino", LOG_INFO, "Enabled sensors: %x; Period: %d\n", (int) _enabled, (int) _period); 

        return 0;
}

int arduino_set_sensors(int arduino, unsigned char enabled, unsigned char period)
{
        unsigned char c[3];

        c[0] = CMD_SET_SENSORS;
        c[1] = enabled;
        c[2] = period;

        int n = write(arduino, c, 3);
        if (n != 3) {
                logMessage("arduino", LOG_ERROR, "Failed to write the 'set sensors' command\n"); 
                return -1;
        }
        usleep(10000);

        return 0;
}


json_object_t config_load()
{
        char buffer[512];

        json_object_t config = json_load(configFile, buffer, 512);
        if (json_isnull(config)) {
                logMessage("arduino", LOG_ERROR, "%s\n", buffer); 
                return json_null();
        } 
        return config;
}

int config_get_enabled_sensors(json_object_t config)
{
        json_object_t enable;
        int b = 0;

        json_object_t sensors = json_object_get(config, "sensors");
        if (json_isnull(sensors) || !json_isobject(sensors)) {
                logMessage("arduino", LOG_ERROR, "Failed to get the sensors configuration\n"); 
                return -1;
        } 

        enable = json_object_get(sensors, "rht03_1");
        if (json_isnull(enable)) 
                logMessage("arduino", LOG_WARNING, "Failed to read rht03_1 settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                logMessage("arduino", LOG_INFO, "Enabling internal RHT03 sensors\n"); 
                b |= RHT03_1_FLAG;
        } else {
                logMessage("arduino", LOG_INFO, "Disabling internal RHT03 sensor\n"); 
        }

        enable = json_object_get(sensors, "rht03_2");
        if (json_isnull(enable)) 
                logMessage("arduino", LOG_WARNING, "Failed to read rht03_2 settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                logMessage("arduino", LOG_INFO, "Enabling external RHT03 sensor\n"); 
                b |= RHT03_2_FLAG;
        } else {
                logMessage("arduino", LOG_INFO, "Disabling external RHT03 sensor\n"); 
        }

        enable = json_object_get(sensors, "sht15_1");
        if (json_isnull(enable)) 
                logMessage("arduino", LOG_WARNING, "Failed to read sht15_1 settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                logMessage("arduino", LOG_INFO, "Enabling internal SHT15 sensor\n"); 
                b |= SHT15_1_FLAG;
        } else {
                logMessage("arduino", LOG_INFO, "Disabling internal SHT15 sensor\n"); 
        }

        enable = json_object_get(sensors, "sht15_2");
        if (json_isnull(enable)) 
                logMessage("arduino", LOG_WARNING, "Failed to read sht15_2 settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                logMessage("arduino", LOG_INFO, "Enabling external SHT15 sensor\n"); 
                b |= SHT15_2_FLAG;
        } else {
                logMessage("arduino", LOG_INFO, "Disabling external SHT15 sensor\n"); 
        }

        enable = json_object_get(sensors, "light");
        if (json_isnull(enable)) 
                logMessage("arduino", LOG_WARNING, "Failed to read light sensor settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                logMessage("arduino", LOG_INFO, "Enabling luminosity sensor\n"); 
                b |= LUMINOSITY_FLAG;
        } else {
                logMessage("arduino", LOG_INFO, "Disabling luminosity sensors\n"); 
        }

        enable = json_object_get(sensors, "soil");
        if (json_isnull(enable)) 
                logMessage("arduino", LOG_WARNING, "Failed to read soil humidity settings\n"); 
        else if (json_isstring(enable) && json_string_equals(enable, "yes")) {
                logMessage("arduino", LOG_INFO, "Enabling soil humidity sensor\n"); 
                b |= SOIL_FLAG;
        } else {
                logMessage("arduino", LOG_INFO, "Disabling soil humidity sensor\n"); 
        }

        return b;
}

int config_get_sensors_period(json_object_t config)
{
        int period;
        json_object_t s;

        json_object_t sensors = json_object_get(config, "sensors");
        if (json_isnull(sensors) || !json_isobject(sensors)) {
                logMessage("arduino", LOG_ERROR, "Failed to get the sensors configuration\n"); 
                return -1;
        } 

        s = json_object_get(sensors, "update");
        if (json_isnull(s) || !json_isstring(s)) {
                logMessage("arduino", LOG_ERROR, "Failed to read sensors update period\n"); 
                return 0;
        } else if (json_string_equals(s, "debug")) {
                period = 1;
        } else {
                period = atoi(json_string_value(s));
        }

        if (period > 255) {
                logMessage("arduino", LOG_WARNING, "Invalid update period %d > 255\n", period);
                return -1;
        } else if (period < 1) {
                logMessage("arduino", LOG_WARNING, "Invalid update period %d < 255\n", period);
                return -1;
        }

        logMessage("arduino", LOG_INFO, "Sensors update period is %d\n", period);

        return period;
}



int opensensordata_get_datastream(const char* name)
{
        char buffer[512];
        char filename[512];

        snprintf(filename, 511, "%s/etc/opensensordata/%s.json", homeDir, name);
        filename[511] = 0;

        json_object_t def = json_load(filename, buffer, 512);
        if (json_isnull(def)) {
                logMessage("arduino", LOG_ERROR, "%s\n", buffer); 
                return -1;
        } else if (!json_isobject(def)) {
                logMessage("arduino", LOG_ERROR, "Invalid datastream definition: %s\n", filename); 
                return -1;
        }

        int id = -1;
        json_object_t v = json_object_get(def, "id");
        if (json_isnull(v)) {
                logMessage("arduino", LOG_ERROR, "Invalid datastream id: %s\n", filename); 
                return -1;
        }
        if (json_isstring(v)) {
                id = atoi(json_string_value(v));
        } else if (json_isnumber(v)) {
                id = json_number_value(v);
        } else {
                logMessage("arduino", LOG_ERROR, "Invalid datastream id: %s\n", filename); 
                return -1;
        }
        
        return id;
}

int opensensordata_get_datastreams(unsigned char enabled, int* ids)
{
        int count = 0;
        if (enabled & RHT03_1_FLAG) {
                ids[count++] = opensensordata_get_datastream("t");
                ids[count++] = opensensordata_get_datastream("rh");
        }
        if (enabled & RHT03_2_FLAG) {
                ids[count++] = opensensordata_get_datastream("tx");
                ids[count++] = opensensordata_get_datastream("rhx");
        }
        if (enabled & SHT15_1_FLAG) {
                ids[count++] = opensensordata_get_datastream("t2");
                ids[count++] = opensensordata_get_datastream("rh2");
        }
        if (enabled & SHT15_2_FLAG) {
                ids[count++] = opensensordata_get_datastream("tx2");
                ids[count++] = opensensordata_get_datastream("rhx2");
        }
        if (enabled & LUMINOSITY_FLAG) {
                ids[count++] = opensensordata_get_datastream("lum");
        }
        if (enabled & SOIL_FLAG) {
                ids[count++] = opensensordata_get_datastream("soil");
        }

        return count;
}


static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options] command [command-options]\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-c | --config        Configuration file\n"
                 "Commands:\n"
                 "enable-sensors       Enable/disable the sensors\n"
                 "store-data           Get the sensor data and store them in a file\n"
                 "poweroff minutes     Power off the Raspberry Pi in N minutes\n"
                 "millis               Get arduino's current clock\n"
                 "pump seconds         Turn on the pump for N minutes\n",
                 argv[0]);
}

static void print_version(FILE* fp)
{
        fprintf (fp,
                 "P2P Food Lab Sensorbox\n"
                 "  arduino-set\n"
                 "  Version %d.%d.%d\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

void parse_arguments(int argc, char **argv)
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
                        print_version(stdout);
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




int configure_sensors()
{
        int arduino =  -1; 
        unsigned char enabled, period;
        json_object_t config;
        int v;

        config = config_load(configFile);
        if (json_isnull(config)) return -1;

        v = config_get_enabled_sensors(config);
        if (v < 0) return -1;
        enabled = (unsigned char) (v & 0xff);

        v = config_get_sensors_period(config);
        if (v < 0) return -1;
        period = (unsigned char) (v & 0xff);

        arduino = arduino_connect(bus, address);
        if (arduino < 0) return -1;

        if (arduino_set_sensors(arduino, enabled, period) != 0) {
                arduino_disconnect(arduino);
                return -1;
        }

        arduino_disconnect(arduino);

        return 0;
}

int download_data(int arduino, 
                 time_t delta, 
                 int* datastreams, 
                 int numDatastreams, 
                 int numFrames,
                 const char* filename)
{
        int err = -1;

        FILE* fp = fopen(filename, "a");
        if (fp == NULL) {
                logMessage("arduino", LOG_ERROR, "Failed to open the output file: %s\n", filename); 
                return -1;
        }

        if (i2c_smbus_write_byte(arduino, CMD_TRANSFER) == -1) {
                logMessage("arduino", LOG_ERROR, "Failed to send the 'transfer' command\n");
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

int store_data()
{
        int arduino =  -1; 
        int err = -1; 

        arduino = arduino_connect(bus, address);
        if (arduino < 0) {
                return -1;
        }

        if (arduino_suspend(arduino) != 0) 
                goto error_recovery;

        time_t now = time(NULL);
        unsigned long millis;
        if (arduino_millis(arduino, &millis) != 0) {
                arduino_resume(arduino, 0);
                goto error_recovery;
        }
        logMessage("arduino", LOG_INFO, "Current time on the arduino: %u msec\n", millis); 

        time_t delta = now - ((millis + 500) / 1000);

        unsigned char enabled;
        unsigned char period;

        int v = arduino_get_sensors(arduino, &enabled, &period);
        if (v == -1) {
                arduino_resume(arduino, 0);
                goto error_recovery;
        }

        int datastreams[16];
        int numDatastreams = opensensordata_get_datastreams(enabled, datastreams);
        if (numDatastreams == -1) {
                arduino_resume(arduino, 0);
                goto error_recovery;
        }

        logMessage("arduino", LOG_INFO, "Found %d datastreams\n", numDatastreams); 
        if (numDatastreams == 0) {
                arduino_resume(arduino, 0);
                return 0;
        }

        int numFrames = arduino_get_frames(arduino);
        if (numFrames == -1) {
                arduino_resume(arduino, 0);
                goto error_recovery;
        }
        logMessage("arduino", LOG_INFO, "Found %d measurement frames\n", numFrames); 
        
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
                arduino_resume(arduino, 0);
                goto error_recovery;
        }

        if (arduino_resume(arduino, 1) != 0) 
                goto error_recovery;

        logMessage("arduino", LOG_INFO, "Download successful\n"); 
        err = 0; 

 error_recovery:

        arduino_disconnect(arduino);
        return err;
}


int poweroff(int minutes)
{
        int arduino =  -1; 
                
        arduino = arduino_connect(bus, address);
        if (arduino < 0) {
                return -1;
        }

        if (arduino_poweroff(arduino, minutes) != 0) {
                arduino_disconnect(arduino);
                return -1;
        }

        arduino_disconnect(arduino);

        return 0;
}

int pump(int seconds)
{
        int arduino =  -1; 
                
        arduino = arduino_connect(bus, address);
        if (arduino < 0) {
                return -1;
        }

        if (arduino_pump(arduino, seconds) != 0) {
                arduino_disconnect(arduino);
                return -1;
        }

        arduino_disconnect(arduino);

        return 0;
}

int millis(unsigned long* m)
{
        int arduino =  -1; 
                
        arduino = arduino_connect(bus, address);
        if (arduino < 0) {
                return -1;
        }

        if (arduino_millis(arduino, m) != 0) {
                arduino_disconnect(arduino);
                return -1;
        }

        arduino_disconnect(arduino);

        return 0;
}

int main(int argc, char **argv)
{
        int err = 0;

        //setLogFile(stderr);

        parse_arguments(argc, argv);

        if (optind >= argc) {
                usage(stderr, argc, argv);
                exit(1);
        }

        const char* command = argv[optind];

        if (strcmp(command, "enable-sensors") == 0) {
                logMessage("arduino", LOG_INFO, "Enabling/disabling sensors\n");
                for (int attempt = 0; attempt < 3; attempt++) {
                        logMessage("arduino", LOG_INFO, "Attempt %d/3\n", attempt+1);                
                        err = configure_sensors();
                        if (err == 0) break;
                }
                if (err != 0) exit(1);

        } else if (strcmp(command, "store-data") == 0) {
                logMessage("arduino", LOG_INFO, "Storing sensor data\n");                
                for (int attempt = 0; attempt < 10; attempt++) {
                        logMessage("arduino", LOG_INFO, "Attempt %d/10\n", attempt+1);                
                        err = store_data();
                        if (err == 0) break;
                }
                if (err != 0) exit(1);

        } else if (strcmp(command, "millis") == 0) {
                logMessage("arduino", LOG_INFO, "Reading arduino timer\n");                
                for (int attempt = 0; attempt < 10; attempt++) {
                        logMessage("arduino", LOG_INFO, "Attempt %d/10\n", attempt+1);
                        unsigned long m;
                        err = millis(&m);
                        if (err == 0) {
                                printf("%lu\n", m);
                                break;
                        }
                }
                if (err != 0) exit(1);

        } else if (strcmp(command, "poweroff") == 0) {
                logMessage("arduino", LOG_INFO, "Configuring poweroff/wakeup\n");                

                if (optind + 1 >= argc) {
                        logMessage("arduino", LOG_ERROR, "Poweroff: missing argument\n");
                        exit(1);
                }

                const char* d = argv[optind+1];
                int minutes = atoi(d);
                if ((minutes <= 0) && (minutes > 65536)) {
                        logMessage("arduino", LOG_ERROR, "Poweroff: argument out of range\n");
                        exit(1);
                }

                for (int attempt = 0; attempt < 5; attempt++) {
                        logMessage("arduino", LOG_INFO, "Attempt %d/5\n", attempt+1);
                        err = poweroff(minutes);
                        if (err == 0) break;
                }
                if (err != 0) exit(1);

        } else if (strcmp(command, "pump") == 0) {
                logMessage("arduino", LOG_INFO, "Starting pump\n");                

                if (optind + 1 >= argc) {
                        logMessage("arduino", LOG_ERROR, "Pump: missing argument\n");
                        exit(1);
                }

                const char* d = argv[optind+1];
                int seconds = atoi(d);
                if ((seconds <= 0) && (seconds > 3600)) {
                        logMessage("arduino", LOG_ERROR, "Pump: argument out of range\n");
                        exit(1);
                }

                err = pump(seconds);
                if (err != 0) exit(1);

        } else {
                logMessage("arduino", LOG_WARNING, "Unknown command %s\n", command);
        }
        
        return 0;
}

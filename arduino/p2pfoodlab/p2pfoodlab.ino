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
   along wit
   h this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <Wire.h>
#include <DHT22.h>

#define SLAVE_ADDRESS           0x04

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

#define STATE_MEASURING         1
#define STATE_RESETSTACK        2
#define STATE_SUSPEND           3

#define RHT03_1_PIN             12
#define RHT03_2_PIN             9
#define SHT15_1_DATA_PIN        4
#define SHT15_1_CLOCK_PIN       3
#define LUMINOSITY_PIN          A2
#define RPi_PIN                 2
#define PUMP_PIN                10

#define SENSOR_TRH              (1 << 0)
#define SENSOR_TRHX             (1 << 1)
#define SENSOR_LUM              (1 << 2)
#define SENSOR_SOIL             (1 << 3)

#define MAX_SLEEP               10000

#define DEBUG 1

#if DEBUG
#define DebugPrint(_s) { Serial.println(_s); } 
#define DebugPrintValue(_s,_v) { Serial.print(_s); Serial.println(_v); } 
#else
#define DebugPrint(_s) 
#define DebugPrintValue(_s,_w)
#endif

/* The update period and poweroff/wakeup times are measured in
   minutes. */
unsigned char sensors = SENSOR_TRH;
unsigned char state = STATE_MEASURING; 
unsigned char period = 1; 
unsigned char measure = 1;
unsigned short read_index = 0;
unsigned short poweroff = 0;
unsigned short wakeup = 0;
unsigned long last_minute = 0;
unsigned long suspend_start = 0;
unsigned char new_sensors = SENSOR_TRH;
unsigned char new_period = 1; 
unsigned char new_state = 0; 
unsigned char new_pump = 0; 
unsigned short new_wakeup = 0;
unsigned char i2c_recv_buf[5];
unsigned char i2c_recv_len = 0;
unsigned char i2c_send_buf[4];
unsigned char i2c_send_len = 0;

#if DEBUG
unsigned char i2c_recv = 0;
unsigned char i2c_send = 0;
#endif

DHT22 rht03_1(RHT03_1_PIN);
DHT22 rht03_2(RHT03_2_PIN);

// CRC-8 - based on the CRC8 formulas by Dallas/Maxim
// code released under the therms of the GNU GPL 3.0 license
// http://www.leonardomiliani.com/2013/un-semplice-crc8-per-arduino/?lang=en
static unsigned char crc8(unsigned char crc, const unsigned char *data, unsigned short len) 
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

/*
 * Stack 
 */

/* The timestamps and sensor data are pushed onto the stack until the
   RPi downloads it. */
#define STACK_SIZE 128

typedef union _stack_entry_t {
        unsigned long i;
        float f;
} stack_entry_t;

typedef struct _stack_t {
        unsigned char checksum;
        unsigned char frames;
        unsigned char framesize;
        unsigned short sp;
        stack_entry_t values[STACK_SIZE];
} stack_t;

stack_t _stack;

static void stack_clear()
{
        _stack.sp = 0;
        _stack.frames = 0;
        _stack.checksum = 0;
}

#define stack_set_framesize(__framesize)  { _stack.framesize = __framesize; }
#define stack_frame_begin()               (_stack.sp)
#define stack_frame_unroll(__sp)          { _stack.sp = __sp; }
#define stack_address()                   ((unsigned char*) &_stack.values[0])
#define stack_bytesize()                  (_stack.frames * _stack.framesize * sizeof(stack_entry_t))
#define stack_frame_bytesize()            (_stack.framesize * sizeof(stack_entry_t))
#define stack_geti(__n)                   (_stack.values[__n].i)
#define stack_checksum()                  (_stack.checksum)
#define stack_num_frames()                (_stack.frames)

static void stack_frame_end()
{
        /* This update should be called with interupts disabled!
           Without it, we might be sending back the wrong checksum
           and/or number of frames when an I2C request comes in. */
        unsigned char* data = stack_address();
        unsigned short offset = stack_bytesize();
        unsigned short len = stack_frame_bytesize();
        unsigned char crc = crc8(_stack.checksum, data + offset, len);
        _stack.checksum = crc;
        _stack.frames++;
}

static int stack_pushf(float value)
{
        if (_stack.sp < STACK_SIZE) {
                _stack.values[_stack.sp++].f = value;
                return 1;
        }
        DebugPrint("  STACK FULL");
        return 0;
}

static int stack_pushi(unsigned long value)
{
        if (_stack.sp < STACK_SIZE) {
                _stack.values[_stack.sp++].i = value;
                return 1;
        }
        DebugPrint("  STACK FULL");
        return 0;
}

/*
 * Time functions
 */

static unsigned long _start = 0;

static unsigned long getseconds()
{
        static unsigned long _seconds = 0;
        unsigned long s = millis() / 1000;

        while (s < _seconds)
                s += 4294967; // 2^32 milliseconds
        _seconds = s;
        return _seconds;
}

static unsigned long time(unsigned long t)
{
        unsigned long s = getseconds();
        if (t)
                _start = t - s;
        return _start + s;
}

#define getminutes() (getseconds() / 60)
#define hastime() (_start != 0)


/*
 * I2C interupt handlers
 */

static void receive_data(int len)
{
#if DEBUG
        i2c_recv = 1;
#endif

        i2c_recv_len = 0;
        
        for (int i = 0; i < len; i++) {
                int v = Wire.read();
                if (i < sizeof(i2c_recv_buf)) 
                        i2c_recv_buf[i2c_recv_len++] = v & 0xff;
                //Serial.println(v);
        }

        unsigned char command = i2c_recv_buf[0];

        if ((command == DS1374_REG_TOD0) 
            && (i2c_recv_len == 5)) {
                unsigned long t;
                t = i2c_recv_buf[1];
                t = (t << 8) | i2c_recv_buf[2];
                t = (t << 8) | i2c_recv_buf[3];
                t = (t << 8) | i2c_recv_buf[4];
                time(t);

        } else if ((command == CMD_POWEROFF) 
                   && (i2c_recv_len == 3)) {
                new_wakeup = (i2c_recv_buf[1] << 8) | i2c_recv_buf[2];

        } else if ((command == CMD_SENSORS) 
                   && (i2c_recv_len == 2)) {
                new_sensors = i2c_recv_buf[1];

        } else if ((command == CMD_PERIOD) 
                   && (i2c_recv_len == 2)) {
                new_period = i2c_recv_buf[1];

        } else if ((command == CMD_STATE) 
                   && (i2c_recv_len == 2)) {
                new_state = i2c_recv_buf[1];

        } else if (command == CMD_FRAMES) {
                // Do nothing here

        } else if (command == CMD_START) {
                read_index = 0;

        } else if (command == CMD_CHECKSUM) {
                // Do nothing here

        } else if (command == CMD_READ) {
                // Do nothing here

        } else if (command == CMD_PUMP) {
                new_pump = i2c_recv_buf[1];
        }
}

static void send_data()
{
        unsigned char command = i2c_recv_buf[0];

        if (command == DS1374_REG_TOD0) {
                unsigned long t = time(0);
                i2c_send_len = 4;
                i2c_send_buf[0] = (t & 0xff000000) >> 24;
                i2c_send_buf[1] = (t & 0x00ff0000) >> 16;
                i2c_send_buf[2] = (t & 0x0000ff00) >> 8;
                i2c_send_buf[3] = (t & 0x000000ff);

        } else if ((command == DS1374_REG_TOD1)
                   || (command == DS1374_REG_TOD2)
                   || (command == DS1374_REG_TOD3)) {
                i2c_send_len = 4;
                i2c_send_buf[0] = 0;
                i2c_send_buf[1] = 0;
                i2c_send_buf[2] = 0;
                i2c_send_buf[3] = 0;

        } else if ((command == DS1374_REG_WDALM0)
                   || (command == DS1374_REG_WDALM1)
                   || (command == DS1374_REG_WDALM2)) {
                i2c_send_len = 3;
                i2c_send_buf[0] = 0;
                i2c_send_buf[1] = 0;
                i2c_send_buf[2] = 0;

        } else if ((command == DS1374_REG_CR)
                   || (command == DS1374_REG_SR)) {
                i2c_send_len = 1;
                i2c_send_buf[0] = 0;

        } else if (command == CMD_POWEROFF) {
                i2c_send_len = 2;
                i2c_send_buf[0] = (wakeup & 0xff00) >> 8;
                i2c_send_buf[1] = (wakeup & 0x00ff);

        } else if (command == CMD_SENSORS) {
                i2c_send_len = 1;
                i2c_send_buf[0] = sensors;

        } else if (command == CMD_PERIOD) {
                i2c_send_len = 1;
                i2c_send_buf[0] = period;

        } else if (command == CMD_STATE) {
                i2c_send_len = 1;
                i2c_send_buf[0] = state;

        } else if (command == CMD_FRAMES) {
                i2c_send_len = 2;
                i2c_send_buf[0] = (stack_num_frames() & 0xff00) >> 8; 
                i2c_send_buf[1] = (stack_num_frames() & 0x00ff);

        } else if (command == CMD_START) {
                /* nothing to do here */

        } else if (command == CMD_READ) {
                unsigned char* ptr = stack_address();
                unsigned short len = stack_bytesize();
                i2c_send_len = 4;
                if ((len == 0) || (read_index >= len)) {
                        i2c_send_buf[0] = 0;
                        i2c_send_buf[1] = 0;
                        i2c_send_buf[2] = 0;
                        i2c_send_buf[3] = 0;
                } else  {
                        i2c_send_buf[0] = ptr[read_index++];
                        i2c_send_buf[1] = ptr[read_index++];
                        i2c_send_buf[2] = ptr[read_index++];
                        i2c_send_buf[3] = ptr[read_index++]; 
                }

        } else if (command == CMD_PUMP) {

        } else if (command == CMD_CHECKSUM) {
                i2c_send_len = 1;
                i2c_send_buf[0] = stack_checksum();
        }
        
        Wire.write(i2c_send_buf, i2c_send_len);

#if DEBUG
        i2c_send = 1;
#endif
}

/*
 * Sensors & measurements
 */

static float get_luminosity()
{
        return 0.0f;
}

static float get_soilhumidity()
{
        return 0.0f;
}

static int get_rht03(DHT22* sensor, float* t, float* h)
{ 
        delay(2000);
  
        for (int i = 0; i < 10; i++) {
                DHT22_ERROR_t errorCode = sensor->readData();
                if (errorCode == DHT_ERROR_NONE) {
                        *t = sensor->getTemperatureC();
                        *h = sensor->getHumidity();
                        return 0;
                }
                //Serial.print("rht03 error ");
                //Serial.println(errorCode);
                delay(200);  
        }
        return -1;
}

static void measure_sensors()
{  
        DebugPrint("  measure_sensors");

        if (!hastime()) {
                DebugPrint("  *TIME NOT SET*");
                return;
        }

        unsigned short old_sp = stack_frame_begin();
        
        if (state != STATE_MEASURING) {
                DebugPrint("  *STATE != MEASURING*");
                return;
        }
        if (new_state != 0) {
                /* Handle state change first */
                DebugPrint("  *NEW_STATE IS SET*");
                return;
        }

        if (!stack_pushi(time(0)))
                goto unroll_stack;

        if (sensors & SENSOR_TRH) {
                float t, rh; 
                if (get_rht03(&rht03_1, &t, &rh) == 0) {
                        if (!stack_pushf(t))
                                goto unroll_stack;
                        if (!stack_pushf(rh))
                                goto unroll_stack;
                        DebugPrintValue("  t ", t);
                        DebugPrintValue("  rh ", rh);
                } else {
                        if (!stack_pushf(-300.0f))
                                goto unroll_stack;
                        if (!stack_pushf(-1.0f))
                                goto unroll_stack;
                }
        }
        if (sensors & SENSOR_TRHX) {
                float t, rh; 
                if (get_rht03(&rht03_2, &t, &rh) == 0) {
                        if (!stack_pushf(t))
                                goto unroll_stack;
                        if (!stack_pushf(rh))
                                goto unroll_stack;
                        DebugPrintValue("  tx ", t);
                        DebugPrintValue("  rhx ", rh);
                } else {
                        if (!stack_pushf(-300.0f))
                                goto unroll_stack;
                        if (!stack_pushf(-1.0f))
                                goto unroll_stack;
                }
        }

        if (sensors & SENSOR_LUM) {
                float luminosity = get_luminosity(); 
                if (!stack_pushf(luminosity))
                        goto unroll_stack;
                DebugPrintValue("  lum ", luminosity);
        }

        if (sensors & SENSOR_SOIL) {
                float humidity = get_soilhumidity(); 
                if (!stack_pushf(humidity))
                        goto unroll_stack;
                DebugPrintValue("  soil ", humidity);
        }


        // Block I2C interupts
        noInterrupts();

        if (new_state != 0) {
                /* We've been interrupted by an I2C state change
                   request during the measurements. Roll back. */
                stack_frame_unroll(old_sp);
        } else {
                /* Update the frame count and the checksum. */
                stack_frame_end();
        }

        // Enable I2C interupts
        interrupts();


        // DEBUG
        /* if (1) { */
        /*         unsigned char* data = stack_address(); */
        /*         int len = stack_bytesize(); */
        /*         unsigned char crc2 = crc8(0, data, len); */
        /*         DebugPrintValue("CRC (1): ", _stack.checksum); */
        /*         DebugPrintValue("CRC (2): ", crc2);         */
        /*         for (int i = 0; i < len; i++) { */
        /*                 Serial.print(data[i], HEX); */
        /*                 if ((i % 4) == 3) */
        /*                         Serial.println(); */
        /*                 else  */
        /*                         Serial.print(" "); */
        /*         } */
        /* } */

        return;

 unroll_stack:
        stack_frame_unroll(old_sp);
        return;
}


/*
 * Utility functions
 */

static unsigned char count_sensors(unsigned char s)
{  
        unsigned char n = 0;
        if (s & SENSOR_TRH) 
                n += 2;
        if (s & SENSOR_TRHX)
                n += 2;
        if (s & SENSOR_LUM) 
                n += 1;
        if (s & SENSOR_SOIL)
                n += 1;
        return n;
}

static void blink(int count, int msec)
{  
        int i;
        for (i = 0; i < count; i++) {
                digitalWrite(13, HIGH);
                delay(msec); 
                digitalWrite(13, LOW);
                delay(100); 
        }                
}

static void handle_updates()
{  
#if DEBUG
        /* if (i2c_recv) { */
        /*         for (int i = 0; i < i2c_recv_len; i++) */
        /*                 DebugPrintValue("  I2C receive buffer: ", i2c_recv_buf[i]); */
        /*         i2c_recv = 0; */
        /* } */
        /* if (i2c_send) { */
        /*         for (int i = 0; i < i2c_send_len; i++) */
        /*                 DebugPrintValue("  I2C send buffer: ", i2c_send_buf[i]); */
        /*         i2c_send = 0; */
        /* } */
#endif
        
        if (sensors != new_sensors) {
                DebugPrintValue("  new sensor settings: ", new_sensors);
                sensors = new_sensors;
                stack_clear();
                stack_set_framesize(1 + count_sensors(sensors));
        }
        
        if (period != new_period) {
                DebugPrintValue("  new period: ", new_period);
                period = new_period;
        }

        if (new_state != 0) {
                if ((state != new_state)
                    && ((new_state == STATE_MEASURING)
                        || (new_state == STATE_RESETSTACK)
                        || (new_state == STATE_SUSPEND))) {
                        DebugPrintValue("  new state settings: ", new_state);
                        state = new_state;
                        if (state == STATE_SUSPEND) 
                                suspend_start = getminutes();
                }
                new_state = 0;
        }

        if (new_wakeup != wakeup) {
                DebugPrintValue("  poweroff: wakeup at ", new_wakeup); 
                poweroff = 2;
                wakeup = new_wakeup;
                if (wakeup < 3) 
                        wakeup = 3;
                new_wakeup = 0;
        }
}

/*
 * Arduino main functions
 */

void setup() 
{
        Serial.begin(9600);

        pinMode(13, OUTPUT);
        digitalWrite(13, LOW);

        // initialize i2c as slave
        Wire.begin(SLAVE_ADDRESS);
        
        // define callbacks for i2c communication
        Wire.onReceive(receive_data);
        Wire.onRequest(send_data);

        // By default, start-up the RPi. The RPi will
        // tell the Arduino when to shut it down again.
        // However, first make sure the RPi was completely
        // shut down (pull down the pin) so it boots up correctly.
        pinMode(RPi_PIN, OUTPUT);

        digitalWrite(RPi_PIN, LOW);
        delay(1000);  
        digitalWrite(RPi_PIN, HIGH);

        pinMode(PUMP_PIN, OUTPUT);
        digitalWrite(PUMP_PIN, LOW);

        last_minute = getminutes();
        stack_clear();
        stack_set_framesize(1 + count_sensors(sensors));

        DebugPrint("Ready.");  
}

void loop()
{  
        unsigned long seconds = getseconds();
        unsigned long minutes = seconds / 60;
        unsigned long sleep = 0;

        handle_updates();

        DebugPrint("--begin loop--"); 
        DebugPrintValue("  time         ", time(0)); 
        DebugPrintValue("  seconds      ", seconds); 
        DebugPrintValue("  minutes      ", minutes); 
        DebugPrintValue("  stack size   ", _stack.sp); 
        DebugPrintValue("  stack frames ", _stack.frames); 
        DebugPrintValue("  measure      ", measure);
        DebugPrintValue("  period       ", period);
        DebugPrintValue("  poweroff     ", poweroff);
        DebugPrintValue("  wakeup       ", wakeup);
        DebugPrintValue("  state        ", state);

        if (state == STATE_SUSPEND) {
                
                /* Do short sleeps until the transfer is done. */
                sleep = 100;

                /* In case the data download failed and the arduino
                   was not resumed correctly, start measuring again
                   after one minute. */
                if (minutes - suspend_start > 3) {
                        DebugPrint("  TRANSFER TIMEOUT"); 
                        state = STATE_MEASURING;
                }

        } else if (state == STATE_RESETSTACK) {
                
                /* Handle a stack reset request after a data transfer
                   or a change in the sensor configuration. */
                DebugPrint("  stack reset"); 
                stack_clear();
                state = STATE_MEASURING;

        } else if (state == STATE_MEASURING) {

                /* Measure (and other stuff) */

                blink(1, 100);

                while (last_minute < minutes) {

                        if (measure > 0) {
                                measure--;
                                if (measure == 0) {
                                        measure_sensors();
                                        measure = period;
                                }
                        }
                        if (poweroff > 0) {
                                poweroff--;
                                if (poweroff == 0) {
                                        digitalWrite(RPi_PIN, LOW);
                                        DebugPrint("  POWEROFF"); 
                                }
                        }
                        if (wakeup > 0) {
                                wakeup--;
                                if (wakeup == 0) {
                                        digitalWrite(RPi_PIN, HIGH);
                                        DebugPrint("  WAKEUP"); 
                                }
                        }

                        last_minute++;
                }

                /* Subtract the time it took to take the measurements
                   from a half minute sleep. */
                sleep = MAX_SLEEP - 1000 * (getseconds() - seconds); 
                if (sleep > MAX_SLEEP)
                        sleep = 0;

        } else {
                state = STATE_MEASURING;
        }

        DebugPrint("--end loop--"); 
        DebugPrint(""); 

        delay(sleep); 
}


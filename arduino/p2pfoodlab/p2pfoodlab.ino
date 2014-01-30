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

#define RHT03_1_FLAG            (1 << 0)
#define RHT03_2_FLAG            (1 << 1)
#define SHT15_1_FLAG            (1 << 2)
#define SHT15_2_FLAG            (1 << 3)
#define LUMINOSITY_FLAG         (1 << 4)
#define SOIL_FLAG               (1 << 5)

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
unsigned char sensors = RHT03_1_FLAG;
unsigned char state = STATE_MEASURING; 
unsigned char period = 1; 
unsigned char measure = 1;
unsigned short read_index = 0;
unsigned short poweroff = 0;
unsigned short wakeup = 0;
unsigned long last_minute = 0;
unsigned long suspend_start = 0;
unsigned char new_sensors = RHT03_1_FLAG;
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

/* The timestamps and sensor data are pushed onto the stack until the
   RPi downloads it. */
#define STACK_SIZE 128
unsigned short frames = 0;
unsigned short sp = 0;

typedef union _stack_entry_t {
        unsigned long i;
        float f;
} stack_entry_t;

stack_entry_t stack[STACK_SIZE];

void stack_clear()
{
        sp = 0;
        frames = 0;
}

void stack_reset(int newsp)
{
        sp = newsp;
}

int stack_pushf(float value)
{
        if (sp < STACK_SIZE) {
                stack[sp++].f = value;
                return 1;
        }
        DebugPrint("  STACK FULL");
        return 0;
}

int stack_pushi(unsigned long value)
{
        if (sp < STACK_SIZE) {
                stack[sp++].i = value;
                return 1;
        }
        DebugPrint("  STACK FULL");
        return 0;
}

unsigned long getseconds()
{
        static unsigned long _seconds = 0;
        unsigned long s = millis() / 1000;

        while (s < _seconds)
                s += 4294967; // 2^32 milliseconds
        _seconds = s;
        return _seconds;
}

unsigned long getminutes()
{
        return getseconds() / 60;
}

static unsigned long _start = 0;

unsigned long time(unsigned long t)
{
        unsigned long s = getseconds();
        if (t)
                _start = t - s;
        return _start + s;
}

unsigned char hastime()
{
        return _start != 0;
}

void receive_data(int byteCount)
{
#if DEBUG
        i2c_recv = 1;
#endif

        i2c_recv_len = 0;
        
        for (int i = 0; i < byteCount; i++) {
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

        } else if (command == CMD_READ) {
                // Do nothing here

        } else if (command == CMD_PUMP) {
                new_pump = i2c_recv_buf[1];
        }
}

void send_data()
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
                i2c_send_buf[0] = (frames & 0xff00) >> 8; 
                i2c_send_buf[1] = (frames & 0x00ff);

        } else if (command == CMD_START) {
                /* nothing to do here */

        } else if (command == CMD_READ) {
                unsigned long* data = (unsigned long*) &stack[0];
                unsigned long v = data[read_index];
                i2c_send_len = 4;
                i2c_send_buf[0] = (v & 0xff000000) >> 24; 
                i2c_send_buf[1] = (v & 0x00ff0000) >> 16; 
                i2c_send_buf[2] = (v & 0x0000ff00) >> 8; 
                i2c_send_buf[3] = (v & 0x000000fff); 
                if (++read_index >= sp)
                        read_index = sp - 1;

        } else if (command == CMD_PUMP) {
        }
        
        Wire.write(i2c_send_buf, i2c_send_len);

#if DEBUG
        i2c_send = 1;
#endif
}

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

        DebugPrint("Ready.");  
}

float get_luminosity()
{
        delay(100); 
        float x = 100.0f * analogRead(LUMINOSITY_PIN) / 1024.0f;
        return x;
}

int get_rht03(DHT22* sensor, float* t, float* h)
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

void measure_sensors()
{  
        DebugPrint("  measure_sensors");

        if (!hastime()) {
                DebugPrint("  *TIME NOT SET*");
                return;
        }

        unsigned short old_sp = sp;        

        if (!stack_pushi(time(0)))
                goto reset_stack;

        if (sensors & LUMINOSITY_FLAG) {
                float luminosity = get_luminosity(); 
                if (!stack_pushf(luminosity))
                        goto reset_stack;
                DebugPrintValue("  lum ", luminosity);
        }
        if (sensors & RHT03_1_FLAG) {
                float t, rh; 
                if (get_rht03(&rht03_1, &t, &rh) == 0) {
                        if (!stack_pushf(t))
                                goto reset_stack;
                        if (!stack_pushf(rh))
                                goto reset_stack;
                        DebugPrintValue("  t ", t);
                        DebugPrintValue("  rh ", rh);
                } else {
                        if (!stack_pushf(-300.0f))
                                goto reset_stack;
                        if (!stack_pushf(-1.0f))
                                goto reset_stack;
                }
        }
        if (sensors & RHT03_2_FLAG) {
                float t, rh; 
                if (get_rht03(&rht03_2, &t, &rh) == 0) {
                        if (!stack_pushf(t))
                                goto reset_stack;
                        if (!stack_pushf(rh))
                                goto reset_stack;
                        DebugPrintValue("  tx ", t);
                        DebugPrintValue("  rhx ", rh);
                } else {
                        if (!stack_pushf(-300.0f))
                                goto reset_stack;
                        if (!stack_pushf(-1.0f))
                                goto reset_stack;
                }
        }

        frames++;
        return;

 reset_stack:
        stack_reset(old_sp);
        return;
}

void blink(int count, int msec)
{  
        int i;
        for (i = 0; i < count; i++) {
                digitalWrite(13, HIGH);
                delay(msec); 
                digitalWrite(13, LOW);
                delay(100); 
        }                
}

void handle_updates()
{  
#if DEBUG
        if (i2c_recv) {
                for (int i = 0; i < i2c_recv_len; i++)
                        DebugPrintValue("  I2C receive buffer: ", i2c_recv_buf[i]);
                i2c_recv = 0;
        }
        if (i2c_send) {
                for (int i = 0; i < i2c_send_len; i++)
                        DebugPrintValue("  I2C send buffer: ", i2c_send_buf[i]);
                i2c_send = 0;
        }
#endif
        
        if (sensors != new_sensors) {
                DebugPrintValue("  new sensor settings: ", new_sensors);
                sensors = new_sensors;
                stack_clear();
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
        DebugPrintValue("  stack size   ", sp); 
        DebugPrintValue("  stack frames ", frames); 
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


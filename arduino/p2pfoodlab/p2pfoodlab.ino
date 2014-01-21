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

#define RHT03_1_PIN 12
#define RHT03_2_PIN 9
#define SHT15_1_DATA_PIN 4
#define SHT15_1_CLOCK_PIN 3
#define LUMINOSITY_PIN A2
#define RPi_PIN 2
#define PUMP_PIN 10

#define SLAVE_ADDRESS 0x04

#define CMD_MASK 0xf0
#define ARG_MASK 0x0f
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

#define RHT03_1_FLAG (1 << 0)
#define RHT03_2_FLAG (1 << 1)
#define SHT15_1_FLAG (1 << 2)
#define SHT15_2_FLAG (1 << 3)
#define LUMINOSITY_FLAG (1 << 4)
#define SOIL_FLAG (1 << 5)

#define DEBUG 1

#if DEBUG
#define DebugPrintValue(_s,_v) { Serial.print(_s); Serial.println(_v); } 
#else
#define DebugPrintValue(_s,_w)
#endif

/* The number of minutes before the Atmel timer overflows and wraps
   around. */
#define MINUTE_OVERFLOW 71582

/* The update period and poweroff/wakeup times are measured in
   minutes. */
unsigned char command;
unsigned char do_update = 1;
unsigned char send_byte = 0;
unsigned char enabled_sensors = RHT03_1_FLAG;
unsigned short send_index = 0;
unsigned short period = 1; 
unsigned long measure_at = 0;
unsigned long poweroff_at = 0;
unsigned long wakeup_at = 0;
unsigned long curtime = 0;
unsigned long suspend_start = 0;

/* The timestamps and sensor data is pushed onto the stack until the
   RPi downloads it. */
#define STACK_SIZE 256
unsigned short frames = 0;
unsigned short sp = 0;
float stack[STACK_SIZE];

DHT22 rht03_1(RHT03_1_PIN);
DHT22 rht03_2(RHT03_2_PIN);


void stack_reset()
{
        sp = 0;
        frames = 0;
}

void stack_push(float value)
{
        if (sp < STACK_SIZE)
                stack[sp++] = value;
}

void receive_data(int byteCount)
{
        int v;
        unsigned char c[8];

        for (int i = 0; i < byteCount; i++) {
                v = Wire.read();
                if (i < 8) c[i] = v & 0xff;
                //Serial.println(v);
        }

        command = c[0];
        send_byte = 0;

        if ((command & CMD_MASK) == CMD_SET_SENSORS) {
                if (byteCount >= 3) {
                        unsigned char old = enabled_sensors;
                        enabled_sensors = c[1];
                        period = c[2];
                        /* Serial.print("enabled_sensors "); */
                        /* Serial.println(enabled_sensors); */
                        if (old != enabled_sensors) {
                                /* Serial.println("set sensors: resetting stack"); */
                                stack_reset();
                        }
                }

        } else if ((command & CMD_MASK) == CMD_GET_MILLIS) {
                //Serial.println("Get millis");
                curtime = millis();

        } else if ((command & CMD_MASK) == CMD_SUSPEND) {
                //Serial.println("Suspend");
                do_update = 0; // Stop the main loop
                suspend_start = millis();

        } else if ((command & CMD_MASK) == CMD_TRANSFER) {
                //Serial.println("TX");
                send_index = 0;

        } else if ((command & CMD_MASK) == CMD_RESUME) {
                //Serial.println("Resume");
                unsigned char reset = command & 0x0f;
                if (reset) stack_reset();
                do_update = 1; // Start the main loop

        } else if ((command & CMD_MASK) == CMD_PUMP) {
                Serial.println("PUMP");
                if ((command & ARG_MASK) == 0)
                        digitalWrite(PUMP_PIN, LOW);
                else
                        digitalWrite(PUMP_PIN, HIGH);
                send_index = 0;

        } else if ((command & CMD_MASK) == CMD_SET_POWEROFF) {
                unsigned long now = millis() / 60000; // in minutes
                if (byteCount >= 3) {
                        unsigned long minutes = (c[1] << 8) | c[2];
                        if (minutes <  2) minutes = 2;
                        poweroff_at = now + 1;
                        wakeup_at = now + minutes;
                        if (poweroff_at == 0) poweroff_at = 1;
                        if (poweroff_at >= MINUTE_OVERFLOW) poweroff_at -= MINUTE_OVERFLOW;
                        if (wakeup_at == 0) wakeup_at = 1;
                        if (wakeup_at >= MINUTE_OVERFLOW) wakeup_at -= MINUTE_OVERFLOW;
                }
        Serial.println("Set poweroff/wakeup");
        Serial.print("bytecount="); 
        Serial.println(byteCount); 

        Serial.print("poweroff_at ");
        Serial.println(poweroff_at);

        Serial.print("wakeup_at ");
        Serial.println(wakeup_at);

        } else if ((command & CMD_MASK) == CMD_GET_POWEROFF) {
                // Nothing to do.
        }
}

void send_data()
{
        if ((command & CMD_MASK) == CMD_GET_MILLIS) {
                unsigned char c;
                switch (send_byte) {
                case 0: c = (curtime & 0xff000000) >> 24; break;
                case 1: c = (curtime & 0x00ff0000) >> 16; break;
                case 2: c = (curtime & 0x0000ff00) >> 8; break;
                case 3: c = (curtime & 0x000000ff); break;
                default: c = 0;
                }                
                Wire.write(c);
                send_byte++;
                //Serial.println("millis[]");

        } else if ((command & CMD_MASK) == CMD_GET_FRAMES) {
                unsigned char c;
                switch (send_byte) {
                case 0: c = (frames & 0xff00) >> 8; break;
                case 1: c = (frames & 0x00ff); break;
                default: c = 0;
                }                
                Wire.write(c);
                send_byte++;

        } else if ((command & CMD_MASK) == CMD_GET_SENSORS) {
                unsigned char c;
                switch (send_byte) {
                case 0: c = enabled_sensors; break;
                case 1: c = period; break;
                default: c = 0;
                }
                Wire.write(c);
                send_byte++;
                
                /* Serial.print("Get_sensors: enabled_sensors "); */
                /* Serial.println(enabled_sensors); */
                /* Serial.print("Get_sensors: period "); */
                /* Serial.println(period); */

        } else if ((command & CMD_MASK) == CMD_TRANSFER) {
                unsigned long* data = (unsigned long*) &stack[0];
                unsigned long v = data[send_index];
                unsigned char c;
  
                switch (send_byte) {
                case 0: c = (v & 0xff000000) >> 24; break;
                case 1: c = (v & 0x00ff0000) >> 16; break;
                case 2: c = (v & 0x0000ff00) >> 8; break;
                case 3: c = (v & 0x000000fff); break;
                default: c = 0;
                }
  
                Wire.write(c);
                //Serial.println("Sent 1 byte");
                
                send_byte++;
                if (send_byte == 4) {
                        send_byte = 0;
                        send_index++;
                }
                if (send_index >= sp) {
                        send_index = 0;
                }
        } else if ((command & CMD_MASK) == CMD_GET_POWEROFF) {
                unsigned char c;
                switch (send_byte) {
                case 0: c = (poweroff_at & 0xff000000) >> 24; break;
                case 1: c = (poweroff_at & 0x00ff0000) >> 16; break;
                case 2: c = (poweroff_at & 0x0000ff00) >> 8; break;
                case 3: c = (poweroff_at & 0x000000ff); break;
                case 4: c = (wakeup_at & 0xff000000) >> 24; break;
                case 5: c = (wakeup_at & 0x00ff0000) >> 16; break;
                case 6: c = (wakeup_at & 0x0000ff00) >> 8; break;
                case 7: c = (wakeup_at & 0x000000ff); break;
                default: c = 0;
                }                
                Wire.write(c);
                send_byte++;
                //Serial.println("millis[]");

        }
}

void setup() 
{
        Serial.begin(9600);

        pinMode(13, OUTPUT);
        digitalWrite(13, HIGH);

        // initialize i2c as slave
        Wire.begin(SLAVE_ADDRESS);
        
        // define callbacks for i2c communication
        Wire.onReceive(receive_data);
        Wire.onRequest(send_data);

        // By default, start-up the RPi. The RPi will
        // tell the Arduino when to shut it down again.
        pinMode(RPi_PIN, OUTPUT);
        digitalWrite(RPi_PIN, HIGH);

        pinMode(PUMP_PIN, OUTPUT);
        digitalWrite(PUMP_PIN, LOW);

#if DEBUG
        Serial.println("Ready.");  
#endif
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

void measure_sensors(float start)
{  

        //Serial.println("measure_sensors");

        stack_push(start);

        if (enabled_sensors & LUMINOSITY_FLAG) {
                float luminosity = get_luminosity(); 
                stack_push(luminosity);
                DebugPrintValue("lum ", luminosity);
        }
        if (enabled_sensors & RHT03_1_FLAG) {

                //Serial.println("RHT03_1");

                float t, rh; 
                if (get_rht03(&rht03_1, &t, &rh) == 0) {
                        stack_push(t);
                        stack_push(rh);
                        DebugPrintValue("t ", t);
                        DebugPrintValue("rh ", rh);
                } else {
                        stack_push(-300.0f);
                        stack_push(-1.0f);
                }
        }
        if (enabled_sensors & RHT03_2_FLAG) {

                //Serial.println("RHT03_2");

                float t, rh; 
                if (get_rht03(&rht03_2, &t, &rh) == 0) {
                        stack_push(t);
                        stack_push(rh);
                        DebugPrintValue("t ", t);
                        DebugPrintValue("rh ", rh);
                } else {
                        stack_push(-300.0f);
                        stack_push(-1.0f);
                }
        }

        frames++;
}

#define TIME_ELAPSED(__cur,__alarm)  ((__cur >= __alarm) && (__cur < __alarm + 3))

void loop()
{  
        unsigned long now = millis();
        unsigned long minutes = now / 60000;

        Serial.println("loop");

        if (!do_update) {
                /* We're transferring data. Do short sleeps until the
                   transfer is done. */
                digitalWrite(13, HIGH);
                delay(100); 
                digitalWrite(13, LOW);
                delay(100);                 
                digitalWrite(13, HIGH);
                delay(100); 
                digitalWrite(13, LOW);
                delay(100); 
                digitalWrite(13, HIGH);
                delay(100); 
                digitalWrite(13, LOW);
                delay(100); 

                /* In case the data download failed and the arduino
                   was not resumed correctly, start measuring again
                   after one minute. */
                if (now - suspend_start > 60000) 
                        do_update = 1;

                return;
        }

        digitalWrite(13, HIGH);

#if DEBUG
        Serial.println("----"); 
        Serial.print("minute ");
        Serial.println(minutes); 

        Serial.print("poweroff_at ");
        Serial.println(poweroff_at);

        Serial.print("wakeup_at ");
        Serial.println(wakeup_at);
#endif

        if (TIME_ELAPSED(minutes, measure_at)) {
                measure_sensors((float) now / 1000.0f);
                measure_at += period;
                if (measure_at >= MINUTE_OVERFLOW) 
                        measure_at -= MINUTE_OVERFLOW;
        }
        if ((poweroff_at != 0) && TIME_ELAPSED(minutes, poweroff_at)) {
                digitalWrite(RPi_PIN, LOW);
                poweroff_at = 0;
        }
        if ((wakeup_at != 0) && TIME_ELAPSED(minutes, wakeup_at)) {
                digitalWrite(RPi_PIN, HIGH);
                wakeup_at = 0;
        }

        delay(100); 
        digitalWrite(13, LOW);

        /* Subtract the time it took to take the measurements
           from the half minute sleep. */
        unsigned long sleep = 30000 - (millis() - now); 
        if (sleep <= 30000)
                delay(sleep); 
        //delay(3000); 
}


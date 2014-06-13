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
#include <Narcoleptic.h>
#include <SoftwareSerial.h>

#define DEBUG 1

#define SLAVE_ADDRESS           0x04
#define V_REF                   3.3f

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

#define DEBUG_STACK             (1 << 0)
#define DEBUG_STATE             (1 << 1)

#define STATE_MEASURING         0
#define STATE_RESETSTACK        1
#define STATE_SUSPEND           2

#define RHT03_1_PIN             4
#define RHT03_2_PIN             2
#define LUMINOSITY_PIN          A2
#define RPi_PIN                 9
#define BAT_USB_PIN             A3
#define PUMP_PIN                8
#define SERIAL_RX_PIN           5
#define SERIAL_TX_PIN           3

#define SENSOR_TRH              (1 << 0)
#define SENSOR_TRHX             (1 << 1)
#define SENSOR_LUM              (1 << 2)
#define SENSOR_USBBAT           (1 << 3)
#define SENSOR_SOIL             (1 << 4)

#define DEFAULT_SLEEP           20000

#if DEBUG
SoftwareSerial debugSerial(SERIAL_RX_PIN, SERIAL_TX_PIN); // RX, TX
#define DebugPrint(_s) { Serial.print(_s); debugSerial.print(_s); } 
#define DebugPrintln(_s) { Serial.println(_s); debugSerial.println(_s); } 
#define DebugPrintlnHex(_s) { Serial.println(_s, HEX); debugSerial.println(_s, HEX); } 
#define DebugPrintValue(_s,_v) { Serial.print(_s); Serial.println(_v); debugSerial.print(_s); debugSerial.println(_v); } 
#else
#define DebugPrint(_s)  {}
#define DebugPrintln(_s) {}
#define DebugPrintlnHex(_s) {}
#define DebugPrintValue(_s,_w) {}
#endif

typedef short sensor_value_t; 

/* The update period and poweroff/wakeup times are measured in
   minutes. */

typedef struct _shortstate_t {
        int sensors: 8;
        int suspend: 1;
        int linux_running: 1;
        int debug: 2;
        int reset_stack: 1;
        int measure_now: 1;
        unsigned char period; 
        unsigned short wakeup;
        unsigned char measurement_index; 
} shortstate_t;

typedef struct _longstate_t {
        int sensors: 8;
        int suspend: 1;
        int linux_running: 1;
        int debug: 2;
        int reset_stack: 1;
        unsigned char period; 
        unsigned short wakeup;
        unsigned char measure;
        unsigned short poweroff;
        unsigned short read_index;
        unsigned long minutes;
        unsigned long suspend_start;
        unsigned char command;
        sensor_value_t measurements[10];
} longstate_t;

longstate_t state;
shortstate_t new_state;

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
 * Time functions
 */

static unsigned long _start = 0;

#define getmilliseconds()  (millis() + Narcoleptic.millis())

static unsigned long getseconds()
{
        static unsigned long _seconds = 0;
        unsigned long s = getmilliseconds() / 1000;

        while (s < _seconds)
                s += 4294967; // 2^32 milliseconds
        _seconds = s;
        return _seconds;
}

static unsigned long time(unsigned long t)
{
        unsigned long s = getseconds();
        if (t) {
                _start = t - s;
        }
        return _start + s;
}

#define getminutes() (getseconds() / 60)
#define hastime() (_start != 0)

/*
 * Stack 
 */

/* The timestamps and sensor data are pushed onto the stack until the
   RPi downloads it. */
#define STACK_SIZE 168

typedef struct _stack_t {
        unsigned long offset;
        unsigned short sp;
        unsigned short frames;
        unsigned char framesize;
        unsigned char checksum;
        unsigned char disabled;
        sensor_value_t values[STACK_SIZE];
} stack_t;

stack_t _stack;

static void stack_clear()
{
        DebugPrintln("*** STACK RESET"); 
        _stack.disabled = 0;
        _stack.sp = 0;
        _stack.frames = 0;
        _stack.checksum = 0;
        _stack.offset = time(0);
}

#define stack_set_framesize(__framesize)  { _stack.framesize = __framesize; }
#define stack_frame_begin()               (_stack.sp)
#define stack_frame_unroll(__sp)          { _stack.sp = __sp; }
#define stack_address()                   ((unsigned char*) &_stack.values[0])
#define stack_bytesize()                  (_stack.frames * _stack.framesize * sizeof(sensor_value_t))
#define stack_frame_bytesize()            (_stack.framesize * sizeof(sensor_value_t))
#define stack_geti(__n)                   (_stack.values[__n].i)
#define stack_checksum()                  (_stack.checksum)
#define stack_offset()                    (_stack.offset)
#define stack_num_frames()                (_stack.frames)
#define stack_disable()                   { _stack.disabled = 1; }
#define stack_enable()                    { _stack.disabled = 0; }

static void stack_frame_end()
{
        if (!hastime()) 
                return; // skip

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

static int stack_pushdate(unsigned long t)
{
        if (!hastime() || _stack.disabled) {
                return 1; // skip
        } else if (_stack.sp < STACK_SIZE) {
                _stack.values[_stack.sp++] = (sensor_value_t) ((t - _stack.offset) / 60);
                return 1;
        } else {
                DebugPrintln("*** STACK FULL ***");
                return 0;
        }
}

static int stack_push(short value)
{
        if (!hastime() || _stack.disabled) {
                return 1; // skip
        } else if (_stack.sp < STACK_SIZE) {
                _stack.values[_stack.sp++] = (sensor_value_t) value;
                return 1;
        } else {
                DebugPrintln("*** STACK FULL ***");
                return 0;
        }
}

static void stack_print()
{
        DebugPrintln("STACK"); 
        DebugPrint("T0  "); DebugPrintln(_stack.offset); 
        DebugPrint("SP  "); DebugPrint(_stack.sp); DebugPrint("/"); DebugPrintln(STACK_SIZE); 
        DebugPrint("#F  "); DebugPrintln(_stack.frames); 
        DebugPrint("FSz "); DebugPrintln(_stack.framesize); 
        DebugPrint("Sum "); DebugPrintlnHex(_stack.checksum); 

        unsigned short index = 0;

        for (unsigned short frame = 0; frame < _stack.frames; frame++) {
                DebugPrint(frame); 
                DebugPrintln("----\t----\t----"); 

                DebugPrint("T\t"); 
                DebugPrint(_stack.values[index]); 
                DebugPrint("\t"); 
                DebugPrintlnHex(_stack.values[index]); 
                index++;

                for (unsigned short val = 1; val < _stack.framesize; val++) {
                        DebugPrint(val); 
                        DebugPrint("\t"); 
                        DebugPrint(_stack.values[index]); 
                        DebugPrint("\t"); 
                        DebugPrintlnHex(_stack.values[index]); 
                        index++;
                }
        }
        DebugPrintln("--------");
}


/*
 * I2C interupt handlers
 */

static void receive_data(int len)
{
        unsigned char recv_buf[5];
        unsigned char recv_len = 0;
                
        for (int i = 0; i < len; i++) {
                int v = Wire.read();
                if (i < sizeof(recv_buf)) 
                        recv_buf[recv_len++] = v & 0xff;
                //DebugPrintln(v);
        }

        state.command = recv_buf[0];

        if ((state.command == DS1374_REG_TOD0) 
            && (recv_len == 5)) {
                unsigned long t;
                int hadtime = hastime();
                t = recv_buf[1];
                t = (t << 8) | recv_buf[2];
                t = (t << 8) | recv_buf[3];
                t = (t << 8) | recv_buf[4];
                time(t);
                if (!hadtime) stack_clear();

        } else if ((state.command == CMD_POWEROFF) 
                   && (recv_len == 3)) {
                new_state.wakeup = (recv_buf[1] << 8) | recv_buf[2];

        } else if ((state.command == CMD_SENSORS) 
                   && (recv_len == 2)) {
                new_state.sensors = recv_buf[1];

        } else if ((state.command == CMD_PERIOD) 
                   && (recv_len == 2)) {
                new_state.period = recv_buf[1];

        } else if ((state.command == CMD_STATE) 
                   && (recv_len == 2)) {
                new_state.reset_stack = (recv_buf[1] == STATE_RESETSTACK);
                new_state.suspend = (recv_buf[1] == STATE_SUSPEND);

        } else if (state.command == CMD_FRAMES) {
                // Do nothing here

        } else if (state.command == CMD_START) {
                state.read_index = 0;

        } else if (state.command == CMD_CHECKSUM) {
                // Do nothing here

        } else if (state.command == CMD_OFFSET) {
                // Do nothing here

        } else if (state.command == CMD_READ) {
                // Do nothing here

        } else if (state.command == CMD_PUMP) {
                // TODO

        } else if ((state.command == CMD_DEBUG) 
                   && (recv_len == 1)) {
                new_state.debug = recv_buf[1];

        } else if (state.command == CMD_MEASURENOW) {
                new_state.measure_now = 1;
                
        } else if (state.command == CMD_MEASUREMENT0) {
                new_state.measurement_index = 0;
                
        } else if (state.command == CMD_GETMEASUREMENT) { 
                // Do nothing here
        }
}

static void send_data()
{
        unsigned char send_buf[4];
        unsigned char send_len = 0;

        if (state.command == DS1374_REG_TOD0) {
                unsigned long t = time(0);
                send_len = 4;
                send_buf[0] = (t & 0xff000000) >> 24;
                send_buf[1] = (t & 0x00ff0000) >> 16;
                send_buf[2] = (t & 0x0000ff00) >> 8;
                send_buf[3] = (t & 0x000000ff);

        } else if ((state.command == DS1374_REG_TOD1)
                   || (state.command == DS1374_REG_TOD2)
                   || (state.command == DS1374_REG_TOD3)) {
                send_len = 4;
                send_buf[0] = 0;
                send_buf[1] = 0;
                send_buf[2] = 0;
                send_buf[3] = 0;

        } else if ((state.command == DS1374_REG_WDALM0)
                   || (state.command == DS1374_REG_WDALM1)
                   || (state.command == DS1374_REG_WDALM2)) {
                send_len = 3;
                send_buf[0] = 0;
                send_buf[1] = 0;
                send_buf[2] = 0;

        } else if ((state.command == DS1374_REG_CR)
                   || (state.command == DS1374_REG_SR)) {
                send_len = 1;
                send_buf[0] = 0;

        } else if (state.command == CMD_POWEROFF) {
                send_len = 2;
                send_buf[0] = (state.wakeup & 0xff00) >> 8;
                send_buf[1] = (state.wakeup & 0x00ff);

        } else if (state.command == CMD_SENSORS) {
                send_len = 1;
                send_buf[0] = state.sensors;

        } else if (state.command == CMD_PERIOD) {
                send_len = 1;
                send_buf[0] = state.period;

        } else if (state.command == CMD_STATE) {
                send_len = 1;
                send_buf[0] = state.suspend;

        } else if (state.command == CMD_FRAMES) {
                send_len = 2;
                send_buf[0] = (stack_num_frames() & 0xff00) >> 8; 
                send_buf[1] = (stack_num_frames() & 0x00ff);

        } else if (state.command == CMD_START) {
                /* nothing to do here */

        } else if (state.command == CMD_READ) {
                unsigned char* ptr = stack_address();
                unsigned short len = stack_bytesize();
                send_len = sizeof(sensor_value_t);
                if ((len == 0) || (state.read_index >= len)) {
                        for (int k = 0; k < sizeof(sensor_value_t); k++)
                                send_buf[k] = 0;
                } else  {
                        for (int k = 0; k < sizeof(sensor_value_t); k++)
                                send_buf[k] = ptr[state.read_index++]; // FIXME: arbitrary handling of endianess...
                }

        } else if (state.command == CMD_PUMP) {

        } else if (state.command == CMD_CHECKSUM) {
                send_len = 1;
                send_buf[0] = stack_checksum();

        } else if (state.command == CMD_OFFSET) {
                unsigned long t = stack_offset();
                send_len = 4;
                send_buf[0] = (t & 0xff000000) >> 24;
                send_buf[1] = (t & 0x00ff0000) >> 16;
                send_buf[2] = (t & 0x0000ff00) >> 8;
                send_buf[3] = (t & 0x000000ff);

        } else if (state.command == CMD_GETMEASUREMENT) { 
                unsigned char* ptr = (unsigned char*) &state.measurements[new_state.measurement_index++];
                send_len = sizeof(sensor_value_t);
                for (int k = 0; k < sizeof(sensor_value_t); k++)
                        send_buf[k] = ptr[k]; // FIXME: arbitrary handling of endianess...
        }
        
        Wire.write(send_buf, send_len);
}

/*
 * Sensors & measurements
 */

static short _usbbat = 0;
static unsigned char _usbbat_count = 0;

static short _get_usbbat()
{
        int i, s;
        for (i = 0; i < 10; i++) {
                analogRead(BAT_USB_PIN);
                delay(10);
        }
        s = 0;
        for (i = 0; i < 10; i++) {
                s += analogRead(BAT_USB_PIN);
                delay(10);
        }
        s = (s + 5) / 10;
        float v = V_REF * 2.0f * s / 1024.0f;
        short r = (short) (100 * v);
        return r;
}

static void update_level_usb_batttery()
{
        int count = (int) _usbbat_count;
        int sum = count * (int) _usbbat;
        int v = _get_usbbat();
        sum += v;
        count++;
        _usbbat = (short) ((sum + count / 2) / count);
        _usbbat_count++;
}

static short get_level_usb_batttery()
{
        short v = _usbbat;
        _usbbat = 0;
        _usbbat_count = 0;
        return v;
}

static short _luminosity = 0;

static short _get_luminosity()
{
        int i, a;
        for (i = 0; i < 5; i++) {
                a = analogRead(LUMINOSITY_PIN);
                delay(100);
        }
        int v = 0;
        for (i = 0; i < 4; i++) {
                a = analogRead(LUMINOSITY_PIN);
                v += a;
                delay(10);
        }
        short r = (short) (v + 2) / 4;
        return r;
}

static void update_luminosity()
{
        float alpha = 0.9f;
        float y = (float) _luminosity;
        float x = (float) _get_luminosity();
        y = (x + alpha * y) / (1.0f + alpha);
        _luminosity = (short) (y + 0.5f);
}

static short get_luminosity()
{
        short v = _luminosity;
        _luminosity = 0;
        return v;
}

static short get_soilhumidity()
{
        return 0;
}

static int get_rht03(DHT22* sensor, short* t, short* h)
{ 
        delay(2000);
  
        for (int i = 0; i < 10; i++) {
                DHT22_ERROR_t errorCode = sensor->readData();
                if (errorCode == DHT_ERROR_NONE) {
                        *t = 10 * sensor->getTemperatureCInt();
                        *h = 10 * sensor->getHumidityInt();
                        return 0;
                }
                DebugPrintValue("** rht03 error: ", errorCode);
                delay(500);  
        }
        return -1;
}

static void update_sensors()
{  
        if (state.sensors & SENSOR_LUM)
                update_luminosity(); 

        if (state.sensors & SENSOR_USBBAT)
                update_level_usb_batttery(); 
}

static void measure_sensors()
{  
        DebugPrintln("MEASURES");

        if (!hastime())
                DebugPrintln("** TIME NOT SET **");

        unsigned short old_sp = stack_frame_begin();
        unsigned char index = 0;
        
        if (state.suspend) {
                DebugPrintln("**  SUSPENDED **");
                return;
        }

        if (!stack_pushdate(time(0)))
                goto unroll_stack;

        if (state.sensors & SENSOR_TRH) {
                short t, rh; 
                if (get_rht03(&rht03_1, &t, &rh) == 0) {
                        state.measurements[index++] = t;
                        state.measurements[index++] = rh;
                        if (!stack_push(t))
                                goto unroll_stack;
                        if (!stack_push(rh))
                                goto unroll_stack;
                        DebugPrintValue("  t ", t);
                        DebugPrintValue("  rh ", rh);
                } else {
                        if (!stack_push(-30000))
                                goto unroll_stack;
                        if (!stack_push(-1))
                                goto unroll_stack;
                }
        }
        if (state.sensors & SENSOR_TRHX) {
                short t, rh; 
                if (get_rht03(&rht03_2, &t, &rh) == 0) {
                        state.measurements[index++] = t;
                        state.measurements[index++] = rh;
                        if (!stack_push(t))
                                goto unroll_stack;
                        if (!stack_push(rh))
                                goto unroll_stack;
                        DebugPrintValue("  tx ", t);
                        DebugPrintValue("  rhx ", rh);
                } else {
                        if (!stack_push(-30000))
                                goto unroll_stack;
                        if (!stack_push(-1))
                                goto unroll_stack;
                }
        }

        if (state.sensors & SENSOR_LUM) {
                short luminosity = get_luminosity(); 
                state.measurements[index++] = luminosity;
                if (!stack_push(luminosity))
                        goto unroll_stack;
                DebugPrintValue("  lum ", luminosity);
        }

        if (state.sensors & SENSOR_USBBAT) {
                short lev = get_level_usb_batttery(); 
                state.measurements[index++] = lev;
                if (!stack_push(lev))
                        goto unroll_stack;
                DebugPrintValue("  usbbat ", lev);
        }

        if (state.sensors & SENSOR_SOIL) {
                short humidity = get_soilhumidity(); 
                state.measurements[index++] = humidity;
                if (!stack_push(humidity))
                        goto unroll_stack;
                DebugPrintValue("  soil ", humidity);
        }

        // Block I2C interupts
        noInterrupts();

        if (state.suspend) {
                /* We've been interrupted by an I2C mode change
                   request during the measurements. Roll back. */
                stack_frame_unroll(old_sp);
        } else {
                /* Update the frame count and the checksum. */
                stack_frame_end();
        }

        // Enable I2C interupts
        interrupts();

        DebugPrintln("--------");

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
        if (s & SENSOR_USBBAT) 
                n += 1;
        if (s & SENSOR_SOIL)
                n += 1;
        return n;
}

static void blink(int count, int msec_on, int msec_off = 0)
{  
        int i;
        for (i = 0; i < count; i++) {
                digitalWrite(13, HIGH);
                delay(msec_on); 
                digitalWrite(13, LOW);
                delay(msec_off); 
        }                
}

static void print_state()
{  
        DebugPrintln("STATE"); delay(200);
        DebugPrint("T "); DebugPrintln(time(0)); delay(200);
        DebugPrint("M "); DebugPrintln(state.minutes); delay(200);
        DebugPrint("SP "); DebugPrintln(_stack.sp); delay(200);
        DebugPrint("#F "); DebugPrintln(_stack.frames); delay(200);
        DebugPrint("P "); DebugPrintln(state.period); delay(200);
        DebugPrint("Sus "); DebugPrintln(state.suspend); delay(200);
        DebugPrint("Mes "); DebugPrintln(state.measure); delay(200);
        DebugPrint("Pow "); DebugPrintln(state.poweroff); delay(200);
        DebugPrint("Wup "); DebugPrintln(state.wakeup); delay(200);
        DebugPrintln("--------");
}

static void handle_updates(unsigned long minutes)
{  
        if (state.sensors != new_state.sensors) {
                DebugPrintValue("*** new sensor settings: ", new_state.sensors);
                state.sensors = new_state.sensors;
                stack_clear();
                stack_set_framesize(1 + count_sensors(state.sensors));
        }
        
        if (state.reset_stack != new_state.reset_stack) {
                state.reset_stack = new_state.reset_stack;
        }

        if (state.period != new_state.period) {
                DebugPrintValue("*** new period: ", new_state.period);
                state.period = new_state.period;
                if (state.period > state.measure) 
                        state.measure = state.period;
        }

        if (state.suspend != new_state.suspend) {
                DebugPrintValue("*** suspend: ", new_state.suspend);
                state.suspend = new_state.suspend;
                if (state.suspend) 
                        state.suspend_start = minutes;
                
        }

        if (new_state.wakeup != 0) {
                if (new_state.wakeup != state.wakeup) {
                        DebugPrintValue("  poweroff: wakeup at ", new_state.wakeup); 
                        state.poweroff = 2;
                        state.wakeup = new_state.wakeup;
                        if (state.wakeup < 3) 
                                state.wakeup = 3;
                }
                new_state.wakeup = 0;
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

        state.suspend = 0;
        state.sensors = SENSOR_TRH | SENSOR_TRHX | SENSOR_LUM | SENSOR_USBBAT;
        state.linux_running = 1;
        state.debug = 0;
        state.reset_stack = 0;
        state.period = 1;
        state.wakeup = 0;
        state.measure = 1;
        state.poweroff = 0;
        state.read_index = 0;
        state.minutes = getminutes();
        state.suspend_start = 0;
        state.command =  0xff;

        new_state.suspend = 0;
        new_state.sensors =  SENSOR_TRH | SENSOR_TRHX | SENSOR_LUM | SENSOR_USBBAT;
        new_state.linux_running = 1;
        new_state.debug = 0;
        new_state.reset_stack = 0;
        
        new_state.period = 1;
        new_state.wakeup = 0;

        stack_clear();
        stack_set_framesize(1 + count_sensors(state.sensors));

#if DEBUG
        debugSerial.begin(9600);
#endif
        DebugPrintln("Ready.");  
}

static void poweroff_linux()
{
        digitalWrite(RPi_PIN, LOW);
        DebugPrintln("*** POWEROFF"); 
        state.linux_running = 0;
        state.poweroff = 0;
}

static void wakeup_linux()
{
        digitalWrite(RPi_PIN, HIGH);
        DebugPrintln("*** WAKEUP"); 
        state.linux_running = 1;
        state.wakeup = 0;
}

void loop()
{  
        unsigned long seconds = getseconds();
        unsigned long minutes = seconds / 60;
        unsigned long sleep = 0;

        handle_updates(minutes);

#if DEBUG
        if (debugSerial.available()) {
                char c = debugSerial.read();
                debugSerial.write(c);

                switch (c) {
                case 'd':
                        state.debug |= DEBUG_STACK | DEBUG_STATE;
                        break;

                case 'r':
                        stack_clear();
                        break;

                case 'w':
                        wakeup_linux();
                        break;

                case 'p':
                        poweroff_linux();
                        break;

                case 'm':
                        stack_disable();
                        measure_sensors();
                        stack_enable();
                        break;
                default:
                        break;
                }
        }
#endif

        if (Serial.available()) {
                int c = Serial.read();
                if (c == 'd') 
                        state.debug |= DEBUG_STACK | DEBUG_STATE;
                /* if (c == 'R') { */
                /*         unsigned char* ptr = stack_address(); */
                /*         unsigned short len = stack_bytesize(); */
                        
                /*         send_len = 4; */
                /*         if ((len == 0) || (state.read_index >= len)) { */
                /*                 send_buf[0] = 0; */
                /*                 send_buf[1] = 0; */
                /*                 send_buf[2] = 0; */
                /*                 send_buf[3] = 0; */
                /*         } else  { */
                /*                 send_buf[0] = ptr[state.read_index++]; */
                /*                 send_buf[1] = ptr[state.read_index++]; */
                /*                 send_buf[2] = ptr[state.read_index++]; */
                /*                 send_buf[3] = ptr[state.read_index++];  */
                /*         } */
                /* } */
        }

        if (state.debug & DEBUG_STATE) {
                print_state();
                state.debug &= ~DEBUG_STATE;
        }

        if (state.debug & DEBUG_STACK) {
                stack_print();
                state.debug &= ~DEBUG_STACK;
        }

        if (new_state.measure_now) {
                stack_disable();
                measure_sensors();
                stack_enable();
                new_state.measure_now = 0;
        }

        if (state.suspend) {
                
                /* Do short sleeps until the transfer is done. */
                sleep = 100;

                /* In case the data download failed and the arduino
                   was not resumed correctly, start measuring again
                   after one minute. */
                if (minutes - state.suspend_start > 3) {
                        DebugPrintln("*** TRANSFER TIMEOUT ***"); 
                        new_state.suspend = 0;
                }

        } else if (state.reset_stack) {
                
                /* Handle a stack reset request after a data transfer
                   or a change in the sensor configuration. */
                stack_clear();
                state.reset_stack = 0;
                new_state.reset_stack = 0;

        } else {

                /* Measure (and other stuff) */

                while (state.minutes < minutes) {

                        update_sensors();

                        if (state.measure > 0) {
                                state.measure--;
                                if (state.measure == 0) {
                                        blink(1, 100);
                                        measure_sensors();
                                        //print_stack(); // DEBUG
                                        stack_print(); // DEBUG
                                        state.measure = state.period;
                                }
                        }
                        if (state.poweroff > 0) {
                                state.poweroff--;
                                if (state.poweroff == 0)
                                        poweroff_linux();
                        }
                        if (state.wakeup > 0) {
                                state.wakeup--;
                                if (state.wakeup == 0)
                                        wakeup_linux();
                        }

                        state.minutes++;
#if DEBUG
                        print_state();
#endif
                }

                sleep = DEFAULT_SLEEP;

        }

        if (sleep) {
                if (state.linux_running)
                        delay(sleep); 
                else 
                        Narcoleptic.delay(sleep); 
        }
}


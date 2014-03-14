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
#include <DHT22.h>

#define RHT03_1_PIN             4
#define RHT03_2_PIN             2
#define LUMINOSITY_PIN          A2
#define BAT_USB_PIN             A3
#define BAT_AAA_PIN             A3
#define V_REF                   3.3f

#define DebugPrint(_s) { Serial.println(_s); } 
#define DebugPrintValue(_s,_v) { Serial.print(_s); Serial.println(_v); } 

DHT22 rht03_1(RHT03_1_PIN);
DHT22 rht03_2(RHT03_2_PIN);

static short get_level_usb_batttery()
{
        int a = analogRead(BAT_USB_PIN);
        DebugPrintValue("  A3 ", a);
        float v = V_REF * 2.0f * a / 1024.0f;
        short r = (short) (100 * v);
        return r;
}

static short get_level_aaa_batttery()
{
        return analogRead(BAT_AAA_PIN);
}

static short get_luminosity()
{
        return analogRead(LUMINOSITY_PIN);
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
                DebugPrintValue("rht03 error: ", errorCode);
                delay(500);  
        }
        return -1;
}

static void measure_sensors()
{  
        DebugPrint("-----------------");
        DebugPrint("  measure");

        if (1) {
                short t, rh; 
                if (get_rht03(&rht03_1, &t, &rh) == 0) {
                        DebugPrintValue("  t ", t);
                        DebugPrintValue("  rh ", rh);
                }
        }
        if (1) {
                short t, rh; 
                if (get_rht03(&rht03_2, &t, &rh) == 0) {
                        DebugPrintValue("  tx ", t);
                        DebugPrintValue("  rhx ", rh);
                }
        }

        if (1) {
                short luminosity = get_luminosity(); 
                DebugPrintValue("  lum ", luminosity);
        }

        if (1) {
                short level = get_level_usb_batttery();
                DebugPrintValue("  usbbat ", level);
        }

        if (0) {
                short humidity = get_soilhumidity(); 
                DebugPrintValue("  soil ", humidity);
        }
}

void setup() 
{
        Serial.begin(9600);
        DebugPrint("Ready.");  
}

void loop()
{  
        measure_sensors();
        delay(5000); 
}


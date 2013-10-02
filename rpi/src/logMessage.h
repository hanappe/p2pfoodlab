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
#ifndef _LOGMESSAGE_H
#define _LOGMESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_DEBUG 0
#define LOG_INFO 1
/* App can continue: */
#define LOG_WARNING 2
/* App cannot continue: */
#define LOG_ERROR 3

int getLogLevel();
void setLogLevel(int level);
void setLogFile(FILE* file);

void logMessage(const char* app, int level, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // _LOGMESSAGE_H

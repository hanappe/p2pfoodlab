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
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include "logMessage.h"

static int _logLevel = 0;
static const char* _logFilename = "/var/p2pfoodlab/log.txt";
static FILE* _logFile = NULL;

void setLogFile(FILE* file)
{
        _logFile = file;
}

int getLogLevel()
{
        return _logLevel;
}

void setLogLevel(int level)
{
        _logLevel = level;
        if (_logLevel < LOG_DEBUG) 
                _logLevel = LOG_DEBUG;
        else if (_logLevel > LOG_ERROR)
                _logLevel = LOG_ERROR;
}

static char _buffer[256];
static time_t _lastTime = 0;

static const char* getTimestamp()
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        if (tv.tv_sec != _lastTime) {
                struct tm r;
                localtime_r(&tv.tv_sec, &r);
                snprintf(_buffer, 256, "%04d%02d%02d-%02d%02d%02d",
                         1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, 
                         r.tm_hour, r.tm_min, r.tm_sec);
        }
        return _buffer;
}

void logMessage(const char* app, int level, const char* format, ...)
{


        if ((level < _logLevel) || (level > LOG_ERROR))
                return;
        FILE* fp = _logFile;
        if (fp == NULL) 
                fp = fopen(_logFilename, "a");
        if (fp) {
                va_list ap;
                va_start(ap, format);
                fprintf(fp, "[%s @ %s] ", app, getTimestamp());
                switch (level) {
                case LOG_DEBUG: fprintf(fp, "Debug: "); break;
                case LOG_INFO: fprintf(fp, "Info: "); break;
                case LOG_WARNING: fprintf(fp, "Warning: "); break;
                case LOG_ERROR: fprintf(fp, "Error: "); break;
                }
                vfprintf(fp, format, ap);
                va_end(ap);
                fflush(fp);
        }
        if (_logFile == NULL) 
                fclose(fp);
}

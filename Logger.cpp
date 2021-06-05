//
//  Logger.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include <stdio.h>
#include <stdarg.h>
#include "Logger.hpp"

#define LOGBUFLEN 16
#define LOGBUFMAX 20

char strbuffer[LOGBUFMAX][256];
int logwriteptr;
int logreadptr;

void initlog()
{
    logwriteptr = 0;
    logreadptr = 0;
}

void log( const char* const fmt, ... )
{
    int nextptr = logwriteptr+1;
    if ( nextptr >= LOGBUFLEN ) nextptr = 0;
    if ( nextptr == logreadptr ) return;
    va_list ap;
    va_start(ap,fmt);
    vsprintf(strbuffer[logwriteptr],fmt,ap);
    logwriteptr = nextptr;
}

void logput()
{
    int lastptr = logwriteptr;
    while ( logreadptr != lastptr ) {
	puts(strbuffer[logreadptr]);
	int nextptr = logreadptr+1;
	if ( nextptr >= LOGBUFLEN ) nextptr = 0;
	logreadptr = nextptr;
    }
}

Logger::Logger() {
}

Logger::~Logger() {
}

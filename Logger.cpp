//
//  Logger.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include <stdarg.h>
#include "ahiru_common.hpp"
#include "Logger.hpp"

// 文字列出力型のログ
#define LOGBUFLEN 16
#define LOGBUFMAX 20

char strbuffer[LOGBUFMAX][256];
int logwriteptr;
int logreadptr;
int logskipped;

void log( const char* const fmt, ... )
{
    int nextptr = logwriteptr+1;
    if ( nextptr >= LOGBUFLEN ) nextptr = 0;
    if ( nextptr == logreadptr ) {
	++logskipped;
	return;
    }
    va_list ap;
    va_start(ap,fmt);
    vsprintf(strbuffer[logwriteptr],fmt,ap);
    logwriteptr = nextptr;
}

// 数値出力型のログ
#define MAXLOGGERS 16
Logger *(loggers[MAXLOGGERS]);
int loggerCount;

Logger::Logger( const char *varnm ) {
    index = readcount = writecount = 0;
    varname = varnm;
    if ( loggerCount < MAXLOGGERS ) loggers[loggerCount++] = this;
}

Logger::~Logger() {
}

void Logger::logging( int32_t value )
{
    values[writecount][index++] = value;
    if ( index >= LOGCACHELEN ) {
	writecount = 1 - writecount;
	index = 0;
    }
}

// ログの初期化
void initlog()
{
    logwriteptr = 0;
    logreadptr = 0;
    logskipped = 0;
    loggerCount = 0;
}

// ログの出力
void logput()
{
    // 文字列出力型のログ
    int lastptr = logwriteptr;
    while ( logreadptr != lastptr ) {
	puts(strbuffer[logreadptr]);
	int nextptr = logreadptr+1;
	if ( nextptr >= LOGBUFLEN ) nextptr = 0;
	logreadptr = nextptr;
    }
    if ( logskipped > 0 ) {
	printf("log skipped %d lines\n",logskipped);
	logskipped = 0;
    }

    // 数値出力型のログ
    /*
    for ( int i = 0; i < loggerCount; ++i ) {
	Logger* logr = loggers[i];
	if ( logr->readcount == logr->writecount ) continue;
	int32_t* vals = logr->values[logr->readcount];
	printf("%s %d %d\n",logr->varname,(int)vals[0],(int)vals[LOGCACHELEN-1]);
	logr->readcount = logr->writecount;
	break;
    }
    */
    char buf[LOGCACHELEN];
    for ( int i = 0; i < loggerCount; ++i ) {
	Logger* logr = loggers[i];
	if ( logr->readcount == logr->writecount ) continue;
	int32_t* vals = logr->values[logr->readcount];
	int32_t max, min;
	max = min = vals[1] - vals[0];
	for ( int j = 2; j < LOGCACHELEN; ++j ) {
	    int32_t dif = vals[j] - vals[j-1];
	    if ( dif < min ) min = dif;
	    if ( dif > max ) max = dif;
	}
	int32_t offset = (min + max)/2 + (0x7e - 0x21)/2 + 0x22;
	for ( int j = 1; j < LOGCACHELEN; ++j ) {
	    int32_t dif = vals[j] - vals[j-1] + offset;
	    buf[j-1] = ( dif < 0x21 ) ? 0x21 : ( dif > 0x7e ) ? 0x7e : dif;
	}
	buf[LOGCACHELEN-1] = 0;
	printf("%s %d %d %s\n",logr->varname,(int)vals[0],(int)offset,buf);

	/*
	for ( int j = 0; j < LOGCACHELEN; ++j ) {
	    printf(" %d",(int)vals[j]);
	}
	printf("\n");
	*/

	logr->readcount = logr->writecount;
    }
}

//
//  Logger.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include<string.h>
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
    va_list ap;
    va_start(ap,fmt);
    vlog(fmt,ap);
}

void vlog( const char* const fmt, va_list ap )
{
    int nextptr = logwriteptr+1;
    if ( nextptr >= LOGBUFLEN ) nextptr = 0;
    if ( nextptr == logreadptr ) {
	++logskipped;
	return;
    }
    vsprintf(strbuffer[logwriteptr],fmt,ap);
    logwriteptr = nextptr;
}

// 数値出力型のログ
#define MAXLOGGERS 10
Logger loggers[MAXLOGGERS];

Logger::Logger() {
    clear();
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

Logger* findLogger( const char* const varname, bool toCreate )
{
    for ( int i = 0; i < MAXLOGGERS; ++i ) {
	if ( strcmp(loggers[i].varname,varname) == 0 ) return &loggers[i];
    }
    if ( !toCreate ) return NULL;
    for ( int i = 0; i < MAXLOGGERS; ++i ) {
	if ( loggers[i].varname == NULL ) {
	    loggers[i].varname = varname;
	    return &loggers[i];
	}
    }
    return NULL;
}

// ロギング開始する
// 開始できたら ture を返す
bool startLogging( const char* const varname )
{
    Logger* logr = findLogger(varname,true);
    if ( logr == NULL ) return false;
    logr->tobedumped = false;
    return true;
}

// ログ出力する
// ログ出力できたら ture を返す
bool logging( const char* const varname, int32_t value )
{
    Logger* logr = findLogger(varname,false);
    if ( logr == NULL || logr->tobedumped ) return false;
    logr->logging(value);
    return true;
}

// ロギング停止する
// ロギング停止できたら ture を返す
bool stopLogging( const char* const varname )
{
    Logger* logr = findLogger(varname,false);
    if ( logr == NULL ) return false;
    logr->tobedumped = true;
    return true;
}

// ログの初期化
void initlog()
{
    logwriteptr = 0;
    logreadptr = 0;
    logskipped = 0;
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
    char buf[LOGCACHELEN];
    for ( int i = 0; i < MAXLOGGERS; ++i ) {
	Logger* logr = &loggers[i];
	if ( logr->varname == NULL ) continue;
	const char* varname = logr->varname;
	if ( logr->tobedumped ) logr->varname = NULL;
	if ( logr->readcount != logr->writecount ) {
	    int offset = logr->dumpTo(buf,LOGCACHELEN);
	    printf("%s %d %d %s\n",varname,(int)logr->values[logr->readcount][0],offset,buf);
	    logr->readcount = logr->writecount;
	}
	if ( logr->tobedumped && logr->index > 0 ) {
	    int offset = logr->dumpTo(buf,logr->index);
	    printf("%s %d %d %s\n",varname,(int)logr->values[logr->readcount][0],offset,buf);
	    logr->index = 0;
	}
    }
}

int Logger::dumpTo( char* buf, int maxlen )
{
    int32_t* vals = values[readcount];
    int32_t max, min;
    max = min =
	( maxlen == 0 ) ? 0 :
	( maxlen == 1 ) ? vals[0] :
	vals[1] - vals[0];
    for ( int j = 2; j < maxlen; ++j ) {
	int32_t dif = vals[j] - vals[j-1];
	if ( dif < min ) min = dif;
	if ( dif > max ) max = dif;
    }
    int32_t offset = (0x7e - 0x21)/2 + 0x22 - (min + max)/2;
    for ( int j = 1; j < maxlen; ++j ) {
	int32_t dif = vals[j] - vals[j-1] + offset;
	buf[j-1] = ( dif < 0x21 ) ? 0x21 : ( dif > 0x7e ) ? 0x7e : dif;
    }
    buf[maxlen-1] = 0;
    return offset;
}

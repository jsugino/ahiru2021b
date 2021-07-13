//
//  Logger.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Logger_hpp
#define Logger_hpp

#include <stdarg.h>

extern void initlog();

// 文字列出力型のログ
extern void log( const char* const fmt, ... );
extern void vlog( const char* const fmt, va_list ap );
extern void logput();

// 数値出力型のログ
extern bool startLogging( const char* const varname ); // 開始できたら ture を返す
extern bool logging( const char* const varname, int32_t value ); // ロギングできたら ture を返す
extern bool stopLogging( const char* const varname ); // 停止できたら ture を返す

#define LOGCACHELEN 250

class Logger {
private:
protected:
public:
    const char *varname;
    int32_t values[2][LOGCACHELEN];
    int index;
    int readcount;
    int writecount;
    bool tobedumped;

    Logger();
    ~Logger();
    void clear() { index = readcount = writecount = 0; varname = NULL; tobedumped = false; }
    void logging( int32_t value );
    int dumpTo( char* buf, int maxlen );
};

#endif /* Logger_hpp */

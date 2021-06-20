//
//  Logger.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Logger_hpp
#define Logger_hpp

extern void initlog();
extern void log( const char* const fmt, ... );
extern void logput();

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

    Logger( const char *varnm );
    ~Logger();
    void logging( int32_t value );
};

#endif /* Logger_hpp */

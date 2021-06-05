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

class Logger {
private:
protected:
public:
    Logger();
    ~Logger();
};

#endif /* Logger_hpp */

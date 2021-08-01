#include "ahiru_common.hpp"
#include "Operator.hpp"

class Trial : public Operator
{
private:
    void (Trial::*trialMethod)();
public:
    Trial( Machine* machine );
    virtual ~Trial();

    // 台形制御実験用
    void azimuthCheck();
    void rampCheck();

    // 台形制御測定用
    void rampTest();
    void rampTestMid();

    // 超音波センサー値確認用
    void sonarTest();

    virtual void trial();
};

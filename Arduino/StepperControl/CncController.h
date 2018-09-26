#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include "CncSpeedController.h"
#include "LastErrorCodes.h"

#include "AnalogPinController/CncInterface.h"

class CncStepper;
class CncController {

  private:
    
    CncStepper* X;
    CncStepper* Y;
    CncStepper* Z;

    CncSpeedController speedController;
    LastErrorCodes* errorInfo;

    int minEnablePulseWide;

    long posReplyThresholdX;
    long posReplyThresholdY;
    long posReplyThresholdZ;

    long positionCounter;
    long positionCounterOverflow;
    
    bool posReplyState;
    bool probeMode;
    bool pause;
    bool I2CAvailable;

    unsigned long lastHeartbeat;

    I2CData lastI2CData;

    inline bool observeSerialFrontByte();
    inline bool observeEnablePin();
    inline bool observeToolState();
    
    inline bool stepAxisXYZ();

    inline bool evaluateAnalogLimitPin(CncInterface::ILS::States& ls);
    inline bool evaluateAnalogSupportPin(CncInterface::ISP::States& ss);

    inline bool evaluateSupportButtonState(unsigned short idx);
    inline bool evaluateSupportSwitchState(unsigned short idx);

    //////////////////////////////////////////////////////////////////////////////
  public:
    //////////////////////////////////////////////////////////////////////////////
    CncController(LastErrorCodes& lec);
                  
    ~CncController();

    //////////////////////////////////////////////////////////////////////////////
    long isReadyToRun();

    //////////////////////////////////////////////////////////////////////////////
    void setupSpeedController();

    const CncSpeedController& getSpeedController() const { return speedController; }
    void setSpeedValue(double fm);
 
    //////////////////////////////////////////////////////////////////////////////
    // this method id used by the CNC stepper to broadbast the current stepper limit information
    bool sendCurrentLimitStates(bool force = false);
    
    //////////////////////////////////////////////////////////////////////////////
    bool evaluateLimitState(const CncStepper* stepper, long& limit);
    bool evaluateLimitStates(long& xLimit, long& yLimit, long& zLimit);

    //////////////////////////////////////////////////////////////////////////////
    bool heartbeat();
    bool idle();

    //////////////////////////////////////////////////////////////////////////////
    bool evaluateSupportButton1State()              { return evaluateSupportButtonState(1); }
    bool evaluateSupportButton2State()              { return evaluateSupportButtonState(2); }
    bool evaluateSupportButton3State()              { return evaluateSupportButtonState(3); }
        
    //////////////////////////////////////////////////////////////////////////////
    void sendCurrentPositions(unsigned char pid, bool force);
    void broadcastInterrupt();

    //////////////////////////////////////////////////////////////////////////////
    void broadcastPause(bool state);

    //////////////////////////////////////////////////////////////////////////////
    CncStepper* getStepperX()                const  { return X; }
    CncStepper* getStepperY()                const  { return Y; }
    CncStepper* getStepperZ()                const  { return Z; }

    //////////////////////////////////////////////////////////////////////////////
    void reset();

    //////////////////////////////////////////////////////////////////////////////
    void setPosReplyState(bool s)                   { posReplyState = s; }
    bool getPosReplyState()                  const  { return posReplyState; }

    void setPosReplyThresholdX(long t)              { posReplyThresholdX = t; }
    void setPosReplyThresholdY(long t)              { posReplyThresholdY = t; }
    void setPosReplyThresholdZ(long t)              { posReplyThresholdZ = t; }
    
    long getPosReplyThresholdX()             const  { return posReplyThresholdX; }
    long getPosReplyThresholdY()             const  { return posReplyThresholdY; }
    long getPosReplyThresholdZ()             const  { return posReplyThresholdZ; }

    //////////////////////////////////////////////////////////////////////////////
    // to get the whole unsigned int32_t range the controller starts counting with MIN_LONG
    void resetPositionCounter()                     { positionCounter = MIN_LONG; positionCounterOverflow = 0L; }
    long getPositionCounter()                const  { return positionCounter; }
    long getPositionCounterOverflow()        const  { return positionCounterOverflow; }
    inline void incPositionCounter();

    //////////////////////////////////////////////////////////////////////////////
    void printConfig();

    //////////////////////////////////////////////////////////////////////////////
    bool isProbeMode()                       const  { return probeMode; }
    void setProbeMode(bool state = true)            { probeMode = state; }
    void enableProbeMode()                          { setProbeMode(true); }
    void disableProbeMode()                         { setProbeMode(false); }
    
    //////////////////////////////////////////////////////////////////////////////
    bool enableStepperPin(bool state = true);
    bool disableStepperPin()                        { return enableStepperPin(false); }

    //////////////////////////////////////////////////////////////////////////////
    bool renderAndStepAxisXYZ(int32_t x1, int32_t y1, int32_t z1);

    //////////////////////////////////////////////////////////////////////////////
    bool evaluateI2CAvailable();
    bool evaluateI2CData();
    bool isI2CAvailable() const { return I2CAvailable; }

};

#endif

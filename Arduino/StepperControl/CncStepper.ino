#include <SoftwareSerial.h>
#include "CommonFunctions.h"
#include "CncStepper.h"

//////////////////////////////////////////////////////////////////////////////
CncStepper::CncStepper(CncController* crtl, char a, byte stpPin, byte dirPin, byte lmtPin, LastErrorCodes& lec)
//////////////////////////////////////////////////////////////////////////////
: INCREMENT_DIRECTION_VALUE(NORMALIZED_INCREMENT_DIRECTION_VALUE)
, interrupted(false)
, calculateDuration(false)
, minReached(false)
, maxReached(false)
, minLimitCnt(0)
, maxLimitCnt(0)
, directionPin(dirPin)
, stepPin(stpPin)
, limitPin(lmtPin)
, axis(a)
, steps(1L)
, pitch(0.0)
, validPitch(false)
, dirPulseWidth(1)
, lowPulsWidth(500)
, highPulsWidth(500)
, avgStepDuartion(0L)
, stepDirection(SD_UNKNOWN)
, stepCounter(0L)
, stepCounterOverflow(0L)
, curPos(0L)
, posReplyThresholdCount(0L)
, controller(crtl)
, errorInfo(lec)
{
}
//////////////////////////////////////////////////////////////////////////////
CncStepper::~CncStepper() {
//////////////////////////////////////////////////////////////////////////////
}
/////////////////////////////////////////////////////////////////////////////////////
int32_t CncStepper::isReadyToRun() {
/////////////////////////////////////////////////////////////////////////////////////
  int32_t ret = 1;

  if ( interrupted == true ) {
    errorInfo.setNextErrorInfo(E_INTERRUPT, BLANK + axis);
    ret = 0;
  }

  if ( readLimitState() != LimitSwitch::LIMIT_UNSET ) {
    errorInfo.setNextErrorInfo(E_LIMIT_SWITCH_ACTIVE, BLANK + axis);
    ret = 0;
  }
  
  return ret;  
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::printConfig() {
//////////////////////////////////////////////////////////////////////////////
  unsigned char pidIncrementDirectionValue = PID_UNKNOWN;
  
  switch ( axis ) {
    case 'X': pidIncrementDirectionValue = PID_INCREMENT_DIRECTION_VALUE_X; 
              break;
    case 'Y': pidIncrementDirectionValue = PID_INCREMENT_DIRECTION_VALUE_Y; 
              break;
    case 'Z': pidIncrementDirectionValue = PID_INCREMENT_DIRECTION_VALUE_Z; 
              break;
  }

  #define PRINT_PARAMETER( Pid, value ) \
    Serial.print(BLANK); \
    Serial.print(BLANK); \
    Serial.print(Pid);   Serial.print(TEXT_SEPARATOR); \
    Serial.print(value); Serial.write(TEXT_CLOSE);

  Serial.print(PID_AXIS); Serial.print(TEXT_SEPARATOR); Serial.print(axis); Serial.write(TEXT_CLOSE);

    PRINT_PARAMETER(PID_PULSE_WIDTH_OFFSET_DIR,           dirPulseWidth)
    PRINT_PARAMETER(PID_PULSE_WIDTH_OFFSET_LOW,           lowPulsWidth)
    PRINT_PARAMETER(PID_PULSE_WIDTH_OFFSET_HIGH,          highPulsWidth)
    PRINT_PARAMETER(PID_STEPS,                            getSteps())
    PRINT_PARAMETER(PID_PITCH,                            getPitch())
    PRINT_PARAMETER(PID_AVG_STEP_DURATION,                avgStepDuartion)
    PRINT_PARAMETER(PID_STEP_PIN,                         getStepPin())
    PRINT_PARAMETER(PID_DIR_PIN,                          getDirectionPin())
    PRINT_PARAMETER(pidIncrementDirectionValue,           getIncrementDirectionValue())
    PRINT_PARAMETER(PID_MIN_SWITCH,                       minReached)
    PRINT_PARAMETER(PID_MAX_SWITCH,                       maxReached)
    PRINT_PARAMETER(PID_LIMIT,                            readLimitState())
    PRINT_PARAMETER(PID_LAST_STEP_DIR,                    getStepDirection())

  #undef PRINT_PARAMETER
}
/////////////////////////////////////////////////////////////////////////////////////
void CncStepper::incStepCounter() { 
/////////////////////////////////////////////////////////////////////////////////////
  // detect overflows
  if ( stepCounter == MAX_LONG ) { 
    stepCounter = MIN_LONG;
    stepCounterOverflow++;
  }
    
  stepCounter++;
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::setPitch(const double p) { 
//////////////////////////////////////////////////////////////////////////////
  validPitch = ( p > 0.0 ); 
  if ( validPitch ) 
    pitch = p;
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::reset() {
//////////////////////////////////////////////////////////////////////////////
  stepDirection         = SD_UNKNOWN;
  avgStepDuartion       = 0L;
  interrupted           = false;
  minReached            = false;
  maxReached            = false;

  resetPosReplyThresholdCouter();
  resetStepCounter();
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::resetDirectionPin() {
//////////////////////////////////////////////////////////////////////////////
  digitalWrite(directionPin, LOW);
}
//////////////////////////////////////////////////////////////////////////////
int32_t CncStepper::calcStepsForMM(int32_t mm) {
//////////////////////////////////////////////////////////////////////////////
  if ( isPitchValid() == false )
    return 0;

  if ( mm == 0 )
    return 0;

  return mm * steps / pitch;
}  
//////////////////////////////////////////////////////////////////////////////
void CncStepper::sendCurrentLimitStates(bool force) {
//////////////////////////////////////////////////////////////////////////////
  controller->sendCurrentLimitStates(force);
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::broadcastInterrupt() {
//////////////////////////////////////////////////////////////////////////////
  controller->broadcastInterrupt();
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::setMinReached(bool state) {
//////////////////////////////////////////////////////////////////////////////
  // avoid hysteresis
  if ( maxReached == true ) {
    minLimitCnt++;
    
    if ( minLimitCnt == 3 ) {
      minLimitCnt = 0;
      minReached = state;
    }
   
  } else {
    minReached = state;
  }

  sendCurrentLimitStates(FORCE);
}
//////////////////////////////////////////////////////////////////////////////
void CncStepper::setMaxReached(bool state) {
//////////////////////////////////////////////////////////////////////////////  
  // avoid hysteresis
  if ( minReached == true ) {
    maxLimitCnt++;
    
    if ( maxLimitCnt == 3 ) {
      maxLimitCnt = 0;
      maxReached = state;
    }
    
  } else {
    maxReached = state;
  }

  sendCurrentLimitStates(FORCE);
}
//////////////////////////////////////////////////////////////////////////////
int32_t CncStepper::readLimitState(int dir) {
//////////////////////////////////////////////////////////////////////////////
  if ( digitalRead(limitPin) == LimitSwitch::LIMIT_SWITCH_OFF )
    return LimitSwitch::LIMIT_UNSET;

  // determine which one . . .
  switch ( dir ) {
    case DIRECTION_INC:   return LimitSwitch::LIMIT_MAX;
    case DIRECTION_DEC:   return LimitSwitch::LIMIT_MIN;
    default:              ;
  }

  // in this case the direction is unclear, try to get more information by calling the controller
  int32_t limit = LimitSwitch::LIMIT_UNKNOWN;
  if ( controller->evaluateLimitState(this, limit) == true )
    return limit;

  // in this case no valid limit information available
  return LimitSwitch::LIMIT_UNKNOWN;
}
//////////////////////////////////////////////////////////////////////////////
int32_t CncStepper::getLimitState() {
//////////////////////////////////////////////////////////////////////////////  
  if ( minReached == true ) return LimitSwitch::LIMIT_MIN;
  if ( maxReached == true ) return LimitSwitch::LIMIT_MAX;
  
  return LimitSwitch::LIMIT_UNSET;
}
//////////////////////////////////////////////////////////////////////////////
bool CncStepper::checkLimit(int dir) {
//////////////////////////////////////////////////////////////////////////////
  int val = digitalRead(limitPin);
  if ( val == LimitSwitch::LIMIT_SWITCH_ON ) {

    // unclear sitiuation avoid movement!
    if ( stepDirection == SD_UNKNOWN ) {
      sendCurrentLimitStates(FORCE);
      broadcastInterrupt();
      return true;
    }

    // enable the move in the opposite direction
    if ( minReached && dir > 0 )
      return false;
      
    // enable the move in the opposite direction
    if ( maxReached && dir < 0 )
      return false;
      
    switch ( dir ) {
      
      case DIRECTION_INC:   setMaxReached(true);
                            return true;
      
      case DIRECTION_DEC:   setMinReached(true);
                            return true;
    }
  } else {
    // reset limit state
    if ( minReached == true || maxReached == true ) {
      setMinReached(false);
      setMaxReached(false);
    }
  }

  return false;
}
//////////////////////////////////////////////////////////////////////////////
bool CncStepper::setDirection(int32_t steps) {
//////////////////////////////////////////////////////////////////////////////
  if      ( steps <  0 ) return setDirection(SD_DEC);
  else if ( steps >= 0 ) return setDirection(SD_INC);

  return false;
}
//////////////////////////////////////////////////////////////////////////////
bool CncStepper::setDirection(const StepDirection sd) {
//////////////////////////////////////////////////////////////////////////////
  if ( stepDirection == sd )
    return true;
  
  stepDirection = sd;
  
  if ( controller->isProbeMode() == OFF ) {
    // The functions getIn/DecrementDirectionValue() switches the physical direction of "stepDirection".
    // The rest of the stepper logic isn't affected because this is to overrule the stepper cabling only
    // and the physical min and max position staying unchanged
    const bool dir = (stepDirection == SD_INC ? getIncrementDirectionValue() : getDecrementDirectionValue());
    
    digitalWrite(directionPin, dir);
    // dont sleep here because a portion of time appears automatically before stepping
    // delayMicroseconds(dirPulseWidth);
  }
  
  return true;
}
//////////////////////////////////////////////////////////////////////////////
bool CncStepper::performNextStep() {
//////////////////////////////////////////////////////////////////////////////
  uint32_t tsStartStepping = calculateDuration ? micros() : 0;

  // first check
  // -----------------------------------------------------------
  // avoid everything in this states
  
  if ( isInterrupted() )
    return false;

  if ( stepDirection == SD_UNKNOWN ) {
    interrupted = true;
    return false;
  }

  if ( checkLimit(stepDirection) == true ) {
    // Case:  A limit is activ
    // TODO: ALWAYS CREATE AN ERROR
    return true;
  }

  // then stepping . . .
  if ( controller->isProbeMode() == OFF ) { 
    
    // start the step puls
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(highPulsWidth); 

    // stop the step puls
    digitalWrite(stepPin, LOW);
    // dont sleep here because a portion of time appears automatically before 
    // the next performNextStep() call appears
    // delayMicroseconds(lowPulsWidth);
  } else {
     delayMicroseconds(highPulsWidth + lowPulsWidth);
  }

  // ----------------------------------------------------------
  // position handling -/+1
  curPos += stepDirection;
  incPosReplyThresholdCouter();
  incStepCounter();

  if ( calculateDuration )
    calcStepLoopDuration(micros() - tsStartStepping);
    
  return true;
}
//////////////////////////////////////////////////////////////////////////////
inline void CncStepper::calcStepLoopDuration(uint32_t lastDuration) {
//////////////////////////////////////////////////////////////////////////////
  if ( avgStepDuartion == 0L )  avgStepDuartion = lastDuration;
  else                          avgStepDuartion = (double)((avgStepDuartion + lastDuration)/2);    
}


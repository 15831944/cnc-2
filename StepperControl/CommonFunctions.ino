#include "CommonValues.h"

//////////////////////////////////////////////////////////////
void sleepMicroseconds(unsigned long usec) {
//////////////////////////////////////////////////////////////
  long milli = usec / 1000;
  long micro = usec % 1000;

  delay(milli);
  delayMicroseconds(micro);
}
//////////////////////////////////////////////////////////////
inline bool dblCompare(double a, double b) {
//////////////////////////////////////////////////////////////
  return (abs(a - b) < epsilon);
}
/////////////////////////////////////////////////////////////////////////////////////
inline bool dblCompareNull(double a) {
/////////////////////////////////////////////////////////////////////////////////////
   return dblCompare(a, 0.0);
}
/////////////////////////////////////////////////////////////////////////////////////
inline bool convertLongToDouble(const long val, double& ret) {
/////////////////////////////////////////////////////////////////////////////////////
  if ( val == MIN_LONG || val == MAX_LONG ) {
    ret = 0.0;
    return false;
  }
  
  ret = ((double)val)/DBL_FACT; 
  return true;
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeByteValue(unsigned char val) {
/////////////////////////////////////////////////////////////////////////////////////
    Serial.write(val);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterValues(unsigned char pid, long val) {
/////////////////////////////////////////////////////////////////////////////////////  
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeByteValue(1);
  writeLongValue(val);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterValues(unsigned char pid, long val1, long val2) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeByteValue(2);
  writeLongValue(val1);
  writeLongValue(val2);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterValues(unsigned char pid, long val1, long val2, long val3) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeByteValue(3);
  writeLongValue(val1);
  writeLongValue(val2);
  writeLongValue(val3);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeLongValue(long val) {
/////////////////////////////////////////////////////////////////////////////////////
  unsigned char buf[sizeof(long)];
  memcpy(buf, &val, sizeof(long));
  Serial.write(buf, sizeof(buf));
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeLongValue(unsigned char pid, long val) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeLongValue(val);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeLongValues(unsigned char pid, long val1, long val2) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeLongValue(val1);
  writeLongValue(val2);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeLongValues(unsigned char pid, long val1, long val2, long val3) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeLongValue(val1);
  writeLongValue(val2);
  writeLongValue(val3);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void sendHeartbeat() {
/////////////////////////////////////////////////////////////////////////////////////
  unsigned long val = millis();
  long v1 = 0;
  long v2 = 0;
  
  if ( val > MAX_LONG ) {
    v1 = MAX_LONG;
    v2 = val - MAX_LONG;    
  } else {
    v1 = val;
    v2 = 0;
  }

  writeLongValues(PID_HEARTBEAT, v1, v2);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void pushMessage(const char type, const char* msg) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_MSG);
  
  switch ( type ) {
    case MT_WARNING:  Serial.write(MT_WARNING); break;
    case MT_ERROR:    Serial.write(MT_ERROR);   break;
    default:          Serial.write(MT_INFO);    break; 
  }
  
  Serial.print(msg);
  Serial.write(MBYTE_CLOSE);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void pushInfoMessage(const char* msg) {
/////////////////////////////////////////////////////////////////////////////////////
  pushMessage(MT_INFO, msg);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void pushWarningMessage(const char* msg) {
/////////////////////////////////////////////////////////////////////////////////////
  pushMessage(MT_WARNING, msg);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void pushErrorMessage(const char* msg) {
/////////////////////////////////////////////////////////////////////////////////////
  pushMessage(MT_ERROR, msg);
}

/////////////////////////////////////////////////////////////////////////////////////
inline bool peakSerial(unsigned char& c) {
  c = '\0';
  
  if ( Serial.available() == 0 )
    return false;
  
  c = Serial.peek();
  return true;
}
/////////////////////////////////////////////////////////////////////////////////////
inline bool checkSerialForPauseCommands(bool currentPauseState) {
/////////////////////////////////////////////////////////////////////////////////////
    bool ret = currentPauseState;
    
    if ( Serial.available() > 0 ) {
      
      switch ( Serial.peek() ) {
        case 'p': ret = false; 
                  // remove the cmd for serial
                  Serial.read(); 
                  break;
                  
        case 'P': ret = true;
                  // remove the cmd for serial
                  Serial.read();  
                  break;
      }
    }

    return ret;
}






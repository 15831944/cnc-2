#include <Wire.h>
#include "CommonValues.h"

//////////////////////////////////////////////////////////////
PinType getPinType(uint8_t pin) {
//////////////////////////////////////////////////////////////
uint8_t port = digitalPinToPort(pin);

if ( port == NOT_A_PIN ) 
  return PT_UNKNOWN;

    // this is uno specific!
    switch ( pin ) {
      case A0: 
      case A1: 
      case A2:
      case A3:
      case A4: 
      case A5: return PT_ANALOG;
    }
    
    return PT_DIGITAL;
}
//////////////////////////////////////////////////////////////
uint8_t getPinMode(uint8_t pin) {
//////////////////////////////////////////////////////////////
  uint8_t bit  = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  if ( port == NOT_A_PIN ) 
    return UNKNOWN_PIN;

  // Is there a bit we can check?
  if ( bit == 0 ) 
    return UNKNOWN_PIN;

  // Is there only a single bit set?
  if ( bit & (bit - 1) ) 
    return UNKNOWN_PIN;

  volatile uint8_t *reg, *out;
  reg = portModeRegister(port);
  out = portOutputRegister(port);

  if      (*reg & bit)  return OUTPUT;
  else if (*out & bit)  return INPUT_PULLUP;
  else                  return INPUT;

  return UNKNOWN_PIN;
}
//////////////////////////////////////////////////////////////
void sleepMicroseconds(unsigned long usec) {
//////////////////////////////////////////////////////////////
  long milli = usec / 1000;
  long micro = usec % 1000;

  if ( milli )
    delay(milli);
    
  delayMicroseconds(micro);
}
//////////////////////////////////////////////////////////////
bool readI2CSlave(I2CData& data) {
//////////////////////////////////////////////////////////////
  Wire.requestFrom(I2C_DEVICE_ID, I2C_BYTE_COUNT);

  if ( Wire.available() <= 0 )
    return false;

  unsigned int counter = 0;
  while ( Wire.available()) {
    switch ( counter ) {
      case I2C_BYTE_LIMIT_STATE:     data.limitState    = Wire.read();  break;
      case I2C_BYTE_SUPPORT_STATE:   data.supportState  = Wire.read();  break;
      default:  return false;
    }
    counter++;
  }

  return true;
}
//////////////////////////////////////////////////////////////
bool sendDataToI2CSlave(const byte data[], unsigned int len) {
//////////////////////////////////////////////////////////////
  if ( len == 0 )
    return false;

  Wire.beginTransmission(I2C_DEVICE_ID);
  int bytes = Wire.write(data, len);            
  byte ret = Wire.endTransmission();  

  /* ret = 
    0: success
    1: data too long to fit in transmit buffer
    2: received NACK on transmit of address
    3: received NACK on transmit of data
    4: other error 
  */
  
  return ( ret == 0 && bytes == (int)len );
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
inline void writeByteValue(unsigned char b) {
/////////////////////////////////////////////////////////////////////////////////////
    Serial.write(b);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeByteValue(unsigned char pid, unsigned char b) {
/////////////////////////////////////////////////////////////////////////////////////  
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeByteValue(b);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeByteValues(unsigned char pid, unsigned char b[], unsigned short size) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  
  for ( unsigned short i=0; i<size; i++ ) {
    writeByteValue(b[i]);
  }
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterListValue(unsigned char pid, long val) {
/////////////////////////////////////////////////////////////////////////////////////  
  Serial.write(pid);
  writeByteValue(1);
  writeLongValue(val);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterListValues(unsigned char pid, long val1, long val2) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(pid);
  writeByteValue(2);
  writeLongValue(val1);
  writeLongValue(val2);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterListValues(unsigned char pid, long val1, long val2, long val3) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(pid);
  writeByteValue(3);
  writeLongValue(val1);
  writeLongValue(val2);
  writeLongValue(val3);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterSingleValue(unsigned char pid, long val) {
/////////////////////////////////////////////////////////////////////////////////////  
  Serial.write(RET_SOH);
  writeGetterListValue(pid, val);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterSingleValues(unsigned char pid, long val1, long val2) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  writeGetterListValues(pid, val1, val2);  
}
/////////////////////////////////////////////////////////////////////////////////////
inline void writeGetterSingleValues(unsigned char pid, long val1, long val2, long val3) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  writeGetterListValues(pid, val1, val2, val3);  
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
inline void writeLongValues(unsigned char pid, long val1, long val2, long val3, long val4) {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(pid);
  writeLongValue(val1);
  writeLongValue(val2);
  writeLongValue(val3);
  writeLongValue(val4);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void sendHeartbeat() {
/////////////////////////////////////////////////////////////////////////////////////
  Serial.write(RET_SOH);
  Serial.write(PID_HEARTBEAT);
  Serial.write((unsigned char)sizeof(long));
  writeLongValue(millis() % MAX_LONG);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void sendHeartbeat(unsigned char limitState) {
/////////////////////////////////////////////////////////////////////////////////////
  unsigned char byteCount = sizeof(long) + 4;

  Serial.write(RET_SOH);
  Serial.write(PID_HEARTBEAT);
  Serial.write(byteCount);
  writeLongValue(millis() % MAX_LONG);

  writeByteValue(limitState);
  writeByteValue(255);
  writeByteValue(255);
  writeByteValue(255);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void sendHeartbeat(unsigned char limitState, unsigned char supportState) {
/////////////////////////////////////////////////////////////////////////////////////
  unsigned char byteCount = sizeof(long) + 4;

  Serial.write(RET_SOH);
  Serial.write(PID_HEARTBEAT);
  Serial.write(byteCount);
  writeLongValue(millis() % MAX_LONG);
  for (unsigned int i=0; i<I2C_BYTE_COUNT; i++) {
    switch ( i ) {
      case I2C_BYTE_LIMIT_STATE:    writeByteValue(limitState);   break;
      case I2C_BYTE_SUPPORT_STATE:  writeByteValue(supportState); break;
    }
  }
  writeByteValue(255);
  writeByteValue(255);
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
  Serial.flush();
}

/////////////////////////////////////////////////////////////////////////////////////
inline void pushInfoMessage(const unsigned char mid) {
/////////////////////////////////////////////////////////////////////////////////////
  char msg[2];
  msg[0] = MT_MID_FLAG;
  msg[1] = mid;
  
  pushInfoMessage(msg);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void pushWarningMessage(const unsigned char mid) {
/////////////////////////////////////////////////////////////////////////////////////
  char msg[2];
  msg[0] = MT_MID_FLAG;
  msg[1] = mid;
  
  pushWarningMessage(msg);
}
/////////////////////////////////////////////////////////////////////////////////////
inline void pushErrorMessage(const unsigned char mid) {
/////////////////////////////////////////////////////////////////////////////////////
  char msg[2];
  msg[0] = MT_MID_FLAG;
  msg[1] = mid;
  
  pushErrorMessage(msg);
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
/////////////////////////////////////////////////////////////////////////////////////
  c = CMD_INVALID;
  
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
        case 'p': ret = PAUSE_INACTIVE; 
                  // remove the cmd for serial
                  Serial.read(); 
                  break;
                  
        case 'P': ret = PAUSE_ACTIVE;
                  // remove the cmd for serial
                  Serial.read();  
                  break;
      }
    }

    return ret;
}





#ifndef ARDUINO_DECODER_MOVE
#define ARDUINO_DECODER_MOVE

#include <SoftwareSerial.h>
#include "ArdoVal.h"
#include "DecBase.h"
#include "MainLoop.h"

// ------------------------------------------------------------------------
class ArduinoCmdDecoderMove : public ArduinoCmdDecoderBase {

  public:
    static const short MAX_MOVE_VALUES = 3;
    
    struct Result {
      byte        cmd = CMD_INVALID;
      int32_t     dx  = 0L;
      int32_t     dy  = 0L;
      int32_t     dz  = 0L;

      void reset() {
        cmd             = CMD_INVALID;
        dx = dy = dz    = 0L;
      }
    };
    
    ArduinoCmdDecoderMove()
    : ArduinoCmdDecoderBase()
    , result  ()
    , b       {0, 0, 0, 0}
    , v       {0, 0, 0}
    , size    (0)
    , count   (0)
    {}
    
    virtual ~ArduinoCmdDecoderMove() 
    {}

  protected:
    virtual byte process(const ArduinoCmdDecoderMove::Result& mv) = 0;

  private:
  
    Result          result;  
    byte            b[MAX_MOVE_VALUES + 1];
    int32_t         v[MAX_MOVE_VALUES];
    uint8_t         size;
    uint8_t         count;

    // ----------------------------------------------------------------------
    void reset() {
      result.reset();
      size    = 0;
      count   = 0;
    }

  public:

    // ----------------------------------------------------------------------
    byte decodeMove(byte cmd) {
      typedef ArduinoMainLoop AML;

      reset();
      result.cmd = cmd;  


#warning decode valueCount as the second byte after the PID

/*

      //fetch 1 to max 3 long values
      while ( true ) {
        
        size = AML::readSerialBytesWithTimeout(b, sizeof(int32_t)) != sizeof(int32_t);
        if ( size != sizeof(int32_t) ) {

          if ( size != 0 ) {
            ARDO_DEBUG_VALUE("size",  (int)size )
            ARDO_DEBUG_VALUE("count", (int)count )
            AML::pushMessage(MT_ERROR, E_INVALID_MOVE_CMD);
            return RET_ERROR;
          }
        }

        // order the received bytes the an int32_t value
        if ( convertBytesToInt32(b, v[count]) == false ) {
          AML::pushMessage(MT_ERROR, E_INVALID_MOVE_CMD);
          return RET_ERROR;
        }
        
        if ( ++count == MAX_MOVE_VALUES )
          break;
      }
*/

      // Wait a protion of time. This is very importent 
      // to give the serial a chance to take a breath
      //AE::delayMicroseconds(1000);
      AML::waitForSerialData();

      //fetch 1 to max 3 long values
      while ( (size = Serial.available()) > 0 ) {
        size = Serial.readBytes(b, sizeof(int32_t));
        
        if ( size != sizeof(int32_t) ) {
          ArduinoMainLoop::pushMessage(MT_ERROR, E_INVALID_MOVE_CMD);
          return RET_ERROR;
        }

        // order the received bytes the an int32_t value
        if ( convertBytesToInt32(b, v[count]) == false ) {
          ArduinoMainLoop::pushMessage(MT_ERROR, E_INVALID_MOVE_CMD);
          return RET_ERROR;
        }
        
        if ( ++count == MAX_MOVE_VALUES )
          break;
          
      } // while

      // delegate values
      switch ( count ) {
        case 1:   result.dx = 0;    result.dy = 0;    result.dz = v[0]; break;
        case 2:   result.dx = v[0]; result.dy = v[1]; result.dz = 0;    break;
        case 3:   result.dx = v[0]; result.dy = v[1]; result.dz= v[2];  break;
        
        default:  AML::pushMessage(MT_ERROR, E_INVALID_MOVE_CMD);
                  return RET_ERROR;
      }

      // process . . .
      return process(result);
    }
};

#endif

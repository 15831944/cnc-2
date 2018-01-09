#ifndef CNC_ARDUINO_INCLUDES
#define CNC_ARDUINO_INCLUDES

#include <string>
#include "../StepperControl/cv.h"

///////////////////////////////////////////////////////////////////////////////////////////
class ArduinoCMDs {
	public:
		static std::string cmds[MAX_CMDS];
		static std::string ret;
		
		/////////////////////////////////////////////////////////////////////////
		ArduinoCMDs() {
		}
		
		/////////////////////////////////////////////////////////////////////////
		~ArduinoCMDs() {
		} 
		
		/////////////////////////////////////////////////////////////////////////
		static void init();
		
		/////////////////////////////////////////////////////////////////////////
		static const char* getCMDLabel(unsigned int id);
		static const char* getCMDLabel(unsigned int id, std::string& retVal);
};

///////////////////////////////////////////////////////////////////////////////////////////
class ArduinoPIDs {
	public:
		static std::string pids[MAX_PIDS];
		static std::string ret;
		
		/////////////////////////////////////////////////////////////////////////
		ArduinoPIDs() {
		}
		
		/////////////////////////////////////////////////////////////////////////
		~ArduinoPIDs() {
		} 
		
		/////////////////////////////////////////////////////////////////////////
		static void init();
		
		/////////////////////////////////////////////////////////////////////////
		static const char* getPIDLabel(unsigned int id);
		static const char* getPIDLabel(unsigned int id, std::string& retVal);
		static const char* getPIDLabelWithDefault(unsigned int id, const std::string& defaultValue);
		
		static bool exists(unsigned int id);
};

///////////////////////////////////////////////////////////////////////////////////////////
class ArduinoErrorCodes {
	public:
		static std::string errorCodes[MAX_ERROR_CODES];
		static std::string ret;
		
		/////////////////////////////////////////////////////////////////////////
		ArduinoErrorCodes() {
		}
		
		/////////////////////////////////////////////////////////////////////////
		~ArduinoErrorCodes() {
		}
		
		/////////////////////////////////////////////////////////////////////////
		static void init();
		
		/////////////////////////////////////////////////////////////////////////
		static const char* getECLabel(unsigned int id);
		static const char* getECLabel(unsigned int id, std::string& retVal);

};

///////////////////////////////////////////////////////////////////////////////////////////
class ArduinoDigitalPins {
	public:
		static std::string pins[MAX_PINS];
		static std::string ret;
		
		/////////////////////////////////////////////////////////////////////////
		ArduinoDigitalPins() {
		}
		
		/////////////////////////////////////////////////////////////////////////
		~ArduinoDigitalPins() {
		} 
		
		/////////////////////////////////////////////////////////////////////////
		static void init();
		
		/////////////////////////////////////////////////////////////////////////
		static const char* getPinLabel(unsigned int id);
		static const char* getPinLabel(unsigned int id, std::string& retVal);
};

///////////////////////////////////////////////////////////////////////////////////////////
class ArduinoAnalogPins {
	public:
		static std::string pins[MAX_PINS];
		static std::string ret;
		
		/////////////////////////////////////////////////////////////////////////
		ArduinoAnalogPins() {
		}
		
		/////////////////////////////////////////////////////////////////////////
		~ArduinoAnalogPins() {
		} 
		
		/////////////////////////////////////////////////////////////////////////
		static void init();
		
		/////////////////////////////////////////////////////////////////////////
		static const char* getPinLabel(unsigned int id);
		static const char* getPinLabel(unsigned int id, std::string& retVal);
};

///////////////////////////////////////////////////////////////////////////////////////////
class Helper {
  
  public:
    static const char* cnvToBinStr(unsigned char c, std::string ret) {
      for(int k = 7; k >=0 ; --k) {
        if( c & (1 << k) )  ret[7-k] = '1';
        else                ret[7-k] = '0'; 
      }
      ret[8] = '\0';
      return ret.c_str();
    }
};

#endif
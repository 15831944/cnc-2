#ifndef ARDUINO_DATA_H
#define ARDUINO_DATA_H

#include <map>
#include <wx/string.h>
#include "ArduinoEnvWrapper.h"

namespace AE {
	
	static const int minPinIndex 	= PN_D0;
	static const int maxPinIndex 	= PN_A5;
	
	static const int minDPinIndex 	= PN_D0;
	static const int maxDPinIndex	= PN_D13;
	
	static const int minAPinIndex 	= PN_A0;
	static const int maxAPinIndex 	= PN_A5;

	static const int maxPins 		= maxPinIndex + 1;
	
	// -----------------------------------------------------
	struct PinData {
		char 	type  	= '\0';
		int 	name 	=   -1;
		int 	uidx 	=   -1;
		char 	mode  	= '\0';
		int 	value 	=   -1;

		PinData()
		: type('\0')
		, name(-1)
		, uidx(-1)
		, mode('\0')
		, value(-1)
		{}

		PinData(char t, int n, int i, char m='\0', int v=-1)
		: type(t)
		, name(n)
		, uidx(i)
		, mode(m)
		, value(v)
		{}
	};

	// -----------------------------------------------------
	struct TransferData {
		
		PinData pins[maxPins];
		
		int32_t		stepperDirX		= DIRECTION_UNKNOWN;
		int32_t		stepperDirY		= DIRECTION_UNKNOWN;
		int32_t		stepperDirZ		= DIRECTION_UNKNOWN;
		
		int32_t		stepperPosX		= 0;
		int32_t		stepperPosY		= 0;
		int32_t		stepperPosZ		= 0;

		TransferData();
		~TransferData();
		
		bool isSomethingChanged(const TransferData& ref);
	};
	
	// stores all configuraion values
	struct ExtraConfiguration {
		bool traceGetters		= false;
		bool traceSetters		= false;
	};
	
	// stores all configuraion values
	struct TraceInformation {
		int32_t		stepperDirX		= DIRECTION_UNKNOWN;
		int32_t		stepperDirY		= DIRECTION_UNKNOWN;
		int32_t		stepperDirZ		= DIRECTION_UNKNOWN;
		
		int32_t		stepperPosX		= 0;
		int32_t		stepperPosY		= 0;
		int32_t		stepperPosZ		= 0;
	};
	
	struct ArduinoData {
		
		typedef std::map<PinName, PinData> 	PinMap;
		
		PinMap  				pins;
		ExtraConfiguration		exterConfig;
		TraceInformation		traceInfo;
		
		void fillTransferData(TransferData& td);
		
		//------------------------------------------------------------
		// initialize setup
		ArduinoData();
		~ArduinoData();
		
		void 				pinMode(PinName pin, PinMode pm);
		void 				digitalWrite(unsigned int pin, bool pl);
		void 				digitalWrite(PinName pin, PinLevel pl);
		PinLevel 			digitalRead(PinName pin);

		uint16_t			analogRead(PinName pin);
		void				analogWrite(PinName pin, int value);

		static bool 		isPin(unsigned int pin);
		static bool 		isDigitalPin(PinName pin);
		static bool 		isAnalogPin(PinName pin);

		static PinLevel 	convertPinLevel(bool state);
		static PinName 		convertPinName(unsigned char pin);
		static PinName		convertPinName(const char type, int name);
		static wxString		buildDislpayName(PinName pin);
		static wxString		buildDislpayName(const char type, int name);
	};
	
};

#endif

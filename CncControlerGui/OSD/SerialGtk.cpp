#ifdef __WXGTK__

#include "SerialGtk.h"

///////////////////////////////////////////////////////////////////
SerialGtk::SerialGtk() 
: connected(false)
///////////////////////////////////////////////////////////////////
{
}
///////////////////////////////////////////////////////////////////
SerialGtk::~SerialGtk() {
///////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////
bool SerialGtk::connect(const char* portName) {
///////////////////////////////////////////////////////////////////
	//We're not yet connected
	connected = false;
	return connected;
}
///////////////////////////////////////////////////////////////////
void SerialGtk::disconnect(void) {
///////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////
void SerialGtk::purge(void) {
///////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////
int SerialGtk::readData(void *buffer, unsigned int nbChar) {
///////////////////////////////////////////////////////////////////
	return 0;
}
///////////////////////////////////////////////////////////////////
bool SerialGtk::writeData(void *buffer, unsigned int nbByte) {
///////////////////////////////////////////////////////////////////
	return false;
}


#endif
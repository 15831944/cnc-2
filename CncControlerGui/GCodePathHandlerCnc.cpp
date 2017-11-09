#include "GCodePathHandlerCnc.h"

//////////////////////////////////////////////////////////////////
GCodePathHandlerCnc::GCodePathHandlerCnc(CncControl* cnc) 
: GCodePathHandlerBase()
, cncControl(cnc)
, previousPos(0.0, 0.0, 0.0)
{
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
}
//////////////////////////////////////////////////////////////////
GCodePathHandlerCnc::~GCodePathHandlerCnc() {
//////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////
void GCodePathHandlerCnc::initNextClientId(long id) {
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
	cncControl->setClientId(id);
}
//////////////////////////////////////////////////////////////////
void GCodePathHandlerCnc::switchToolState(bool state) {
//////////////////////////////////////////////////////////////////
	state == true ? cncControl->switchToolOn() : cncControl->switchToolOff();
}
//////////////////////////////////////////////////////////////////
void GCodePathHandlerCnc::prepareWorkImpl() {
//////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////
void GCodePathHandlerCnc::finishWorkImpl() {
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
	
	currentPos.setXYZ(0.0, 0.0, 0.0);
	
	cncControl->moveXYZToZeroPos();
	cncControl->switchToolOff();
}
//////////////////////////////////////////////////////////////////
bool GCodePathHandlerCnc::processLinearMove(bool alreadyRendered) {
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
	return cncControl->moveAbsLinearMetricXYZ(currentPos.getX(), currentPos.getY(), currentPos.getZ(), alreadyRendered);
}
//////////////////////////////////////////////////////////////////
bool GCodePathHandlerCnc::changeCurrentFeedSpeedXYZ(CncSpeed s, double value) {
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
	
	cncControl->changeCurrentFeedSpeedXYZ(s, value);
	return true;
}

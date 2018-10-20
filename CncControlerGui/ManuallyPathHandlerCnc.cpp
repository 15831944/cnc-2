#include "ManuallyPathHandlerCnc.h"

/////////////////////////////////////////////////////////////
ManuallyPathHandlerCnc::ManuallyPathHandlerCnc(CncControl* cnc)
: PathHandlerBase()
, cncControl(cnc)
, lastSpeedType(CncSpeedRapid)
, lastSpeedValue(0.0)
/////////////////////////////////////////////////////////////
{
}
/////////////////////////////////////////////////////////////
ManuallyPathHandlerCnc::~ManuallyPathHandlerCnc() {
/////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////
void ManuallyPathHandlerCnc::logMeasurementStart() {
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
	cncControl->getSerial()->startMeasurement();
}
//////////////////////////////////////////////////////////////////
void ManuallyPathHandlerCnc::logMeasurementEnd() {
//////////////////////////////////////////////////////////////////
	wxASSERT(cncControl);
	cncControl->getSerial()->stopMeasurement();
}
//////////////////////////////////////////////////////////////////
void ManuallyPathHandlerCnc::prepareWork() {
//////////////////////////////////////////////////////////////////
	wxASSERT( cncControl != NULL );
	cncControl->resetDurationCounter();
}
//////////////////////////////////////////////////////////////////
void ManuallyPathHandlerCnc::finishWork() {
//////////////////////////////////////////////////////////////////
	// curently nothing todo
}
//////////////////////////////////////////////////////////////////
void ManuallyPathHandlerCnc::switchToolState(bool state) {
//////////////////////////////////////////////////////////////////
	state == true ? cncControl->switchToolOn() : cncControl->switchToolOff();
}
//////////////////////////////////////////////////////////////////
bool ManuallyPathHandlerCnc::processLinearMove(const MoveDefinition& md) {
//////////////////////////////////////////////////////////////////
	wxASSERT( cncControl != NULL );
	
	// tool handling
	switchToolState(md.toolState);
	
	// speed handling
	#warning md.speedType is obsolete here
	if ( md.speedType != lastSpeedType  || md.f != lastSpeedValue )
		cncControl->changeCurrentFeedSpeedXYZ_MM_MIN(md.f);
	
	// move
	if ( md.absoluteMove == true ) {
		currentPos.setX(md.x);
		currentPos.setY(md.y);
		currentPos.setZ(md.z);
	} else {
		currentPos.incX(md.x);
		currentPos.incY(md.y);
		currentPos.incZ(md.z);
	}
	
	bool ret = cncControl->moveAbsLinearMetricXYZ(currentPos.getX(), currentPos.getY(), currentPos.getZ(), false);
	if ( ret == false && md.correctLimit == true ) {
		ret = cncControl->correctLimitPositions();
	}
	
	switchToolState(false);
	
	return ret;
}

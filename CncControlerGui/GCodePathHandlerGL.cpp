#include "GCodePathHandlerGL.h"

//////////////////////////////////////////////////////////////////
GCodePathHandlerGL::GCodePathHandlerGL(CncGCodePreview* gl) 
: GCodePathHandlerBase()
, glControl(gl)
, currentSpeed(CncSpeed::CncSpeedRapid)
{
//////////////////////////////////////////////////////////////////
	wxASSERT(glControl);
}
//////////////////////////////////////////////////////////////////
GCodePathHandlerGL::~GCodePathHandlerGL() {
//////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////
void GCodePathHandlerGL::prepareWorkImpl() {
//////////////////////////////////////////////////////////////////
	wxASSERT(glControl);
	glControl->pushProcessMode();
}
//////////////////////////////////////////////////////////////////
void GCodePathHandlerGL::finishWorkImpl() {
//////////////////////////////////////////////////////////////////
	wxASSERT(glControl);
	glControl->popProcessMode();
}
//////////////////////////////////////////////////////////////////
bool GCodePathHandlerGL::processLinearMove(bool alreadyRendered) {
//////////////////////////////////////////////////////////////////
	wxASSERT(glControl);
	
	#warning - consider unit!
	
	static GLI::VerticeDoubleData vd;
	vd.setVertice(0L, currentSpeed, currentPos);
	glControl->appendVertice(vd);
	
	return true; 
}
//////////////////////////////////////////////////////////////////
bool GCodePathHandlerGL::changeWorkSpeedXY(CncSpeed s) {
//////////////////////////////////////////////////////////////////
	currentSpeed = s;
	return true;
}


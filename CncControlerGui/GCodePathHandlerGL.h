#ifndef GCODE_PATH_HANDLER_GL_H
#define GCODE_PATH_HANDLER_GL_H

#include "GCodePathHandlerBase.h"
#include "3D/CncGCodePreview.h"
#include "CncPosition.h"

class GCodePathHandlerGL : public GCodePathHandlerBase {
	
	public:
		GCodePathHandlerGL(CncGCodePreview* gl);
		virtual ~GCodePathHandlerGL();
		
		virtual const char* getName() { return "GCodePathHandlerGL"; }
		
		virtual bool isPathListUsed() { return false; }
		virtual void switchToolState(bool state) {}
		virtual bool shouldAToolChangeProcessed() { return false; }
		
		virtual void logMeasurementStart() {}
		virtual void logMeasurementEnd() {}

	protected:
		
		virtual bool processLinearMove(bool alreadyRendered);
		virtual bool changeCurrentFeedSpeedXYZ(CncSpeedMode sm, double value = 0.0);
		virtual bool initNextPath();
		virtual void prepareWorkImpl();
		virtual void finishWorkImpl();
		
	private:
		CncGCodePreview* glControl;
		CncSpeedMode currentSpeed;
};

#endif
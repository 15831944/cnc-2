#ifndef CNC_GL_CANVAS_H
#define CNC_GL_CANVAS_H

#include "3D/GLContextCncPath.h"
#include "3D/CncMetricRulerSetup.h"
#include "CncPosition.h"
#include "3D/GLInclude.h"

///////////////////////////////////////////////////
class CncGlCanvas : public wxGLCanvas {
		
	public:
		CncGlCanvas(wxWindow *parent, int *attribList = NULL);
		virtual ~CncGlCanvas();
		
		void viewTop() 		{ view(GLContextBase::ViewMode::V2D_TOP); }
		void viewBottom() 	{ view(GLContextBase::ViewMode::V2D_BOTTOM); }
		void viewLeft() 	{ view(GLContextBase::ViewMode::V2D_LEFT); }
		void viewRight()	{ view(GLContextBase::ViewMode::V2D_RIGHT); }
		void viewFront() 	{ view(GLContextBase::ViewMode::V2D_FRONT); }
		void viewRear() 	{ view(GLContextBase::ViewMode::V2D_REAR); }
		
		void viewIso1() 	{ view(GLContextBase::ViewMode::V3D_ISO1); }
		void viewIso2() 	{ view(GLContextBase::ViewMode::V3D_ISO2); }
		void viewIso3() 	{ view(GLContextBase::ViewMode::V3D_ISO3); }
		void viewIso4() 	{ view(GLContextBase::ViewMode::V3D_ISO4); }
		
		void incScale();
		void decScale();
		
		void initReshape();
		void reshapeRelative(int dx, int dy);
		
		bool isContextValid()	{ return lastSetCurrent; }
		
	protected:
		GLContextCncPathBase* 	context;
		wxPoint 				lastReshape;
		bool					lastSetCurrent;
		bool 					mouseMoveMode;
		
		void onMouse(wxMouseEvent& event);
		void view(GLContextBase::ViewMode fm);
};
	
#endif
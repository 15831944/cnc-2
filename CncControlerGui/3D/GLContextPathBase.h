#ifndef OPENGL_CONTEXT_PATH_BASE_H
#define OPENGL_CONTEXT_PATH_BASE_H

#include <cfloat>
#include <vector>
#include "3D/GLCncPathData.h"
#include "3D/GLLineCluster.h"
#include "3D/GLContextBase.h"

/////////////////////////////////////////////////////////////////
class GLContextCncPathBase : public GLContextBase {
	
	public:
	
		enum DrawType { DT_POINTS, DT_LINES, DT_LINE_STRIP };
		
		/////////////////////////////////////////////////////////
		GLContextCncPathBase(wxGLCanvas* canvas);
		virtual ~GLContextCncPathBase();
		
		/////////////////////////////////////////////////////////
		void clearPathData();
		
		/////////////////////////////////////////////////////////
		const GLI::GLCncPath& getPathData() {
			return cncPath;
		}
		
		/////////////////////////////////////////////////////////
		void tracePathData(std::ostream& out) {
			out << "Size: " << cncPath.size() << std::endl;
			unsigned long cnt = 0;
			for( GLI::GLCncPath::iterator it = cncPath.begin(); it < cncPath.end(); ++it ) {
				out << ' ';
				out << wxString::Format("%6lu", cnt++) << ": ";
				out << wxString::Format("%+.10lf", it->getX()) << ", ";
				out << wxString::Format("%+.10lf", it->getY()) << ", ";
				out << wxString::Format("%+.10lf", it->getZ());
				out << std::endl;
			}
		}
		
		/////////////////////////////////////////////////////////
		void appendPathData(const GLI::GLCncPathVertices& cpv);
		void appendPathData(const GLI::GLCncPath& cp);
		
		/////////////////////////////////////////////////////////
		void setCurrentClientId(long id) { currentClientId = id; }
		
		/////////////////////////////////////////////////////////
		GLContextCncPathBase::DrawType getDrawType() 		{ return drawType; }
		void setDrawType(GLContextCncPathBase::DrawType t) 	{ drawType = t; }
		
		// ruler
		/////////////////////////////////////////////////////////
		GLI::GLAxisRuler& getRulerX() 			{ return ruler.xAxis; }
		GLI::GLAxisRuler& getRulerY() 			{ return ruler.yAxis; }
		GLI::GLAxisRuler& getRulerZ() 			{ return ruler.zAxis; }
		
		GLI::GLHelpLineXYPlane& getXYPlane()	{ return ruler.helpLinesXY; }
		GLI::GLHelpLineXZPlane& getXZPlane()	{ return ruler.helpLinesXZ; }
		GLI::GLHelpLineYZPlane& getYZPlane()	{ return ruler.helpLinesYZ; }
		
		// origin
		/////////////////////////////////////////////////////////
		int getOriginX() { return ( viewPort != NULL ? viewPort->getX() : 0 ); }
		int getOriginY() { return ( viewPort != NULL ? viewPort->getY() : 0 ); }
		
		long getPathItemCount()			{ return cncPath.size(); }
		
		void setVirtualEnd(long val) 	{ cncPath.setVirtualEnd(val); }
		void setVirtualEndToFirst() 	{ cncPath.setVirtualEndToFirst(); }
		void setVirtualEndToLast() 		{ cncPath.setVirtualEndToLast(); }
		
		long previewNextVertexId()		{ return cncPath.previewNextVertexId(); }
		long previewPreviousVertexId()	{ return cncPath.previewPreviousVertexId(); }
		
		void incVirtualEnd() 			{ cncPath.incVirtualEnd(); } 
		void decVirtualEnd() 			{ cncPath.decVirtualEnd(); }
		void incVirtualEndById() 		{ cncPath.incVirtualEndById(); }
		void decVirtualEndById() 		{ cncPath.decVirtualEndById(); }
		
		void spoolVertiesForCurrentId() { cncPath.spoolVertiesForCurrentId(); }
		
		const long getVirtualEnd() 		{ return cncPath.getVirtualEnd(); }
		const long getVirtualEndAsId()	{ return cncPath.getVirtualEndAsId(); }
		
		void activateNotifications() 	{ cncPath.activateNotifications(); }
		void deactivateNotifications() 	{ cncPath.deactivateNotifications(); }
		
		void registerCallback(GLI::GLCncPath::Callback* cb) { cncPath.registerCallback(cb); }
		
	protected:
		
		GLI::GLCncPath		cncPath;
		GLI::GLXYZRuler		ruler;
		DrawType 			drawType;
		
		long 				currentClientId;
		
		wxColour			rulerColourX;
		wxColour			rulerColourY;
		wxColour			rulerColourZ;
		
		virtual float getAutoScaleFactor() { return ( options.autoScale ? cncPath.getAutoScaleFact() : 1.0); }
		
		virtual void determineModel();
		virtual void markCurrentPosition();
		
		void drawBoundBox();
		void drawRuler();
		
	private:
		
		void drawPoints();
		void drawLines();
		void drawLineStrips();
		
		void drawPoint(const GLI::GLCncPathVertices& vCurr);
		void drawLine(const GLI::GLCncPathVertices& vCurr, const GLI::GLCncPathVertices& vPrev);
		void drawLineStrip(const GLI::GLCncPathVertices& vCurr);

};

#endif
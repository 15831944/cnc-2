#ifndef CNC_SVG_CONTROL_H
#define CNC_SVG_CONTROL_H

#define WX_SVG_SUPPORT
#define WX_WEBVIEW_SUPPORT


	#ifdef WX_SVG_SUPPORT
		#include <wxSVG/svgctrl.h>
		#define SVG_VIEWER_CLASS_PARENT 		wxSVGCtrl
	#else
		#ifdef WX_WEBVIEW_SUPPORT
			#include <wx/webview.h>
			#define SVG_VIEWER_CLASS_PARENT 	wxWebView
		#esle
			#define SVG_VIEWER_CLASS_PARENT		wxPanel
		#endif
	#endif

//////////////////////////////////////////////////////////////////////////////
class CncSvgViewer : public SVG_VIEWER_CLASS_PARENT {
	
	public:
		CncSvgViewer(wxWindow* parent) 
		: SVG_VIEWER_CLASS_PARENT(parent)
		{ 
			init();
		}
		
		~CncSvgViewer() {
		}
		
		void clear();
		bool loadFile(const wxString& filename);
		
	protected:
		void OnMouse(wxMouseEvent& event);
		
	private:
		void init();
		
	wxDECLARE_EVENT_TABLE();
};

//////////////////////////////////////////////////////////////////////////////
class CncSvgBuilder : public CncSvgViewer {
	
	public:
		CncSvgBuilder(wxWindow* parent) 
		: CncSvgViewer(parent)
		{
			wxASSERT(IsKindOf(wxCLASSINFO(wxSVGCtrl)));
		}
		
		~CncSvgBuilder() {
		}
};

#endif
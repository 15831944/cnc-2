#ifndef CNC_TOOL_MAGAZINE_H
#define CNC_TOOL_MAGAZINE_H

#include "CncConfig.h"
#include "wxcrafter.h"

class CncToolMagazine : public CncToolMagazineBase
{
	public:
		CncToolMagazine(wxWindow* parent);
		virtual ~CncToolMagazine();
		
	protected:
		virtual void cancel(wxCommandEvent& event);
		virtual void addTool(wxCommandEvent& event);
		virtual void editTool(wxCommandEvent& event);
		virtual void removeTool(wxCommandEvent& event);
		virtual void selectedTool(wxListEvent& event);
		
		void configurationUpdated(wxCommandEvent& event);
		
		wxDECLARE_EVENT_TABLE();
		
	private:
		long lastSelectedItem;
		bool insertState;
		
		void selectedItem(const unsigned int index);
		void setInsertState(bool state);
		void getConfigToolMagazine();
		void setConfigToolMagazine();
		bool checkIfIdAlreadyExists(const int newId);
};

#endif // CNCTOOLMAGAZINE_H
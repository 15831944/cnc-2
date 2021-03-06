#ifndef CNC_FILE_VIEW_LIST_CTRL_H
#define CNC_FILE_VIEW_LIST_CTRL_H

#include <wx/filename.h>
#include <wx/timer.h>
#include "CncLargeScaleListCtrl.h"

class wxFileConfig;
class CncLruFileViewListCtrl : public CncLargeScaledListCtrl {
		
	private:
		
		enum EventType { UNKNOWN, PREVIEW, OPEN };
		
		const char* lruSection = "LRU_List";
		const char* lruPrefix  = "LRU_FILE_";

		typedef std::vector<wxFileName> LruList;
		LruList 		lruList;
		wxTimer* 		eventTimer;
		wxListItemAttr 	defaultItemAttr;
		wxListItemAttr 	selectedItemAttr;
		wxMenu* 		popupMenu;
		bool			isLeaveEventActive;
		unsigned int 	maxSize;
		EventType		lastEventType;
			
		virtual wxString OnGetItemText(long item, long column) const;
		virtual int OnGetItemColumnImage(long item, long column) const;
		virtual wxListItemAttr* OnGetItemAttr(long item) const;
		
		void onEventTimer(wxTimerEvent& event);
		void onSize(wxSizeEvent& event);
		void onKeyDown(wxKeyEvent& event);
		void onRightDown(wxMouseEvent& event);
		void onLeaveWindow(wxMouseEvent& event);
		void onSelectListItem(wxListEvent& event);
		void onActivateListItem(wxListEvent& event);
		
		void removeSelectedItem();
		void updateListControl();
		
	protected:
		virtual bool isItemValid(long item) const;
		
	public:
		
		static const int miRemoveLruListEntry		= 8000;
		
		static const int COL_FILE 					= 0;
		static const int TOTAL_COL_COUNT			= 1;
		static const int COL_STRECH					= COL_FILE;
		
		CncLruFileViewListCtrl(wxWindow *parent, unsigned int ms, long style);
		virtual ~CncLruFileViewListCtrl();
		
		virtual bool Enable(bool enable=true);
		
		void updateColumnWidth();
		void selectFirstItem();
		
		unsigned int getFileCount() const ;
		const char* getFileName(unsigned int pos);
		
		bool addFile(const wxString& f);
		bool removeFile(unsigned int idx);
		bool removeFile(const wxString& f);
		bool load(wxFileConfig* config);
		bool save(wxFileConfig* config);
		
		wxDECLARE_NO_COPY_CLASS(CncLruFileViewListCtrl);
		wxDECLARE_EVENT_TABLE();
};


class CncFileViewListCtrl : public CncLargeScaledListCtrl {
	
	public:
		enum FileListImage {
			FTI_FOLDER_UP 		= 0,
			FTI_FOLDER    		= 1,
			FTI_FILE      		= 2,
			FTI_ERROR     		= 3,
			FTI_FILE_SELECTED 	= 4
		};
		
		struct FileEntry {
			FileListImage	imageIdx;
			wxString		fileName;
			
			FileEntry(const wxString& n, FileListImage i) 
			: imageIdx(i)
			, fileName(n)
			{}
		};
		
		typedef std::vector<FileEntry> FileEntryList;
	
	private:
		
		enum EventType { UNKNOWN, PREVIEW, OPEN };
		
		FileEntryList 	fileEntries;
		wxTimer* 		eventTimer;

		wxListItemAttr defaultItemAttr;
		wxListItemAttr selectedItemAttr;
		EventType		lastEventType;
		
		virtual wxString OnGetItemText(long item, long column) const;
		virtual int OnGetItemColumnImage(long item, long column) const;
		virtual wxListItemAttr* OnGetItemAttr(long item) const;
		
		void onEventTimer(wxTimerEvent& event);
		void onSize(wxSizeEvent& event);
		void onLeaveWindow(wxMouseEvent& event);
		void onSelectListItem(wxListEvent& event);
		void onActivateListItem(wxListEvent& event);
		
	protected:
		virtual bool isItemValid(long item) const;

	public:
		static const int COL_FILE 			= 0;
		static const int TOTAL_COL_COUNT	= 1;
		static const int COL_STRECH			= COL_FILE;
		
		CncFileViewListCtrl(wxWindow *parent, long style);
		virtual ~CncFileViewListCtrl();
		
		virtual bool Enable(bool enable=true);
		
		void updateColumnWidth();
		
		void deleteAllEntries();
		void addFileEntry(const wxString& name, FileListImage fii);
		bool selectFileInList(const wxString& fileName);
		
		wxDECLARE_NO_COPY_CLASS(CncFileViewListCtrl);
		wxDECLARE_EVENT_TABLE();
};

#endif
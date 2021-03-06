#ifndef FILENAMESERVICE_H
#define FILENAMESERVICE_H

#include <ostream>
#include "CncCommon.h"
#include "wx/string.h"

class CncFileNameService {
	
	private:
	
		static wxString _appTempDir;
		static wxString _ret;
		static wxString _executablePath;
		static wxString _homeDirectory;
		static wxString _tempDirectoryOS;
		static wxString _tempDirectoryApp;
		static wxString _tempDirectorySession;
		static wxString _configDirectory;
		static wxString _baseDirectory;
		static wxString _databaseDirectory;
		static wxString _session;
		static wxString _configFileName;
		static wxString _lruFileName;
		static wxString _preconfiguredSpeedConfigFileName;
		
		static void deleteFile(wxString fn);
	
	public:
		
		static void init();
		static void sessionHousekeeping();
		
		static void trace(std::ostream& os);
		static const wxString& getConfigFileName()				{ _ret = _configDirectory + _configFileName; return _ret; }
		static const wxString& getLruFileName()					{ _ret = _configDirectory + _lruFileName; return _ret; }
		static const wxString& getSpeedConfigFileName()			{ _ret = _configDirectory + _preconfiguredSpeedConfigFileName; return _ret; }
		
		static const wxString& getExecutableDir() 				{ return _executablePath; }
		static const wxString& getHomeDir() 					{ return _homeDirectory; }
		static const wxString& getBaseDir() 					{ return _baseDirectory; }
		static const wxString& getTempDirOS() 					{ return _tempDirectoryOS; }
		static const wxString& getTempDirApp() 					{ return _tempDirectoryApp; }
		static const wxString& getTempDirSession() 				{ return _tempDirectorySession; }
		static const wxString& getConfigDir()					{ return _configDirectory; }
		static const wxString& getDatabaseDir() 				{ return _databaseDirectory; }
		
		static const char* getSession()							{ return _session; }
		
		static const char* getTempFileName(TemplateFormat f);
		static const char* getCncTemplatePreviewFileName(TemplateFormat f);
		
		static const char* getStackTraceFileName()	 			{ _ret = _tempDirectorySession + "CncStackTrace.txt"; 		return _ret; }
		
		static const char* getCncOutboundTxtFileName() 			{ _ret = _tempDirectorySession + "CncOutboundEmu.txt"; 		return _ret; }
		static const char* getCncOutboundSvgFileName() 			{ _ret = _tempDirectorySession + "CncOutboundEmu.svg"; 		return _ret; }
		static const char* getCncOutboundGCodeFileName() 		{ _ret = _tempDirectorySession + "CncOutboundEmu.gcode"; 	return _ret; }
		static const char* getCncOutboundBinFileName() 			{ _ret = _tempDirectorySession + "CncOutboundEmu.bct"; 		return _ret; }
		
		static const char* getCncOutboundTempFileName()			{ _ret = _tempDirectorySession + "CncOutboundTempEmu.xml"; 	return _ret; }
		static const char* getCncDrawPaneTraceFileName() 		{ _ret = _tempDirectorySession + "CncDrawPaneTrace.txt"; 	return _ret; }
		
		static const char* getBlankHtmlPageFileName()			{ _ret = _tempDirectorySession + "CncBlank.html"; 			return _ret; }
		static const char* getErrorHtmlPageFileName()			{ _ret = _tempDirectorySession + "CncError.html"; 			return _ret; }
		
		static void deleteCncOutboundSvgFileName() 				{ deleteFile(wxString(getCncOutboundSvgFileName())); }
		static void deleteCncOutboundTempFileName() 			{ deleteFile(wxString(getCncOutboundTempFileName())); }
		static void deleteCncDrawPaneTraceFileName() 			{ deleteFile(wxString(getCncDrawPaneTraceFileName())); }
		
};

#endif
#include <iostream>
#include <iomanip>  
#include <sstream>
#include <sstream>
#include <fstream>
#include <cctype>
#include <math.h>
#include <wx/datetime.h>
#include <wx/debug.h>
#include <wx/msgdlg.h>
#include <wx/aboutdlg.h>
#include <wx/fileconf.h>
#include <wx/filedlg.h>
#include <wx/event.h>
#include <wx/utils.h> 
#include <wx/textentry.h>
#include <wx/valnum.h>
#include <wx/dcclient.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/evtloop.h>
#include <wx/dataview.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/txtstrm.h>
#include <wx/vscroll.h>
#include <wx/textdlg.h>
#include <wx/clipbrd.h>
#include <wx/version.h> 
#include <boost/version.hpp>
#include "OSD/CncUsbPortScanner.h"
#include "OSD/CncAsyncKeyboardState.h"
#include "OSD/webviewOSD.h"
#include "GamepadEvent.h"
#include "SerialThread.h"
#include "CncExceptions.h"
#include "CncLoggerProxy.h"
#include "CncSourceEditor.h"
#include "CncOutboundEditor.h"
#include "CncGameportController.h"
#include "CncNumberFormatter.h"
#include "GlobalFunctions.h"
#include "SerialThreadStub.h"
#include "SerialPort.h"
#include "CncPosition.h"
#include "CncPatternDefinitions.h"
#include "CncUnitCalculator.h"
#include "CncStartPositionResolver.h"
#include "CncFileNameService.h"
#include "CncFilePreviewWnd.h"
#include "CncSpeedPlayground.h"
#include "SVGPathHandlerCnc.h"
#include "ManuallyParser.h"
#include "SVGFileParser.h"
#include "GCodeFileParser.h"
#include "BinaryPathHandlerCnc.h"
#include "CncArduino.h"
#include "SvgEditPopup.h"
#include "HexDecoder.h"
#include "UnitTestFrame.h"
#include "CncReferencePosition.h"
#include "CncSpeedMonitor.h"
#include "CncUsbConnectionDetected.h"
#include "CncConnectProgress.h"
#include "CncSha1Wrapper.h"
#include "CncMonitorSplitterWindow.h"
#include "CncMotionVertexTrace.h"
#include "CncTemplateObserver.h"
#include "CncFileViewLists.h"
#include "CncExternalViewBox.h"
#include "GL3DOptionPane.h"
#include "GL3DDrawPane.h"
#include "CncPreprocessor.h"
#include "CncGCodeSequenceListCtrl.h"
#include "CncStatisticsPane.h"
#include "CncSvgControl.h"
#include "CncOpenGLContextObserver.h"
#include "CncOSEnvironmentDialog.h"
#include "CncContext.h"
#include "CncManuallyMoveCoordinates.h"
#include "CncLastProcessingTimestampSummary.h"
#include "CncFileDialog.h"
#include "CncArduinoEnvironment.h"
#include "CncLCDPositionPanel.h"
#include "CncUserEvents.h"
#include "GlobalStrings.h"
#include "wxCrafterLCDPanel.h"
#include "wxCrafterImages.h"
#include "MainFrame.h"

#ifdef __WXMSW__
	// special includes for WindowPoc handling. 
	// they have to be at the end of the list to avoid compilation errors
	#include <windows.h>
	#include <dbt.h>
#endif

////////////////////////////////////////////////////////////////////
extern GlobalConstStringDatabase 	globalStrings;
extern void GlobalStreamRedirectionReset();

////////////////////////////////////////////////////////////////////
unsigned int CncGampadDeactivator::referenceCounter = 0;
unsigned int CncTransactionLock::referenceCounter   = 0;

////////////////////////////////////////////////////////////////////
// app defined events
	wxDEFINE_EVENT(wxEVT_UPDATE_MANAGER_THREAD, 			UpdateManagerEvent);
	wxDEFINE_EVENT(wxEVT_GAMEPAD_THREAD, 					GamepadEvent);
	wxDEFINE_EVENT(wxEVT_SERIAL_THREAD, 					SerialEvent);
	wxDEFINE_EVENT(wxEVT_PERSPECTIVE_TIMER, 				wxTimerEvent);
	wxDEFINE_EVENT(wxEVT_DEBUG_USER_NOTIFICATION_TIMER, 	wxTimerEvent);
	wxDEFINE_EVENT(wxEVT_TRACE_FROM_THREAD,					wxThreadEvent);
	wxDEFINE_EVENT(wxEVT_DISPATCH_ALL,						wxThreadEvent);
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// app defined event table
	wxBEGIN_EVENT_TABLE(MainFrame, MainFrameBClass)
		EVT_CLOSE(													MainFrame::onClose)
		EVT_COMMAND(wxID_ANY, wxEVT_INDIVIDUAL_CTRL_COMMAND, 		MainFrame::onIndividualCommand)
		EVT_COMMAND(wxID_ANY, wxEVT_CONFIG_UPDATE_NOTIFICATION, 	MainFrame::onConfigurationUpdated)
		EVT_TIMER(wxEVT_PERSPECTIVE_TIMER, 							MainFrame::onPerspectiveTimer)
		EVT_TIMER(wxEVT_DEBUG_USER_NOTIFICATION_TIMER, 				MainFrame::onDebugUserNotificationTimer)
	wxEND_EVENT_TABLE()
////////////////////////////////////////////////////////////////////

#define MF_PRINT_LOCATION_CTX_FILE			//	CNC_PRINT_LOCATION
#define MF_PRINT_LOCATION_CTX_SOMETHING		//	CNC_PRINT_LOCATION

#define cncDELETE( p ) { wxDELETE( p ); APPEND_LOCATION_TO_STACK_TRACE_FILE_A("finalized dtor of '"#p"'"); }

////////////////////////////////////////////////////////////////////
class CncRunEventFilter : public wxEventFilter {
	public:
		CncRunEventFilter() {
			wxEvtHandler::AddFilter(this);
		}
		virtual ~CncRunEventFilter() {
			wxEvtHandler::RemoveFilter(this);
		}
		
		// currently no filter active
		virtual int FilterEvent(wxEvent& event) {
			return Event_Skip;

			//const wxEventType t = event.GetEventType();
			//const wxWindow* wnd = (wxWindow*)event.GetEventObject();
			//return Event_Ignore;
		}
};
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
GlobalConfigManager::GlobalConfigManager(MainFrame* mf, wxPropertyGridManager* pgMgrSetup, wxFileConfig* globalConfig) {
////////////////////////////////////////////////////////////////////
	APPEND_LOCATION_TO_STACK_TRACE_FILE
	
	wxASSERT(globalConfig);
	wxASSERT(pgMgrSetup);
	wxASSERT(mf);
	
	// setup configuration
	CncConfig::setupGlobalConfigurationGrid(pgMgrSetup, *globalConfig);
	CncConfig::globalCncConfig = new CncConfig(mf);
	
	// load the file configuration
	CncConfig::globalCncConfig->loadConfiguration(*globalConfig);
	
	// at least initialize
	CncConfig::globalCncConfig->init();
}
////////////////////////////////////////////////////////////////////
GlobalConfigManager::~GlobalConfigManager() {
////////////////////////////////////////////////////////////////////
	cncDELETE( CncConfig::globalCncConfig );
	APPEND_LOCATION_TO_STACK_TRACE_FILE
}

////////////////////////////////////////////////////////////////////
wxFrame* THE_FRAME = NULL;

MainFrameBase::MainFrameBase(wxWindow* parent)
: MainFrameBClass(parent)
, logger(				new CncLoggerProxy(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_loggerPlaceholder->GetWindowStyle()               ))
, startupTrace(			new CncTextCtrl   (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_startupTracePlaceholder->GetWindowStyle()         ))
, tmpTraceInfo(			new CncTextCtrl   (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_tmpTraceInfoPlaceholder->GetWindowStyle()         ))
, controllerMsgHistory(	new CncTextCtrl   (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_controllerMsgHistoryPlaceholder->GetWindowStyle() ))
////////////////////////////////////////////////////////////////////
{
	// If this timers are already started stop it here!
	// This is with respect that they should be started only if all corresponding controls
	// are well created, which takes a portion of time which will increase with the
	// amount of controls.
	// Restarting it at a later event (may be Show()) of class MainFrame will ensure that.

	if ( m_startupTimer->IsRunning() )	m_startupTimer->Stop();
	if ( m_serialTimer ->IsRunning() )	m_serialTimer ->Stop();
	if ( m_traceTimer  ->IsRunning() )	m_traceTimer  ->Stop();

	THE_FRAME = this;
	
	GblFunc::cloneAttributes(m_loggerPlaceholder, 					logger);
	GblFunc::cloneAttributes(m_startupTracePlaceholder, 			startupTrace);
	GblFunc::cloneAttributes(m_tmpTraceInfoPlaceholder, 			tmpTraceInfo);
	GblFunc::cloneAttributes(m_controllerMsgHistoryPlaceholder, 	controllerMsgHistory);
	
	GblFunc::replaceControl(m_loggerPlaceholder, 					logger);
	GblFunc::replaceControl(m_startupTracePlaceholder, 				startupTrace);
	GblFunc::replaceControl(m_tmpTraceInfoPlaceholder, 				tmpTraceInfo);
	GblFunc::replaceControl(m_controllerMsgHistoryPlaceholder, 		controllerMsgHistory);
	
	tmpTraceInfo->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainFrameBase::traceTextUpdated), NULL, this);
}
///////////////////////////////////////////////////////////////////
MainFrameBase::~MainFrameBase() {
///////////////////////////////////////////////////////////////////
	APPEND_LOCATION_TO_STACK_TRACE_FILE_A("Entry . . .")
	
	GlobalStreamRedirectionReset();
	
	// beautifying only
	logger->GetParent()->SetBackgroundColour(*wxBLACK);
	
	//Unbinding
	tmpTraceInfo->Disconnect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainFrameBase::traceTextUpdated), NULL, this);

	// clear
	cncDELETE( controllerMsgHistory );
	cncDELETE( tmpTraceInfo );
	cncDELETE( startupTrace );
	cncDELETE( logger );
	
	THE_FRAME = NULL;
	
	//hosuekeeping
	CncFileNameService::sessionHousekeeping();
	
	APPEND_LOCATION_TO_STACK_TRACE_FILE_A("Finalized . . .")
}

///////////////////////////////////////////////////////////////////
MainFrame::MainFrame(wxWindow* parent, wxFileConfig* globalConfig)
: MainFrameBase(parent)
, GlobalConfigManager(this, GetPgMgrSetup(), globalConfig)
, updateManagerThread(NULL)
, gamepadThread(NULL)
, serialThread(NULL)
, cncSpeedPlayground(NULL)
, isDebugMode(false)
, isZeroReferenceValid(false)
, canClose(true)
, evaluatePositions(true)
, ignoreDirControlEvents(false)
, runConfirmationInfo(RunConfirmationInfo::Wait)
, traceTimerCounter(0)
, lastPortName(wxT(""))
, defaultPortName(wxT(""))
, cnc(new CncControl(CncEMU_NULL))
, lruFileView(NULL)
, sourceEditor(NULL)
, outboundEditor(NULL)
, motionMonitor(NULL)
, fileView(NULL)
, mainFilePreview(NULL)
, outboundFilePreview(NULL)
, monitorFilePreview(NULL)
, toolMagazine(NULL)
, positionSpy(NULL)
, setterList(NULL)
, speedMonitor(NULL)
, motionVertexCtrl(NULL)
, cncPreprocessor(NULL)
, parsingSynopisis(NULL)
, gCodeSequenceList(NULL)
, cncSummaryListCtrl(NULL)
, serialSpyListCtrl(NULL)
, cncGameportDlg(new CncGameportController(this))
, outboundEditorSvgView(NULL)
, navigatorPanel(NULL)
, optionPane3D(NULL)
, drawPane3D(NULL)
, statisticsPane(NULL)
, cnc3DVSplitterWindow(NULL)
, cnc3DHSplitterWindow(NULL)
, templateObserver(NULL)
, spyDetailWindow(NULL)
, openGLContextObserver(new CncOpenGLContextObserver(this))
, cncOsEnvDialog(new CncOSEnvironmentDialog(this))
, cncExtMainPreview(NULL)
, cncArduinoEnvironment(new CncArduinoEnvironment(this))
, cncLCDPositionPanel(NULL)
, cncManuallyMoveCoordPanel(NULL)
, perspectiveHandler(globalConfig, m_menuPerspective)
, config(globalConfig)
, lruStore(new wxFileConfig(wxT("CncControllerLruStore"), wxEmptyString, CncFileNameService::getLruFileName(), CncFileNameService::getLruFileName(), wxCONFIG_USE_RELATIVE_PATH | wxCONFIG_USE_NO_ESCAPE_CHARACTERS))
, outboundNbInfo(new NotebookInfo(m_outboundNotebook))
, templateNbInfo(new NotebookInfo(m_templateNotebook))
, lastMonitorPreviewFileName(_(""))
, pngAnimation(NULL)
, stcFileContentPopupMenu(NULL)
, stcEmuContentPopupMenu(NULL)
, inboundFileParser(NULL)
, perspectiveTimer(new wxTimer(this, wxEVT_PERSPECTIVE_TIMER))
, debugUserNotificationTimer(new wxTimer(this, wxEVT_DEBUG_USER_NOTIFICATION_TIMER))
, guiControls()
, menuItems()
, refPositionDlg(new CncReferencePosition(this))
{
///////////////////////////////////////////////////////////////////
	APPEND_THREAD_IDTO_STACK_TRACE_FILE;


	// initialize update manager thread
	initializeUpdateManagerThread();
	
	// initialize gamepad thread
	initializeGamepadThread();
	
	// initialize serial thread
	initializeSerialThread();

	// setup aui clear
	hideAllAuiPanes();
	
	// decocate application
	setIcons();

	// do this definitely here later it will causes a crash 
	installCustControls();

	// debugger configuration
	FileParser::installDebugConfigPage(m_debuggerPropertyManagerGrid);
	
	regiterAllMenuItems();
	
	// bind 
	this->Bind(wxEVT_CHAR_HOOK, 					&MainFrame::globalKeyDownHook, 				this);
	this->Bind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadAppPosUpdate, 			this, MainFrame::EventId::APP_POS_UPDATE);
	this->Bind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadCtlPosUpdate, 			this, MainFrame::EventId::CTL_POS_UPDATE);
	this->Bind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadHeartbeat, 				this, MainFrame::EventId::HEARTBEAT);
	this->Bind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadCompletion, 			this, MainFrame::EventId::COMPLETED);
	this->Bind(wxEVT_DISPATCH_ALL, 					&MainFrame::onThreadDispatchAll,			this, MainFrame::EventId::DISPATCH_ALL);
	this->Bind(wxEVT_TRACE_FROM_THREAD, 			&MainFrame::onThreadPostInfo,	 			this, MainFrame::EventId::POST_INFO);
	this->Bind(wxEVT_TRACE_FROM_THREAD, 			&MainFrame::onThreadPostWarning, 			this, MainFrame::EventId::POST_WARNING);
	this->Bind(wxEVT_TRACE_FROM_THREAD, 			&MainFrame::onThreadPostError,	 			this, MainFrame::EventId::POST_ERROR);
	this->Bind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadInitialized,		this, MainFrame::EventId::INITIALIZED);
	this->Bind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadCompletion, 		this, MainFrame::EventId::COMPLETED);
	this->Bind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadUpadte, 			this, MainFrame::EventId::GAMEPAD_STATE);
	this->Bind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadHeartbeat, 		this, MainFrame::EventId::GAMEPAD_HEARTBEAT);
	this->Bind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadInitialized, 		this, MainFrame::EventId::INITIALIZED);
	this->Bind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadCompletion, 		this, MainFrame::EventId::COMPLETED);
	this->Bind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadHeartbeat, 		this, MainFrame::EventId::SERIAL_HEARTBEAT);
	this->Bind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadMessage, 			this, MainFrame::EventId::SERIAL_MESSAGE);
	this->Bind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadData, 			this, MainFrame::EventId::SERIAL_DATA);
	this->Bind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadPinNotification, 	this, MainFrame::EventId::SERIAL_PIN_NOTIFICATION);
	
	this->Bind(wxEVT_CNC_NAVIGATOR_PANEL, 			&MainFrame::onNavigatorPanel, 				this);
	
	const wxFont font = THE_CONTEXT->outboundListBookFont;
	m_notebookConfig->GetListView()->SetFont(font);
	m_listbookMonitor->GetListView()->SetFont(font);
	m_listbookPostProcessor->GetListView()->SetFont(font);
}
///////////////////////////////////////////////////////////////////
MainFrame::~MainFrame() {
///////////////////////////////////////////////////////////////////
	GblFunc::appendToStackTraceFile("\nShuting down ......................");
	
	// stop the serial timer and wait interval to finish the work behind
	auto stopTimer = [&](wxTimer* timer) {
		if ( timer->IsRunning() )
			timer->Stop();
	};
	
	stopTimer( perspectiveTimer );
	stopTimer( debugUserNotificationTimer );
	stopTimer( m_startupTimer );
	stopTimer( m_serialTimer );
	stopTimer( m_traceTimer );
	
	if ( cnc != NULL )
		waitActive(m_serialTimer->GetInterval());
	
	perspectiveHandler.destroyUserPerspectives();
	
	// unbind 
	this->Unbind(wxEVT_CHAR_HOOK, 					&MainFrame::globalKeyDownHook, 				this);
	this->Unbind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadAppPosUpdate, 			this, MainFrame::EventId::APP_POS_UPDATE);
	this->Unbind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadCtlPosUpdate, 			this, MainFrame::EventId::CTL_POS_UPDATE);
	this->Unbind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadHeartbeat, 				this, MainFrame::EventId::HEARTBEAT);
	this->Unbind(wxEVT_UPDATE_MANAGER_THREAD, 		&MainFrame::onThreadCompletion, 			this, MainFrame::EventId::COMPLETED);
	this->Unbind(wxEVT_DISPATCH_ALL, 				&MainFrame::onThreadDispatchAll, 			this, MainFrame::EventId::DISPATCH_ALL);
	this->Unbind(wxEVT_TRACE_FROM_THREAD, 			&MainFrame::onThreadPostInfo,	 			this, MainFrame::EventId::POST_INFO);
	this->Unbind(wxEVT_TRACE_FROM_THREAD, 			&MainFrame::onThreadPostWarning, 			this, MainFrame::EventId::POST_WARNING);
	this->Unbind(wxEVT_TRACE_FROM_THREAD,	 		&MainFrame::onThreadPostError,	 			this, MainFrame::EventId::POST_ERROR);
	this->Unbind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadInitialized, 	this, MainFrame::EventId::INITIALIZED);
	this->Unbind(wxEVT_GAMEPAD_THREAD,	 			&MainFrame::onGamepadThreadCompletion,	 	this, MainFrame::EventId::COMPLETED);
	this->Unbind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadUpadte, 			this, MainFrame::EventId::GAMEPAD_STATE);
	this->Unbind(wxEVT_GAMEPAD_THREAD, 				&MainFrame::onGamepadThreadHeartbeat, 		this, MainFrame::EventId::GAMEPAD_HEARTBEAT);
	this->Unbind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadInitialized, 		this, MainFrame::EventId::INITIALIZED);
	this->Unbind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadCompletion, 		this, MainFrame::EventId::COMPLETED);
	this->Unbind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadHeartbeat, 		this, MainFrame::EventId::SERIAL_HEARTBEAT);
	this->Unbind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadMessage, 			this, MainFrame::EventId::SERIAL_MESSAGE);
	this->Unbind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadData, 			this, MainFrame::EventId::SERIAL_DATA);
	this->Unbind(wxEVT_SERIAL_THREAD, 				&MainFrame::onSerialThreadPinNotification, 	this, MainFrame::EventId::SERIAL_PIN_NOTIFICATION);

	this->Unbind(wxEVT_CNC_NAVIGATOR_PANEL, 		&MainFrame::onNavigatorPanel, 				this);
	
	// explicit delete the motion monitor pointer here, beacause the motion monitor class
	// considers the Mainframe THE_CONFIG->getTheApp() pointer in its dtor 
	// and this crashes definitly if the MainFame dtor is already passed
	// the delete below avoid this behaviour
	cncDELETE ( motionMonitor );
	cncDELETE ( cncGameportDlg );

	wxASSERT(lruStore);
	wxASSERT(lruFileView);
	lruFileView->save(lruStore);
	lruStore->Flush();
	cncDELETE ( lruStore );
	
	cncDELETE( openGLContextObserver );
	cncDELETE( cncExtMainPreview );
	cncDELETE( cncArduinoEnvironment );
	cncDELETE( cncOsEnvDialog );
	cncDELETE( outboundNbInfo );
	cncDELETE( templateNbInfo );
	cncDELETE( cnc );
	cncDELETE( statisticsPane );
	cncDELETE( drawPane3D );
	cncDELETE( optionPane3D );
	cncDELETE( motionMonitor );
	cncDELETE( templateObserver );
	cncDELETE( lruFileView );
	cncDELETE( sourceEditor );
	cncDELETE( outboundEditor );
	cncDELETE( fileView );
	cncDELETE( mainFilePreview );
	cncDELETE( outboundFilePreview );
	cncDELETE( monitorFilePreview );
	cncDELETE( toolMagazine );
	cncDELETE( positionSpy );
	cncDELETE( setterList );
	cncDELETE( motionVertexCtrl );
	cncDELETE( speedMonitor );
	cncDELETE( cncPreprocessor );
	cncDELETE( parsingSynopisis );
	cncDELETE( cncSummaryListCtrl );
	cncDELETE( serialSpyListCtrl );
	cncDELETE( outboundEditorSvgView );
	cncDELETE( gCodeSequenceList );
	cncDELETE( navigatorPanel );
	cncDELETE( cnc3DVSplitterWindow );
	cncDELETE( cnc3DHSplitterWindow );
	
	wxASSERT(config);
	config->Flush();
	cncDELETE( config );
	
	DeletePendingEvents();

	// log exit dtor
	APPEND_LOCATION_TO_STACK_TRACE_FILE_A("Finalized . . .")
}
///////////////////////////////////////////////////////////////////
void MainFrame::globalKeyDownHook(wxKeyEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( refPositionDlg && refPositionDlg->IsShownOnScreen() ) {
		wxPostEvent(refPositionDlg, event);
		event.Skip(false);
		return;
	}
	
	if ( motionMonitor && motionMonitor->IsShownOnScreen() ) {
		// This is necessary to avoid the default notebook key handling
		if ( motionMonitor->HasFocus() ) {
			motionMonitor->onKeyDown(event);
			event.Skip(false);
			return;
		}
	}
	
	event.Skip(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::umPostEvent(const UpdateManagerThread::Event& evt) { 
///////////////////////////////////////////////////////////////////
	if ( updateManagerThread == NULL ) 
		return;
		
	if ( updateManagerThread->IsPaused() == true ) {
		wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
		updateManagerThread->Resume();
	}

	updateManagerThread->postEvent(evt); 
	
	// update speed monitor
	if ( evt.hasFeedSpeedInfo() ) {
		
		static CncSpeedMonitor::SpeedData sd;
		sd.configured_MM_MIN = evt.pos.configuredSpeedValue;
		sd.received_MM_MIN	 = evt.pos.currentSpeedValue;
		sd.measured_MM_MIN	 = cnc != NULL ? cnc->getMeasuredFeedSpeed_MM_MIN() : 0.0; 
		
		speedMonitor->setCurrentFeedSpeedValue(sd);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::ShowAuiToolMenu(wxAuiToolBarEvent& event) {
///////////////////////////////////////////////////////////////////
	// overides the wxcrafter generted method
	event.Skip();
	
	if (event.IsDropDownClicked()) {
		wxAuiToolBar* toolbar = wxDynamicCast(event.GetEventObject(), wxAuiToolBar);
	
		if (toolbar) {
			wxAuiToolBarItem* item = toolbar->FindTool(event.GetId());
			if (item) {
				std::map<int, wxMenu*>::iterator iter = m_dropdownMenus.find(item->GetId());
				if (iter != m_dropdownMenus.end()) {
					event.Skip(false);
					wxPoint pt = event.GetItemRect().GetBottomLeft();
					pt.y++;
					
					// dont use the toolbar event handler because this will generate a crash while cnc::waitActive is in action!
					// toolbar->PopupMenu(iter->second, pt);
					this->PopupMenu(iter->second, pt);
					
				}
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////
void MainFrame::onConfigurationUpdated(wxCommandEvent& event) {
////////////////////////////////////////////////////////////////////////////
	// currently nothing to do
	//std::clog << "MainFrame::configurationUpdated(wxCommandEvent& event)" << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::setRefPostionState(bool state) {
///////////////////////////////////////////////////////////////////
	isZeroReferenceValid = state;

	wxBitmap bmp;
	if ( isZeroReferenceValid == true ) bmp = ImageLib24().Bitmap("BMP_TRAFFIC_LIGHT_GREEN"); 
	else 								bmp = ImageLib24().Bitmap("BMP_TRAFFIC_LIGHT_RED");
	
	wxString tip("Reference position state: ");
	// display ref pos mode too
	if ( isZeroReferenceValid == true ) {
		wxMemoryDC mdc(bmp);
		mdc.SetFont(wxFontInfo(8).FaceName("Arial"));
		mdc.SetTextForeground(wxColor(0, 0, 0));
		mdc.DrawText(wxString::Format("%d", (int)THE_CONFIG->getReferencePositionMode()), {5,1});
		bmp = mdc.GetAsBitmap();
		
		tip.append("Valid\n");
		tip.append(wxString::Format("Reference position mode: %d", (int)THE_CONFIG->getReferencePositionMode()));
		
	} else {
		tip.append("Not Valid");
		
	}
	
	m_refPosState->SetToolTip(tip);
	m_refPosState->SetBitmap(bmp);
	m_statusBar->Refresh();
	m_statusBar->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::disableGuiControls() {
///////////////////////////////////////////////////////////////////
	enableGuiControls(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::registerMenuItem(wxMenuItem* item) {
///////////////////////////////////////////////////////////////////
	if ( item == NULL )
		return;
	
	if ( item->IsSeparator() )
		return;
		
	if ( item->IsSubMenu() ) {
		wxMenu* menu = item->GetSubMenu();
		for ( unsigned int i = 0; i < menu->GetMenuItemCount (); i++ )
			registerMenuItem(menu->FindItemByPosition(i));
	}
	
	CncApp::MenuInfo mi;
	mi.lastEnableState 	= true;
	menuItems[item] = mi;
}
///////////////////////////////////////////////////////////////////
void MainFrame::unregisterMenuItem(wxMenuItem* item) {
///////////////////////////////////////////////////////////////////
	if ( item == NULL )
		return;
		
	auto it = menuItems.find(item);
	if ( it != menuItems.end() )
		menuItems.erase (it);       
}
///////////////////////////////////////////////////////////////////
void MainFrame::regiterAllMenuItems() {
///////////////////////////////////////////////////////////////////
	std::function<void(wxMenu* menu)> registerItem = [&](wxMenu* menu) {
		if ( menu == NULL )
			return;
			
		for ( unsigned int i = 0; i < menu->GetMenuItemCount (); i++ ) {
			wxMenuItem* item = menu->FindItemByPosition(i);
			if ( item->IsSubMenu() )
				registerItem(item->GetSubMenu());
			
			if ( item->IsSeparator() )
				continue;
			
			CncApp::MenuInfo mi;
			mi.lastEnableState 	= true;
			
			menuItems[item] = mi;
		}
	};
	
	menuItems.clear();
	
	for ( unsigned int m = 0; m < m_menuBar->GetMenuCount(); m++) {
		wxMenu* menu = m_menuBar->GetMenu(m);
		registerItem(menu);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableMenuItems(bool state) {
///////////////////////////////////////////////////////////////////
	for ( auto it = menuItems.begin(); it != menuItems.end(); ++it ) {
		CncApp::MenuInfo& mi = it->second;
		
		if ( it->first == NULL )
			continue;
			
		if ( state == true ) {
			it->first->Enable(mi.lastEnableState);
			
		} else {
			mi.lastEnableState = it->first->IsEnabled();
			it->first->Enable(false);
			
			menuItems[it->first] = mi;
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::registerGuiControl(wxWindow* ctrl) {
///////////////////////////////////////////////////////////////////
	if ( ctrl == NULL )
		return;
		
	CncApp::WindowInfo wi;
	wi.ctrl 			= ctrl;
	wi.lastEnableState 	= ctrl->IsThisEnabled();
		
	guiControls.push_back(wi);
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableGuiControls(bool state) {
///////////////////////////////////////////////////////////////////
	for ( auto it = guiControls.begin(); it != guiControls.end(); ++it ) {
		CncApp::WindowInfo& wi = *it;
		
		if ( wi.ctrl == NULL )
			continue;
			
		if ( state == true ) {
			wi.ctrl->Enable(wi.lastEnableState);
			
		} else {
			wi.lastEnableState = wi.ctrl->IsThisEnabled();
			wi.ctrl->Enable(false);
			
			*it = wi;
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::installCustControls() {
///////////////////////////////////////////////////////////////////

	// 3D splitter window
	wxWindow* parent 		= m_3DSplitterPlaceholder->GetParent();
	cnc3DHSplitterWindow 	= new CncMonitorHSplitterWindow(parent);
	cnc3DVSplitterWindow 	= new CncMonitorVSplitterWindow(cnc3DHSplitterWindow);
	
	statisticsPane 			= new CncStatisticsPane(cnc3DHSplitterWindow);
	cnc3DHSplitterWindow->SplitHorizontally(cnc3DVSplitterWindow, statisticsPane, 0);
	
	drawPane3D    			= new GL3DDrawPane(cnc3DVSplitterWindow);
	optionPane3D  			= new GL3DOptionPane(cnc3DVSplitterWindow);
	cnc3DVSplitterWindow->SplitVertically(drawPane3D, optionPane3D, 0);
	
	GblFunc::replaceControl(m_3DSplitterPlaceholder, cnc3DHSplitterWindow);
	
	// Montion Monitor
	motionMonitor = new CncMotionMonitor(this);
	GblFunc::replaceControl(drawPane3D->GetDrawPane3DPlaceHolder(), motionMonitor);
	drawPane3D->setMotionMonitor(motionMonitor);
	optionPane3D->setMotionMonitor(motionMonitor);
	statisticsPane->setMotionMonitor(motionMonitor);
	activate3DPerspectiveButton(m_3D_Perspective1);
	
	// Template observer
	templateObserver = new CncTemplateObserver(this);
	GblFunc::replaceControl(m_panelTemplateObserverPlaceholder, templateObserver);
	
	// LRU List 
	lruFileView = new CncLruFileViewListCtrl(this, 24, wxLC_SINGLE_SEL);
	GblFunc::replaceControl(m_lruListPlaceholder, lruFileView);
	
	// Source Editor
	sourceEditor = new CncSourceEditor(this);
	GblFunc::replaceControl(m_stcFileContent, sourceEditor);
	
	// Outbound Editor
	outboundEditor = new CncOutboundEditor(this);
	GblFunc::replaceControl(m_outboundFileSource, outboundEditor);
	
	// File View
	fileView = new CncFileView(this);
	GblFunc::replaceControl(m_mainFileViewPlaceholder, fileView);
	
	// Inbound File Preview
	mainFilePreview = new CncFilePreview(this, "GLMainPreview");
	GblFunc::replaceControl(m_filePreviewPlaceholder, mainFilePreview);
	
	// Outbound File Preview
	outboundFilePreview = new CncFilePreview(this, "GLOutboundPreview");
	GblFunc::replaceControl(m_outboundPreviewPlaceholder, outboundFilePreview);

	// File Preview
	monitorFilePreview = new CncFilePreview(this, "GLMontiorPreview");
	GblFunc::replaceControl(m_monitorTemplatePreviewPlaceHolder, monitorFilePreview);
	
	// external preview
	cncExtMainPreview = new CncExternalViewBox(this);
	cncExtMainPreview->setupView(0, mainFilePreview, 		"External File Preview . . . ");
	cncExtMainPreview->setupView(1, monitorFilePreview, 	"External Template Preview . . . ");

	// tool magazine
	toolMagazine = new CncToolMagazine(this); 
	GblFunc::replaceControl(m_toolMagazinePlaceholder, toolMagazine);
	
	// pos spy control
	positionSpy = new CncPosSpyListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL); 
	GblFunc::replaceControl(m_positionSpy, positionSpy);

	// pos spy control
	setterList = new CncSetterListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL); 
	GblFunc::replaceControl(m_setterList, setterList);
	
	// motion vertex list control
	motionVertexCtrl = new CncMotionVertexTrace(this); 
	GblFunc::replaceControl(m_motionVertexPlaceholder, motionVertexCtrl);
	
	// speed monitor control
	speedMonitor = new CncSpeedMonitor(this); 
	GblFunc::replaceControl(m_speedMonitorPlaceholder, speedMonitor);
	
	// preprocessor control
	cncPreprocessor = new CncPreprocessor(this); 
	GblFunc::replaceControl(m_preprocessorPlaceholder, cncPreprocessor);
	
	// parsing synopsis
	parsingSynopisis = new CncParsingSynopsisTrace(this); 
	GblFunc::replaceControl(m_motionSynopsisPlaceholder, parsingSynopisis);

	// summary list
	cncSummaryListCtrl = new CncSummaryListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL); 
	GblFunc::replaceControl(m_cncSummaryListCtrl, cncSummaryListCtrl);
	
	// serial spy list control
	serialSpyListCtrl = new CncSerialSpyListCtrl(this, wxLC_VRULES | wxLC_SINGLE_SEL); 
	GblFunc::replaceControl(m_serialSpyPlaceholder, serialSpyListCtrl);
	
	// Outbound editor svg viewer
	outboundEditorSvgView = new CncSvgViewer(this); 
	GblFunc::replaceControl(m_outboundEditorSvgViewPlaceholder, outboundEditorSvgView);
	
	// GCode Sequence control
	gCodeSequenceList = new CncGCodeSequenceListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL); 
	GblFunc::replaceControl(m_gCodeSequenceListPlaceholder, gCodeSequenceList);

	// navigator panel
	CncNavigatorPanel::Config cfg;
	cfg.innerCircle = true;
	cfg.shortFormat = false;
	cfg.margin		= 0;
	cfg.alignment	= wxALIGN_CENTER_HORIZONTAL;
	cfg.initToolTipMapAsCoordSytem();
	navigatorPanel = new CncNavigatorPanel(this, cfg); 
	GblFunc::replaceControl(m_navigationPanelPlaceholder, navigatorPanel);
	
	cncLCDPositionPanel = new CncLCDPositionPanel(this);
	GblFunc::replaceControl(m_lcdPositionPlaceholder, cncLCDPositionPanel);
	
	cncManuallyMoveCoordPanel = new CncManuallyMoveCoordinates(this);
	GblFunc::replaceControl(m_manuallyMoveCoordPlaceholder, cncManuallyMoveCoordPanel);
}
///////////////////////////////////////////////////////////////////
void MainFrame::registerGuiControls() {
///////////////////////////////////////////////////////////////////
	registerGuiControl(m_btCloseSecurePanel);
	
	registerGuiControl(navigatorPanel);
	registerGuiControl(sourceEditor);
	registerGuiControl(outboundEditor);
	registerGuiControl(fileView);
	registerGuiControl(lruFileView);

	registerGuiControl(m_searchConnections);
	registerGuiControl(m_btnOrigin);
	registerGuiControl(m_btnRuler);
	registerGuiControl(m_btnHelpLines);
	registerGuiControl(m_btnBoundBox);
	registerGuiControl(m_btnPosMarker);
	registerGuiControl(m_btnFlyPath);
	registerGuiControl(m_btToggleOutboundEditorWordWrap);
	registerGuiControl(m_rcSecureDlg);
	registerGuiControl(m_btProbeMode);
	registerGuiControl(m_btSelectReferences);
	registerGuiControl(m_btSelectManuallyMove);
	registerGuiControl(m_btSelectSetup);
	registerGuiControl(m_btSelectTemplate);
	registerGuiControl(m_btSelectInboundTest);
	registerGuiControl(m_btSelectInboundPreview);
	registerGuiControl(m_btSelectCncPreview);
	registerGuiControl(m_btSelectTemplatePreview);
	registerGuiControl(m_cbRenderResolution);
	registerGuiControl(m_3D_Refreh);
	registerGuiControl(m_3D_Clear);
	registerGuiControl(m_cbContentPosSpy);
	registerGuiControl(m_testToolPowerBtn);
	registerGuiControl(m_testToggleEndSwitch);
	registerGuiControl(m_portSelector);
	registerGuiControl(m_portSelectorSec);
	registerGuiControl(m_connect);
	registerGuiControl(m_connectSec);
	registerGuiControl(m_testDimModeX);
	registerGuiControl(m_testDimModeY);
	registerGuiControl(m_testDimModeZ);
	registerGuiControl(m_testDimTakeX);
	registerGuiControl(m_testDimTakeY);
	registerGuiControl(m_testDimTakeZ);
	registerGuiControl(m_testDimTakeAll);
	registerGuiControl(m_refPosition);
	registerGuiControl(m_zToTop);
	registerGuiControl(m_zToBottom);
	registerGuiControl(m_xToMin);
	registerGuiControl(m_yToMin);
	registerGuiControl(m_zToMin);
	registerGuiControl(m_xToMax);
	registerGuiControl(m_yToMax);
	registerGuiControl(m_zToMax);
	registerGuiControl(m_xToMid);
	registerGuiControl(m_yToMid);
	registerGuiControl(m_zToMid);
	registerGuiControl(m_homeDefintion);
	registerGuiControl(m_moveHome);
	registerGuiControl(m_moveXYtoZero);
	registerGuiControl(m_moveXYZtoZero);
	registerGuiControl(m_moveZtoZero);
	registerGuiControl(m_zeroMoveModeXY);
	registerGuiControl(m_zeroMoveModeXYZ);
	registerGuiControl(m_zeroMoveModeZ);
	registerGuiControl(m_clearLogger);
	registerGuiControl(m_displayInterval);
	registerGuiControl(m_btRequestCtlConfig);
	registerGuiControl(m_btRequestControllerPins);
	registerGuiControl(m_copyLogger);
	registerGuiControl(m_btSvgToggleWordWrap);
	registerGuiControl(m_switchMonitoing);
	registerGuiControl(m_testCountX);
	registerGuiControl(m_testCountY);
	registerGuiControl(m_testCountZ);
	registerGuiControl(m_testDistanceX);
	registerGuiControl(m_testDistanceY);
	registerGuiControl(m_testDistanceZ);
	registerGuiControl(m_testFirstX);
	registerGuiControl(m_testFirstY);
	registerGuiControl(m_testFirstZ);
	registerGuiControl(m_testModeX);
	registerGuiControl(m_testModeY);
	registerGuiControl(m_testModeZ);

	//...
	
	// already managed by sourceEditor
	//registerGuiControl(m_openSourceExtern);
	//registerGuiControl(m_openSvgExtern);
	//registerGuiControl(m_saveTemplate);
	//registerGuiControl(m_reloadTemplate);
	//registerGuiControl(m_removeTemplate);
	//registerGuiControl(m_renameTemplate);

}
///////////////////////////////////////////////////////////////////
void MainFrame::displayNotification(const char type, wxString title, wxString message, unsigned int timeout) {
///////////////////////////////////////////////////////////////////
	wxSharedPtr<wxNotificationMessageBase> dlg;
	dlg = new wxGenericNotificationMessage(title, message, this);
	switch ( type ) {
		case 'E':	dlg->SetFlags(wxICON_ERROR); break;
		case 'W':	dlg->SetFlags(wxICON_WARNING); break;
		default:	dlg->SetFlags(wxICON_INFORMATION); break;
	}
	
	dlg->Show(timeout);
	// important to free the shared Pointer!
	dlg.reset();
}
///////////////////////////////////////////////////////////////////
void MainFrame::displayReport(int id) {
///////////////////////////////////////////////////////////////////
	return;
	
	PidList pidList;
	
	switch ( id ) {
		case 1:			pidList.push_back(PID_I2C_LIMIT_VALUE);
						pidList.push_back(PID_I2C_SUPPORT_VALUE);
						//pidList.push_back(PID_SPEED_OFFSET_Y);
						//pidList.push_back(PID_SPEED_OFFSET_Z);
						break;
	}
	
	cnc->displayGetterList(pidList);
}

///////////////////////////////////////////////////////////////////
void MainFrame::testFunction1(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	cnc::trc.logInfoMessage("Test function 1");
}
///////////////////////////////////////////////////////////////////
void MainFrame::testFunction2(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	cnc::trc.logInfoMessage("Test function 2");
	
	static const char* xxxx[] = {"F", "Z"};
	std::cout << xxxx[0][0] << std::endl;
	std::cout << xxxx[1][0] << std::endl;
	
	std::cout << (int)xxxx[0][0] << std::endl;
	std::cout << (int)xxxx[1][0] << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::testFunction3(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	cnc::trc.logInfoMessage("Test function 3");
}
///////////////////////////////////////////////////////////////////
void MainFrame::testFunction4(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	cnc::trc.logInfoMessage("Test function 4");

	

}
/////////////////////////////////////////////////////////////////////
void MainFrame::onCloseSecureRunAuiPane(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( THE_CONTEXT->secureModeInfo.isActivatedByStartup == true ) {
		
		Close();
		return;
	}
	
	activateSecureMode(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::activateSecureMode(bool state) {
///////////////////////////////////////////////////////////////////
	// log the state
	THE_CONTEXT->secureModeInfo.isActive = state;
	
	// some control handling
	const bool useIt   = THE_CONTEXT->secureModeInfo.useIt;
	m_loadTemplateSec->Enable(!useIt);
	getLogger()->setShowOnDemandState(!useIt);
	
	// switch the state
	if ( THE_CONTEXT->secureModeInfo.isActive == true ) {
		perspectiveHandler.logCurrentPerspective();
		hideAllAuiPanes(true);
		
		if ( IsFullScreen() == false )
			ShowFullScreen(true);
		
		showAuiPane("SecureRunPanel", 	false);
		showAuiPane("StatusBar", 		false);
		
		GblFunc::swapControls(m_secMonitorPlaceholder, 	motionMonitor);
		GblFunc::swapControls(m_secLoggerPlaceholder, 	getLogger());
		GblFunc::swapControls(m_secZViewPlaceholder, 	m_zView);
		
		GblFunc::swapControls(m_fileViewsPlaceholder, 	m_fileViews);
		
		
		getLogger()->setShowOnDemandState(false);
		
	} else {
		
		if ( IsFullScreen() == true )
			ShowFullScreen(false);
		
		perspectiveHandler.restoreLoggedPerspective();
		
		GblFunc::swapControls(motionMonitor, 		m_secMonitorPlaceholder);
		GblFunc::swapControls(getLogger(), 			m_secLoggerPlaceholder);
		GblFunc::swapControls(m_zView, 				m_secZViewPlaceholder);
		
		GblFunc::swapControls(m_fileViews,			m_fileViewsPlaceholder);
		
		getLogger()->setShowOnDemandState(m_showLoggerOnDemand->GetValue());
	}
	
	GetAuimgrMain()->Update();
	
	if (  THE_CONTEXT->secureModeInfo.isActive == true ) {
		if ( THE_CONTEXT->secureModeInfo.isActivatedByStartup == true ) {
			wxFileName fn(getCurrentTemplateFileName());
			if ( fn.Exists() == false ) {
				wxCommandEvent dummy;
				openTemplate(dummy);
			}
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceGccVersion(std::ostream& out) {
///////////////////////////////////////////////////////////////////
	out << __GNUC__
		<< "."
		<< __GNUC_MINOR__
		<< "."
		<< __GNUC_PATCHLEVEL__
		<< std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceWxWidgetsVersion(std::ostream& out) {
///////////////////////////////////////////////////////////////////
	out << wxMAJOR_VERSION
		<< "."
		<< wxMINOR_VERSION
		<< "."
		<< wxRELEASE_NUMBER;
		
		if ( wxSUBRELEASE_NUMBER > 0 ) {
			out << " - "
			    << wxSUBRELEASE_NUMBER;
		}
		
		out << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceWxSvgVersion(std::ostream& out) {
///////////////////////////////////////////////////////////////////
	// It isn't better available currently
	out << "1.5.21";
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceBoostVersion(std::ostream& out) {
///////////////////////////////////////////////////////////////////
	out << BOOST_VERSION / 100000
		<< "."
		<< BOOST_VERSION / 100 % 1000
		<< "."
		<< BOOST_VERSION % 100 
		<< std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceWoodworkingCncVersion(std::ostream& out) {
///////////////////////////////////////////////////////////////////
	out << globalStrings.programVersion << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::startupTimer(wxTimerEvent& event) {
///////////////////////////////////////////////////////////////////
	m_startupTimer->Stop();
	
	if ( THE_CONTEXT->secureModeInfo.isActivatedByStartup == false ) {
		// Setup AUI Windows menue
		perspectiveHandler.loadPerspective("Default");
		decorateViewMenu();
		
	} else {
		// Setup secure mode
		activateSecureMode(true);
		
	}
	
	{
		CncNanoTimestamp ts1 = 0;
		
		// force the OpenGL as well as GLEW initialization
		{
			// wait intil the main windows is shown
			// this is with respect to the calls below
			ts1 = CncTimeFunctions::getNanoTimestamp();
			while ( IsShown() == false ) {
				dispatchAll(); 
				
				if ( CncTimeFunctions::getTimeSpanToNow(ts1) > 2000 * 1000 * 1000 ) { 
					std::cerr << "MainFrame::startupTimer(): Timeout reached for 'MainFrame::IsShown()'" << std::endl;
					break; 
				}
			}
		}
		
		// wait until the mainframe is shown on screen
		ts1 = CncTimeFunctions::getNanoTimestamp();
		while ( IsShownOnScreen() == false ) {
			dispatchAll(); 
			
			if ( CncTimeFunctions::getTimeSpanToNow(ts1) > 2000 * 1000 * 1000 ) { 
				std::cerr 	<< "MainFrame::startupTimer(): Timeout reached for 'MainFrame::IsShownOnScreen()'" << std::endl;
				break; 
			}
		}
		
		// select each cnc OpenGL canvas to force the OpenGL as well 
		// as GLEW initialization for each context
		{
			m_monitorViewSelector->SetSelection(MonitorBookSelection::VAL::CNC_PANEL);
			m_monitorViewBook->SetSelection(MonitorBookSelection::VAL::CNC_PANEL);
			
				m_outboundNotebook->SetSelection(OutboundSelection::VAL::POSTPOCESSOR_PANAL);
					m_listbookPostProcessor->SetSelection(PostProcessorSelection::VAL::OUTBOUND_PREVIEW);
				
				m_outboundNotebook->SetSelection(OutboundSelection::VAL::MOTION_MONITOR_PANAL);
				
			m_monitorViewSelector->SetSelection(MonitorBookSelection::VAL::TEMPLATE_PANEL);
			m_monitorViewBook->SetSelection(MonitorBookSelection::VAL::TEMPLATE_PANEL);
		}
		
		// Version infos
		std::stringstream ss;
		ss.str(""); traceGccVersion(ss); 			THE_CONTEXT->versionInfoMap["gcc"] 			= ss.str().c_str();
		ss.str(""); traceWxWidgetsVersion(ss); 		THE_CONTEXT->versionInfoMap["wxWidgets"] 	= ss.str().c_str();
		ss.str(""); traceWxSvgVersion(ss); 			THE_CONTEXT->versionInfoMap["wxSVG"] 		= ss.str().c_str();
		ss.str(""); traceBoostVersion(ss); 			THE_CONTEXT->versionInfoMap["boost"] 		= ss.str().c_str();
		ss.str(""); traceWoodworkingCncVersion(ss); THE_CONTEXT->versionInfoMap["program"] 		= ss.str().c_str();
		
		traceSessionId();
		std::cout << std::endl;
	}
	
	if ( THE_CONFIG->getAutoOpenOglObserverFlag() == true) {
		wxCommandEvent dummy;
		onOpenGLContextObserver(dummy);
	}
	
	// Check open gl state
	if ( GLCommon::isReady() == false ) GL_CTX_OBS->appendMessage('E', CNC_LOG_FUNCT, wxString::Format("OpenGL interface isn't ready!"));
	else								GL_CTX_OBS->appendMessage('I', CNC_LOG_FUNCT, wxString::Format("Application Startup Marker . . . "));
	
	// Auto connect ?
	if ( THE_CONFIG->getAutoConnectFlag() )
		connectSerialPortDialog();

	if ( THE_CONTEXT->secureModeInfo.isActivatedByStartup == false ) {
		
		openInitialTemplateFile();
		
		// Auto process ?
		if ( THE_CONFIG->getAutoProcessFlag() ) {
			defineMinMonitoring();
			processTemplateWrapper();
			defineNormalMonitoring();
		}
	}
	
	GblFunc::appendToStackTraceFile("Ready ............................");
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceTimer(wxTimerEvent& event) {
///////////////////////////////////////////////////////////////////
	// trace info handling
	CncTextCtrl* ctrl = getTrace();
	if ( ctrl && ctrl->GetValue().IsEmpty() )	traceTimerCounter = 0;
	else 										traceTimerCounter += event.GetInterval();
	
	if ( traceTimerCounter > 4000 ) {
		traceTimerCounter = 0;
		ctrl->Clear();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::serialTimer(wxTimerEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( cnc == NULL )
		return;
		
	if ( cnc->isConnected() == false )
		return;
	
	// give the cnc or serial port a change to do
	// something periodically e. g. pause a thread
	cnc->onPeriodicallyAppEvent();
	
	// pause the update manager thread if nothing more is to do
	static unsigned int counter = 0;
	if ( updateManagerThread != NULL ) {
		if ( updateManagerThread->IsPaused() == false ) {
			
			if ( positionSpy != NULL && positionSpy->IsShownOnScreen() == true ) {
				
				if ( updateManagerThread->somethingLeftToDo() == false ) {
					// a counter of 10 means 5 seconds
					if ( ++counter > 10 ) {
						wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
						updateManagerThread->Pause();
						
						counter = 0;
					}
				}
			}
			else {
				// put it to bed if the posSpyList isn't visible
				updateManagerThread->Pause();
			}
		}
	}
	
	// idle handling
	if ( m_miRqtIdleMessages->IsChecked() == true ) {
		
		// stop the time to avoid overlapping idle request
		m_serialTimer->Stop();
		
		// it's very import to deactivate the notifications during idle processing
		// because instead every config change (sc()) will release a notification
		// as a result the processing slows down significantly.
		CncConfig::NotificationDeactivator cfgNotDeactivation(false);
		
		// it's very important to avoid event handling during the idle processing
		// to prevent the start of furter commands
		THE_CONTEXT->setAllowEventHandling(false);
		
		// request the idle information
		cnc->sendIdleMessage();
		
		// reconstructed the previous event handling mode
		THE_CONTEXT->setAllowEventHandling(m_menuItemAllowEvents->IsChecked());
		
		// restart due to the previous Stop() command
		m_serialTimer->Start();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::onPaintSpeedPanel(wxPaintEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( cnc == NULL )
		return;
	
	static wxColour col(0, 128, 255);
	static wxBrush  brush(col);
	static wxPen    barPen(col, 1, wxSOLID);
	static wxPen    wpPen(*wxRED, 1, wxSOLID);
	
	const wxSize size   = m_speedPanel->GetSize();
	unsigned int pos    = size.GetWidth();
	unsigned int height = size.GetHeight();
	
	pos *= (cnc->getRealtimeFeedSpeed_MM_MIN() / THE_CONFIG->getMaxSpeedXYZ_MM_MIN());
	
	// bar
	wxPaintDC dc(m_speedPanel);
	dc.SetPen(barPen);
	dc.SetBrush(brush);
	
	wxRect rect(0, 0, pos, height);
	dc.DrawRectangle(rect);
	
	// watermark for current config
	unsigned int wp = size.GetWidth() 
	                * cnc->getConfiguredFeedSpeed_MM_MIN() 
					/ THE_CONFIG->getMaxSpeedXYZ_MM_MIN();
					
	dc.SetPen(wpPen);
	
	// move wp 2 pixel to the left so see valu = max also.
	dc.DrawLine(wp - 2, 0, wp - 2, height);
	dc.DrawLine(wp - 1, 0, wp - 1, height);
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadAppPosUpdate(UpdateManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( cnc == NULL )
		return;
		
	const CncUnit unit = THE_CONFIG->getDisplayUnit();
	
	// update position
	switch ( unit ) {
		case CncSteps:	// update application position
						m_xAxis->ChangeValue(wxString::Format("%8ld", cnc->getCurAppPos().getX()));
						m_yAxis->ChangeValue(wxString::Format("%8ld", cnc->getCurAppPos().getY()));
						m_zAxis->ChangeValue(wxString::Format("%8ld", cnc->getCurAppPos().getZ()));
						break;
						
		case CncMetric:	// update application position
						m_xAxis->ChangeValue(wxString::Format("%4.3lf", cnc->getCurAppPos().getX() * THE_CONFIG->getDisplayFactX(unit)));
						m_yAxis->ChangeValue(wxString::Format("%4.3lf", cnc->getCurAppPos().getY() * THE_CONFIG->getDisplayFactY(unit)));
						m_zAxis->ChangeValue(wxString::Format("%4.3lf", cnc->getCurAppPos().getZ() * THE_CONFIG->getDisplayFactZ(unit)));
						break;
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadCtlPosUpdate(UpdateManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( cnc == NULL )
		return;
		
	const CncUnit unit = THE_CONFIG->getDisplayUnit();
	
	// update position
	switch ( unit ) {
		case CncSteps:	// update controller position
						m_xAxisCtl->ChangeValue(wxString::Format("%8ld", cnc->getCurCtlPos().getX()));
						m_yAxisCtl->ChangeValue(wxString::Format("%8ld", cnc->getCurCtlPos().getY()));
						m_zAxisCtl->ChangeValue(wxString::Format("%8ld", cnc->getCurCtlPos().getZ()));
						break;
						
		case CncMetric:	// update controller position
						m_xAxisCtl->ChangeValue(wxString::Format("%4.3lf", cnc->getCurCtlPos().getX() * THE_CONFIG->getDisplayFactX(unit)));
						m_yAxisCtl->ChangeValue(wxString::Format("%4.3lf", cnc->getCurCtlPos().getY() * THE_CONFIG->getDisplayFactY(unit)));
						m_zAxisCtl->ChangeValue(wxString::Format("%4.3lf", cnc->getCurCtlPos().getZ() * THE_CONFIG->getDisplayFactZ(unit)));
						break;
	}
	
	// update z view
	m_zView->updateView(cnc->getCurCtlPos().getZ() * THE_CONFIG->getDisplayFactZ(unit));
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadHeartbeat(UpdateManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	// rotate heartbeat star
	wxImage img =m_updateManagerUpdate->GetBitmap().ConvertToImage();
	wxPoint p(m_updateManagerUpdate->GetSize().GetWidth()/2, m_updateManagerUpdate->GetSize().GetHeight()/2);

	m_updateManagerUpdate->SetBitmap(wxBitmap(img.Rotate90()));
	m_updateManagerUpdate->Refresh();
	m_updateManagerUpdate->Update();
	
	// update time consumed
	if ( pngAnimation && pngAnimation->IsRunning() ) {
		logTimeConsumed();
	}
	
	// statistic
	if ( isProcessing() )
		statisticsPane->logStatistics(false);
	
	// feed speed control + feed speed panel
	m_speedPanel->Refresh();

	wxString sValue(_maxSpeedLabel);

	if ( THE_CONTEXT->isProbeMode() == false ) {
		if ( cnc != NULL ) {
			const double dValue = cnc->getRealtimeFeedSpeed_MM_MIN();
			if ( dValue  < 0.0 ) sValue.assign(_maxSpeedLabel);
			else 				 sValue.assign(wxString::Format("%.1lf", dValue));
		}
	}

	m_realtimeFeedSpeed->ChangeValue(sValue);
	
	// update position syp
	if ( updateManagerThread == NULL )
		return;
		
	if ( updateManagerThread->IsPaused() == true )
		return;
		
	bool updatePosCount = false;
	static long lastCount = 0;
	if ( m_btTogglePosSpy->GetValue() == true ) {
		if ( positionSpy->IsShownOnScreen() ) {
			if ( updateManagerThread->fillPositionSpy(positionSpy) > 0 ) {
				positionSpy->Refresh();
			}
		}
		
		if ( lastCount != positionSpy->GetItemCount() ) {
			lastCount = positionSpy->GetItemCount();
			updatePosCount = true;
		}
		
	} else {
		if ( lastCount != positionSpy->GetItemCount() ) {
			lastCount = 0;
			updatePosCount = true;
		}
	}
	
	if ( updatePosCount == true ) {
		m_positionSpyCount->ChangeValue(CncNumberFormatter::toString(((long)(positionSpy->GetItemCount()))));
		m_positionSpyCount->SetToolTip(m_positionSpyCount->GetLabel());
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadDispatchAll(wxThreadEvent& event) {
///////////////////////////////////////////////////////////////////
	dispatchAll();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadCompletion(UpdateManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	std::clog << CNC_LOG_FUNCT << std::endl;
	updateManagerThread = NULL;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadPostInfo(wxThreadEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( event.GetString().IsEmpty() == false )
		std::cout << event.GetString() << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadPostWarning(wxThreadEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( event.GetString().IsEmpty() == false )
		cnc::cex1 << event.GetString() << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onThreadPostError(wxThreadEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( event.GetString().IsEmpty() == false )
		std::cerr << event.GetString() << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onGamepadThreadInitialized(GamepadEvent& event) {
///////////////////////////////////////////////////////////////////
	//std::cout << "MainFrame::onGamepdThreadInitilaized" << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onGamepadThreadCompletion(GamepadEvent& event) {
///////////////////////////////////////////////////////////////////
	std::clog << CNC_LOG_FUNCT << std::endl;
	gamepadThread = NULL;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onGamepadThreadHeartbeat(GamepadEvent& event) {
///////////////////////////////////////////////////////////////////
	decorateGamepadState(event.data.connected);
	
	if ( event.data.connectionStateChanged == true ) {
		if ( event.data.connected == false )	cnc::trc.logWarning(" Gamepad disconnected");
		else									cnc::trc.logWarning(" Gamepad connected");
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::onGamepadThreadUpadte(GamepadEvent& event) {
///////////////////////////////////////////////////////////////////
	onGamepadThreadHeartbeat(event);
	
	if ( event.data.connected == false )
		return;
	
	if ( cncGameportDlg )
		cncGameportDlg->update(event);
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSerialThreadInitialized(SerialEvent& event) {
///////////////////////////////////////////////////////////////////
	//std::cout << CNC_LOG_LOCATION << std::endl;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSerialThreadCompletion(SerialEvent& event) {
///////////////////////////////////////////////////////////////////
	std::cout << CNC_LOG_LOCATION << std::endl;
	serialThread = NULL;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSerialThreadHeartbeat(SerialEvent& event) {
///////////////////////////////////////////////////////////////////
	// rotate heartbeat image
	wxImage img = m_serialThreadHeartbeat->GetBitmap().ConvertToImage();
	if ( img.IsOk() == false )
		return;
		
	m_serialThreadHeartbeat->SetBitmap(wxBitmap(img.Rotate90()));
	m_serialThreadHeartbeat->Refresh();
	m_serialThreadHeartbeat->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSerialThreadMessage(SerialEvent& event) {
///////////////////////////////////////////////////////////////////
	const char type = (char)event.message.type;
	cncArduinoEnvironment->appendMessage(type, event.message.getMessage(), event.message.getContext());
	
	bool alreadyTraced = cncArduinoEnvironment->IsShownOnScreen();
	if ( cncSpeedPlayground && cncSpeedPlayground->IsShownOnScreen() ) {
		alreadyTraced = true;
		
		cncSpeedPlayground->appendArdoMessage(type, event.message.getMessage(), event.message.getContext());
	}
	
	//-------------------------------------------------------------
	auto log2Std = [&](std::ostream & o) {
		o << "ArdoMsg: " << type 
						 << " " 
						 << event.message.getMessage() 
						 /*
						 << std::setw(80) 
						 << " [[ " 
						 << event.message.getContext() 
						 << " ]]"
						 */ 
						 << std::endl;
	};
	
	if ( alreadyTraced == false ) {
		switch ( type ) {
			case 'E':	log2Std(std::cerr); break;
			case 'D':
			case 'W':	log2Std(std::clog); break;
			case 'S':	log2Std(cnc::cex1); break;
			default:	log2Std(std::cout);
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSerialThreadData(SerialEvent& event) {
///////////////////////////////////////////////////////////////////
	//std::cout << CNC_LOG_LOCATION << std::endl;
	cncArduinoEnvironment->update(event.data);
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSerialThreadPinNotification(SerialEvent& event) {
///////////////////////////////////////////////////////////////////
	cncArduinoEnvironment->notifyPinUpdate();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onClose(wxCloseEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( saveTemplateOnDemand() == false )
		return;
		
	if ( canClose == false) {
		cnc::trc.logWarning("Can't close the application due to an active run/debug session");
		return;
	}
	
	// Destroy the serial port connection.
	// It's very important to do this here in case  
	// the serial port has threads
	if ( cnc != NULL ) {
		{
			wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
			cnc->sendHalt();
			cnc->disconnect();
			
			// this will crash if a run  or debug is active
			// this have to be avoided - see can close above
			delete cnc;
			cnc = NULL;
		}
	}
	
	// Destroy the update manager thread
	if ( updateManagerThread != NULL) {
		if ( updateManagerThread->IsRunning() == false) {
			wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
			updateManagerThread->Resume();
		}
		
		updateManagerThread->stop();
		
		while ( true ) {
			{ // was the ~UpdateManagerThreadThread() function executed?
				wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
				if ( !updateManagerThread ) {
					break;
				}
			}
			// wait for thread completion
			wxThread::This()->Sleep(10);
		}
	}
	
	// Destroy the gamepad thread
	if ( gamepadThread != NULL) {
		if ( gamepadThread->IsRunning() == false) {
			wxCriticalSectionLocker enter(pGamepadThreadCS);
			gamepadThread->Resume();
		}
		
		gamepadThread->stop();
		
		while ( true ) {
			{ // was the ~GamepadThread() function executed?
				wxCriticalSectionLocker enter(pGamepadThreadCS);
				if ( !gamepadThread ) 
					break;
			}
			// wait for thread completion
			wxThread::This()->Sleep(10);
		}
		 
	}
	
	// Destroy the serial thread
	if ( serialThread != NULL) {
		if ( serialThread->IsRunning() == false) {
			wxCriticalSectionLocker enter(pSerialThreadCS);
			serialThread->Resume();
		}
		
		serialThread->stop();
		
		while ( true ) {
			{ // was the ~GamepadThread() function executed?
				wxCriticalSectionLocker enter(pSerialThreadCS);
				if ( !serialThread ) 
					break;
			}
			// wait for thread completion
			wxThread::This()->Sleep(10);
		}
		 
	}
	
	cncArduinoEnvironment->activateOnClose(true);
	
	event.Skip();
}
///////////////////////////////////////////////////////////////////
void MainFrame::waitActive(unsigned int milliseconds, bool once) {
///////////////////////////////////////////////////////////////////
	if ( milliseconds == 0 ) {
		if ( once == true )
			dispatchAll();
		
		return;
	}
	
	wxDateTime s = wxDateTime::UNow();
	wxDateTime e = wxDateTime::UNow();

	wxEventLoopBase* evtLoop = wxEventLoopBase::GetActive();
	if ( evtLoop == NULL )
		return;

	while ( wxTimeSpan(e - s).GetMilliseconds() < milliseconds ) {
		while ( evtLoop->Pending() ) {
			evtLoop->Dispatch();
			
			e = wxDateTime::UNow();
			if ( wxTimeSpan(e - s).GetMilliseconds() < milliseconds )
				break;
		}
		
		e = wxDateTime::UNow();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::dispatchNext() {
///////////////////////////////////////////////////////////////////
	wxEventLoopBase* evtLoop = wxEventLoopBase::GetActive();
	if ( evtLoop == NULL )
		return;

	if ( evtLoop->Pending() )
		evtLoop->Dispatch();
}
///////////////////////////////////////////////////////////////////
void MainFrame::dispatchAll() {
///////////////////////////////////////////////////////////////////
	wxEventLoopBase* evtLoop = wxEventLoopBase::GetActive();
	if ( evtLoop == NULL )
		return;
		
	/*
	Please note: This is th fastest version, but evtLoop->Yield() is better then the code below, 
	it also considers timer events, aui-handling etc.
	
	while ( evtLoop->Pending() )
		evtLoop->Dispatch();
	*/

	/*
	wxEVT_CATEGORY_ALL =
		wxEVT_CATEGORY_UI|wxEVT_CATEGORY_USER_INPUT|wxEVT_CATEGORY_SOCKET| \
		wxEVT_CATEGORY_TIMER|wxEVT_CATEGORY_THREAD|wxEVT_CATEGORY_UNKNOWN| \
		wxEVT_CATEGORY_CLIPBOARD
	
	This is the slowest version
	evtLoop->YieldFor(wxEVT_CATEGORY_ALL);
	*/
	
	// the following code is the best compromise, but aui handling isn't perfect
	
	if ( wxTheApp->HasPendingEvents() )
		wxTheApp->ProcessPendingEvents();
		
	while ( evtLoop->Pending() )
		evtLoop->Dispatch();
	
	
	//wxTheApp->Yield();
}
#ifdef __WXMSW__
///////////////////////////////////////////////////////////////////
WXLRESULT MainFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
///////////////////////////////////////////////////////////////////
	return wxFrame::MSWWindowProc ( message, wParam, lParam );
	
	wxString portName("Undefined");
	PDEV_BROADCAST_HDR lpdb = NULL;
	
	if ( message == WM_DEVICECHANGE) {
		// logging
		switch ( wParam ) {
			case DBT_DEVICEARRIVAL:			lpdb = (PDEV_BROADCAST_HDR)lParam;
											if ( lpdb->dbch_devicetype == DBT_DEVTYP_PORT ) {
												PDEV_BROADCAST_PORT pPort = (PDEV_BROADCAST_PORT) lpdb;
												wxString n(pPort->dbcp_name);
												portName.assign(n);
											}
											
											cnc::trc.logInfo(wxString("A new COM device was detected on port: ") << portName);
											break;
											
			case DBT_DEVICEREMOVECOMPLETE:	lpdb = (PDEV_BROADCAST_HDR)lParam;
											if ( lpdb->dbch_devicetype == DBT_DEVTYP_PORT ) {
												PDEV_BROADCAST_PORT pPort = (PDEV_BROADCAST_PORT) lpdb;
												wxString n(pPort->dbcp_name);
												portName.assign(n);
											}
											
											cnc::trc.logInfo(wxString("The COM device was removed from port: ") << portName);
											break;
											
			default: ;
		}
	}
	
	// do all the default stuff here first
	WXLRESULT ret = wxFrame::MSWWindowProc ( message, wParam, lParam );
	
	// do some more actions 
	if ( message == WM_DEVICECHANGE) {
		// update port selector
		if ( wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE )
			decoratePortSelector();
		
		static CncUsbConnectionDetected* dlg = new CncUsbConnectionDetected(this);
		switch ( wParam ) {
			// ask for connect - on demand . . . 
			case DBT_DEVICEARRIVAL:	{
				if ( isProcessing() == false ) {
					
					if ( dlg->IsShown() ) {
						dlg->setPortName(portName);
						return ret;
					}
					
					dlg->setPortName(portName);
					if ( dlg->ShowModal() == wxID_YES ) {
						m_portSelector->SetStringSelection(portName);
						m_portSelectorSec->SetStringSelection(portName);
						connectSerialPortDialog();
					}
				}
			}
			break;
			
			// check if current com connection is effected
			case DBT_DEVICEREMOVECOMPLETE:	{
				// check if the current connection is effected
				if ( lastPortName == portName ) {
					if ( cnc && cnc->isConnected() ) {
						cnc->interrupt("Serial Device Removed");
						cnc->disconnect();
						lastPortName.clear();
						std::cerr << "Connection brocken" << std::endl;
						cnc::trc.logWarning("Connection broken . . .");
					}
					
					if ( dlg->IsShown() ) {
						dlg->EndModal(wxID_NO);
					}
				}
				break;
			}
			
			case DBT_DEVNODES_CHANGED: {
				activateGamepadNotificationsOnDemand(true);
				break;
			}
			
			default: ;
		} 
	}
	
	return ret;
}
#else
	// currently no equivalent
#endif
///////////////////////////////////////////////////////////////////
void MainFrame::searchAvailiablePorts(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	disableControls();
	decoratePortSelector(CncAsyncKeyboardState::isControlPressed());
	enableControls();
	
	if ( m_portSelector->FindString(lastPortName) == wxNOT_FOUND ) {
		m_portSelector->Popup();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::decoratePortSelector(bool list) {
///////////////////////////////////////////////////////////////////
	startAnimationControl();
	m_portSelector   ->Clear();
	m_portSelectorSec->Clear();
	m_portSelectorSec->SetClientSize(200, -1);
	
	//------------------------------------------------------------
	auto appendItem = [&](const wxString& item, const wxBitmap& bitmap = wxNullBitmap) {
		m_portSelector->Append   (item,  bitmap);
		m_portSelectorSec->Append(item,  bitmap);
	};
	
	// add default ports
	if ( lastPortName == _portEmulatorNULL )	appendItem(_portEmulatorNULL,		ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
	else										appendItem(_portEmulatorNULL,		ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
	
	if ( lastPortName == _portEmulatorTEXT )	appendItem(_portEmulatorTEXT,		ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
	else										appendItem(_portEmulatorTEXT,		ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
	
	if ( lastPortName == _portEmulatorSVG )		appendItem(_portEmulatorSVG,		ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
	else										appendItem(_portEmulatorSVG,		ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
	
	if ( lastPortName == _portEmulatorGCODE )	appendItem(_portEmulatorGCODE,		ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
	else										appendItem(_portEmulatorGCODE,		ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
	
	if ( lastPortName == _portEmulatorBIN )		appendItem(_portEmulatorBIN, 		ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
	else										appendItem(_portEmulatorBIN,		ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
	
	if ( lastPortName == _portEmulatorArduino )	appendItem(_portEmulatorArduino,	ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
	else										appendItem(_portEmulatorArduino, 	ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
	
	 
	// add com ports
	int pStart = list == true ?  1 :  0;
	int pEnd   = list == true ? 11 : 32;//256;
	
	for (int i=pStart; i<pEnd; i++) {
		int ret = CncUsbPortScanner::isComPortAvailable(i);
		wxString pn(wxString::Format("COM%d", i));
		
		switch ( ret ) {
			case 0:		if ( cnc && cnc->isConnected() && lastPortName == pn ) 
							appendItem(pn, ImageLibPortSelector().Bitmap("BMP_PS_CONNECTED"));
						else 
							appendItem(pn, ImageLibPortSelector().Bitmap("BMP_PS_ACCESS_DENIED"));
							
						break;
						
			case 1:		appendItem(pn, ImageLibPortSelector().Bitmap("BMP_PS_AVAILABLE"));
						break;
						
			default: 	if ( list == true ) 
							appendItem(pn, ImageLibPortSelector().Bitmap("BMP_PS_UNKNOWN"));
		}
	}
	
	// select the last port, if availiable
	if ( m_portSelector->FindString(lastPortName) != wxNOT_FOUND )
		m_portSelector->SetStringSelection(lastPortName);
		
	if ( m_portSelectorSec->FindString(lastPortName) != wxNOT_FOUND )
		m_portSelectorSec->SetStringSelection(lastPortName);

	stopAnimationControl();
}

///////////////////////////////////////////////////////////////////
void MainFrame::setIcons() {
///////////////////////////////////////////////////////////////////
	// Set icon(s) to the application/dialog
	wxIconBundle app_icons;

	#ifdef DEBUG
			const wxString iconTyp("D");
	#else
		#ifdef GPROF
			const wxString iconTyp("G");
		#else
			const wxString iconTyp("R");
		#endif
	#endif
	
	int sizes[] = {16, 32, 64, 128, 256};

	for ( unsigned int i=0; i<sizeof(sizes)/sizeof(int); i++) {
		wxBitmap iconBmp = ImageLibAppIcons().Bitmap(wxString::Format("BMP_APP_%s%d", iconTyp, sizes[i]));
		wxIcon icn;
		icn.CopyFromBitmap(iconBmp);
		app_icons.AddIcon( icn );
	}
	
	if ( app_icons.GetIconCount() > 0 )
		SetIcons( app_icons );
}
///////////////////////////////////////////////////////////////////
void MainFrame::initializeUpdateManagerThread() {
///////////////////////////////////////////////////////////////////
	// create the thread
	updateManagerThread = new UpdateManagerThread(this);
	wxThreadError error = updateManagerThread->Create();

	if (error != wxTHREAD_NO_ERROR) {
		wxMessageBox( _("Couldn't create update manager thread!") );
		abort();
	}
	
	error = updateManagerThread->Run();
	if (error != wxTHREAD_NO_ERROR) {
		wxMessageBox( _("Couldn't run update manager thread!") );
		abort();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::initializeGamepadThread() {
///////////////////////////////////////////////////////////////////
	// create the thread
	gamepadThread = new GamepadThread(this);
	wxThreadError error = gamepadThread->Create();

	if (error != wxTHREAD_NO_ERROR) {
		wxMessageBox( _("Couldn't create gamepad thread!") );
		abort();
	}
	
	error = gamepadThread->Run();
	if (error != wxTHREAD_NO_ERROR) {
		wxMessageBox( _("Couldn't run gamepad thread!") );
		abort();
	}
	
	activateGamepadNotifications(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::initializeSerialThread() {
///////////////////////////////////////////////////////////////////
	// create the thread
	serialThread = new SerialThread(this);
	wxThreadError error = serialThread->Create();

	if (error != wxTHREAD_NO_ERROR) {
		wxMessageBox( _("Couldn't create serial thread!") );
		abort();
	}
	
	error = serialThread->Run();
	if (error != wxTHREAD_NO_ERROR) {
		wxMessageBox( _("Couldn't run serial thread!") );
		abort();
	}
	
	wxASSERT ( cncArduinoEnvironment != NULL );
	if ( serialThread->IsRunning() )
		cncArduinoEnvironment->notifyStarted();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::Show(bool show) {
///////////////////////////////////////////////////////////////////
	bool ret = MainFrameBClass::Show(show);

	if ( show == true )  {
		toggleMotionMonitorOptionPane(true);
		toggleMotionMonitorStatisticPane(true);

		auto startTimer = [](wxTimer* timer, unsigned int value, const char* name) {

			if ( timer->IsRunning() == false) {
				timer->Start(value);

				APPEND_LOCATION_TO_STACK_TRACE_FILE_A(wxString::Format("Call %s->Start(%d)", name, value));

			} else {
				APPEND_LOCATION_TO_STACK_TRACE_FILE_A(wxString::Format("%s [%d] already started", name, timer->GetInterval()));
			}
		};

		// Now all controls are well created, this is the perfect time to (re)start these timers
		startTimer(m_startupTimer,	250, "m_startupTimer");
		startTimer(m_serialTimer,	500, "m_serialTimer" );
		startTimer(m_traceTimer,	500, "m_traceTimer"  );
	}

	APPEND_LOCATION_TO_STACK_TRACE_FILE_A(wxString::Format("Result = %d; show = %d", ret, show));

	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::initialize(void) {
///////////////////////////////////////////////////////////////////
	createAnimationControl();
	decorateSearchButton();
	switchMonitorButton(true);
	determineRunMode();
	decoratePosSpyConnectButton(false);
	decorateSecureDlgChoice(true);
	registerGuiControls();
	decorateOutboundSaveControls(false);

	// init config pages
	GetConfigurationToolbook()->SetSelection(0);
	m_pgMgrSetup->SelectPage(GetConfigurationToolbook()->GetSelection());

	speedMonitor->init(THE_CONFIG->getMaxSpeedXYZ_MM_MIN());
	m_speedPanel->SetBackgroundColour(wxColour(234, 234, 234));
	
	m_loggerNotebook->SetSelection(LoggerSelection::VAL::CNC);
	
	perspectiveHandler.setupUserPerspectives();
	
	this->SetTitle(wxString(globalStrings.programTitel) + " " + globalStrings.programVersion);
	
	// setup cnc port selector box
	decoratePortSelector();
	
	wxFloatingPointValidator<float> val(3, NULL, wxNUM_VAL_DEFAULT );//, wxNUM_VAL_ZERO_AS_BLANK);
	val.SetRange(0, 100.0);
	val.SetPrecision(3);
	m_testDistanceX->SetValidator(val);
	m_testDistanceY->SetValidator(val);
	m_testDistanceZ->SetValidator(val);
	
	val.SetRange(0, 50000);
	val.SetPrecision(0);
	
	wxTextValidator tVal(wxFILTER_NUMERIC);
	tVal.SetCharIncludes(", ");
	m_cbUCValueFrom->SetValidator(tVal);
	m_metricValueFrom->SetValidator(tVal);
	
	if ( CncConfig::getGlobalCncConfig()->getShowTestMenuFlag() == false )
		m_menuBar->Remove(m_menuBar->FindMenu("Test"));
		
	//initilaize debug state
	m_menuItemDebugSerial->Check(false);
	
	decorateSerialSpy();
	
	resetMinMaxPositions();
	initializeLruMenu();
	initializeCncControl();
	
	m_outboundNotebook->SetSelection(OutboundSelection::VAL::MOTION_MONITOR_PANAL);
	m_listbookMonitor->SetSelection(OutboundMonitorSelection::VAL::MOTION_MONITOR_PANAL);
	m_notebookConfig->SetSelection(OutboundCfgSelection::VAL::SUMMARY_PANEL);
	
	// curve lib resulotion
	THE_CONFIG->setupSelectorRenderResolution();
	
	
	//#warning
	//wxCommandEvent event;
	//openSpeedPlayground(event);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::initializeCncControl() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	if ( THE_CONFIG != NULL ) {
		wxString value;
		
		// initialize display unit
		m_unit->SetStringSelection(THE_CONFIG->getDefaultDisplayUnitAsStr());
		updateUnit();
		
		// initialize com port
		THE_CONFIG->getDefaultPort(value);
		m_portSelector   ->SetStringSelection(value);
		m_portSelectorSec->SetStringSelection(value); //???
		defaultPortName.assign(value);
		
		// initialize update interval
		THE_CONTEXT->setUpdateInterval(m_displayInterval->GetValue());
	}
	 
	// initialize the postion controls
	setControllerZero(true, true, true);
	updateCncConfigTrace();
	
	// z slider
	m_zView->updateView(cnc->requestControllerPos().getZ() * THE_CONFIG->getDisplayFactZ(THE_CONFIG->getDisplayUnit()));
	
	//initilaize debug state
	if ( m_menuItemDebugSerial->IsChecked() ) 	cnc->enableSpyOutput(true);
	else				    					cnc->enableSpyOutput(false); 
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::initializeLruMenu() {
///////////////////////////////////////////////////////////////////
	return lruFileView->load(lruStore);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::openInitialTemplateFile() {
///////////////////////////////////////////////////////////////////
	wxASSERT ( lruFileView != NULL );
	
	wxString value;
	THE_CONFIG->getDefaultTplDir(value);
	fileView->setDefaultPath(value);
	fileView->selectDefaultPath();
	
	THE_CONFIG->getDefaultTplFile(value);
	wxFileName fn;
	if ( value.length() > 0 ) 	fn.Assign(value);
	else 						fn.Assign(lruFileView->getFileName(0));

	if ( fn.Exists() ) {
		m_inputFileName->SetValue(fn.GetFullName());
		m_inputFileName->SetHint(fn.GetFullPath());
		
		openFile();
		prepareAndShowMonitorTemplatePreview(true);
		
	} else {
		THE_CONFIG->getDefaultTplDir(value);
		fileView->openDirectory(value);
		
		selectMainBookSourcePanel();
		return false; 
	}
	
	return true;
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateUnit() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	wxASSERT(cncManuallyMoveCoordPanel);
	
	const wxString unit = m_unit->GetValue();
	
	wxString xAppPos(m_xAxis->GetValue());
	wxString yAppPos(m_yAxis->GetValue());
	wxString zAppPos(m_zAxis->GetValue());
	wxString xCtlPos(m_xAxisCtl->GetValue());
	wxString yCtlPos(m_yAxisCtl->GetValue());
	wxString zCtlPos(m_zAxisCtl->GetValue());
	
	if ( unit == "mm" ) { 
		THE_CONFIG->setDisplayUnit(CncMetric); 
		
		long v; 
		xAppPos.ToLong(&v); m_xAxis->ChangeValue   (wxString::Format("%4.3lf", (double)(v * THE_CONFIG->getDisplayFactX())));
		yAppPos.ToLong(&v); m_yAxis->ChangeValue   (wxString::Format("%4.3lf", (double)(v * THE_CONFIG->getDisplayFactY())));
		zAppPos.ToLong(&v); m_zAxis->ChangeValue   (wxString::Format("%4.3lf", (double)(v * THE_CONFIG->getDisplayFactZ())));
		xCtlPos.ToLong(&v); m_xAxisCtl->ChangeValue(wxString::Format("%4.3lf", (double)(v * THE_CONFIG->getDisplayFactX())));
		yCtlPos.ToLong(&v); m_yAxisCtl->ChangeValue(wxString::Format("%4.3lf", (double)(v * THE_CONFIG->getDisplayFactY())));
		zCtlPos.ToLong(&v); m_zAxisCtl->ChangeValue(wxString::Format("%4.3lf", (double)(v * THE_CONFIG->getDisplayFactZ())));
		
	} else {
		THE_CONFIG->setDisplayUnit(CncSteps);
		
		double v; 
		xAppPos.ToDouble(&v); m_xAxis->ChangeValue   (wxString::Format("%ld", (long)(v * THE_CONFIG->getCalculationFactX())));
		yAppPos.ToDouble(&v); m_yAxis->ChangeValue   (wxString::Format("%ld", (long)(v * THE_CONFIG->getCalculationFactY())));
		zAppPos.ToDouble(&v); m_zAxis->ChangeValue   (wxString::Format("%ld", (long)(v * THE_CONFIG->getCalculationFactZ())));
		xCtlPos.ToDouble(&v); m_xAxisCtl->ChangeValue(wxString::Format("%ld", (long)(v * THE_CONFIG->getCalculationFactX())));
		yCtlPos.ToDouble(&v); m_yAxisCtl->ChangeValue(wxString::Format("%ld", (long)(v * THE_CONFIG->getCalculationFactY())));
		zCtlPos.ToDouble(&v); m_zAxisCtl->ChangeValue(wxString::Format("%ld", (long)(v * THE_CONFIG->getCalculationFactZ())));
	}
	
	cncManuallyMoveCoordPanel->updateUnit();
}
///////////////////////////////////////////////////////////////////
void MainFrame::OnExit(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxUnusedVar(event);
	Close();
}
///////////////////////////////////////////////////////////////////
void MainFrame::OnAbout(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString description;
	description << "Here you find a simple fun project . . .\n\n";
	description << "It's a combination of:\n";
	description << " - Programming,\n";
	description << " - Build electronics and\n";
	description << " - Mechanical work\n";
	
	wxIcon icon;
	wxBitmap bmp = ImageLibBig().Bitmap("BMP_CNC");
	icon.CopyFromBitmap(bmp);
	
	wxUnusedVar(event);
	wxAboutDialogInfo info;
	info.SetName(globalStrings.programTitel);
	info.SetVersion(globalStrings.programVersion);
	info.SetLicence(_("GPL v2 or later"));
	info.AddDeveloper("Hacki Wimmmer");
	info.SetDescription(description);
	info.SetIcon(icon);
	info.SetCopyright(globalStrings.copyRight);
	::wxAboutBox(info);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectPort(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( lastPortName != m_portSelector->GetStringSelection() ) {
		connectSerialPortDialog();
		
		m_portSelectorSec->SetStringSelection(m_portSelector->GetStringSelection());
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectPortSec(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_portSelector->SetStringSelection(m_portSelectorSec->GetStringSelection());
	selectPort(event);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::connectSerialPortDialog() {
///////////////////////////////////////////////////////////////////
	CncConnectProgress dlg(this);
	
	return ( dlg.ShowModal() == wxID_OK);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::connectSerialPort() {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	wxASSERT( cncManuallyMoveCoordPanel );
	
	const wxBitmap bmpC = ImageLib16().Bitmap("BMP_CONNECTED");
	const wxBitmap bmpD = ImageLib16().Bitmap("BMP_DISCONNECTED");
	m_connect->SetBitmap(bmpD);
	m_connect->Refresh();
	m_connect->Update();
	
	startAnimationControl();
	m_serialTimer->Stop();
	
	if ( m_clearSerialSpyOnConnect->GetValue() )
		clearSerialSpy();
		
	const wxString sel(m_portSelector->GetStringSelection());
	if ( sel.IsEmpty() ) 
		return false;
	
	disableControls();
	decorateOutboundSaveControls(false);
	decorateOutboundEditor();
	
	m_miRqtIdleMessages->Check(false);
	m_miRqtIdleMessages->Enable(false);
	
	wxString serialFileName;
	createCncControl(sel, serialFileName);
	if ( cnc == NULL )
		return false;
		
	statisticsPane->setCncControl(cnc);
	
	initializeCncControl();
	selectSerialSpyMode();
	clearPositionSpy();
	
	lastPortName.clear();
	
	bool ret = false;
	if ( (ret = cnc->connect(serialFileName)) == true )  {

		lastPortName.assign(sel);
		clearMotionMonitor();
		clearPositionSpy();

		if ( (ret = cnc->setup()) == true ) {
			
			cnc->isEmulator() ? setRefPostionState(true) : setRefPostionState(false);
			updateCncConfigTrace();
			decorateSwitchToolOnOff(cnc->getToolState());
			selectSerialSpyMode();
			m_connect->SetBitmap(bmpC);
			m_serialTimer->Start();
			
			if ( cnc->canProcessIdle() ) {
				m_miRqtIdleMessages->Check(THE_CONFIG->getRequestIdleRequestFlag());
				m_miRqtIdleMessages->Enable(true);
			}
			
			setControllerZero(true, true, true);
		}
	}
	
	cncManuallyMoveCoordPanel->reset();
	updateSetterList();
	decoratePortSelector();
	m_connect->Refresh();
	m_connect->Update();
	
	stopAnimationControl();
	enableControls();

	return ret;
}
///////////////////////////////////////////////////////////////////
const wxString& MainFrame::createCncControl(const wxString& sel, wxString& serialFileName) {
///////////////////////////////////////////////////////////////////
	// disconnect and delete the current cnc control
	if ( cnc != NULL ) {
		cnc->disconnect();
		delete cnc;
	}
	
	serialFileName.clear();
	cnc = NULL;
	
	struct InitialSetup {
		wxString serialFileName		= "";
		bool probeMode				= false;
		bool secureDlg				= false;
		bool pathListEntries		= false;
		bool moveSequences			= false;
		bool vertexTrace			= false;

	} setup;
	
	if ( sel == _portEmulatorNULL ) {
		cnc = new CncControl(CncEMU_NULL);
		
		setup.serialFileName.assign("dev/null");
		setup.probeMode			= true;
		setup.secureDlg			= false;
		setup.pathListEntries	= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.moveSequences		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.vertexTrace		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
	} 
	else if ( sel == _portEmulatorTEXT ) {
		cnc = new CncControl(CncEMU_TXT);
		
		setup.serialFileName.assign(CncFileNameService::getCncOutboundTxtFileName());
		setup.probeMode			= true;
		setup.secureDlg			= false;
		setup.pathListEntries	= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.moveSequences		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.vertexTrace		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
	} 
	else if ( sel == _portEmulatorSVG ) {
		cnc = new CncControl(CncEMU_SVG);
		
		setup.serialFileName.assign(CncFileNameService::getCncOutboundSvgFileName());
		setup.probeMode			= true;
		setup.secureDlg			= false;
		setup.pathListEntries	= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.moveSequences		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.vertexTrace		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
	}
	else if ( sel == _portEmulatorGCODE ) {
		cnc = new CncControl(CncEMU_GCODE);
		
		setup.serialFileName.assign(CncFileNameService::getCncOutboundGCodeFileName());
		setup.probeMode			= true;
		setup.secureDlg			= false;
		setup.pathListEntries	= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.moveSequences		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.vertexTrace		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
	} 
	else if ( sel == _portEmulatorBIN) {
		cnc = new CncControl(CncEMU_BIN);
		
		setup.serialFileName.assign(CncFileNameService::getCncOutboundBinFileName());
		setup.probeMode			= true;
		setup.secureDlg			= false;
		setup.pathListEntries	= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.moveSequences		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.vertexTrace		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
	} 
	else if ( sel == _portEmulatorArduino) {
		cnc = new CncControl(CncPORT_EMU_ARDUINO);
		
		setup.serialFileName.assign("::Arduino");
		setup.probeMode			= true;
		setup.secureDlg			= false;
		setup.pathListEntries	= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.moveSequences		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
		setup.vertexTrace		= true	&& THE_CONTEXT->secureModeInfo.isActivatedByStartup == false;
	} 
	else {
		cnc = new CncControl(CncPORT);
		
		setup.serialFileName.assign(wxString::Format("\\\\.\\%s", sel));
		setup.probeMode			= false;
		setup.secureDlg			= false;
		setup.pathListEntries	= false;
		setup.moveSequences		= false;
		setup.vertexTrace		= false;
	}
	
	const bool startDisabled = false;
	if ( startDisabled == true ) {
		cnc::cex1 << "MainFrame::createCncControl(): Flag startDisabled is active!" << std::endl;
		
//		bool pathListEntries		= THE_CONFIG->getPreProcessorCntPathListEntries();
//		bool moveSequences			= THE_CONFIG->getPreProcessorCntMoveSequneces();

		setup.pathListEntries	= false;
		setup.moveSequences		= false;
		setup.vertexTrace		= false;
		
		decoratePosSpyConnectButton(false);
		m_menuItemUpdCoors->Check(false);
	}
	
	// config setup
	serialFileName.assign(setup.serialFileName);
	THE_CONTEXT->setProbeMode(setup.probeMode);
	
	decorateSecureDlgChoice(setup.secureDlg);
	
	if ( cncPreprocessor != NULL ) {
		cncPreprocessor->enablePathListEntries(setup.pathListEntries);
		cncPreprocessor->enableMoveSequences(setup.moveSequences);
	}
	
	return serialFileName;
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateOutboundEditor(const char* fileName) {
///////////////////////////////////////////////////////////////////
	wxString url(wxString::Format("%s%s", CncFileNameService::getDatabaseDir(), "NoSerialContentAvailable.svg"));
	if ( wxFileName::Exists(url) == false )
		url.assign("about:blank");

	if ( fileName == NULL ) {
		outboundEditorSvgView->loadFile(url);
		m_simpleBookOutBoundEditor->SetSelection(1);
		
		outboundEditor->clearContent();
		outboundFilePreview->selectEmptyPreview();
		
		return;
	}
	
	if ( wxFileName::Exists(fileName) == false ) {
		outboundEditorSvgView->loadFile(url);
		m_simpleBookOutBoundEditor->SetSelection(1);
		
		std::cerr << "MainFrame::decorateOutboundEditor(): Can't open file: '" 
		          << fileName 
		          << "'" << std::endl;
		return;
	}
	
	m_simpleBookOutBoundEditor->SetSelection(0);
	outboundEditor->openFile(fileName);
	outboundFilePreview->selectPreview(fileName);
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableMenu(wxMenu* m, bool state) {
///////////////////////////////////////////////////////////////////	
	if ( m )
		m_menuBar->EnableTop(m_menuBar->FindMenu(m->GetTitle()), state);
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableMenuItem(wxMenuItem* m, bool state) {
///////////////////////////////////////////////////////////////////
	if ( m ) {
		for (unsigned int i=0; i<m_menuBar->GetMenuCount();i++) {
			wxMenu* menu = m_menuBar->GetMenu(i);
			
			if ( menu->FindItem(m->GetItemLabel()) ) {
				menu->Enable(menu->FindItem(m->GetItemLabel()), state);
				break;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableRunControls(bool state) {
// state == false: processing active
// state == true:  processing inacative
// - triggered by enableControls(state)
///////////////////////////////////////////////////////////////////
	// only for debugging
	if ( false ) {
		std::clog << "state: " 			<< state;
		std::clog << ", debug: " 		<< isDebugMode;
		std::clog << ", pause: " 		<< isPause();
		std::clog << std::endl;
	}
	
	// all buttons of the run control have to be enabled/disabeled here
	// every time
	
	isPause() ? m_rcDebugConfig->Enable(false) : m_rcRun->Enable(state);
	isPause() ? m_rcDebugConfig->Enable(false) : m_rcRunSec->Enable(state);
	
	if ( isDebugMode == false ) {
		
		isPause() ? m_rcRun->Enable(true)              : m_rcRun->Enable(state); 
		isPause() ? m_rcRunSec->Enable(true)           : m_rcRunSec->Enable(state); 
		isPause() ? m_rcDebug->Enable(false)           : m_rcDebug->Enable(state);
		isPause() ? m_rcPause->Enable(false)           : m_rcPause->Enable(!state);
		isPause() ? m_rcPauseSec->Enable(false)        : m_rcPauseSec->Enable(!state);
		isPause() ? m_rcStop->Enable(true)             : m_rcStop->Enable(!state);
		isPause() ? m_rcStopSec->Enable(true)          : m_rcStopSec->Enable(!state);
		isPause() ? m_btnEmergenyStop->Enable(true)    : m_btnEmergenyStop->Enable(!state);
		isPause() ? m_btnEmergenyStopSec->Enable(true) : m_btnEmergenyStopSec->Enable(!state);
		
		m_rcReset->Enable(isPause() == false && state);
		m_rcNextStep->Enable(false);
		m_rcNextBreakpoint->Enable(false);
		m_rcFinish->Enable(false);
		
	} else {
		
		isPause() ? m_rcRun->Enable(false)          : m_rcRun->Enable(state);
		isPause() ? m_rcDebug->Enable(true)         : m_rcDebug->Enable(state);
		isPause() ? m_rcPause->Enable(false)        : m_rcPause->Enable(!state);
		isPause() ? m_rcStop->Enable(true)          : m_rcStop->Enable(!state);
		isPause() ? m_btnEmergenyStop->Enable(true) : m_btnEmergenyStop->Enable(!state);
				
		m_rcReset->Enable(isPause() == false && state);
		
		m_rcNextStep->Enable(!state);
		m_rcNextBreakpoint->Enable(!state);
		m_rcFinish->Enable(!state);
		
		if ( m_rcNextBreakpoint->IsEnabled() )	startDebugUserNotification();
		else									stopDebugUserNotification();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableControls(bool state) {
///////////////////////////////////////////////////////////////////
	// set global state
	canClose = state;
	
	m_secureSplitterMainV->SetSashInvisible(!state);
	m_secureSplitterMainH->SetSashInvisible(!state);
	
	// enable menu bar
	enableMenuItems(state);
	
	// enable all relevant controls
	enableGuiControls(state);
	
	//enable manually controls
	enableTestControls(state);
	
	// run control
	enableRunControls(state);
	
	// at least update motion monitor
	if ( state == true )
		motionMonitor->Refresh();
}
///////////////////////////////////////////////////////////////////
void MainFrame::connect(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	connectSerialPortDialog();
}
///////////////////////////////////////////////////////////////////
void MainFrame::connectSec(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	connectSerialPortDialog();
}
///////////////////////////////////////////////////////////////////
void MainFrame::clearLogger(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	getLogger()->Clear();
}
///////////////////////////////////////////////////////////////////
void MainFrame::setControllerZero(bool x, bool y, bool z) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->resetClientId();
	cnc->setZeroPos(x, y, z);
	setRefPostionState(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUnit(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateUnit();
}
///////////////////////////////////////////////////////////////////
int MainFrame::showReferencePositionDlg(wxString msg) {
///////////////////////////////////////////////////////////////////
	wxASSERT(refPositionDlg);
	
	refPositionDlg->setMessage(msg);
	refPositionDlg->setMeasurePlateThickness(THE_CONFIG->getMeasurePlateThickness());
	
	activateGamepadNotifications(true);
	int ret = refPositionDlg->ShowModal();
	
	if ( ret == wxID_OK ) {
		
		double wpt = refPositionDlg->getWorkpieceThickness();
		THE_CONFIG->setReferenceIncludesWpt(cnc::dblCompareNull(wpt) == false);
		THE_CONFIG->setWorkpieceThickness(wpt);
		THE_CONFIG->setReferencePositionMode(refPositionDlg->getReferenceMode());
		updateCncConfigTrace();
		
		motionMonitor->clear();
		
		setControllerZero(refPositionDlg->shouldZeroX(), 
		                  refPositionDlg->shouldZeroY(), 
						  refPositionDlg->shouldZeroZ()
						 );
						 
	} 
	else {
		cnc::cex1 << " Set reference position aborted . . . " << std::endl;
	}
	
	//selectMonitorBookCncPanel();
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateCncConfigTrace() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->updateCncConfigTrace();
	
	m_infoToolDiameter->SetLabel(wxString::Format("%.3lf", THE_CONFIG->getToolDiameter(THE_CONFIG->getCurrentToolId())));
	m_zView->updateView(cnc->requestControllerPos().getZ() * THE_CONFIG->getDisplayFactZ(THE_CONFIG->getDisplayUnit()));
	collectSummary();
}
///////////////////////////////////////////////////////////////////
void MainFrame::changeWorkpieceThickness() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	double wpt = CncConfig::getGlobalCncConfig()->getWorkpieceThickness();
	
	if ( cnc::dblCompareNull(wpt) == true )	m_lableWorkpieceThickness->SetBitmap(ImageLib16().Bitmap("BMP_NO_WPT"));
	else 									m_lableWorkpieceThickness->SetBitmap(ImageLib16().Bitmap("BMP_WPT"));
		
	m_lableWorkpieceThickness->SetToolTip(wxString::Format("Workpiece thickness: %.3lf mm", wpt));
	m_lableWorkpieceThickness->Refresh();
	setRefPostionState(false);
	
	wxString msg("A workpiece thickness change requires a redefinition of the CNC reference position.\n\n");
	msg << "This have to be done before the next CNC run.\n\n";
	msg << "The set function below can be used to set it directly.";
	
	showReferencePositionDlg(msg);
	m_crossings->ChangeValue(wxString() << CncConfig::getGlobalCncConfig()->getDurationCount());
}
///////////////////////////////////////////////////////////////////
void MainFrame::changeCrossingThickness() {
///////////////////////////////////////////////////////////////////
	m_crossings->ChangeValue(wxString() << CncConfig::getGlobalCncConfig()->getDurationCount());
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineMinMonitoring(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	defineMinMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineMinMonitoring() {
///////////////////////////////////////////////////////////////////
	m_menuItemUpdCoors->Check(false);
	m_menuItemUpdDraw->Check(false);
	m_menuItemDebugSerial->Check(false);
	m_menuItemDisplayParserDetails->Check(false);
	switchMonitorButton(false);
	updateMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineNormalMonitoring(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	defineNormalMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineNormalMonitoring() {
///////////////////////////////////////////////////////////////////
	m_menuItemUpdCoors->Check(true);
	m_menuItemUpdDraw->Check(true);
	m_menuItemDebugSerial->Check(false);
	m_menuItemDisplayParserDetails->Check(true);
	switchMonitorButton(true);
	updateMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::switchMonitoring(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_switchMonitoing->GetValue() == true )
		defineNormalMonitoring();
	else
		defineMinMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::switchMonitorButton(bool state) {
///////////////////////////////////////////////////////////////////
	m_switchMonitoing->SetValue(state);
	
	if ( state == true ) {
		m_switchMonitoing->SetBitmap(ImageLib16().Bitmap("BMP_MONITORING_NORMAL"));
		m_switchMonitoing->SetToolTip("Switch Monitoring to Minimal");
	} else {
		m_switchMonitoing->SetBitmap(ImageLib16().Bitmap("BMP_MONITORING_MINIMAL"));
		m_switchMonitoing->SetToolTip("Switch Monitoring to Normal");
	}
	m_switchMonitoing->Refresh();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineUpdateCoordinates(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineAllowEvents(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineOnlineDrawing(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::defineDebugSerial(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateMonitoring();
	decorateSerialSpy();
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateMonitoring() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	if ( motionMonitor != NULL ) {
		motionMonitor->enable(m_menuItemUpdDraw->IsChecked());
		motionMonitor->Refresh();
	}
	
	THE_CONTEXT->setOnlineUpdateCoordinates(m_menuItemUpdCoors->IsChecked());
	THE_CONTEXT->setAllowEventHandling(m_menuItemAllowEvents->IsChecked());
	THE_CONTEXT->setOnlineUpdateDrawPane(m_menuItemUpdDraw->IsChecked());
	
	cnc->enableSpyOutput(m_menuItemDebugSerial->IsChecked());
	
	if ( isDisplayParserDetails() == false ) {
		m_dvListCtrlSvgUAInboundPathList->DeleteAllItems();
		m_dvListCtrlSvgUAUseDirective->DeleteAllItems();
		m_dvListCtrlSvgUADetailInfo->DeleteAllItems();
		
		m_dvListCtrlSvgUAInboundPathList->Update();
		m_dvListCtrlSvgUAUseDirective->Update();
		m_dvListCtrlSvgUADetailInfo->Update();
		
		THE_APP->getGCodeSequenceList()->clear();
	}
}
///////////////////////////////////////////////////////////////////
const char* MainFrame::getTemplateFormatName(const wxString& fileName) {
///////////////////////////////////////////////////////////////////
	static wxString ext;
	
	wxFileName fn;
	fn.Assign(fileName);
	
	ext.assign(fn.GetExt());
	ext.MakeUpper();
	return ext;
}
///////////////////////////////////////////////////////////////////
const char* MainFrame::getCurrentTemplateFormatName() {
///////////////////////////////////////////////////////////////////
	switch ( getCurrentTemplateFormat() ) {
		case TplManual:
		case TplTest:	return "";
		default: 		return getTemplateFormatName(getCurrentTemplatePathFileName());
	}
	
	return "";
}
///////////////////////////////////////////////////////////////////
TemplateFormat MainFrame::getTemplateFormat(const wxString& fn) {
///////////////////////////////////////////////////////////////////
	return cnc::getTemplateFormatFromFileName(fn);
}
///////////////////////////////////////////////////////////////////
TemplateFormat MainFrame::getCurrentTemplateFormat() {
///////////////////////////////////////////////////////////////////
	wxString fn;
	fn.assign(getCurrentTemplatePathFileName());
		
	switch ( m_mainViewSelector->GetSelection() ) {
		case MainBookSelection::VAL::MANUEL_PANEL:	return TplManual;
		case MainBookSelection::VAL::TEST_PANEL:	return TplTest;
	}
	
	return cnc::getTemplateFormatFromFileName(fn);
}
///////////////////////////////////////////////////////////////////
const wxString& MainFrame::getCurrentTemplateFileName() {
///////////////////////////////////////////////////////////////////
	static wxString ret;
	ret.assign(m_inputFileName->GetValue());
	return ret;
}
///////////////////////////////////////////////////////////////////
const wxString& MainFrame::getCurrentTemplatePathFileName() {
///////////////////////////////////////////////////////////////////
	static wxString ret;
	ret.assign(m_inputFileName->GetHint());
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateExtTemplatePages(TemplateFormat tf) {
///////////////////////////////////////////////////////////////////
	switch ( tf ) {
		case TplSvg:		m_simpleBookSourceExt->SetSelection(SourceExtBookSelection::VAL::USER_AGENT_PANEL);
							break;
							
		case TplBinary:		m_simpleBookSourceExt->SetSelection(SourceExtBookSelection::VAL::NESTED_INFO_PANEL);
							break;
							
		case TplGcode:		m_simpleBookSourceExt->SetSelection(SourceExtBookSelection::VAL::GCODE_SEQUENCE_PANEL);
							break;
							
		default:			m_simpleBookSourceExt->SetSelection(SourceExtBookSelection::VAL::NULL_PANEL);
	}
	
	wxImageList* soureImgList = m_simpleBookSourceExt->GetImageList();
	wxASSERT(soureImgList);
	wxIcon   ico(soureImgList->GetIcon(m_simpleBookSourceExt->GetSelection()));
	wxString txt(m_simpleBookSourceExt->GetPageText(m_simpleBookSourceExt->GetSelection()));
	
	wxImageList* destImgList  = m_templateNotebook->GetImageList();
	wxASSERT(destImgList);
	destImgList->Replace(TemplateBookSelection::VAL::EXT_INFO_PANEL, ico);
	
	m_templateNotebook->SetPageImage(TemplateBookSelection::VAL::EXT_INFO_PANEL, TemplateBookSelection::VAL::EXT_INFO_PANEL);
	m_templateNotebook->SetPageText(TemplateBookSelection::VAL::EXT_INFO_PANEL,  txt);
}
///////////////////////////////////////////////////////////////////
void MainFrame::fillFileDetails(bool fileLoaded, const char* extFileName) {
///////////////////////////////////////////////////////////////////
	wxString details;
	
	if ( fileLoaded == true ) {
		bool evaluate = true;
		
		wxString fileName(getCurrentTemplatePathFileName());
		if ( extFileName != NULL ) {
			if ( wxFileName::Exists(extFileName) == true )	fileName.assign(extFileName);
			else											evaluate = false;
		}
		
		if ( evaluate == true ) {
			
			wxFileName file(fileName);
			details.append(wxString::Format("%s\n", file.GetModificationTime().FormatISOCombined(' ')));
			details.append(wxString::Format("%s",   file.GetHumanReadableSize()));
		}
	}
	
	m_filePreviewDetails->ChangeValue(details);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectEditorToolBox(bool fileLoaded) {
///////////////////////////////////////////////////////////////////
	wxFlexGridSizer* sizer = static_cast<wxFlexGridSizer*>(m_editorToolBox->GetContainingSizer());
	if ( sizer == NULL )
		return;

	bool show = true;
	if ( fileLoaded == false ) {
		show = false;
		
	} else {
		switch ( getCurrentTemplateFormat() ) {
			case TplSvg:		m_editorToolBox->SetSelection(0);
								show = false;
								break;
								
			case TplGcode:		m_editorToolBox->SetSelection(1);
								show = false;
								break;
								
			case TplBinary:		m_editorToolBox->SetSelection(2);
								show = true;
								break;
								
			default:			show = false;
		}
	}
	
	m_editorToolBox->Show(show);
	sizer->Layout();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::openFile(int sourcePageToSelect) {
///////////////////////////////////////////////////////////////////
	// First select the template page to get the rigth result 
	// from getCurrentTemplateFormat
	selectMainBookSourcePanel(sourcePageToSelect);
	
	bool ret = false;
	switch ( getCurrentTemplateFormat() ) {
		
		case TplSvg:	ret = sourceEditor->openFile(getCurrentTemplatePathFileName());
						break;
		
		case TplGcode:	ret = sourceEditor->openFile(getCurrentTemplatePathFileName());
						break;
						
		case TplBinary:	ret = sourceEditor->openFile(getCurrentTemplatePathFileName());
						break;
						
		default:		std::cerr << "MainFrame::openFile(): Unknown Type: " << getCurrentTemplateFormat() << std::endl;
						ret = false;
	}
	
	selectEditorToolBox(ret);
	fillFileDetails(ret);
	
	if ( ret == true ) {
		decorateExtTemplatePages(getCurrentTemplateFormat());
		outboundFilePreview->selectEmptyPreview();
		
		if ( inboundFileParser != NULL )
			inboundFileParser->clearControls();
		
		clearMotionMonitor();
		
		introduceCurrentFile(sourcePageToSelect);
	}
	
	updateFileContentPosition(0, 0);
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::prepareMotionMonitorViewType() {
///////////////////////////////////////////////////////////////////
	const GLContextBase::ModelType mt = sourceEditor->getModelType();
	motionMonitor->setModelType(mt);

	switch ( sourceEditor->getModelViewType() ) {
		case CncDimensions::CncDimension2D:		motionMonitor->viewTop();
												break;
												
		default:								motionMonitor->viewIso1();
	}
	
	motionMonitor->Refresh();
}
///////////////////////////////////////////////////////////////////
void MainFrame::introduceCurrentFile(int sourcePageToSelect) {
///////////////////////////////////////////////////////////////////
	MF_PRINT_LOCATION_CTX_FILE
	
	lruFileView->addFile(getCurrentTemplatePathFileName());
	fileView->selectFileInList(getCurrentTemplatePathFileName());
	
	selectMainBookSourcePanel(sourcePageToSelect);
	
	// publish model type
	prepareMotionMonitorViewType();
}
///////////////////////////////////////////////////////////////////
void MainFrame::prepareNewTemplateFile() {
///////////////////////////////////////////////////////////////////
	const TemplateFormat tf = getCurrentTemplateFormat();

	sourceEditor->SetReadOnly(false);
	sourceEditor->clearContent();
	sourceEditor->prepareNewTemplateFile(tf);
}
///////////////////////////////////////////////////////////////////
void MainFrame::newTemplate(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( saveTemplateOnDemand() == false )
		return;
	
	wxString templateName("..\\Templates\\");
    wxFileDialog newFileDialog(this, 
								_("New Template File"), 
								templateName,
								"",
                                "SVG Files (*.svg)|*.svg|GCode Files (*.ngc;*.gcode)|*.ngc;*.gcode", 
								wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

	if ( newFileDialog.ShowModal() == wxID_CANCEL ) 
        return; 

	wxString ov = getCurrentTemplateFileName();
	wxString oh = getCurrentTemplatePathFileName();

	m_inputFileName->SetValue(newFileDialog.GetFilename());
	m_inputFileName->SetHint(newFileDialog.GetPath());
	
	prepareNewTemplateFile();

	if ( !saveFile() ) {
		m_inputFileName->SetValue(ov);
		m_inputFileName->SetHint(oh);
	} else {
		if ( !openFile() ) {
			m_inputFileName->SetValue(ov);
			m_inputFileName->SetHint(oh);
		} else {
			prepareAndShowMonitorTemplatePreview(true);
		}
	}
	
	introduceCurrentFile();
}
///////////////////////////////////////////////////////////////////
void MainFrame::openTemplate(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(m_inputFileName);
	wxString templateName("..\\Templates\\");
	CncFileDialog openFileDialog(this,
								_("Open Template File"),
								templateName,
								"",
								"SVG Files (*.svg)|*.svg|GCode Files (*.ngc;*.gcode)|*.ngc;*.gcode|Binary Tpl Files (*.bct)|*.bct",
								wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if ( openFileDialog.ShowModal() == wxID_CANCEL ) {
		return;
	}

	wxString ov = getCurrentTemplateFileName();
	wxString oh = getCurrentTemplatePathFileName();

	m_inputFileName->SetValue(openFileDialog.GetFilename());
	m_inputFileName->SetHint(openFileDialog.GetPath());

	if ( !openFile() ) {
		m_inputFileName->SetValue(ov);
		m_inputFileName->SetHint(oh);
		
	} else {
		prepareAndShowMonitorTemplatePreview(true);
		
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::reloadTemplate(int sourcePageToSelect) {
///////////////////////////////////////////////////////////////////
	if ( !openFile(sourcePageToSelect) ) {
		std::cerr << "Error while reloding template: " << getCurrentTemplateFileName().c_str() << std::endl;
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::reloadTemplate(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	reloadTemplate();
}
///////////////////////////////////////////////////////////////////
void MainFrame::reloadTemplateFromButton(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	reloadTemplate();
}
///////////////////////////////////////////////////////////////////
void MainFrame::removeTemplateFromButton(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString fn (getCurrentTemplatePathFileName());
	wxFileName tplFile(fn);
	
	if ( tplFile.Exists() == false ) {
		std::cerr << "MainFrame::removeTemplateFromButton: Can't find: " << fn << std::endl;
		return;
	}
	
	wxString msg("Do you really want to remove the following template from the disk?\n\nTemplate:\n");
	msg.append(fn);
	wxMessageDialog dlg(this, msg, _T("Remove template . . . "), 
		                wxYES|wxNO|wxCANCEL|wxICON_QUESTION|wxCENTRE);
	
	int ret = dlg.ShowModal();
	if ( ret == wxID_YES ) {
		if ( wxRemoveFile(fn) == false ) {
			std::cerr << "MainFrame::removeTemplateFromButton: failed!" << std::endl;
			return;
		}
		
		std::clog << "Template: " << fn << " was removed." << std::endl;
		
		// update file lists
		lruFileView->removeFile(fn);
		lruFileView->save(lruStore);
		fileView->update();
		
		// clear source editor an motion monitor
		sourceEditor->clearContent();
		sourceEditor->DiscardEdits();
		
		clearMotionMonitor();
		m_inputFileName->SetValue("");
		m_inputFileName->SetHint("");
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::renameTemplateFromButton(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString oldFileName (getCurrentTemplatePathFileName());
	wxFileName tplFile(oldFileName);
	
	if ( tplFile.Exists() == false ) {
		std::cerr << "MainFrame::renameTemplateFromButton: Can't find: " << oldFileName << std::endl;
		return;
	}
	
	wxTextEntryDialog dlg(this, "Rename current Template:", "New Name", tplFile.GetFullName());
	dlg.SetMaxLength(256);
	if ( dlg.ShowModal() != wxID_OK )
		return;
	
	// search again - if the dialog was a long time opened the file may be removed otherwise
	if ( tplFile.Exists() == false ) {
		std::cerr << "MainFrame::renameTemplateFromButton: Can't find: " << oldFileName << std::endl;
		return;
	}
	
	// build new file name
	wxString newFileName(tplFile.GetPathWithSep());
	newFileName.append(dlg.GetValue());
	
	wxFileName newTplFile(newFileName);
	if ( newTplFile.Exists() == true ) {
		
		wxString msg("The new template name below already exists. Should it be overwritten?\n\nNew template name:\n");
		msg.append(newFileName);
		wxMessageDialog dlg(this, msg, _T("Template already exists. . . "), 
		                wxYES|wxNO|wxCANCEL|wxICON_QUESTION|wxCENTRE);
		
		if ( dlg.ShowModal() != wxID_YES )
			return;
	}
	
	if ( wxRenameFile(oldFileName, newFileName, true) == false ) {
		std::cerr << "MainFrame::renameTemplateFromButton: wxRenameFile failed: From: " << oldFileName << " to: " << newFileName << std::endl;
		return;
	}
	
	// update file lists
	lruFileView->removeFile(oldFileName);
	lruFileView->addFile(newFileName);
	lruFileView->save(lruStore);
	
	fileView->update();
	
	m_inputFileName->SetValue(newTplFile.GetFullName());
	m_inputFileName->SetHint(tplFile.GetPathWithSep());
}
///////////////////////////////////////////////////////////////////
void MainFrame::openTemplateSourceExtern(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString externalFile(getCurrentTemplatePathFileName());
	wxString tool; CncConfig::getGlobalCncConfig()->getEditorTool(tool);
	
	if ( getCurrentTemplateFormat() == TplBinary)
		BinaryFileParser::extractSourceContentAsFile(getCurrentTemplatePathFileName(), externalFile);
	else
		saveFile();
	
	openFileExtern(tool, externalFile);
}
///////////////////////////////////////////////////////////////////
void MainFrame::openTemplateSvgExtern(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString tool;
	
	switch ( getCurrentTemplateFormat() ) {
		case TplBinary:
					// binary files are readonly, thereforenow saveFile() necessary here
					CncConfig::getGlobalCncConfig()->getBINFileViewer(tool);
					openFileExtern(tool, getCurrentTemplatePathFileName());
					break;
					
		case TplSvg:
					saveFile();
					CncConfig::getGlobalCncConfig()->getSVGFileViewer(tool);
					openFileExtern(tool, getCurrentTemplatePathFileName());
					break;
					
		case TplGcode:
					saveFile();
					CncConfig::getGlobalCncConfig()->getGCodeFileViewer(tool);
					openFileExtern(tool, getCurrentTemplatePathFileName());
					break;
					
		default:
					std::clog << "No external editor availiable for current file: " 
					          << getCurrentTemplatePathFileName().c_str() 
							  << std::endl;
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::activateMainWindow(wxActivateEvent& event) {
///////////////////////////////////////////////////////////////////
	event.Skip(true);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::saveFile() {
///////////////////////////////////////////////////////////////////
	wxASSERT(sourceEditor);
	
	// First select the template page to get the rigth result 
	// by getCurrentTemplateFormat
	selectMainBookSourcePanel();
	
	// Deactivate observer
	CncTemplateObserver::Deactivator observerDeactivator(templateObserver);
	
	bool ret = sourceEditor->saveFile();
	if ( ret == true )
		prepareAndShowMonitorTemplatePreview(true);
	
	return ret;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::saveFileAs() {
///////////////////////////////////////////////////////////////////
	wxASSERT(m_inputFileName);
	
	wxFileDialog saveFileDialog(this, 
	                            _("Save Template File"), 
								getCurrentTemplatePathFileName(), 
								"",
								"SVG Files (*.svg)|*.svg|GCode Files (*.ngc;*.gcode)|*.ngc;*.gcode",  
								wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
								
	if ( saveFileDialog.ShowModal() == wxID_CANCEL ) 
		return true;
		
	bool ret = sourceEditor->saveFileAs(saveFileDialog.GetPath());
	if ( ret == true ) {
		m_inputFileName->SetValue(saveFileDialog.GetFilename());
		m_inputFileName->SetHint(saveFileDialog.GetPath());
		introduceCurrentFile();
	}
	
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::saveTemplate(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	saveFile();
}
///////////////////////////////////////////////////////////////////
void MainFrame::saveTemplateFromButton(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	saveFile();
}
///////////////////////////////////////////////////////////////////
void MainFrame::saveTemplateAs(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	saveFileAs();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processVirtualTemplate() {
///////////////////////////////////////////////////////////////////
	bool ret;
	
	inboundFileParser->enableUserAgentControls(isDisplayParserDetails());
	inboundFileParser->setInboundSourceControl(sourceEditor);
	
	if ( isDebugMode == true ) 	ret = inboundFileParser->processDebug();
	else 						ret = inboundFileParser->processRelease();
	
	return ret;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::isGamepadNotificationActive() {
///////////////////////////////////////////////////////////////////
	if ( gamepadThread == NULL )
		return false;
		
	return gamepadThread->IsRunning();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::isGamepadDialogShown() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cncGameportDlg);
	return cncGameportDlg->IsShown();
}
///////////////////////////////////////////////////////////////////
void MainFrame::activateGamepadNotificationsOnDemand(bool state) {
///////////////////////////////////////////////////////////////////
	if ( CncGampadDeactivator::gamepadNotificationsAllowed() == false ) {
		activateGamepadNotifications(false);
		return;
	}
	
	activateGamepadNotifications(state);
}
///////////////////////////////////////////////////////////////////
void MainFrame::activateGamepadNotifications(bool state) {
///////////////////////////////////////////////////////////////////
	if ( gamepadThread == NULL )
		return;
		
	bool changed = false;
	if ( state == false ) {
		if ( gamepadThread->IsRunning() ) {
			gamepadThread->Pause();
			changed = true;
		}
	} else {
		if ( gamepadThread->IsPaused() ) {
			gamepadThread->Resume();
			changed = true;
		}
	}
	
	decorateGamepadState(gamepadThread->IsRunning());
	
	if ( changed == false)
		;
	/*
	//if ( gamepadThread->IsPaused() )	cnc::trc.logInfoMessage( changed ? "Gamepad thread deactivated" : "Gamepad thread stays deactivated");
	//else								cnc::trc.logInfoMessage( changed ? "Gamepad thread activated"   : "Gamepad thread stays activated");
	if ( gamepadThread->IsPaused() )	std::cout << ( changed ? "Gamepad thread deactivated" : "Gamepad thread stays deactivated") << std::endl;
	else								std::cout << ( changed ? "Gamepad thread activated"   : "Gamepad thread stays activated") << std::endl;
	*/
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processBinaryTemplate() {
///////////////////////////////////////////////////////////////////
	if ( inboundFileParser != NULL )
		delete inboundFileParser;
		
	CncGampadDeactivator cpd(this);
		
	inboundFileParser = new BinaryFileParser(getCurrentTemplatePathFileName().c_str(), new BinaryPathHandlerCnc(cnc));
	return processVirtualTemplate();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processSVGTemplate() {
///////////////////////////////////////////////////////////////////
	if ( inboundFileParser != NULL )
		delete inboundFileParser;
		
	CncGampadDeactivator cpd(this);
		
	inboundFileParser = new SVGFileParser(getCurrentTemplatePathFileName().c_str(), cnc);
	return processVirtualTemplate();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processGCodeTemplate() {
///////////////////////////////////////////////////////////////////
	if ( inboundFileParser != NULL )
		delete inboundFileParser;
		
	CncGampadDeactivator cpd(this);
	
	inboundFileParser = new GCodeFileParser(getCurrentTemplatePathFileName().c_str(), new GCodePathHandlerCnc(cnc));
	return processVirtualTemplate();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processManualTemplate() {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	wxASSERT( cncManuallyMoveCoordPanel );
	
	if ( inboundFileParser != NULL )
		delete inboundFileParser;
		
	ManuallyParser* p = new ManuallyParser(new ManuallyPathHandlerCnc(cnc));
	inboundFileParser = p;

	ManuallyPathHandlerCnc::MoveDefinition move;
	move.speedMode 		= CncSpeedUserDefined;
	move.absoluteMove	= cncManuallyMoveCoordPanel->isAbsoluteMove();
	move.toolState		= cncManuallyMoveCoordPanel->switchToolOn();
	move.correctLimit   = cncManuallyMoveCoordPanel->correctLimitStates();
	
	move.f = cncManuallyMoveCoordPanel->getSpeedValueMM_MIN();
	move.x = cncManuallyMoveCoordPanel->getValueX();
	move.y = cncManuallyMoveCoordPanel->getValueY();
	move.z = cncManuallyMoveCoordPanel->getValueZ();
	
	if ( m_unit->GetValue() == "steps" ) {
		move.x *= THE_CONFIG->getDisplayFactX();
		move.y *= THE_CONFIG->getDisplayFactY();
		move.z *= THE_CONFIG->getDisplayFactZ();
	} 
	
	p->reset(cnc->getCurAppPosMetric());
	p->addMove(move);
	
	bool ret = false;
	if ( isDebugMode == true ) 	ret = p->processDebug();
	else 						ret = p->processRelease();
	
	return ret;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processTestTemplate() {
///////////////////////////////////////////////////////////////////
	wxWindow* page = m_testCaseBook->GetCurrentPage();
	if ( page != NULL ) {
		if 		( page == m_testIntervalPage )	return processTestInterval();
		else if ( page == m_testDimensions )	return processTestDimensions();
		//...
		
		wxString msg;
		msg << "This Test (\"";
		msg << m_testCaseBook->GetPageText(m_testCaseBook->GetSelection()); 
		msg << "\") isn't assigned to the run mode. May be there is an action control on the test page itself.\n\n";
		msg << "Nothing will be done.\n" ;
		
		wxMessageDialog dlg(this, msg, _T("MainFrame::processTestTemplate(): Not registered test . . . "), 
							wxOK|wxCENTRE|wxICON_QUESTION);
		
		dlg.ShowModal();
		return true;
	}
	
	std::cerr << "MainFrame::processTestTemplate(): Invalid page selection!" << std::endl;
	return false;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processTestInterval() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);

	if ( cnc->isConnected() == false ) {
		std::cerr << "Not connetced, nothing will be processed." << std::endl;
		return false;
	}
	
	wxString sel = m_testIntervalMode->GetStringSelection();
	char mode = sel[0];
	
	int countX = m_testCountX->GetValue();
	int countY = m_testCountY->GetValue(); 
	int countZ = m_testCountZ->GetValue();
	
	double xd = 0.0, yd = 0.0, zd = 0.0;
	
	bool modeX = m_testModeX->GetValue();
	bool modeY = m_testModeY->GetValue();
	bool modeZ = m_testModeZ->GetValue();
	
	if ( modeX == true ) {
		wxString xs = m_testDistanceX->GetValue();
		if ( xs != "" )	xs.ToCDouble(&xd);
	} 
	
	if ( modeY == true ) {
		wxString ys = m_testDistanceY->GetValue();
		if ( ys != "" )	ys.ToCDouble(&yd);
	}
	
	if ( modeZ == true ) {
		wxString zs = m_testDistanceZ->GetValue();
		if ( zs != "" )	zs.ToCDouble(&zd);
	} 
	
	wxString firstX = m_testFirstX->GetStringSelection();
	wxString firstY = m_testFirstY->GetStringSelection();
	wxString firstZ = m_testFirstZ->GetStringSelection();
	
	xd = (firstX == "First +" ? +xd : -xd);
	yd = (firstY == "First +" ? +yd : -yd);
	zd = (firstZ == "First +" ? +zd : -zd);
	
	m_testDurationCounterX->SetLabel("-");
	m_testDurationCounterY->SetLabel("-");
	m_testDurationCounterZ->SetLabel("-");
	
	switch ( mode ) {
		case 'A': 	if ( modeX == true ) {
						for( int i=0; i<countX; i++ ) {
							processTestMove(m_testAxisX, m_testDurationCounterX, i, +xd, 0.0, 0.0);
							processTestMove(m_testAxisX, m_testDurationCounterX, i, -xd, 0.0, 0.0);
						}
					}
					
					if ( modeY == true ) {
						for( int i=0; i<countY; i++ ) {
							processTestMove(m_testAxisY, m_testDurationCounterY, i, 0.0, +yd, 0.0);
							processTestMove(m_testAxisY, m_testDurationCounterY, i, 0.0, -yd, 0.0);
						}
					}
					
					if ( modeZ == true ) {
						for( int i=0; i<countZ; i++ ) {
							processTestMove(m_testAxisZ, m_testDurationCounterZ, i, 0.0, 0.0, +zd);
							processTestMove(m_testAxisZ, m_testDurationCounterZ, i, 0.0, 0.0, -zd);
						}
					}
						
					break;
					
		case 'B': 	for( int i=0; i<countX; i++ ) {
						if ( modeX == true ) processTestMove(m_testAxisX, m_testDurationCounterX, i, +xd, 0.0, 0.0);
						if ( modeY == true ) processTestMove(m_testAxisY, m_testDurationCounterY, i, 0.0, +yd, 0.0);
						if ( modeZ == true ) processTestMove(m_testAxisZ, m_testDurationCounterZ, i, 0.0, 0.0, +zd);
						
						if ( modeX == true ) processTestMove(m_testAxisX, m_testDurationCounterX, i, -xd, 0.0, 0.0);
						if ( modeY == true ) processTestMove(m_testAxisY, m_testDurationCounterY, i, 0.0, -yd, 0.0);
						if ( modeZ == true ) processTestMove(m_testAxisZ, m_testDurationCounterZ, i, 0.0, 0.0, -zd);
					}
					
					break;
					
		case 'C': 	for( int i=0; i<countX; i++ ) {
						processTestMove(NULL, NULL, i, +xd, +yd, +zd);
						processTestMove(NULL, NULL, i, -xd, -yd, -zd);
					}
					
					break;
		default: ;// do nothing
	}
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processTestDimensions() {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	
	if ( cnc->isConnected() == false ) {
		std::cerr << "Not connetced, nothing will be processed." << std::endl;
		return false;
	}
	
	cnc->activatePositionCheck(false);
	
	double result = -DBL_MAX;
	bool ret = true;
	
	// meassure X Axis
	if ( m_testDimModeX->GetValue() == true  && ret == true ) {
		m_testDimResultX->SetValue(wxString::Format("%4.3f", 0.0));
		ret = cnc->meassureXDimension(m_testDimMinX, m_testDimMaxX, result);
		if ( ret ) {
			m_testDimMinX->SetValue(false);
			m_testDimMaxX->SetValue(false);
			m_testDimResultStateX->SetValue(true);
			m_testDimResultX->SetValue(wxString::Format("%4.3f", result));
			
			ret = cnc->moveXToMid();
		}
	}
	// meassure z Axis
	if ( m_testDimModeY->GetValue() == true && ret == true ) {
		m_testDimResultY->SetValue(wxString::Format("%4.3f", 0.0));
		ret = cnc->meassureYDimension(m_testDimMinY, m_testDimMaxY, result);
		if ( ret ) {
			m_testDimMinY->SetValue(false);
			m_testDimMaxY->SetValue(false);
			m_testDimResultStateY->SetValue(true);
			m_testDimResultY->SetValue(wxString::Format("%4.3f", result));
			
			ret = cnc->moveYToMid();
		}
	}
	// meassure z Axis
	if ( m_testDimModeZ->GetValue() == true  && ret == true ) {
		m_testDimResultZ->SetValue(wxString::Format("%4.3f", 0.0));
		ret = cnc->meassureZDimension(m_testDimMinZ, m_testDimMaxZ, result);
		if ( ret ) {
			m_testDimMinZ->SetValue(false);
			m_testDimMaxZ->SetValue(false);
			m_testDimResultStateZ->SetValue(true);
			m_testDimResultZ->SetValue(wxString::Format("%4.3f", result));
			
			ret = cnc->moveZToMid();
		}
	}
	
	if ( ret == false ) {
		std::cerr << "MainFrame::processTestDimensions(): Invalid Test!" << std::endl;
	}
	
	cnc->activatePositionCheck(true);
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::testDimTakeOverAll(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	testDimTakeOverX(event);
	testDimTakeOverY(event);
	testDimTakeOverZ(event);
}
///////////////////////////////////////////////////////////////////
void MainFrame::testDimTakeOverX(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	double v;
	wxString val = m_testDimResultX->GetValue();
	val.ToDouble(&v);
	
	if ( v > 0.0 )
		CncConfig::getGlobalCncConfig()->setMaxDimensionX(v);
}
///////////////////////////////////////////////////////////////////
void MainFrame::testDimTakeOverY(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	double v;
	wxString val = m_testDimResultY->GetValue();
	val.ToDouble(&v);
	
	if ( v > 0.0 )
		CncConfig::getGlobalCncConfig()->setMaxDimensionY(v);
}
///////////////////////////////////////////////////////////////////
void MainFrame::testDimTakeOverZ(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	double v;
	wxString val = m_testDimResultZ->GetValue();
	val.ToDouble(&v);

	if ( v > 0.0 )
		CncConfig::getGlobalCncConfig()->setMaxDimensionZ(v);
}
///////////////////////////////////////////////////////////////////
void MainFrame::processTestMove(wxStaticText* axis, wxStaticText* counter, int c, double xd, double yd, double zd) {
///////////////////////////////////////////////////////////////////
	if ( axis != NULL ) {
		axis->SetForegroundColour(*wxWHITE);
		axis->SetBackgroundColour(*wxBLACK);
		axis->Refresh();
	} else {
		m_testAxisX->SetForegroundColour(*wxWHITE);
		m_testAxisY->SetForegroundColour(*wxWHITE);
		m_testAxisZ->SetForegroundColour(*wxWHITE);
		m_testAxisX->SetBackgroundColour(*wxBLACK);
		m_testAxisY->SetBackgroundColour(*wxBLACK);
		m_testAxisZ->SetBackgroundColour(*wxBLACK);
		m_testAxisX->Refresh();
		m_testAxisY->Refresh();
		m_testAxisZ->Refresh();
	}
		
	if ( counter != NULL ) {
		counter->SetLabel(wxString::Format("%d", c + 1));
	} else {
		m_testDurationCounterX->SetLabel(wxString::Format("%d", c + 1));
		m_testDurationCounterY->SetLabel(wxString::Format("%d", c + 1));
		m_testDurationCounterZ->SetLabel(wxString::Format("%d", c + 1));
	}
		
	dispatchAll();
	cnc->manualSimpleMoveMetric3D(xd, yd, zd);
	dispatchAll();
	
	if ( axis != NULL ) {
		axis->SetForegroundColour(*wxBLACK);
		axis->SetBackgroundColour(*wxLIGHT_GREY);
		axis->Refresh();
	} else {
		m_testAxisX->SetForegroundColour(*wxBLACK);
		m_testAxisY->SetForegroundColour(*wxBLACK);
		m_testAxisZ->SetForegroundColour(*wxBLACK);
		m_testAxisX->SetBackgroundColour(*wxLIGHT_GREY);
		m_testAxisY->SetBackgroundColour(*wxLIGHT_GREY);
		m_testAxisZ->SetBackgroundColour(*wxLIGHT_GREY);
		m_testAxisX->Refresh();
		m_testAxisY->Refresh();
		m_testAxisZ->Refresh();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::testCountXSpinCtl(wxSpinEvent& event) {
///////////////////////////////////////////////////////////////////
	// do nothing
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectTestIntervalMode(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString sel = m_testIntervalMode->GetStringSelection();
	char mode = sel[0];
	
	m_testDurationCounterX->SetLabel("-");
	m_testDurationCounterY->SetLabel("-");
	m_testDurationCounterZ->SetLabel("-");

	m_testCountY->Enable(mode == 'A');
	m_testCountZ->Enable(mode == 'A');
	
	if ( mode != 'A' ) {
		int val = m_testCountX->GetValue();
		
		m_testCountY->SetValue(val);
		m_testCountZ->SetValue(val);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::testCountXUpdated(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString sel = m_testIntervalMode->GetStringSelection();
	char mode = sel[0];
	
	if ( mode != 'A' ) {
		int val = m_testCountX->GetValue();
		
		m_testCountY->SetValue(val);
		m_testCountZ->SetValue(val);
	}
}
///////////////////////////////////////////////////////////////////
bool MainFrame::checkIfRunCanBeProcessed(bool confirm) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	if ( evaluatePositions == true && cnc->validateAppAgainstCtlPosition() == false ) {
		
		wxString msg("Validate positions failed\n");
		msg << "\nPC pos        : ";
		msg << cnc->getCurAppPos().getX(); msg << ",";
		msg << cnc->getCurAppPos().getY(); msg << ",";
		msg << cnc->getCurAppPos().getZ();
		msg << "\nController pos: ";
		msg << cnc->requestControllerPos().getX();  msg << ",";
		msg << cnc->requestControllerPos().getY();  msg << ",";
		msg << cnc->requestControllerPos().getZ();
		msg << "\n\nThe run command will be aborted!";

		wxMessageDialog dlg(this, msg, _T("CNC Position check . . . "), 
		                    wxOK|wxCENTRE|wxICON_ERROR);
 	
		dlg.ShowModal();
		setRefPostionState(false);
		return false;
	}
	
	cnc->evaluateLimitState();
	if ( cnc->isALimitSwitchActive() ) {
		// always return false to reconfigure zero in this sitiuation
		std::clog << "MainFrame::checkIfRunCanBeProcessed(): Limit switch detected" << std::endl;
		return false;
	}
	
	return (confirm == true ? showConfigSummaryAndConfirmRun() : true);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::checkReferencePositionState() {
///////////////////////////////////////////////////////////////////
	const CncDoublePosition refPos(CncStartPositionResolver::getReferencePosition());
	const bool zero = ( cnc->getCurAppPosMetric() != refPos );

	if ( isZeroReferenceValid == false ) {
		const TemplateFormat tf = getCurrentTemplateFormat();
		
		if ( tf != TplManual && tf != TplTest ) {
			wxString msg("The current reference position isn't valid due to a setup change or it isn't not initialized yet.\n");
			
			const int ret = showReferencePositionDlg(msg);
			if ( ret == wxID_OK && zero == false ) {
				cnc::trc.logInfoMessage("Reference Position is fixed now. Please restart");
				
				// Safety: Always return false in this case because this will
				// stopp the current startet run. 
				return false;
			}
	
			// check current position
			if ( zero ) {
				wxASSERT( cnc != NULL );

				bool openDlg = false;
				switch ( THE_CONFIG->getRunConfirmationModeAsChar() ) {
					// Always
					case 'A':	openDlg = true; break;
					// Serial Port only
					case 'S': 	openDlg = !cnc->isEmulator(); break;
					// Never
					default:	openDlg = false;
				}

				CncStartPositionResolver dlg(this);
				int ret = wxID_OK;

				if ( openDlg == true ) 	ret = dlg.ShowModal();
				else 					ret = dlg.resolve();

				if ( ret == wxID_OK )
					cnc::trc.logInfoMessage("Reference Position is fixed now. Please restart");
				
				// Safety: Always return false in this case because this will
				// stopp the current startet run. 
				if ( openDlg == true )
					return false;
			}
		}
	}
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool MainFrame::showConfigSummaryAndConfirmRun() {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
		
	switch ( THE_CONFIG->getRunConfirmationModeAsChar() ) {
		// Always
		case 'A':	break;
		
		// Serial Port only
		case 'S': 	if ( cnc->isEmulator() )
						return true;
						
					break;
		// Never
		default:	return true;
	}
	
	// check template format
	if ( getCurrentTemplateFormat() != TplSvg && getCurrentTemplateFormat() != TplGcode )
		return true;
	
	// control handling
	disableControls();
	m_btCancelRun->Enable(true);
	m_btConfirmRun->Enable(true);
	
	// select summary page
	m_outboundNotebook->SetSelection(OutboundSelection::VAL::SUMMARY_PANEL);
	m_notebookConfig->SetSelection(OutboundCfgSelection::VAL::SUMMARY_PANEL);
	
	// wait for user feedback
	runConfirmationInfo = RunConfirmationInfo::Wait;
	wxDateTime start 	= wxDateTime::UNow();
	wxDateTime now 		= wxDateTime::UNow();
	wxColour c1         = wxColor(227, 227, 227);
	wxColour c2         = wxColor(109, 109, 190);
	wxString msg		= "Waiting for user feedback. Please confirm to run . . . ";
	if ( m_btConfirmRun->GetBackgroundColour() == c1 )	m_btConfirmRun->SetBackgroundColour(c2);
	else												m_btConfirmRun->SetBackgroundColour(c1);
	cnc::trc.logWarning(msg);
	
	while ( runConfirmationInfo == RunConfirmationInfo::Wait ) {
		dispatchAll();
		now = wxDateTime::UNow();
		wxTimeSpan diff = now - start;
		if ( diff.GetSeconds() >= 1 ) {
			start = now;
			if ( m_btConfirmRun->GetBackgroundColour() == c1 ) {
				cnc::trc.logWarning(msg);
				m_btConfirmRun->SetBackgroundColour(c2);
			} else {
				cnc::trc.clear();
				m_btConfirmRun->SetBackgroundColour(c1);
			}
		}
	}
	
	cnc::trc.clear();
	m_btConfirmRun->SetBackgroundColour(c1);
	enableControls();
	m_btCancelRun->Enable(false);
	m_btConfirmRun->Enable(false);
	
	return (runConfirmationInfo == RunConfirmationInfo::Confirmed);
}
///////////////////////////////////////////////////////////////////
void MainFrame::confirmRun(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	runConfirmationInfo = RunConfirmationInfo::Confirmed;
}
///////////////////////////////////////////////////////////////////
void MainFrame::cancelRun(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	runConfirmationInfo = RunConfirmationInfo::Canceled;
}
///////////////////////////////////////////////////////////////////
void MainFrame::collectSvgSpecificSummary() {
///////////////////////////////////////////////////////////////////
	typedef CncSummaryListCtrl::ParameterType PT;
	CncConfig* cc = CncConfig::getGlobalCncConfig();
	
	cncSummaryListCtrl->addHeadline(CncSummaryListCtrl::ParameterType::PT_HEADLINE, "SVG specific Settings");

	cncSummaryListCtrl->addParameter(PT::PT_SVG, "Z axis values:", "", "");
	cncSummaryListCtrl->addParameter(PT::PT_SVG, "  Max durations", 				wxString::Format("%u",		cc->getMaxDurations()),							"#");
	cncSummaryListCtrl->addParameter(PT::PT_SVG, "  Workpiece offset", 				wxString::Format("%4.3f",	cc->getWorkpieceOffset()),						"mm");
	cncSummaryListCtrl->addParameter(PT::PT_SVG, "  Max duration thickness", 		wxString::Format("%4.3f",	cc->getMaxDurationThickness()),					"mm");
	cncSummaryListCtrl->addParameter(PT::PT_SVG, "  Calculated durations", 			wxString::Format("%u",		cc->getDurationCount()),						"#");
	cncSummaryListCtrl->addParameter(PT::PT_SVG, "  Current Z distance", 			wxString::Format("%4.3f",	cc->getCurZDistance()),							"mm");
	cncSummaryListCtrl->addParameter(PT::PT_SVG, "  Wpt is included", 				wxString::Format("%d",		cc->getReferenceIncludesWpt()),					"-");
	
	for (unsigned int i=0; i<cc->getMaxDurations(); i++) {
		if ( cc->getDurationThickness(i) != 0.0 ) {
			cncSummaryListCtrl->addParameter(PT::PT_SVG, 
			                                 wxString::Format("   Duration step[%02u]", i), 
			                                 wxString::Format("%4.3f",	cc->getDurationThickness(i)),
			                                 "mm");
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::collectGCodeSpecificSummary() {
///////////////////////////////////////////////////////////////////
	typedef CncSummaryListCtrl::ParameterType PT;
	CncConfig* cc = CncConfig::getGlobalCncConfig();
	
	cncSummaryListCtrl->addHeadline(CncSummaryListCtrl::ParameterType::PT_HEADLINE, "GCode specific Settings");
	cncSummaryListCtrl->addParameter(PT::PT_GCODE, "Sample . . . .", 					wxString::Format("%4.3f",	cc->getReplyThresholdMetric()),						"xx");
}
///////////////////////////////////////////////////////////////////
void MainFrame::collectSummary() {
///////////////////////////////////////////////////////////////////
	if ( cnc == NULL )
		return;
		
	typedef CncSummaryListCtrl::ParameterType PT;
		
	CncConfig* cc = CncConfig::getGlobalCncConfig();
	cncSummaryListCtrl->clear();
	
	cncSummaryListCtrl->addHeadline(PT::PT_HEADLINE, "Common Settings");
	cncSummaryListCtrl->addParameter(PT::PT_COMMON, "Default Tool", 					cc->getDefaultToolParamAsString(),												"-");
	cncSummaryListCtrl->addParameter(PT::PT_COMMON, "Workpiece thickness", 				wxString::Format("%4.3f", 	cc->getWorkpieceThickness()),						"mm");
	//cncSummaryListCtrl->addParameter(PT::PT_COMMON, "Curve lib resolution", 			wxString::Format("%0.3f", 	cc->getRenderResolution()),							"-");
	cncSummaryListCtrl->addParameter(PT::PT_COMMON, "Default Papid speed", 				wxString::Format("%4.3f", 	cc->getDefaultRapidSpeed_MM_MIN()),					"mm/min");
	cncSummaryListCtrl->addParameter(PT::PT_COMMON, "Default Work speed", 				wxString::Format("%4.3f", 	cc->getDefaultWorkSpeed_MM_MIN()),					"mm/min");
	cncSummaryListCtrl->addParameter(PT::PT_COMMON, "Reply Threshold", 					wxString::Format("%4.3f",	cc->getReplyThresholdMetric()),						"mm");

	// type specific . . .
	switch ( getCurrentTemplateFormat() ) {
		case TplSvg:	collectSvgSpecificSummary();
						break;
		
		case TplGcode:	collectGCodeSpecificSummary();
						break;
						
		default:		; // do nothing
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateSetterList() {
///////////////////////////////////////////////////////////////////
	if ( updateManagerThread == NULL )
		return;
		
	if ( updateManagerThread->IsPaused() == true ) {
		wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
		updateManagerThread->Resume();
	}
	
	if ( setterList->IsFrozen() == false )
		setterList->Freeze();
		
	int sizeBefore = setterList->getItemCount();
	if ( updateManagerThread->fillSetterList(setterList) > 0 ) {
		if ( sizeBefore == 0 )
			setterList->updateColumnWidth();
		
		if ( setterList->IsShownOnScreen() ) {
			setterList->Refresh();
			setterList->EnsureVisible(0L);
		}
	}

	if ( setterList->IsFrozen() == true )
		setterList->Thaw();

	m_setterListCount->SetValue(wxString::Format("# %ld", (long)setterList->GetItemCount()));
}
///////////////////////////////////////////////////////////////////
void MainFrame::nootebookConfigChanged(wxListbookEvent& event) {
///////////////////////////////////////////////////////////////////
	// check if currentla a run is active and return if so to avoid 
	// a controller request during this mode
	bool runActive = !m_rcRun->IsEnabled();

	m_dvListCtrlControllerConfig->DeleteAllItems();
	m_dvListCtrlControllerPins->DeleteAllItems();
	
	unsigned int sel = event.GetSelection();
	if ( (wxWindow*)event.GetEventObject() == m_notebookConfig ) {
		
		wxCommandEvent dummyEvent;
		
		switch ( sel ) {
			case OutboundCfgSelection::VAL::SUMMARY_PANEL:
					collectSummary();
					break;
					
			case OutboundCfgSelection::VAL::CNC_PIN_PANEL:
					if ( runActive == true ) {
						cnc::trc.logWarning("During an active run controller requests are not possible! Ty it later again.");
						return;
					}
					requestControllerPinsFromButton(dummyEvent);
					break;
					
			case OutboundCfgSelection::VAL::CNC_CONFIG_PANEL:
					if ( runActive == true ) {
						cnc::trc.logWarning("During an active run controller requests are not possible! Ty it later again.");
						return;
					}
					requestControllerConfigFromButton(dummyEvent);
					break;
		}
	}
}
///////////////////////////////////////////////////////////////////
bool MainFrame::processTemplateWrapper(bool confirm) {
///////////////////////////////////////////////////////////////////
	try {
		wxASSERT(cnc);
		if ( cnc->isInterrupted() == true ) {
			std::cerr << "MainFrame::processTemplateWrapper(): Run aborted - Interrupt detected!" << std::endl;
			return false;
		}
		
		bool ret = true;
		
		CncRunEventFilter cef;
		
		// deactivate idle requests
		CncTransactionLock ctl(this);
		
		// Deactivate observer
		CncTemplateObserver::Deactivator observerDeactivator(templateObserver);
		
		// it's very import to deactivate the notifications during a run
		// because instead every config change (sc()) will release a notification
		// this will be the case for example if the SVG path handler changes
		// the z -axis values . . .
		// as a result the processing slows down significantly.
		CncConfig::NotificationDeactivator cfgNotDeactivation;
		
		THE_CONTEXT->setAllowEventHandling(true);
		cnc->resetSetterMap();
		cnc->processSetter(PID_SEPARATOR, SEPARARTOR_RUN);
		cnc->enableProbeMode(THE_CONTEXT->isProbeMode());
		cnc->enableStepperMotors(ENABLE_STATE_ON);

		wxString fn (getCurrentTemplatePathFileName());
		if ( fn.IsEmpty() == true )
			return false;
		
		// do this before the clearing opertions below, 
		// because then the ref pos correction will be also removed
		// as well as the previous drawing
		if ( checkReferencePositionState() == false )
			return false;
		
		wxASSERT( cncManuallyMoveCoordPanel );
		const TemplateFormat tf = getCurrentTemplateFormat();
		bool clearMM = true;
		
		if ( tf == TplManual )  clearMM = cncManuallyMoveCoordPanel->shouldClearMontionMonitor();
		else  					cncManuallyMoveCoordPanel->resetClearViewState();
		
		if ( clearMM )  
			clearMotionMonitor();
			
		decorateOutboundSaveControls(false);
		cncPreprocessor->clearAll();
		
		if ( cnc->isReadyToRun() == false ) {
			std::cerr << "MainFrame::processTemplateWrapper: Controller isn't ready to run: Run was rejected!" << std::endl;
			ret = false;
		}
		
		if ( ret == true )
			ret = checkIfRunCanBeProcessed(confirm);
			
		if ( ret == true ) {
			// This instance starts and stops the speed monitor
			CncSpeedMonitorRunner smr(speedMonitor, THE_CONFIG->getMaxSpeedXYZ_MM_MIN());
			ret = processTemplateIntern();
			
			// refresh some periphery
			motionMonitor->updateMonitorAndOptions();
			statisticsPane->updateReplayPane();
		}
		else {
			std::cerr << "MainFrame::processTemplateWrapper(): checkIfRunCanBeProcessed() failed" << std::endl;
		}
		
		// prepare final statements
		wxString probeMode(THE_CONTEXT->isProbeMode() ? "ON" :"OFF");
		if ( ret == false) {
			wxString hint("not successfully");
			cnc::cex1 << wxString::Format("%s - Processing(probe mode = %s) finished %s . . .", wxDateTime::UNow().FormatISOTime(), probeMode, hint) << std::endl;
			ctl.setErrorMode();
			
		} 
		else {
			std::clog << wxString::Format("%s - Processing(probe mode = %s) finished successfully . . .", wxDateTime::UNow().FormatISOTime(), probeMode) << std::endl;
		}
		
		Serial::Trigger::EndRun endRun;
		endRun.succcess = ret;
		cnc->processTrigger(endRun);

		decorateOutboundSaveControls(cnc->isOutputAsTemplateAvailable());

		return ret;
	}
	catch (const CncInterruption& ex) {
		std::cerr << "MainFrame::processTemplateWrapper(): Exception received:" 
		          << std::endl
				  << ex.what()
				  << std::endl;
	}
	catch (...) {
		std::cerr << "MainFrame::processTemplateWrapper(): Unhandled Exception!" << std::endl;
	}
	
	return false;
}
///////////////////////////////////////////////////////////////////
// don't call this method directly, instead use processTemplateWrapper
bool MainFrame::processTemplateIntern() {
///////////////////////////////////////////////////////////////////
	startAnimationControl();

	Serial::Trigger::BeginRun begRun;
		begRun.parameter.SRC.fileName		= getCurrentTemplatePathFileName();
		begRun.parameter.SRC.fileType		= getCurrentTemplateFormatName();
		begRun.parameter.SET.hardwareResX 	= THE_CONFIG->getDisplayFactX();
		begRun.parameter.SET.hardwareResY 	= THE_CONFIG->getDisplayFactY();
		begRun.parameter.SET.hardwareResZ 	= THE_CONFIG->getDisplayFactZ();
		begRun.parameter.PRC.user			= "Hacki Wimmer";
	cnc->processTrigger(begRun);
	
	if ( m_clearSerialSpyBeforNextRun->GetValue() )
		clearSerialSpy();
	
	clearPositionSpy();
	
	if ( THE_CONTEXT->secureModeInfo.isActive == false ) {
		showAuiPane("Outbound");
		selectMonitorBookCncPanel();
		
		// select draw pane
		m_outboundNotebook->SetSelection(OutboundSelection::VAL::MOTION_MONITOR_PANAL);
		m_listbookMonitor->SetSelection(OutboundMonitorSelection::VAL::MOTION_MONITOR_PANAL);
	}
	
	// select template Page
	if ( m_mainViewSelector->GetSelection() != MainBookSelection::VAL::MANUEL_PANEL && 
	     m_mainViewSelector->GetSelection() != MainBookSelection::VAL::TEST_PANEL && 
	     m_mainViewSelector->GetSelection() != MainBookSelection::VAL::SOURCE_PANEL &&
	     m_mainViewSelector->GetSelection() != MainBookSelection::VAL::SETUP_PANEL) {
		selectMainBookSourcePanel();
	}
	
	THE_CONTEXT->timestamps.logTotalTimeStart();
	motionMonitor->pushProcessMode();
	
	updateStepDelay();
	disableControls();
	resetMinMaxPositions();
	updateCncConfigTrace();
	
	/*
	THE_CONTEXT->setAllowEventHandling(true);
	cnc->resetSetterMap();
	cnc->processSetter(PID_SEPARATOR, SEPARARTOR_RUN);
	cnc->enableProbeMode(THE_CONTEXT->isProbeMode());
	cnc->enableStepperMotors(ENABLE_STATE_ON);
	*/
	
	bool ret = false;
	switch ( getCurrentTemplateFormat() ) {
		
		case TplBinary:
			if ( saveTemplateOnDemand() == false )
				break;
			clearMotionMonitor();
			// measurement handling will be done by the corespondinf file parser
			ret = processBinaryTemplate();
			break;
			
		case TplSvg:
			if ( saveTemplateOnDemand() == false )
				break;
			clearMotionMonitor();
			// measurement handling will be done by the corespondinf file parser
			ret = processSVGTemplate();
			break;
			
		case TplGcode:
			if ( saveTemplateOnDemand() == false )
				break;
			clearMotionMonitor();
			// measurement handling will be done by the corespondinf file parser
			ret = processGCodeTemplate();
			break;
			
		case TplManual:
			ret = processManualTemplate();
			break;
			
		case TplTest:
			clearMotionMonitor();
			cnc->startSerialMeasurement();
			ret = processTestTemplate();
			cnc->stopSerialMeasurement();
			break;
			
		default:
			; // do nothing
	}
	
	// Check positions
	if ( cnc->validateAppAgainstCtlPosition() == false ) {
		if ( cnc->isInterrupted() == false ) {
			std::cerr << "Validate positions failed" << std::endl;
			std::cerr << "PC pos        : " << cnc->getCurAppPos() << std::endl;
			std::cerr << "Controller pos: " << cnc->requestControllerPos() << std::endl;
			setRefPostionState(false);
		}
		
		ret = false;
	}
	
	if ( cnc->getPositionOutOfRangeFlag() == true ) {
		//if ( cnc->isInterrupted() == false ) {
			std::cerr << "Out of range: During the last run the position limits were exceeded." << std::endl;
			CncLongPosition::Watermarks wm = cnc->getWaterMarks();
			CncLongPosition min(wm.xMin, wm.yMin, wm.zMin);
			CncLongPosition max(wm.xMax, wm.yMax, wm.zMax);
			
			cnc->isPositionOutOfRange(min, true);
			cnc->isPositionOutOfRange(max, true);
			
			setRefPostionState(false);
		//}
	}
	
	cnc->enableStepperMotors(ENABLE_STATE_OFF);
	
	motionMonitor->popProcessMode();
	
	if ( ret )
		cnc->updateDrawControl();
		
	logTimeConsumed();
	statisticsPane->logStatistics();
	displayReport(1);
		
	enableControls();
	stopAnimationControl();
	
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::logTimeConsumed() {
///////////////////////////////////////////////////////////////////
	THE_CONTEXT->timestamps.logTotalTimeEnd();
	wxString value;
	m_cmdDuration->ChangeValue(THE_CONTEXT->timestamps.getTotalTimeConsumedFormated(value));

	std::stringstream ss;
	ss << THE_CONTEXT->timestamps;
	m_cmdDuration->SetToolTip(ss.str().c_str());
}
///////////////////////////////////////////////////////////////////
void MainFrame::resetMinMaxPositions() {
///////////////////////////////////////////////////////////////////	
	if ( cnc == NULL )
		return;
		
	cnc->resetWatermarks();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::saveTemplateOnDemand() {
///////////////////////////////////////////////////////////////////
	wxString msg(wxString::Format("The current template file\n\n '%s'\n\nwas modified. Save it?", getCurrentTemplatePathFileName()));
	
	if ( sourceEditor->IsModified() == true ) {
		wxMessageDialog dlg(this, msg, _T("Save template . . . "), 
		                    wxYES|wxNO|wxCANCEL|wxICON_QUESTION|wxCENTRE);
 	
		int ret = dlg.ShowModal();
	
		if ( ret == wxID_YES ) {
			saveFile();
		} else if ( ret == wxID_CANCEL ) {
			return false;
		}
	}

	return true;
}
///////////////////////////////////////////////////////////////////
void MainFrame::prepareAndShowMonitorTemplatePreview(bool force) {
///////////////////////////////////////////////////////////////////
	if ( force == true )
		lastMonitorPreviewFileName.clear();
	
	// check if a preview update is necessary
	if ( lastMonitorPreviewFileName == getCurrentTemplatePathFileName() && sourceEditor->GetModify() == false)
		return;
		
	MF_PRINT_LOCATION_CTX_FILE
	
	lastMonitorPreviewFileName = getCurrentTemplatePathFileName();
	
	// wxString tfn(CncFileNameService::getCncTemplatePreviewFileName(getCurrentTemplateFormat()));
	// this causes file access errors between this app and the internet explorer
	// write a temp file instead to have anytime a new one
	wxString tfn(CncFileNameService::getTempFileName(getCurrentTemplateFormat()));
	
	TemplateFormat tf = getCurrentTemplateFormat();
	switch ( tf ) {
		case TplSvg:		tfn.append(".svg");
							break;
							
		case TplGcode:		tfn.append(".gcode");
							break;
							
		case TplBinary:		if ( BinaryFileParser::extractSourceContentAsFile(getCurrentTemplatePathFileName(), tfn) == true ) {
								openMonitorPreview(tfn);
								// no further copy necessay
								return;
								
							} else {
								std::cerr << CNC_LOG_FUNCT << ": Can't create preview!" << std::endl;
								return;
								
							}
							
		default:			// do nothing;
							break;
	}
	
	if ( tfn.IsEmpty() ) {
		std::cerr << CNC_LOG_FUNCT << ": Invalid file name!" << std::endl;
		return;
	}
	
	// create a copy to avoid a modification of sourceEditor
	wxTextFile file(tfn);
	if ( !file.Exists() )
		file.Create();
	
	if ( file.Open() ) {
		file.Clear();
		
		for (long i=0; i<sourceEditor->GetNumberOfLines(); i++) {
			wxString line = sourceEditor->GetLineText(i);
			file.AddLine(line);
		}
		
		file.Write();
		file.Close();
		
		openMonitorPreview(tfn);

	} else {
		std::cerr << "Error creating a temp file: " << tfn.c_str() << std::endl;
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::emergencyStop(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	std::cerr << "Emergency Stop detected" << std::endl;
	cnc->interrupt("Emergency Stop detected");
	setRefPostionState(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::navigateX(CncDirection d) {
///////////////////////////////////////////////////////////////////
	double fact = 1;
	bool shtKey = CncAsyncKeyboardState::isShiftPressed();
	bool ctlKey = CncAsyncKeyboardState::isControlPressed();
	bool altKey = CncAsyncKeyboardState::isAltPressed();

	if ( altKey ) {
		fact /= 100;
	} else {
		if 		( shtKey && !ctlKey ) fact *= 2;
		else if ( ctlKey && !shtKey ) fact /= 10;
		else if ( ctlKey &&  shtKey ) fact *= 10;
	}

	switch ( d ) {
		case CncClockwise: 	fact *= +1; break;
		case CncAnticlockwise: fact *= -1; break;
		default: ;
	}

	if ( cnc->isInterrupted() ) {
		std::cerr << "CNC controller is interrupted, nothing will be done."<< std::endl;
		return;
	}
	
	cnc->manualSimpleMoveMetric(fact, 0.0, 0.0);
}
///////////////////////////////////////////////////////////////////
void MainFrame::navigateY(CncDirection d) {
///////////////////////////////////////////////////////////////////
	double fact = 1;
	bool shtKey = CncAsyncKeyboardState::isShiftPressed();
	bool ctlKey = CncAsyncKeyboardState::isControlPressed();
	bool altKey = CncAsyncKeyboardState::isAltPressed();

	if ( altKey ) {
		fact /= 100;
	} else {
		if 		( shtKey && !ctlKey ) fact *= 2;
		else if ( ctlKey && !shtKey ) fact /= 10;
		else if ( ctlKey &&  shtKey ) fact *= 10;
	}

	switch ( d ) {
		case CncClockwise: 	fact *= +1; break;
		case CncAnticlockwise: fact *= -1; break;
		default: ;
	}
	
	if ( cnc->isInterrupted() ) {
		std::cerr << "CNC controller is interrupted, nothing will be done."<< std::endl;
		return;
	}
	
	cnc->manualSimpleMoveMetric(0.0, fact, 0.0);
}
///////////////////////////////////////////////////////////////////
void MainFrame::navigateZ(CncDirection d) {
///////////////////////////////////////////////////////////////////
	double fact = 1;
	bool shtKey = CncAsyncKeyboardState::isShiftPressed();
	bool ctlKey = CncAsyncKeyboardState::isControlPressed();
	bool altKey = CncAsyncKeyboardState::isAltPressed();

	if ( altKey ) {
		fact /= 100;
	} else {
		if 		( shtKey && !ctlKey ) fact *= 2;
		else if ( ctlKey && !shtKey ) fact /= 10;
		else if ( ctlKey &&  shtKey ) fact *= 10;
	}

	switch ( d ) {
		case CncClockwise: 	fact *= +1; break;
		case CncAnticlockwise: fact *= -1; break;
		default: ;
	}

	if ( cnc->isInterrupted() ) {
		std::cerr << "CNC controller is interrupted, nothing will be done."<< std::endl;
		return;
	}

	cnc->manualSimpleMoveMetric(0.0, 0.0, fact);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestVersion(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	std::stringstream ss;
	cnc->processCommand(CMD_PRINT_VERSION, ss);
	cnc::trc.logInfoMessage(ss);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestTimestamp(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	std::stringstream ss;
	cnc->processCommand(CMD_PRINT_TIMESTAMP, ss);
	cnc::trc.logInfoMessage(ss);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestHeartbeat(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	std::stringstream dummy;
	cnc->processCommand(CMD_HEARTBEAT, dummy);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestConfig(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->processCommand(CMD_PRINT_CONFIG, std::clog);
	m_outboundNotebook->SetSelection(OutboundSelection::VAL::SUMMARY_PANEL);
	m_notebookConfig->SetSelection(OutboundCfgSelection::VAL::CNC_CONFIG_PANEL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestControllerConfigFromButton(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_btRequestCtlConfig->Enable(false);
	requestConfig(event);
	m_btRequestCtlConfig->Enable(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestCurrentPos(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	CncLongPosition pos = cnc->requestControllerPos();
	std::stringstream ss;
	ss << "Current controller position: " << pos;
	cnc::trc.logInfoMessage(ss.str().c_str());
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestCurrentLimitState(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->evaluateLimitState();
	cnc::trc.logInfoMessage("Limit controles were updated . . .");
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestCurrentLimitStateIcon(wxMouseEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->evaluateLimitState();
	cnc::trc.logInfoMessage("Limit controles were updated . . .");
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestReset(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	requestReset();
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestReset() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	getLogger()->Clear();
	cnc->processSetter(PID_SEPARATOR, SEPARARTOR_RESET);
	cnc->setup(true);
	
	clearMotionMonitor();
	updateSetterList();
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestInterrupt(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->sendInterrupt();
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateFileContentPosition(long x, long y) {
///////////////////////////////////////////////////////////////////
	// try to select current  line as client id
	if ( motionMonitor != NULL ) {
		motionMonitor->setCurrentClientId(y);
		motionMonitor->Refresh();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestEnableStepperMotors(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->enableStepperMotors(m_miMotorEnableState->IsChecked() ? ENABLE_STATE_ON : ENABLE_STATE_OFF);
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveHome(wxCommandEvent& event) {
/*
a) X(min), Y(min), Z(Top)
b) X(max), Y(max), Z(Top)
c) X(mid), Y(mid), Z(Top)
d) X(mid), Y(mid), Z(mid)
*/
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);
	
	if ( isZeroReferenceValid == false ) {
		cnc::trc.logError("The current reference position isn't valid. Therefore, can't move home");
		return;
	}
	
	disableControls();
	selectMonitorBookCncPanel();

	wxString sel = m_homeDefintion->GetStringSelection();
	const char c = sel[0];
	switch (c) {
		case 'a':	cnc->moveZToMaxLimit();
					cnc->moveXToMinLimit();
					cnc->moveYToMinLimit();
					break;
		case 'b':	cnc->moveZToMaxLimit();
					cnc->moveXToMaxLimit();
					cnc->moveYToMaxLimit();
					break;
		case 'c':	cnc->moveZToMaxLimit();
					cnc->moveXToMid();
					cnc->moveYToMid();
					break;
		case 'd':	cnc->moveZToMid();
					cnc->moveXToMid();
					cnc->moveYToMid();
					break;
		default:	std::cerr << "MainFrame::moveHome: invalid selection!" << std::endl;
	}
	
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveXToMid(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);
	
	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveXToMid();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveYToMid(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);
	
	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveYToMid();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveZToMid(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);
	
	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveZToMid();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveToZeroXY(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	wxString sel = m_zeroMoveModeXY->GetStringSelection();
	char mode = sel[0];

	CncDimensions dim = CncDimension1D;
	switch ( mode ) {
		case '2':	dim = CncDimension2D; break;
	}

	disableControls();
	cnc->simpleMoveXYToZeroPos(dim);
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveToZeroXYZ(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	wxString sel = m_zeroMoveModeXYZ->GetStringSelection();
	char mode = sel[0];

	CncDimensions dim = CncDimension1D;
	switch ( mode ) {
		case '2':	dim = CncDimension2D; break;
		case '3':	dim = CncDimension3D; break;
	}

	disableControls();
	cnc->simpleMoveXYZToZeroPos(dim);
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveToZeroZ(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->simpleMoveZToZeroPos();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveXToMin(wxCommandEvent& event){
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveXToMinLimit();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveXToMax(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveXToMaxLimit();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveYToMin(wxCommandEvent& event){
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveYToMinLimit();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveYToMax(wxCommandEvent& event){
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveYToMaxLimit();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveZToMin(wxCommandEvent& event){
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveZToMinLimit();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveZToMax(wxCommandEvent& event){
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);
	
	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveZToMaxLimit();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveZToTop(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveZToTop();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::moveZToBottom(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT( cnc );
	CncTransactionLock ctl(this);

	selectMonitorBookCncPanel();
	
	disableControls();
	cnc->moveZToBottom();
	enableControls();
}
///////////////////////////////////////////////////////////////////
void MainFrame::openConfigurationFile(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString tool;
	CncConfig::getGlobalCncConfig()->getEditorTool(tool);
	openFileExtern(tool, CncFileNameService::getConfigFileName());
}
///////////////////////////////////////////////////////////////////
void MainFrame::openExternalEditor(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString tool;
	CncConfig::getGlobalCncConfig()->getEditorTool(tool);
	openFileExtern(tool, _(""));
}
///////////////////////////////////////////////////////////////////
void MainFrame::openPyCam(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString tool;
	CncConfig::getGlobalCncConfig()->getPyCamTool(tool);
	openFileExtern(tool, _(""));
}
///////////////////////////////////////////////////////////////////
void MainFrame::openCalculator(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxString cmd("calc");
	openFileExtern(cmd, _(""));
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableTestControls(bool state) {
///////////////////////////////////////////////////////////////////
	// handle interval mode
	if ( state == false ) {
		wxString sel = m_testIntervalMode->GetStringSelection();
		char mode = sel[0];

		m_testCountY->Enable(mode == 'A');
		m_testCountZ->Enable(mode == 'A');
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUAInboundPathList(wxDataViewEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_dvListCtrlSvgUAInboundPathList->HasSelection() ) {
		int sel = m_dvListCtrlSvgUAInboundPathList->GetSelectedRow();
		if ( inboundFileParser != NULL ) {
			m_dvListCtrlSvgUADetailInfo->DeleteAllItems();
			inboundFileParser->displayUserAgentDetailInfo(sel);
			m_dvListCtrlSvgUADetailInfo->Refresh();
			m_dvListCtrlSvgUADetailInfo->Update();
			
			wxVariant val;
			m_dvListCtrlSvgUAInboundPathList->GetValue(val, sel, 0);
		}
	}
	
	m_selectedUAInfo->SetValue("");
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUADetailInfo(wxDataViewEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_dvListCtrlSvgUADetailInfo->HasSelection() ) {
		int sel = m_dvListCtrlSvgUADetailInfo->GetSelectedRow();
		m_selectedUAInfo->SetValue(m_dvListCtrlSvgUADetailInfo->GetTextValue(sel, 1));
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUAUseDirectiveList(wxDataViewEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_dvListCtrlSvgUAUseDirective->HasSelection() ) {
		int sel = m_dvListCtrlSvgUAUseDirective->GetSelectedRow();
		m_selectedUAInfo->SetValue(m_dvListCtrlSvgUAUseDirective->GetTextValue(sel, 1));
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateRenderResolution() {
///////////////////////////////////////////////////////////////////
	THE_CONFIG->setRenderResolution(m_cbRenderResolution->GetStringSelection());
	updateCncConfigTrace();
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateRenderResolution(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateRenderResolution();
}
///////////////////////////////////////////////////////////////////
void MainFrame::openPreview(CncFilePreview* ctrl, const wxString& fn) {
///////////////////////////////////////////////////////////////////
	wxASSERT(ctrl);
	
	if      ( ctrl == mainFilePreview )			m_currentFileMangerPreviewFileName->ChangeValue(fn);
	else if ( ctrl == monitorFilePreview)		m_currentInboundFilePreviewFileName->ChangeValue(fn);
	
	if ( cncExtMainPreview != NULL ) {
		if      ( ctrl == mainFilePreview )		cncExtMainPreview->setStatusTextRight(wxFileName(fn).GetFullName());
		else if ( ctrl == monitorFilePreview)	cncExtMainPreview->setStatusTextRight(m_inputFileName->GetValue());
	}
	
	TemplateFormat tf = getTemplateFormat(fn);
	switch ( tf ) {
		case TplSvg:		ctrl->selectPreview(fn);
							break;
							
		case TplGcode:		ctrl->selectPreview(fn);
							break;
							
		case TplBinary:		{
								wxString tmpSourceFileName;
								if ( BinaryFileParser::extractSourceContentAsFile(fn, tmpSourceFileName) ) {
									openPreview(ctrl, tmpSourceFileName);
								}
								
								break;
							}
		
		default:			if ( fn.IsEmpty() == false )
								cnc::trc.logError(wxString::Format("Can't preview: '%s'", fn));
								
							fillFileDetails(false);
							return;
	}
	
	fillFileDetails(true, fn);
}
///////////////////////////////////////////////////////////////////
void MainFrame::openMainPreview(const wxString& fn) {
///////////////////////////////////////////////////////////////////
	MF_PRINT_LOCATION_CTX_FILE
	
	selectMainBookPreviewPanel();
	openPreview(mainFilePreview, fn);
}
///////////////////////////////////////////////////////////////////
void MainFrame::openMonitorPreview(const wxString& fn) {
///////////////////////////////////////////////////////////////////
	MF_PRINT_LOCATION_CTX_FILE

	selectMonitorBookTemplatePanel();
	openPreview(monitorFilePreview, fn);
}
///////////////////////////////////////////////////////////////////
void MainFrame::openFileFromFileManager(const wxString& f) {
///////////////////////////////////////////////////////////////////
	selectMainBookSourcePanel();
	selectMonitorBookTemplatePanel();
	
	if ( saveTemplateOnDemand() == false ) {
		prepareAndShowMonitorTemplatePreview();
		return;
	}

	wxFileName fn(f);
	m_inputFileName->SetValue(fn.GetFullName());
	m_inputFileName->SetHint(fn.GetFullPath());

	openFile();
	prepareAndShowMonitorTemplatePreview(sourceEditor->IsModified());
}
///////////////////////////////////////////////////////////////////
bool MainFrame::filePreviewListLeave() {
///////////////////////////////////////////////////////////////////
	if ( m_keepFileManagerPreview->IsChecked() )
		return false;
	
	if ( CncAsyncKeyboardState::isControlPressed() )
		return false;

	if ( cncExtMainPreview != NULL && cncExtMainPreview->IsShownOnScreen() == true ) {
		cncExtMainPreview->selectView(CncExternalViewBox::Preview::TEMPLATE);
		return false;
	}
	
	if ( m_mainViewBook->GetSelection() != MainBookSelection::VAL::PREVIEW_PANEL )
		return false;
		
	selectMainBookSourcePanel();
	return true;
}
///////////////////////////////////////////////////////////////////
void MainFrame::onChangePreviewMode(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	const bool isExtViewActive = m_externFileManagerPreview->IsChecked();
	
	//m_keepFileManagerPreview->SetValue( isExtViewActive);
	//m_keepFileManagerPreview->Enable  (!isExtViewActive);
		
	if ( cncExtMainPreview != NULL ) {
		
		// deselect main preview if active
		if ( m_mainViewBook->GetSelection() == MainBookSelection::VAL::PREVIEW_PANEL )
			selectMainBookSourcePanel();
		
		// deselect monitor preview if active
		if ( m_monitorViewBook->GetSelection() == MonitorBookSelection::VAL::TEMPLATE_PANEL )
			selectMonitorBookCncPanel();
		
		// prepare extern preview
		cncExtMainPreview->selectView(CncExternalViewBox::Preview::TEMPLATE);
		cncExtMainPreview->Show(isExtViewActive);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMainBookSourcePanel(int sourcePageToSelect) {
///////////////////////////////////////////////////////////////////
	m_mainViewSelector->SetSelection(MainBookSelection::VAL::SOURCE_PANEL);
	m_mainViewBook->SetSelection(MainBookSelection::VAL::SOURCE_PANEL);
	
	if ( sourcePageToSelect < 0 || sourcePageToSelect > (int)(m_templateNotebook->GetPageCount() - 1) )
		sourcePageToSelect = TemplateBookSelection::VAL::SOURCE_PANEL;
	
	m_templateNotebook->SetSelection(sourcePageToSelect);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMainBookPreviewPanel() {
///////////////////////////////////////////////////////////////////
	MF_PRINT_LOCATION_CTX_FILE
	
	if ( m_externFileManagerPreview->IsChecked() == false ) {
		m_mainViewSelector->SetSelection(MainBookSelection::VAL::PREVIEW_PANEL);
		m_mainViewBook->SetSelection(MainBookSelection::VAL::PREVIEW_PANEL);
		
	} else {
		
		cncExtMainPreview->selectView(CncExternalViewBox::Preview::FILE);
		if ( cncExtMainPreview->IsShownOnScreen() == false )
			cncExtMainPreview->Show();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMainBookSetupPanel() {
	m_mainViewSelector->SetSelection(MainBookSelection::VAL::SETUP_PANEL);
	m_mainViewBook->SetSelection(MainBookSelection::VAL::SETUP_PANEL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMainBookReferencePanel() {
	m_mainViewSelector->SetSelection(MainBookSelection::VAL::REFERENCE_PANEL);
	m_mainViewBook->SetSelection(MainBookSelection::VAL::REFERENCE_PANEL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMainBookManuelPanel() {
	m_mainViewSelector->SetSelection(MainBookSelection::VAL::MANUEL_PANEL);
	m_mainViewBook->SetSelection(MainBookSelection::VAL::MANUEL_PANEL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMainBookTestPanel() {
	m_mainViewSelector->SetSelection(MainBookSelection::VAL::TEST_PANEL);
	m_mainViewBook->SetSelection(MainBookSelection::VAL::TEST_PANEL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::mainViewSelectorSelected(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	MF_PRINT_LOCATION_CTX_FILE
	
	m_mainViewBook->SetSelection(m_mainViewSelector->GetSelection());
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMonitorBookCncPanel() {
///////////////////////////////////////////////////////////////////
	m_monitorViewSelector->SetSelection(MonitorBookSelection::VAL::CNC_PANEL);
	m_monitorViewBook->SetSelection(MonitorBookSelection::VAL::CNC_PANEL);
	
	m_outboundNotebook->SetSelection(OutboundSelection::VAL::MOTION_MONITOR_PANAL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectMonitorBookTemplatePanel() {
///////////////////////////////////////////////////////////////////
	MF_PRINT_LOCATION_CTX_FILE
	
	if ( m_externFileManagerPreview->IsChecked() == false ) {
		prepareAndShowMonitorTemplatePreview(sourceEditor->IsModified());
	
		m_monitorViewSelector->SetSelection(MonitorBookSelection::VAL::TEMPLATE_PANEL);
		m_monitorViewBook->SetSelection(MonitorBookSelection::VAL::TEMPLATE_PANEL);
	} else {
		
		selectMonitorBookCncPanel();
		
		cncExtMainPreview->selectView(CncExternalViewBox::Preview::TEMPLATE);
		if ( cncExtMainPreview->IsShownOnScreen() == false )
			cncExtMainPreview->Show();
		
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::monitorViewSelectorSelected(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_monitorViewSelector->GetSelection() == MonitorBookSelection::VAL::TEMPLATE_PANEL)
		prepareAndShowMonitorTemplatePreview(sourceEditor->IsModified());
		
	m_monitorViewBook->SetSelection(m_monitorViewSelector->GetSelection());
}
///////////////////////////////////////////////////////////////////
void MainFrame::toggleAuiPane(wxWindow* pane, wxMenuItem* menu, bool update) {
///////////////////////////////////////////////////////////////////
	if ( pane == NULL )
		return;
	
	if ( GetAuimgrMain()->GetPane(pane).IsShown() )	hideAuiPane(pane, menu);
	else											showAuiPane(pane, menu);
	
	if ( update )
		GetAuimgrMain()->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::showAuiPane(wxWindow* pane, wxMenuItem* menu, bool update) {
///////////////////////////////////////////////////////////////////
	if ( pane == NULL )
		return;
		
	GetAuimgrMain()->GetPane(pane).Show();
	
	if ( menu != NULL )
		menu->Check(true);
		
	if ( update )
		GetAuimgrMain()->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::hideAuiPane(wxWindow* pane, wxMenuItem* menu, bool update) {
///////////////////////////////////////////////////////////////////
	if ( pane == NULL )
		return;
		
	GetAuimgrMain()->GetPane(pane).Hide();
	GetAuimgrMain()->GetPane(pane).DestroyOnClose(false);
	pane->Close(true);
	
	if ( pane == getAUIPaneByName("SerialSpy") )
		enableSerialSpy(false);
	
	if ( menu != NULL )
		menu->Check(false);
		
	if ( update )
		GetAuimgrMain()->Update();
}
///////////////////////////////////////////////////////////////////
wxWindow* MainFrame::getAUIPaneByName(const wxString& name) {
///////////////////////////////////////////////////////////////////
	if      ( name == "Toolbar" ) 			return m_auibarMain;
	else if ( name == "SourceView")			return m_winMainView;
	else if ( name == "Logger")				return m_winLoggerView;
	else if ( name == "Outbound")			return m_winMonitorView;
	else if ( name == "TemplateManager")	return m_winFileView;
	else if ( name == "StatusBar")			return m_statusBar;
	else if ( name == "SerialSpy")			return m_serialSpyView;
	else if ( name == "UnitCalculator")		return m_svgUnitCalulator;
	else if ( name == "Debugger")			return m_debuggerView;
	else if ( name == "PositionMonitor")	return m_positionMonitorView;
	else if ( name == "ZView")				return m_panelZView;
	else if ( name == "SecureRunPanel")		return m_secureRunPanel;
	
	return NULL;
}
///////////////////////////////////////////////////////////////////
wxMenuItem* MainFrame::getAUIMenuByName(const wxString& name) {
///////////////////////////////////////////////////////////////////
	if      ( name == "Toolbar" ) 			return m_miToolbar;
	else if ( name == "StatusBar")			return m_miViewStatusbar;
	else if ( name == "SourceView")			return m_miViewMainView;
	else if ( name == "Logger")				return m_miViewLogger;
	else if ( name == "Outbound")			return m_miViewMonitor;
	else if ( name == "TemplateManager")	return m_miViewTemplateManager;
	else if ( name == "SerialSpy")			return m_miViewSpy;
	else if ( name == "UnitCalculator")		return m_miViewUnitCalculator;
	else if ( name == "Debugger")			return m_miViewDebugger;
	else if ( name == "PositionMonitor")	return m_miViewPosMonitor;
	else if ( name == "ZView")				return m_miViewZAxis;
	
	return NULL;
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateViewMenu() {
///////////////////////////////////////////////////////////////////
	wxAuiPaneInfoArray panes = m_auimgrMain->GetAllPanes();
	for (unsigned int i = 0; i < panes.GetCount(); ++i) {
		wxMenuItem* mi = getAUIMenuByName(panes.Item(i).name);
		if ( mi )
			mi->Check(panes.Item(i).window->IsShown());
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::toggleAuiPane(const wxString& name, bool update) {
///////////////////////////////////////////////////////////////////
	wxWindow* w = getAUIPaneByName(name);
	if ( w == NULL ) {
		std::cerr << "MainFrame::toggleAuiPane: Invalid pane window name: " << name << std::endl;
		return;
	}
	wxMenuItem* m = getAUIMenuByName(name);
	if ( w == NULL ) {
		std::cerr << "MainFrame::toggleAuiPane: Invalid pane window name: " << name << std::endl;
		return;
	}
	
	toggleAuiPane(w, m, update);
}
///////////////////////////////////////////////////////////////////
void MainFrame::showAuiPane(const wxString& name, bool update) {
///////////////////////////////////////////////////////////////////
	wxWindow* w = getAUIPaneByName(name);
	if ( w == NULL ) {
		std::cerr << "MainFrame::toggleAuiPane: Invalid pane window name: " << name << std::endl;
		return;
	}
	wxMenuItem* m = getAUIMenuByName(name);
	if ( w == NULL ) {
		std::cerr << "MainFrame::toggleAuiPane: Invalid pane window name: " << name << std::endl;
		return;
	}
	
	showAuiPane(w, m, update);
}
///////////////////////////////////////////////////////////////////
void MainFrame::hideAuiPane(const wxString& name, bool update) {
///////////////////////////////////////////////////////////////////
	wxWindow* w = getAUIPaneByName(name);
	if ( w == NULL ) {
		std::cerr << "MainFrame::toggleAuiPane: Invalid pane window name: " << name << std::endl;
		return;
	}
	wxMenuItem* m = getAUIMenuByName(name);
	if ( w == NULL ) {
		std::cerr << "MainFrame::toggleAuiPane: Invalid pane window name: " << name << std::endl;
		return;
	}
	
	hideAuiPane(w, m, update);
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewToolbar(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane(m_auibarMain, m_miToolbar);
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewStatusbar(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("StatusBar");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewMainView(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("SourceView");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewTemplateManager(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("TemplateManager");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewLogger(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("Logger");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewMonitor(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("Outbound");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewZAxis(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("ZView");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewSpy(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("SerialSpy");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewUnitCalculator(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("UnitCalculator");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewDebugger(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("Debugger");
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewPosistionMonitor(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	toggleAuiPane("PositionMonitor");
}
///////////////////////////////////////////////////////////////////
void MainFrame::closeAuiPane(wxAuiManagerEvent& evt) {
///////////////////////////////////////////////////////////////////
	wxMenuItem* menu = getAUIMenuByName(evt.pane->name);
	if ( menu != NULL )
		menu->Check(!evt.pane->window->IsShown());

	if ( evt.pane->window == m_serialSpyView )
		enableSerialSpy(false);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onPerspectiveTimer(wxTimerEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( perspectiveTimer->IsRunning() == true )
		perspectiveTimer->Stop();
		
	if ( m_miToolbar->IsChecked() )
		showAuiPane("Toolbar");
		
	if ( m_miViewStatusbar->IsChecked() )
		showAuiPane("StatusBar");
		
	//GetAuimgrMain()->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::maximizeAuiPane(wxAuiManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	// always do this
	perspectiveHandler.logCurrentPerspective();
	
	if ( event.pane->window == m_winMonitorView || event.pane->window == m_winMainView ) {
		perspectiveTimer->Start(20);
	}

	event.Skip(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::activateAuiPane(wxAuiManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	event.Skip(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::buttonAuiPane(wxAuiManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	enum { 
		AUI_CLOSE_BTN_ID 	= 101,
		AUI_MIN_MAX_BTN_ID 	= 102,
		AUI_PIN_BTN_ID 		= 104
	};
	
	event.Skip(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::restoreAuiPane(wxAuiManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	// prevent the standard handling 
	// the restore handling will be done be the code below
	event.Veto(true);
	
	perspectiveHandler.restoreLoggedPerspective();
}
///////////////////////////////////////////////////////////////////
void MainFrame::renderAuiPane(wxAuiManagerEvent& event) {
///////////////////////////////////////////////////////////////////
	// do nothing else!
	event.Skip(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::hideAllAuiPanes(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	hideAllAuiPanes();
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewAllAuiPanes(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	viewAllAuiPanes();
}
///////////////////////////////////////////////////////////////////
void MainFrame::hideAllAuiPanes(bool update) {
///////////////////////////////////////////////////////////////////
	wxAuiPaneInfoArray panes = m_auimgrMain->GetAllPanes();
	for (unsigned int i = 0; i < panes.GetCount(); ++i)
		hideAuiPane( panes.Item(i).name, false);
		
	if ( update == true )
		GetAuimgrMain()->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::viewAllAuiPanes(bool withSpy) {
///////////////////////////////////////////////////////////////////
	wxAuiPaneInfoArray panes = m_auimgrMain->GetAllPanes();
	for (unsigned int i = 0; i < panes.GetCount(); ++i) {
		const wxString name(panes.Item(i).name);
		if ( name != "SecureRunPanel" )
			showAuiPane(name, false);
	}
	
	GetAuimgrMain()->Update();

	if ( withSpy )
		showAuiPane(m_serialSpyView, m_miViewSpy);
}
///////////////////////////////////////////////////////////////////
void MainFrame::disableSlider(wxMouseEvent& event) {
///////////////////////////////////////////////////////////////////
	event.Skip(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::outboundBookChanged(wxNotebookEvent& event) {
///////////////////////////////////////////////////////////////////
	unsigned int sel = event.GetSelection();
	
	if ( (wxWindow*)event.GetEventObject() == m_outboundNotebook ) {
		switch ( sel ) {
			case OutboundSelection::VAL::MOTION_MONITOR_PANAL:
			{
				if ( cnc )
					cnc->updatePreview3D();
					
				break;
			}
			case OutboundSelection::VAL::PREPOCESSOR_PANAL:
			{
				cncPreprocessor->updateContent();
				break;
			}

		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::changeMonitorListBook(wxListbookEvent& event) {
///////////////////////////////////////////////////////////////////
	unsigned int sel = event.GetSelection();
	
	if ( (wxWindow*)event.GetEventObject() == m_listbookMonitor ) {
		switch ( sel ) {
			
			case OutboundMonitorSelection::VAL::CNC_SETTER_PANEL:
			{
				updateSetterList();
				break;
			}
			
			case OutboundMonitorSelection::VAL::MOTION_VERTEX_TRACE:
			{
				startAnimationControl();
				motionMonitor->initVertexListCtr();
				stopAnimationControl();
				break;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::outboundBookChanging(wxNotebookEvent& event) {
///////////////////////////////////////////////////////////////////
	// currently nothing to do
	// unsigned int sel = event.GetSelection();
	// if ( (wxWindow*)event.GetEventObject() == m_outboundNotebook ) {
	// }
}
///////////////////////////////////////////////////////////////////
void MainFrame::displayUserAgent(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateMonitoring();
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateToolControls(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	updateMonitoring();
}

///////////////////////////////////////////////////////////////////
void MainFrame::updateStepDelay() {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->setStepDelay(m_stepDelay->GetValue());
	
	wxString val;
	val << m_stepDelay->GetValue();
	val << " ms";
	m_stepDelayValue->SetLabel(val);
}
///////////////////////////////////////////////////////////////////
void MainFrame::stepDelayChanged(wxScrollEvent& event) {
///////////////////////////////////////////////////////////////////
	m_miCfgCustom->Check();
	updateStepDelay();
}
///////////////////////////////////////////////////////////////////
void MainFrame::stepDelayThumbtrack(wxScrollEvent& event) {
///////////////////////////////////////////////////////////////////
	m_miCfgCustom->Check();
	updateStepDelay();
}
///////////////////////////////////////////////////////////////////
void MainFrame::createAnimationControl() {
///////////////////////////////////////////////////////////////////
	if ( pngAnimation != NULL )
		delete pngAnimation;
		
	pngAnimation = NULL;
	
	wxBitmap bmp = ImageLib16().Bitmap("ANIMATION_SPRITE");
	if ( bmp.IsOk() )
		pngAnimation = new wxPNGAnimation(m_activityPanel, bmp, wxHORIZONTAL, wxSize(80, 7), 0);
}
///////////////////////////////////////////////////////////////////
void MainFrame::startAnimationControl() {
///////////////////////////////////////////////////////////////////
	if ( pngAnimation != NULL ) {
		if ( pngAnimation->IsRunning() )
			pngAnimation->Stop();
			
		pngAnimation->Start(50);
		pngAnimation->Update();
	}
	
	wxColor color(255,128,128);
	m_cmdDuration->SetForegroundColour(color);
	m_cmdDuration->Refresh();
	
	THE_CONTEXT->timestamps.logTotalTimeStart();
}
///////////////////////////////////////////////////////////////////
void MainFrame::stopAnimationControl() {
///////////////////////////////////////////////////////////////////
	m_cmdDuration->SetForegroundColour(*wxBLACK);
	m_cmdDuration->Refresh();
	
	if ( pngAnimation != NULL ) {
		if ( pngAnimation->IsRunning() ) {
			pngAnimation->Stop();
			pngAnimation->Update();

			THE_CONTEXT->timestamps.logTotalTimeEnd();
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::dclickDurationCount(wxMouseEvent& event) {
///////////////////////////////////////////////////////////////////
	const wxString msg(m_cmdDuration->GetToolTipText());
	if ( msg.IsEmpty() == false ) {
		
		CncLastProcessingTimestampSummary dlg (this);
		wxStringTokenizer tokenizer(msg, "\n");
		
		// remove first line
		if ( tokenizer.HasMoreTokens() )
			tokenizer.GetNextToken();
			
		// token rest
		while ( tokenizer.HasMoreTokens() ) {
			wxString token = tokenizer.GetNextToken();
			if ( token.IsEmpty() )
				continue;
				
			token.Replace("*", "", true);
			const wxString key(token.BeforeFirst(':').Trim().Trim(true));
			const wxString val(token.AfterFirst(':').Trim().Trim(true));
			
			dlg.addTimestamp(key, val);
		}
		
		dlg.ShowModal();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateOutboundSaveControls(bool state) { 
///////////////////////////////////////////////////////////////////
	m_miSaveEmuOutput->Enable(state);
	m_btSaveOutboundAsTemplate1->Enable(state);
	m_btSaveOutboundAsTemplate2->Enable(state);
	
	if ( state == true ) {
		wxString outboundFile(cnc->getPortName());
		decorateOutboundEditor(outboundFile);
		
	} else {
		decorateOutboundEditor();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateSearchButton() {
///////////////////////////////////////////////////////////////////
	wxBitmap bmp = ImageLib16().Bitmap("BMP_OK16");
	if ( bmp.IsOk() )
		m_sourceEditSearchState->SetBitmap(bmp);
}
///////////////////////////////////////////////////////////////////
int MainFrame::getSvgEditSearchFlags() {
///////////////////////////////////////////////////////////////////
	int ret = 0;
	//wxSTC_FIND_WORDSTART
	if ( m_tbCaseSensitive->GetValue() == true ) ret |= wxSTC_FIND_MATCHCASE;
	if ( m_tbWholeWord->GetValue() == true ) ret |= wxSTC_FIND_WHOLEWORD;
	if ( m_tbRegEx->GetValue() == true ) ret |= wxSTC_FIND_REGEXP;
	
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::updateSvgSearchControl() {
///////////////////////////////////////////////////////////////////
	wxString find = m_sourceEditSearch->GetValue();
	
	int pos = m_sourceEditSearch->FindString(find);
	if ( pos >= 0 ) {
		m_sourceEditSearch->Delete((unsigned int)pos);
	}
	
	m_sourceEditSearch->Insert(find, 0);
	m_sourceEditSearch->ChangeValue(find);
}
///////////////////////////////////////////////////////////////////
void MainFrame::svgEditFind(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_sourceEditSearch->GetValue().IsEmpty() )
		return;

	wxString find = m_sourceEditSearch->GetValue();
	fixRegexParen(find);
	
	updateSvgSearchControl();
	
	sourceEditor->SetCurrentPos(sourceEditor->GetCurrentPos() + m_sourceEditSearch->GetValue().Len() + 1);
	sourceEditor->SearchAnchor();
	int ret = sourceEditor->SearchNext(getSvgEditSearchFlags(), find);
	
	if ( ret == wxNOT_FOUND ) {
		sourceEditor->ClearSelections();
		sourceEditor->SearchAnchor();
	} else {
		ensureSvgEditLineIsVisible(ret);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::svgEditFindPrev(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_sourceEditSearch->GetValue().IsEmpty() )
		return;
		
	wxString find = m_sourceEditSearch->GetValue();
	fixRegexParen(find);

	updateSvgSearchControl();
	
	int ncp = sourceEditor->GetCurrentPos() - m_sourceEditSearch->GetValue().Len() - 1;
	if( ncp < 0 )
		ncp = sourceEditor->GetLastPosition();
		
	sourceEditor->SetCurrentPos(ncp);
	sourceEditor->SearchAnchor();
	int ret = sourceEditor->SearchPrev(getSvgEditSearchFlags(), find);
	
	if ( ret == wxNOT_FOUND ) {
		sourceEditor->ClearSelections();
		sourceEditor->SearchAnchor();
	} else {
		ensureSvgEditLineIsVisible(ret);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::svgEditSearchTextChanged(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	searchConditionsChanged();
}
///////////////////////////////////////////////////////////////////
void MainFrame::searchConditionsChanged() {
///////////////////////////////////////////////////////////////////
	wxString find = m_sourceEditSearch->GetValue();
	wxString toolTip("");
	wxBitmap bmp = ImageLib16().Bitmap("BMP_OK16");
	m_sourceEditSearchState->SetBitmap(bmp);
	
	fixRegexParen(find);
	sourceEditor->SearchAnchor();
	int ret = sourceEditor->SearchNext(getSvgEditSearchFlags(), find);
	
	if( ret == wxNOT_FOUND ) {
		sourceEditor->ClearSelections();
		bmp = ImageLib16().Bitmap("BMP_WARNING16");
		m_sourceEditSearchState->SetBitmap(bmp);

		toolTip << "'";
		toolTip << find;
		toolTip << "' not found";
		
	} else {
		ensureSvgEditLineIsVisible(ret);
	}
	
	m_sourceEditSearchState->Show();
	m_sourceEditSearchState->Refresh();
	m_sourceEditSearchState->SetToolTip(toolTip);
	m_sourceEditStatus->SetLabel(toolTip);
}
///////////////////////////////////////////////////////////////////
wxString& MainFrame::fixRegexParen(wxString& find) {
///////////////////////////////////////////////////////////////////
	if( getSvgEditSearchFlags() & wxSTC_FIND_REGEXP) {
		// Scintilla's REGEX group markers are \( and \)
		// while wxRegEx is usig bare ( and ) and the escaped version for
		// non regex manner
		find.Replace("\\(", "/<!@#$");
		find.Replace("\\)", "/>!@#$");
		find.Replace("(", "<!@#$");
		find.Replace(")", ">!@#$");

		find.Replace("/<!@#$", "(");
		find.Replace("/>!@#$", ")");
		find.Replace("<!@#$", "\\(");
		find.Replace(">!@#$", "\\)");
	}
	
	return find;
}
///////////////////////////////////////////////////////////////////
void MainFrame::svgEditSelected(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	// does currently nothing
}
///////////////////////////////////////////////////////////////////
void MainFrame::ensureSvgEditLineIsVisible(int line) {
///////////////////////////////////////////////////////////////////
	if( line == wxNOT_FOUND ) {
		line = sourceEditor->LineFromPosition(sourceEditor->GetSelectionStart());
	}
	
	int linesOnScreen = sourceEditor->LinesOnScreen();
	if(!((line > sourceEditor->GetFirstVisibleLine()) && (line < (sourceEditor->GetFirstVisibleLine() + linesOnScreen)))) {
		// To place our line in the middle, the first visible line should be
		// the: line - (linesOnScreen / 2)
		int firstVisibleLine = line - (linesOnScreen / 2);
		if( firstVisibleLine < 0 ) {
			firstVisibleLine = 0;
		}
		sourceEditor->SetFirstVisibleLine(firstVisibleLine);
	}
	
	sourceEditor->EnsureVisible(line);
	sourceEditor->ScrollToColumn(0);
	
	int xScrollPosBefore = sourceEditor->GetScrollPos(wxHORIZONTAL);
	sourceEditor->EnsureCaretVisible();
	int xScrollPosAfter = sourceEditor->GetScrollPos(wxHORIZONTAL);
	
	if( xScrollPosBefore != xScrollPosAfter ) {
		// EnsureCaretVisible scrolled the page
		// scroll it a bit more
		int scrollToPos = sourceEditor->GetSelectionStart();
		if(	scrollToPos != wxNOT_FOUND	) {
			sourceEditor->ScrollToColumn(sourceEditor->GetColumn(scrollToPos));
		}
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::toogleSvgEditSearchFlag(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxWindow* wnd = (wxWindow*)event.GetEventObject();
	
	if ( wnd == NULL )
		return;
		
	bool state = ((wxBitmapToggleButton*)wnd)->GetValue();
	wxColor color(227,227,227);
	
	if ( state == true )
		color.Set(191,205,219);
		
	wnd->SetBackgroundColour(color);
	wnd->Update();
	wnd->Refresh();
	
	searchConditionsChanged();
}
///////////////////////////////////////////////////////////////////
void MainFrame::copyLogger(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	getLogger()->SelectAll();
	getLogger()->Copy();
	getLogger()->SelectNone();
}
///////////////////////////////////////////////////////////////////
bool MainFrame::openFileExtern(const wxString& tool, const char* file) {
///////////////////////////////////////////////////////////////////
	wxString f(file);
	return openFileExtern(tool, f);
}
///////////////////////////////////////////////////////////////////
bool MainFrame::openFileExtern(const wxString& tool, wxString& file) {
///////////////////////////////////////////////////////////////////
	startAnimationControl();
	
	wxString cmd(tool);
	if ( file.IsEmpty() == false ) {
		
		if ( wxFileName(file).Exists() == false ) {
			std::cerr << "MainFrame::openFileExtern: Failed:" << std::endl;
			std::cerr << " Can't open:" << file << std::endl;
			stopAnimationControl();
			return false;
		}
		
		cmd += " \"";
		cmd += file;
		cmd += "\"";
	}
	
	cnc::trc.logInfoMessage(wxString::Format("Open: %s", cmd));
	
	wxExecute(cmd);
	wxASSERT(cnc);
	waitActive(1500);

	stopAnimationControl();
	return true;
}
///////////////////////////////////////////////////////////////////
void MainFrame::toggleTemplateWordWrapMode(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	sourceEditor->SetWrapMode(!m_btSvgToggleWordWrap->GetValue());
}
///////////////////////////////////////////////////////////////////
void MainFrame::toggleOutboundEditorWordWrap(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	outboundEditor->SetWrapMode(!m_btToggleOutboundEditorWordWrap->GetValue());
}
///////////////////////////////////////////////////////////////////
const char* MainFrame::getCurrentPortName(wxString& ret) {
///////////////////////////////////////////////////////////////////
	ret.assign("Unknown Port Name");
	if ( cnc != NULL ) {
		wxString cn(cnc->getClassName());
		wxString pn(cnc->getPortName());
		
		ret.assign(wxString::Format("%s", cn));
		
		if ( pn.IsEmpty() == false ) {
			wxFileName fn(pn);
			if ( fn.HasExt() ) {
				pn.assign(fn.GetFullName());
				
			} else {
				pn.Replace("\\","",true);
				pn.Replace(".","",true);
				
			}
			
			ret.append(wxString::Format("::%s", pn));
		}
	}
	
	return ret;
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateRunButton() {
///////////////////////////////////////////////////////////////////
	wxString toolTip;
	toolTip = isPause() ? "Resume " : "Run";
	m_rcRun->SetToolTip(toolTip);
}
///////////////////////////////////////////////////////////////////
void MainFrame::determineRunMode() {
///////////////////////////////////////////////////////////////////
	if ( isDebugMode ) {
		m_svgParseMode->SetLabel("Debug");
		m_svgParseMode->SetForegroundColour(wxColor(255,128,128));
	} else {
		m_svgParseMode->SetLabel("Release");
		m_svgParseMode->SetForegroundColour(wxColor(0,162,232));
	}
	
	m_svgParseMode->Refresh();

	decorateRunButton();
	
	if ( m_externFileManagerPreview->IsChecked() == true && cncExtMainPreview != NULL )
		cncExtMainPreview->selectView(CncExternalViewBox::Preview::TEMPLATE);
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcDebugConfig(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	perspectiveHandler.ensureDebugPerspectiveMinimal();
	
	if ( m_debuggerPropertyManagerGrid->GetPageCount() > 0 )
		m_debuggerPropertyManagerGrid->SelectPage(0);
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcRun() {
///////////////////////////////////////////////////////////////////
	determineRunMode();
	
	if ( THE_CONTEXT->secureModeInfo.useIt == true && m_secureRunPanel->IsShownOnScreen() == false ) {
		activateSecureMode(!THE_CONTEXT->secureModeInfo.isActive);
		return;
	}

	// ensure the monitor is visible, especially if isPause == true
	// because then the processing should be resume
	if ( THE_CONTEXT->secureModeInfo.isActive == false ) {
		showAuiPane("Outbound");
		selectMonitorBookCncPanel();
	}

	// toggle only the pause flag
	if ( isPause() == true ) {
		rcPause();
		return;
	}
	
	// perform a run
	// Store the current interval
	int interval = THE_CONTEXT->getUpdateInterval();
	
	if ( isDebugMode == true ) {
		
		// check if the cuurent port is a cnc and no emulator port
		if ( cnc->getPortType() == CncPORT ) {
			
			wxString msg("Do you really want to debug a COM port?");
			wxMessageDialog dlg(this, msg, _T("Port Check . . . "), 
			                    wxOK|wxCANCEL|wxCENTRE|wxICON_QUESTION);
			
			if ( dlg.ShowModal() == wxID_CANCEL )
				return;
		}
		
		// bring the debug controls in front
		perspectiveHandler.ensureDebugPerspectiveMinimal();
		
		// to see each line during the debug session
		THE_CONTEXT->setUpdateInterval(1);
	} else {
		
		if ( THE_CONTEXT->secureModeInfo.isActive == false ) 
			perspectiveHandler.ensureRunPerspectiveMinimal();
	}
	
	// process
	processTemplateWrapper();
	
	// restore the interval
	THE_CONTEXT->setUpdateInterval(interval);
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcDebug(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	isDebugMode = true;
	rcRun();
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcRun(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	isDebugMode = false;
	rcRun();
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcPause() {
///////////////////////////////////////////////////////////////////
	if ( isPause() )	cnc->sendResume();
	else				cnc->sendPause();
	
	if ( inboundFileParser != NULL ) {
		inboundFileParser->togglePause();
		enableRunControls(inboundFileParser->isPause());
	}
	
	decorateRunButton();
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcPause(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	rcPause();
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcNextBreakpoint(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( inboundFileParser != NULL )
		inboundFileParser->debugNextBreakPoint();
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcNextStep(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( inboundFileParser != NULL )
		inboundFileParser->debugNextStep();
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcFinish(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( inboundFileParser != NULL )
		inboundFileParser->debugFinish();
		
	m_rcNextBreakpoint->Enable(false);
	m_rcNextStep->Enable(false);
	m_rcFinish->Enable(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcStop(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	// toggle only the pause flag
	if ( isPause() == true )
		rcPause(event);
	
	if ( inboundFileParser != NULL )
		inboundFileParser->debugStop();

	cnc->sendHalt();
	cnc::trc.logInfo("Current session is stopped");
}
///////////////////////////////////////////////////////////////////
void MainFrame::rcReset(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	requestReset();
	setRefPostionState(false);
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateSwitchToolOnOff(bool state) {
///////////////////////////////////////////////////////////////////
	m_testToolPowerBtn->SetLabel            (state == TOOL_STATE_OFF ? "Switch Tool 'On'"        : "Switch Tool 'Off'");

	m_testToolPowerState->SetLabel           (state == TOOL_STATE_OFF ? "Tool is switched 'Off'" : "Tool is switched 'On'");
	m_testToolPowerState->SetBackgroundColour(state == TOOL_STATE_OFF ? wxColour(255,128,128)    : *wxGREEN);
	m_testToolPowerState->SetForegroundColour(state == TOOL_STATE_OFF ? *wxWHITE                 : *wxBLACK);
	
	m_testToolPowerState->Refresh(true);
	m_testToolPowerState->Update();
	
	if ( m_testToolPowerState->GetParent() ) {
		m_testToolPowerState->GetParent()->SetBackgroundColour(state == TOOL_STATE_OFF ? wxColour(255,128,128)    : *wxGREEN);
		m_testToolPowerState->GetParent()->Refresh(true);
		m_testToolPowerState->GetParent()->Update();
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::testSwitchToolOnOff(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	if ( cnc->isConnected() == false ) {
		std::cerr << "Not connected, nothing will be processed." << std::endl;
		return;
	}
	
	bool cncToolState = cnc->getToolState();
	
	if ( cncToolState == TOOL_STATE_OFF ) {
		
		const wxString hdl("Switch tool on . . . ");
		const wxString msg("Do you really want to switch the tool power on?");
		wxMessageDialog dlg(this, msg, hdl, wxYES | wxNO | wxICON_QUESTION | wxCENTRE);
		
		if ( dlg.ShowModal() == wxID_NO ) 
			return;
	}
	
	if ( cncToolState == TOOL_STATE_OFF ) {
		cnc->switchToolOn();
		startAnimationControl();
		
	} else {
		cnc->switchToolOff();
		stopAnimationControl();
	}
	
	cncToolState = cnc->getToolState();
	enableControls(cncToolState == TOOL_STATE_OFF);
	
	if ( m_testToolPowerBtn->IsShownOnScreen() )
		m_testToolPowerBtn->Enable(true);
		
	updateSetterList();
}
///////////////////////////////////////////////////////////////////
void MainFrame::testEndSwitchEvaluation(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	if ( cnc->isConnected() == false ) {
		std::cerr << "Not connetced, nothing will be processed." << std::endl;
		return;
	}
	
	if ( m_testToggleEndSwitch->GetValue() == true ) {
		m_testToggleEndSwitch->SetLabel("Stop End Switch Evaluation");
		m_testToggleEndSwitch->SetBackgroundColour(*wxRED);
		m_testToggleEndSwitch->SetForegroundColour(*wxWHITE);
		
		disableControls();
		m_testToggleEndSwitch->Enable(true);
		
		startAnimationControl();
		while ( m_testToggleEndSwitch->GetValue() == true ) {
			dispatchAll();
			
			if ( cnc->isInterrupted() ) {
				m_testToggleEndSwitch->SetValue(false);
				break;
			}
			
			cnc->evaluateLimitState();
		}
		
		enableControls();
		stopAnimationControl();
		
	} 
	
	if ( m_testToggleEndSwitch->GetValue() == false ) {
		m_testToggleEndSwitch->SetLabel("Start End Switch Evaluation");
		m_testToggleEndSwitch->SetBackgroundColour(*wxGREEN);
		m_testToggleEndSwitch->SetForegroundColour(*wxBLACK);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::activate3DPerspectiveButton(wxButton* bt) {
///////////////////////////////////////////////////////////////////
	static wxColour active(171, 171, 171);
	static wxColour inactive(240, 240, 240);

	m_3D_Top->SetBackgroundColour(inactive);
	m_3D_Bottom->SetBackgroundColour(inactive);
	m_3D_Front->SetBackgroundColour(inactive);
	m_3D_Rear->SetBackgroundColour(inactive);
	m_3D_Left->SetBackgroundColour(inactive);
	m_3D_Right->SetBackgroundColour(inactive);
	m_3D_Perspective1->SetBackgroundColour(inactive);
	m_3D_Perspective2->SetBackgroundColour(inactive);
	m_3D_Perspective3->SetBackgroundColour(inactive);
	m_3D_Perspective4->SetBackgroundColour(inactive);

	m_3D_TopSec->SetBackgroundColour(inactive);
	m_3D_BottomSec->SetBackgroundColour(inactive);
	m_3D_FrontSec->SetBackgroundColour(inactive);
	m_3D_RearSec->SetBackgroundColour(inactive);
	m_3D_LeftSec->SetBackgroundColour(inactive);
	m_3D_RightSec->SetBackgroundColour(inactive);
	m_3D_Perspective1Sec->SetBackgroundColour(inactive);
	m_3D_Perspective2Sec->SetBackgroundColour(inactive);
	m_3D_Perspective3Sec->SetBackgroundColour(inactive);
	m_3D_Perspective4Sec->SetBackgroundColour(inactive);
	
	if ( bt != NULL ) {
		bt->SetBackgroundColour(active);
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::showFromFront3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	activate3DPerspectiveButton((wxButton*)event.GetEventObject());
	motionMonitor->viewFront();
}
///////////////////////////////////////////////////////////////////
void MainFrame::showFromRear3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	activate3DPerspectiveButton((wxButton*)event.GetEventObject());
	motionMonitor->viewRear();
}
///////////////////////////////////////////////////////////////////
void MainFrame::showFromTop3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	activate3DPerspectiveButton((wxButton*)event.GetEventObject());
	motionMonitor->viewTop();
}
///////////////////////////////////////////////////////////////////
void MainFrame::showFromBottom3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	activate3DPerspectiveButton((wxButton*)event.GetEventObject());
	motionMonitor->viewBottom();
}
///////////////////////////////////////////////////////////////////
void MainFrame::showFromLeft3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	activate3DPerspectiveButton((wxButton*)event.GetEventObject());
	motionMonitor->viewLeft();
}
///////////////////////////////////////////////////////////////////
void MainFrame::showFromRight3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	activate3DPerspectiveButton((wxButton*)event.GetEventObject());
	motionMonitor->viewRight();
}
///////////////////////////////////////////////////////////////////
void MainFrame::show3D(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxButton* bt = (wxButton*)event.GetEventObject();
	
	if ( bt == m_3D_Perspective1 || bt == m_3D_Perspective1Sec ) {
		activate3DPerspectiveButton(bt);
		motionMonitor->viewIso1();
		
	} else if ( bt == m_3D_Perspective2 || bt == m_3D_Perspective2Sec ) {
		activate3DPerspectiveButton(bt);
		motionMonitor->viewIso2();
		
	} else if ( bt == m_3D_Perspective3 || bt == m_3D_Perspective3Sec ) {
		activate3DPerspectiveButton(bt);
		motionMonitor->viewIso3();
		
	} else if ( bt == m_3D_Perspective4 || bt == m_3D_Perspective4Sec ) {
		activate3DPerspectiveButton(bt);
		motionMonitor->viewIso4();
		
	} 
}
///////////////////////////////////////////////////////////////////
void MainFrame::clearMotionMonitor() {
///////////////////////////////////////////////////////////////////
	if ( cnc )
		cnc->resetClientId();

	motionMonitor->clear();
	statisticsPane->clear();
	parsingSynopisis->clear();
	cncPreprocessor->clearAll();
	
	decorateOutboundEditor();
}
///////////////////////////////////////////////////////////////////
void MainFrame::clearMotionMonitor(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	clearMotionMonitor();
}
///////////////////////////////////////////////////////////////////
void MainFrame::refreshMotionMonitor(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( cnc )
		cnc->updatePreview3D();
}
///////////////////////////////////////////////////////////////////
void MainFrame::testCaseBookChanged(wxListbookEvent& event) {
///////////////////////////////////////////////////////////////////
	unsigned int sel = event.GetSelection();
	
	switch ( sel ) {
		case TestBookSelection::VAL::INTERVAL:
		case TestBookSelection::VAL::DIMENSION:
		case TestBookSelection::VAL::LIMIT:		break;
		
		case TestBookSelection::VAL::TOOL:		if ( cnc != NULL )
													decorateSwitchToolOnOff(cnc->getToolState());
													
												break;
												
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::traceTextUpdated(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	traceTimerCounter = 0;
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestControllerPinsFromButton(wxCommandEvent& event) {
	m_btRequestControllerPins->Enable(false);
	requestPins(event);
	m_btRequestControllerPins->Enable(true);
}
///////////////////////////////////////////////////////////////////
void MainFrame::requestPins(wxCommandEvent& event) {
	wxASSERT(cnc);
	cnc->processCommand(CMD_PRINT_PIN_REPORT, std::clog);
	m_outboundNotebook->SetSelection(OutboundSelection::VAL::SUMMARY_PANEL);
	m_notebookConfig->SetSelection(OutboundCfgSelection::VAL::CNC_PIN_PANEL);
}
///////////////////////////////////////////////////////////////////
void MainFrame::clearControllerMsgHistory(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	getCtrlMessageHistory()->Clear();
}
///////////////////////////////////////////////////////////////////
void MainFrame::clearSerialSpy() {
///////////////////////////////////////////////////////////////////
	wxASSERT(serialSpyListCtrl);
	serialSpyListCtrl->clear();
	serialSpyListCtrl->clearDetails();
}
///////////////////////////////////////////////////////////////////
void MainFrame::clearSerialSpy(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	clearSerialSpy();
}
///////////////////////////////////////////////////////////////////
void MainFrame::markSerialSpy(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	wxASSERT(serialSpyListCtrl);
	wxString defaultValue(wxString::Format("Marker::%06d", serialSpyListCtrl->getItemCount()));
	wxTextEntryDialog dlg(this, "Marker Label:", "Add Spy Marker . . .", defaultValue);
	dlg.SetMaxLength(64);
	
	if ( dlg.ShowModal() == wxID_OK )
		serialSpyListCtrl->addLine(dlg.GetValue(), CncSerialSpyListCtrl::LineType::LT_Marker);
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableSerialSpy(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_menuItemDebugSerial->Check(!m_menuItemDebugSerial->IsChecked());
	updateMonitoring();
	decorateSerialSpy();
}
///////////////////////////////////////////////////////////////////
void MainFrame::enableSerialSpy(bool state) {
///////////////////////////////////////////////////////////////////
	m_menuItemDebugSerial->Check(state);
	updateMonitoring();
	decorateSerialSpy();
}
///////////////////////////////////////////////////////////////////
void MainFrame::decorateSerialSpy() {
///////////////////////////////////////////////////////////////////
	if ( m_menuItemDebugSerial->IsChecked() ) {
		m_enableSerialSpy->SetBitmap(ImageLib16().Bitmap("BMP_CONNECTED")); 
		m_enableSerialSpy->SetToolTip("Disable Serial Spy");
		cnc::spy.enableMessage(); 
	} else {
		m_enableSerialSpy->SetBitmap(ImageLib16().Bitmap("BMP_DISCONNECTED")); 
		m_enableSerialSpy->SetToolTip("Enable Serial Spy");
		cnc::spy.disableMessage();
	}
	
	m_enableSerialSpy->Refresh();
	m_enableSerialSpy->Update();
}
///////////////////////////////////////////////////////////////////
void MainFrame::cfgStepDelayArduino(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_stepDelay->SetValue(int(m_stepDelay->GetMax() * 0.4));
	updateStepDelay();
}
///////////////////////////////////////////////////////////////////
void MainFrame::cfgStepDelayMax(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_stepDelay->SetValue(m_stepDelay->GetMax());
	updateStepDelay();
}
///////////////////////////////////////////////////////////////////
void MainFrame::cfgStepDelayMin(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_stepDelay->SetValue(0);
	updateStepDelay();
}
///////////////////////////////////////////////////////////////////
void MainFrame::cfgStepDelayDropDown(wxAuiToolBarEvent& event) {
///////////////////////////////////////////////////////////////////
	if ( m_miCfgSimulateArduino->IsChecked() )
		cfgStepDelayArduino(event);
	
	if ( m_miCfgStepDelayMin->IsChecked() )
		cfgStepDelayMin(event);
		
	if ( m_miCfgStepDelayMax->IsChecked() )
		cfgStepDelayMax(event);

	event.Skip();
}
///////////////////////////////////////////////////////////////////
void MainFrame::paintDrawPaneWindow(wxPaintEvent& event) {
///////////////////////////////////////////////////////////////////
	// do nothing
}
///////////////////////////////////////////////////////////////////
void MainFrame::displayPGenErrorInfo(const wxString& errorInfo) {
///////////////////////////////////////////////////////////////////
	wxMessageDialog dlg(this, "Path generation failed!", "Path Generator Error Message", wxOK|wxICON_ERROR);
 	dlg.SetExtendedMessage(errorInfo);
	dlg.ShowModal();
}
///////////////////////////////////////////////////////////////////
void MainFrame::openCurrentTemplateInBrowser() {
///////////////////////////////////////////////////////////////////
	wxString ret;
	openFileExtern(THE_CONFIG->getBrowser(ret), getCurrentTemplatePathFileName());
}
///////////////////////////////////////////////////////////////////
void MainFrame::closeUnitCalculator(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	hideAuiPane("UnitCalculator");
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUCUnitFrom(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectUCChangeFrom(event);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUCUnitTo(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectUCChangeFrom(event);
}
///////////////////////////////////////////////////////////////////
void MainFrame::selectUCChangeFrom(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	typedef CncUnitCalculatorBase::Unit Unit;
	
	Unit from = Unit::px;
	Unit to   = Unit::px;
	
	if ( CncUnitCalculatorBase::determineUnit(m_cbUCUnitFrom->GetStringSelection(), from) == false )
		return;
	
	if ( CncUnitCalculatorBase::determineUnit(m_cbUCUnitTo->GetStringSelection(),     to) == false )
		return;

	wxString line	= m_cbUCValueFrom->GetValue();
	
	m_cbUCValueTo->Clear();
	
	if ( line.IsEmpty() == true )
		return;
	
	wxStringTokenizer tokenizer(line, " \t,");
	while ( tokenizer.HasMoreTokens() ) {
		wxString token = tokenizer.GetNextToken();
		if ( token.IsEmpty() )
			continue;
		
		double vFrom, vTo;
		token.ToDouble(&vFrom);
		
		CncUnitCalculator<double> uc(from, to);
		vTo = uc.convert(vFrom);
		
		m_cbUCValueTo->AppendText(wxString::Format("%.3lf", vTo));
		m_cbUCValueTo->AppendText(wxString(tokenizer.GetLastDelimiter()));
	}
}
///////////////////////////////////////////////////////////////////
void MainFrame::unitTestFramework(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	UnitTests test(this, 0, true);
	test.ShowModal();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectManuallyMove(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMainBookManuelPanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectReferences(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMainBookReferencePanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectSetup(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMainBookSetupPanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectTemplate(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMainBookSourcePanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectInboundPreview(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMainBookPreviewPanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectTestPage(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMainBookTestPanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectCncMonitor(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMonitorBookCncPanel();
}
///////////////////////////////////////////////////////////////////
void MainFrame::onSelectTemplatePreview(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	selectMonitorBookTemplatePanel();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onDebugUserNotificationTimer(wxTimerEvent& event) {
/////////////////////////////////////////////////////////////////////
	const wxColour c1(227, 227, 227);
	const wxColour c2(255, 201, 14);
	
	if ( inboundFileParser ) {
		if ( inboundFileParser->isWaitingForUserEvents() == false ) {
			if ( m_rcNextBreakpoint->GetBackgroundColour() != c1 )	m_rcNextBreakpoint->SetBackgroundColour(c1);
			if ( m_rcNextStep->GetBackgroundColour() != c1 )		m_rcNextStep->SetBackgroundColour(c1);
			if ( m_rcFinish->GetBackgroundColour() != c1 )			m_rcFinish->SetBackgroundColour(c1);
			
			return;
		}
	}
	
	if ( m_rcNextBreakpoint->GetBackgroundColour() == c1 )	m_rcNextBreakpoint->SetBackgroundColour(c2);
	else 													m_rcNextBreakpoint->SetBackgroundColour(c1);
	
	if ( m_rcNextStep->GetBackgroundColour() == c1 )		m_rcNextStep->SetBackgroundColour(c2);
	else 													m_rcNextStep->SetBackgroundColour(c1);
	
	if ( m_rcFinish->GetBackgroundColour() == c1 )			m_rcFinish->SetBackgroundColour(c2);
	else 													m_rcFinish->SetBackgroundColour(c1);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::startDebugUserNotification() {
/////////////////////////////////////////////////////////////////////
	const wxColour c(255, 201, 14);
	m_rcNextBreakpoint->SetBackgroundColour(c);
	m_rcNextStep->SetBackgroundColour(c);
	m_rcFinish->SetBackgroundColour(c);
	
	if ( debugUserNotificationTimer->IsRunning() == false )
		debugUserNotificationTimer->Start(1000);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::stopDebugUserNotification() {
/////////////////////////////////////////////////////////////////////
	const wxColour c(227, 227, 227);
	m_rcNextBreakpoint->SetBackgroundColour(c);
	m_rcNextStep->SetBackgroundColour(c);
	m_rcFinish->SetBackgroundColour(c);
	
	if ( debugUserNotificationTimer->IsRunning() == true )
		debugUserNotificationTimer->Stop();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::clearPositionSpy() {
/////////////////////////////////////////////////////////////////////
	typedef UpdateManagerThread::Event Event;
	static Event evt;
	umPostEvent(evt.PosSpyResetEvent());
	
	positionSpy->clear();
	motionMonitor->resetCurrentClientId();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::clearPositionSpy(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	clearPositionSpy();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleTemplateManager(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	toggleAuiPane("TemplateManager");
}
/////////////////////////////////////////////////////////////////////
void MainFrame::loadPerspective(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxMenuItem* mi = m_menuPerspective->FindChildItem(event.GetId());
	if ( mi != NULL ) {
		wxString perspective(mi->GetItemLabelText());
		bool up = CncPerspective::isUserPerspective(perspective);
		
		// strip name
		perspective.assign(perspective.AfterFirst(CncPerspective::getNameEnclose()));
		perspective.assign(perspective.BeforeLast(CncPerspective::getNameEnclose()));
		
		if ( up == true )
			perspective.Prepend(CncPerspective::getNameEnclose());
			
		perspectiveHandler.loadPerspective(perspective);
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::savePerspective(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxMenuItem* mi = m_menuPerspective->FindChildItem(event.GetId());
	if ( mi != NULL ) {
		wxString perspective(mi->GetItemLabelText());
		bool up = CncPerspective::isUserPerspective(perspective);
		
		// strip name
		perspective.assign(perspective.AfterFirst(CncPerspective::getNameEnclose()));
		perspective.assign(perspective.BeforeLast(CncPerspective::getNameEnclose()));

		if ( up == true )
			perspective.Prepend(CncPerspective::getNameEnclose());
			
		perspectiveHandler.savePerspective(perspective);
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::addUserPerspective(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	perspectiveHandler.addUserPerspective();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::renameUserPerspective(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	perspectiveHandler.renameUserPerspective();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::removeUserPerspective(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	perspectiveHandler.removeUserPerspective();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::releaseControllerSetupFromConfig() {
/////////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->setup(false);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::setupGridChanged(wxPropertyGridEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(CncConfig::getGlobalCncConfig());
	CncConfig::getGlobalCncConfig()->setupGridChanged(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::setupGridChanging(wxPropertyGridEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(CncConfig::getGlobalCncConfig());
	CncConfig::getGlobalCncConfig()->setupGridChanging(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::setupGridCommandButton(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(CncConfig::getGlobalCncConfig());
	CncConfig::getGlobalCncConfig()->setupGridCommandButton(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::setupGridSelected(wxPropertyGridEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(CncConfig::getGlobalCncConfig());
	CncConfig::getGlobalCncConfig()->setupGridSelected(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::saveConfiguration(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(CncConfig::getGlobalCncConfig());
	CncConfig::getGlobalCncConfig()->saveConfiguration(*config);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::loadConfiguration(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(CncConfig::getGlobalCncConfig());
	CncConfig::getGlobalCncConfig()->loadConfiguration(*config);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::copyPositionSpy(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( positionSpy->getItemCount() == 0 )
		return;

	// Write some text to the clipboard
	if ( wxTheClipboard->Open() ) {
		startAnimationControl();

		wxString content;
		content.reserve(1024 * 1024);
		
		for ( long i=0; i<positionSpy->getItemCount(); i++ )
			positionSpy->getRow(i).trace(content);
		
		// This data objects are held by the clipboard,
		// so do not delete them in the app.
		wxTheClipboard->SetData( new wxTextDataObject(content) );
		wxTheClipboard->Close();
		
		stopAnimationControl();
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::togglePositionSpy(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	decoratePosSpyConnectButton(m_btTogglePosSpy->GetValue());
}
///////////////////////////////////////////////////////////////////
void MainFrame::decoratePosSpyConnectButton(bool state) {
///////////////////////////////////////////////////////////////////
	wxBitmap bmpOn  = ImageLib16().Bitmap("BMP_CONNECTED");
	wxBitmap bmpOff = ImageLib16().Bitmap("BMP_DISCONNECTED");
	
	state == true ? m_btTogglePosSpy->SetBitmap(bmpOn) 						: m_btTogglePosSpy->SetBitmap(bmpOff);
	state == true ? m_btTogglePosSpy->SetToolTip("Disable Position Spy")	: m_btTogglePosSpy->SetToolTip("Enable Position Spy");
	
	m_btTogglePosSpy->SetValue(state);
	m_btTogglePosSpy->Refresh();
	m_btTogglePosSpy->Update();
	
	if ( state == false )
		clearPositionSpy();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectSourceControlLineNumber(long ln) {
/////////////////////////////////////////////////////////////////////
	ln--;
	if ( ln <= 0L )
		return;
	
	wxASSERT( sourceEditor );
	sourceEditor->selectLineNumber(ln);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectSourceControlLineNumbers(long firstLine, long lastLine) {
/////////////////////////////////////////////////////////////////////
	firstLine--;
	lastLine--;
	if ( lastLine <= 0L || firstLine <= 0L )
		return;
	
	wxASSERT( sourceEditor );
	sourceEditor->selectLineNumbers(firstLine, lastLine);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::tryToSelectClientIds(long firstClientId, long lastClientId, ClientIdSelSource::ID tss) {
/////////////////////////////////////////////////////////////////////
	static bool isRunning = false;
	
	if ( isRunning == true )	return;
	else						isRunning = true;
	
	cnc::trc << wxString::Format("%s->selectClientIds(%ld ... %ld); ", ClientIdSelSource::getTemplateSelSourceAsString(tss), firstClientId, lastClientId);
	
	if ( tss != ClientIdSelSource::TSS_POS_SPY ) {
		if ( positionSpy != NULL )
			positionSpy->searchReferenceById(firstClientId);
	}
	
	if ( tss != ClientIdSelSource::TSS_MONITOR ) {
		if ( motionMonitor != NULL ) {
			motionMonitor->setCurrentClientId(firstClientId);
			motionMonitor->Refresh();
		}
	}
	
	if ( tss != ClientIdSelSource::TSS_PATH_LIST ) {
		if ( cncPreprocessor != NULL )
			cncPreprocessor->selectClientId(firstClientId, CncPreprocessor::LT_PATH_LIST);
	}
	
	if ( tss != ClientIdSelSource::TSS_MOVE_SEQ_OVW ) {
		if ( cncPreprocessor != NULL )
			cncPreprocessor->selectClientId(firstClientId, CncPreprocessor::LT_MOVE_SEQ_OVERVIEW);
	}

	if ( tss != ClientIdSelSource::TSS_MOVE_SEQ ) {
		if ( cncPreprocessor != NULL )
			cncPreprocessor->selectClientId(firstClientId, CncPreprocessor::LT_MOVE_SEQ_CONTENT);
	}
	
	if ( tss != ClientIdSelSource::TSS_VERTEX_DATA_TRACE) {
		if ( motionVertexCtrl != NULL )
				motionVertexCtrl->selectClientId(firstClientId, CncMotionVertexTrace::LT_VERTEX_DATA_TRACE);
	}
	
	if ( tss != ClientIdSelSource::TSS_VERTEX_INDEX_TRACE) {
		if ( motionVertexCtrl != NULL )
			motionVertexCtrl->selectClientId(firstClientId, CncMotionVertexTrace::LT_VERTEX_INDEX_TRACE);
	}
	
	// Important!
	// The editor hast to be the last one, otherwise the selection can be overridden by an 
	// other control which calls tryToSelectClientId(long clientId, TemplateSelSource tss) only
	// and the he first line is then selected only
	if ( tss != ClientIdSelSource::TSS_EDITOR ) {
		selectSourceControlLineNumbers(firstClientId, lastClientId);
	}
	
	isRunning = false;
}
/////////////////////////////////////////////////////////////////////
void MainFrame::tryToSelectClientId(long clientId, ClientIdSelSource::ID tss) {
/////////////////////////////////////////////////////////////////////
	tryToSelectClientIds(clientId, clientId, tss);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectPositionSpyContent(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	typedef UpdateManagerThread::Event Event;
	static Event evt;
	umPostEvent(evt.PosSypContentEvent((UpdateManagerThread::SpyContent)m_cbContentPosSpy->GetSelection()));

	clearPositionSpy();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::loopRepeatTest(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxTextEntryDialog dlg(this, "Loop Repeat Test:", "Loop count . . .", "");
	dlg.SetMaxLength(8);
	dlg.SetTextValidator(wxFILTER_DIGITS);
	
	unsigned int loopCount = 0;
	if ( dlg.ShowModal() == wxID_OK  ) {
		wxString s = dlg.GetValue();
		s.Trim(true).Trim(false);
		
		if ( s.IsEmpty() == false ) {
			long num;
			s.ToLong(&num);
			if ( num > 0 && num < 10000 )
				loopCount = (unsigned int)num;
		}
	}
	
	if ( loopCount == 0 )
		return;
		
	wxString title(GetTitle());
	wxString info;
	long duration = 0;

	// loop
	for ( unsigned int i=0; i<loopCount; i++) {
		
		bool ret = processTemplateWrapper( i == 0 );
		duration += THE_CONTEXT->timestamps.getTotalDurationMillis();

		info.Printf("Loop Counter : % 6d [#]; AVG duration: % 10ld [ms]", i + 1, duration / ( i + 1 ));
		SetTitle(wxString::Format("%s         [%s]", title, info));
		cnc::trc.logInfoMessage(info);

		if ( ret == false )
			break;
			
		waitActive(5);
	}
	
	// sumary
	cnc::trc.logInfoMessage("");
	std::clog << wxString::Format("Loop Repeat Test Sumary: Count: % 6d [#]; AVG Duration: % 10ld [ms]", loopCount, duration / loopCount) << std::endl;
	SetTitle(title);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::refreshSetterList(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	updateSetterList();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::clearSetterList(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	typedef UpdateManagerThread::Event Event;
	static Event evt;
	umPostEvent(evt.SetterListResetEvent());
	
	setterList->clear();
	m_setterListCount->SetValue("# 0");
}
/////////////////////////////////////////////////////////////////////
void MainFrame::goPosSypFirstId(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( positionSpy->GetItemCount() == 0 )
		return;
		
	positionSpy->selectItem(0L);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::goPosSypLastId(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( positionSpy->GetItemCount() == 0 )
		return;
	
	positionSpy->selectItem(positionSpy->GetItemCount() - 1);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::goPosSypNextId(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	long ls = positionSpy->getLastSelection();
	if ( ls == wxNOT_FOUND )
		positionSpy->selectItem(0L);
	
	ls = positionSpy->getLastSelection();
	if ( ls == wxNOT_FOUND )
		return;
	
	if ( positionSpy->goForwardUnitlColumnChange(ls, CncPosSpyListCtrl::COL_REF) == false )
		cnc::trc.logInfo("Position Monitor: Bottom reached . . . ");
	else
		cnc::trc.logInfo("");
}
/////////////////////////////////////////////////////////////////////
void MainFrame::goPosSypPrevId(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	long ls = positionSpy->getLastSelection();
	if ( ls == wxNOT_FOUND )
		positionSpy->selectItem(positionSpy->GetItemCount() - 1);
	
	ls = positionSpy->getLastSelection();
	if ( ls == wxNOT_FOUND )
		return;
	
	if ( positionSpy->goBackwardUnitlColumnChange(ls, CncPosSpyListCtrl::COL_REF) == false )
		cnc::trc.logInfo("Position Monitor: Top reached . . . ");
	else
		cnc::trc.logInfo("");
}
/////////////////////////////////////////////////////////////////////
void MainFrame::searchPosSpy(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxTextEntryDialog dlg(this, "Reference:", "Search position spy reference:", "");
	dlg.SetTextValidator(wxTextValidator(wxFILTER_NUMERIC));
	dlg.SetMaxLength(16);
	
	if ( dlg.ShowModal() != wxID_OK )
		return;
		
	wxString what(dlg.GetValue());
	what.Trim().Trim(true);
	
	bool ret = false;
	if ( what.IsEmpty() == false ) {
		long ref; what.ToLong(&ref);
		ret = positionSpy->searchReferenceById(ref);
	}
	
	if ( ret == false ) 
		cnc::trc.logWarning(wxString::Format("Position Spy Search: Nothing found for '%s'", what));
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectSerialSpyMode() {
/////////////////////////////////////////////////////////////////////
	int sm = m_cbSerialSpyMode->GetSelection();
	switch ( sm ) {
		case 0:		cnc->setSpyMode(Serial::SypMode::SM_READ); 		break;
		case 1:		cnc->setSpyMode(Serial::SypMode::SM_WRITE); 	break;
		default:	cnc->setSpyMode(Serial::SypMode::SM_ALL);		break;
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectSerialSpyMode(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	selectSerialSpyMode();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::displayIntervalChanged(wxScrollEvent& event) {
/////////////////////////////////////////////////////////////////////
	m_displayInterval->SetToolTip(wxString::Format("%d", m_displayInterval->GetValue()));
	THE_CONTEXT->setUpdateInterval(m_displayInterval->GetValue());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::displayIntervalThumbtrack(wxScrollEvent& event) {
/////////////////////////////////////////////////////////////////////
	m_displayInterval->SetToolTip(wxString::Format("%d", m_displayInterval->GetValue()));
	THE_CONTEXT->setUpdateInterval(m_displayInterval->GetValue());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::displayIntervalKeyDown(wxKeyEvent& event) {
/////////////////////////////////////////////////////////////////////
	event.Skip(true);

	m_displayInterval->SetToolTip(wxString::Format("%d", m_displayInterval->GetValue()));
	THE_CONTEXT->setUpdateInterval(m_displayInterval->GetValue());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::clickProbeMode(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	// this forces decorateProbeMode(...) also
	THE_CONTEXT->setProbeMode(m_btProbeMode->GetValue());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::decorateProbeMode(bool probeMode) {
/////////////////////////////////////////////////////////////////////
	if ( probeMode == true ) {
		const wxString tt("Probe mode is ON\nThis disables the stepper motors and the speed controller");
		m_btProbeMode->SetBitmap(ImageLibProbe().Bitmap("BMP_PROBE"));
		m_btProbeMode->SetToolTip(tt);
		
		m_probeModeState->SetBitmap(ImageLib24().Bitmap("BMP_TRAFFIC_LIGHT_YELLOW"));
		m_probeModeState->SetToolTip(tt);
		m_probeModeStateLabel->SetLabel(" ON");
		
	} else {
		const wxString tt("Probe mode is OFF\nThis enables the stepper motors and the speed controller");
		m_btProbeMode->SetBitmap(ImageLibProbe().Bitmap("BMP_RELEASE"));
		m_btProbeMode->SetToolTip(tt);
		
		m_probeModeState->SetBitmap(ImageLib24().Bitmap("BMP_TRAFFIC_LIGHT_DEFAULT"));
		m_probeModeState->SetToolTip(tt);
		m_probeModeStateLabel->SetLabel("OFF");
		
	}
	
	m_btProbeMode->SetValue(probeMode);
	m_btProbeMode->Refresh();
	
	if ( motionMonitor != NULL ) {
		motionMonitor->decorateProbeMode(THE_CONTEXT->isProbeMode());
		motionMonitor->Refresh();
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::dclickUpdateManagerThreadSymbol(wxMouseEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( updateManagerThread != NULL ) {
		if ( updateManagerThread->IsPaused() == true ) {
			wxCriticalSectionLocker enter(pUpdateManagerThreadCS);
			updateManagerThread->Resume();
		}
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::manualContinuousMoveStart(const CncLinearDirection x, const CncLinearDirection y, const CncLinearDirection z) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	
	if ( cnc->isInterrupted() == true ) {
		std::cerr << "Interrupt active. Nothing will be done . . ." << std::endl;
		return;
	}
	
	if ( cnc->isContinuousMoveActive() )
		return;
	
	CncControl::StepSensitivity stepSensitivity = CncControl::StepSensitivity::FINEST;
	unsigned int sel = m_rbStepSensitivity->GetSelection();
	switch ( sel ) {
		case 0: stepSensitivity = CncControl::StepSensitivity::FINEST;		break;
		case 1: stepSensitivity = CncControl::StepSensitivity::FINE; 		break;
		case 2: stepSensitivity = CncControl::StepSensitivity::MEDIUM; 		break;
		case 3: stepSensitivity = CncControl::StepSensitivity::ROUGH; 		break;
		case 4: stepSensitivity = CncControl::StepSensitivity::ROUGHEST; 	break;
	}
	
	CncTransactionLock ctl(this);
	
	cnc->manualContinuousMoveStop();
	
	motionMonitor->pushProcessMode();
	cnc->manualContinuousMoveStart(stepSensitivity, x, y, z);
	motionMonitor->popProcessMode();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::manualContinuousMoveStop() {
/////////////////////////////////////////////////////////////////////
	wxASSERT(cnc);
	cnc->manualContinuousMoveStop();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onNavigatorPanel(CncNavigatorPanelEvent& event) {
/////////////////////////////////////////////////////////////////////
	typedef CncNavigatorPanelEvent::Id Id;
	const Id eventId = (Id)event.GetId();
	
	auto moveStart = [&]() {
		CncLinearDirection x = CncLinearDirection::CncNoneDir;
		CncLinearDirection y = CncLinearDirection::CncNoneDir;
		CncLinearDirection z = CncLinearDirection::CncNoneDir;
		
		bool move = true;
		switch ( event.direction ) {
			case CncNavigatorPanel::Direction::EE: 	x = CncLinearDirection::CncPosDir; break;
			case CncNavigatorPanel::Direction::WW: 	x = CncLinearDirection::CncNegDir; break;
			
			case CncNavigatorPanel::Direction::NN: 	y = CncLinearDirection::CncPosDir; break;
			case CncNavigatorPanel::Direction::SS: 	y = CncLinearDirection::CncNegDir; break;
			
			case CncNavigatorPanel::Direction::CP: 	z = CncLinearDirection::CncPosDir; break;
			case CncNavigatorPanel::Direction::CN: 	z = CncLinearDirection::CncNegDir; break;
			
			case CncNavigatorPanel::Direction::NW:	x = CncLinearDirection::CncNegDir; 
													y = CncLinearDirection::CncPosDir;
													break;
													
			case CncNavigatorPanel::Direction::NE:	x = CncLinearDirection::CncPosDir; 
													y = CncLinearDirection::CncPosDir;
													break;
													
			case CncNavigatorPanel::Direction::SW:	x = CncLinearDirection::CncNegDir; 
													y = CncLinearDirection::CncNegDir;
													break;
													
			case CncNavigatorPanel::Direction::SE:	x = CncLinearDirection::CncPosDir; 
													y = CncLinearDirection::CncNegDir;
													break;
													
			default:								move = false;
		}
		
		if ( move == true )
			manualContinuousMoveStart(x, y, z);
	};
	
	auto moveStop = [&]() {
		manualContinuousMoveStop();
	};
		
	switch ( eventId ) {
		case Id::CNP_ACTIVATE_REGION:	moveStart();
										break;
										
		case Id::CNP_DEACTIVATE_REGION:
		case Id::CNP_LEAVE_PANEL:
		case Id::CNP_KILL_FOCUS:
		case Id::CNP_LEAVE_REGION:
										moveStop();
										break;
		default: ;
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::setReferencePosition(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	showReferencePositionDlg("");
}
/////////////////////////////////////////////////////////////////////
void MainFrame::warmStartController(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( cnc ) {
		cnc->sendSoftwareReset();
		connectSerialPortDialog();
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::changeConfigToolbook(wxToolbookEvent& event) {
/////////////////////////////////////////////////////////////////////
	m_pgMgrSetup->SelectPage(event.GetSelection());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::leaveSerialSpy(wxMouseEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT(serialSpyListCtrl);
	// currently nothing more to do
	//serialSpyListCtrl->clearDetails();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::decorateGamepadState(bool state) {
/////////////////////////////////////////////////////////////////////
	if ( state == true )	m_gamepadState->SetBitmap((ImageLibGamepad().Bitmap("BMP_ACTIVATED")));
	else 					m_gamepadState->SetBitmap((ImageLibGamepad().Bitmap("BMP_DEACTIVATED")));
	
	if ( state == false ) {
		m_gpBmp1->Show(false);
		m_gpBmp2->Show(false);
		m_gpBmp3->Show(false);
		m_gpBmp4->Show(false);
		
		if ( cncGameportDlg )
			cncGameportDlg->trace("Gamepad state not available");
	}
	
	m_gamepadState->Refresh();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::decorateSecureDlgChoice(bool useDlg) {
/////////////////////////////////////////////////////////////////////
	THE_CONTEXT->secureModeInfo.useIt = useDlg;
	if ( THE_CONTEXT->secureModeInfo.useIt == true )	m_rcSecureDlg->SetBitmap((ImageLibSecureRun().Bitmap("BMP_SECURE_DLG_YES")));
	else 												m_rcSecureDlg->SetBitmap((ImageLibSecureRun().Bitmap("BMP_SECURE_DLG_NO")));
	
	m_rcSecureDlg->Refresh();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::rcSecureDlg(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	decorateSecureDlgChoice(!THE_CONTEXT->secureModeInfo.useIt);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::cncMainViewChanged(wxNotebookEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( event.GetOldSelection() == MainBookSelection::VAL::PREVIEW_PANEL ) {
		wxASSERT( lruFileView != NULL );
		lruFileView->selectFirstItem();
	} 
	
	activateGamepadNotificationsOnDemand(true);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::decorateIdleState(bool state) {
/////////////////////////////////////////////////////////////////////
	if ( state == false )
		m_heartbeatState->SetBitmap(ImageLibHeartbeat().Bitmap("BMP_HEART_INACTIVE"));
		
	m_heartbeatState->Refresh();
	
	// in case the state is true, here is nothing to do, because this state will be handled by 
	// SerialControllerCallback(const ContollerInfo& ci) and ci == CITHeartbeat
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleIdleRequests(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	decorateIdleState(event.IsChecked());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::motionMonitorFlyPath(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( motionMonitor == NULL )
		return;
		
	motionMonitor->getContextOptions().toggleOption(motionMonitor->getContextOptions().showFlyPath);
	motionMonitor->reconstruct();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::motionMonitorPostionMarker(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( motionMonitor == NULL )
		return;
	
	motionMonitor->getContextOptions().toggleOption(motionMonitor->getContextOptions().showPosMarker);
	motionMonitor->updateMonitorAndOptions();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::motionMonitorBoundBox(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( motionMonitor == NULL )
		return;
		
	motionMonitor->getContextOptions().toggleOption(motionMonitor->getContextOptions().showBoundBox);
	motionMonitor->updateMonitorAndOptions();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::motionMonitorOrigin(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( motionMonitor == NULL )
		return;
		
	motionMonitor->getContextOptions().toggleOption(motionMonitor->getContextOptions().showOrigin);
	motionMonitor->updateMonitorAndOptions();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::motionMonitorRuler(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( motionMonitor == NULL )
		return;
	
	motionMonitor->getContextOptions().toggleOption(motionMonitor->getContextOptions().showRuler);
	motionMonitor->updateMonitorAndOptions();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::motionMonitorHelpLines(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( motionMonitor == NULL )
		return;
		
	motionMonitor->getContextOptions().toggleOption(motionMonitor->getContextOptions().showHelpLines);
	motionMonitor->updateMonitorAndOptions();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::openSessionDialog(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxString ret;
	openFileExtern(THE_CONFIG->getFileBrowser(ret), CncFileNameService::getTempDirSession());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::traceSessionId(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	traceSessionId();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::traceSessionId() {
/////////////////////////////////////////////////////////////////////
	std::clog << "Woodworking Session:" << std::endl;
	std::cout << " :: ID: " << CncFileNameService::getSession() << std::endl;
}
/////////////////////////////////////////////////////////////////////
BinaryFileParser::ViewType MainFrame::getCurrentBinaryViewMode() {
/////////////////////////////////////////////////////////////////////
	unsigned int sel = m_cbBinaryViewMode->GetSelection();
	switch ( sel ) {
		case 0:		return BinaryFileParser::ViewType::HexRaw;
		case 1:		return BinaryFileParser::ViewType::HexCStyle;
		case 2:		return BinaryFileParser::ViewType::ReadableSteps;
		
		default:	return BinaryFileParser::ViewType::ReadableMetric;
	}
	
	return BinaryFileParser::ViewType::ReadableMetric;
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectBinaryEditorViewMode(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	unsigned int sel = m_cbBinaryViewMode->GetSelection();
	
	BinaryFileParser::ViewType vt;
	switch ( sel ) {
		case 0:		vt = BinaryFileParser::ViewType::HexRaw; 			break;
		case 1:		vt = BinaryFileParser::ViewType::HexCStyle; 		break;
		case 2:		vt = BinaryFileParser::ViewType::ReadableSteps; 	break;
		
		default:	vt = BinaryFileParser::ViewType::ReadableMetric;
	}
	
	sourceEditor->changeBinaryViewType(vt);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::saveOutboundAsNewTplFromMenu(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	saveOutboundAsNewTplFromButton(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::saveOutboundAsNewTplFromButton(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxString outboundFile(cnc->getPortName());
	
	if ( wxFile::Exists(outboundFile) == false ) {
		std::cerr << "MainFrame::saveOutboundAsNewTemplate: Can't found '" << outboundFile << "'" << std::endl;
		return;
	}
	
	wxString headline("Save Outbound as new Template");
	wxString inboundFile(getCurrentTemplatePathFileName());
	inboundFile.append(".");
	inboundFile.append(outboundEditor->getExtention());

	wxString newFile;
	wxFileName fileName(inboundFile);

	while ( true ) {
		wxFileDialog saveFileDialog(this, 
									headline, 
									fileName.GetPath(),
									fileName.GetFullName(),
									"",
									wxFD_SAVE);

		if ( saveFileDialog.ShowModal() == wxID_CANCEL ) { 
			return;
		}
		
		newFile.assign(saveFileDialog.GetPath());
		if ( wxFile::Exists(newFile) == true ) {
			
			wxString msg(wxString::Format("Template '%s'\nalready exists. Overide it?", newFile));
			wxMessageDialog dlg(this, msg, headline, wxYES|wxNO|wxICON_QUESTION|wxCENTRE);
			
			if ( dlg.ShowModal() == wxID_YES ) 
				break; //while
				
		} else {
			break; //while
			
		}
	}
	
	if ( wxCopyFile(outboundFile, newFile, true) == false ) {
		
		std::cerr << "File copy failed:"            << std::endl;
		std::cerr << " from:" << cnc->getPortName() << std::endl;
		std::cerr << " to:"   << newFile	        << std::endl;
		return;
	}
	
	wxString msg("Should the new template opened directly into the editor?");
	wxMessageDialog dlg(this, msg, headline, wxYES|wxNO|wxICON_QUESTION|wxCENTRE);

	if ( dlg.ShowModal() == wxID_YES ) {
		
		wxFileName tpl(newFile);
		m_inputFileName->SetValue(tpl.GetFullName());
		m_inputFileName->SetHint(tpl.GetFullPath());
		
		if ( !openFile() ) {
			std::cerr << "Error while open file: " << outboundFile << std::endl;
			return;
		}
		
		prepareAndShowMonitorTemplatePreview(true);
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::extractSourceAsNewTpl(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	BinaryFileParser parser(getCurrentTemplatePathFileName());
	if ( parser.preface() == false ) {
		std::cerr << "MainFrame::extractSourceAsNewTpl(): Error while prefacing file '" 
				  << getCurrentTemplatePathFileName()
				  << "'" << std::endl;
		return;
	}
	
	wxString origType, origName;
	parser.getSourceParameter(XMLSourceNode_AttribFile, origName);
	parser.getSourceParameter(XMLSourceNode_AttribType, origType);
	
	wxFileName newFile(origName);
	newFile.ClearExt();
	
	wxString newName(newFile.GetFullName());
	newName.append("_Copy.");
	newName.append(origType);
	
	wxString defaultDir;
	THE_CONFIG->getDefaultTplDir(defaultDir);
		
	wxString headline("Save nested source as new Template");
	
	wxFileDialog saveFileDialog(this, 
								headline, 
								defaultDir, 
								newName,
								"",
								wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
								
	if ( saveFileDialog.ShowModal() == wxID_CANCEL ) { 
		return;
	}
	
	std::ofstream out(saveFileDialog.GetPath().c_str().AsChar());
	if ( out.good() == false ) {
		std::cerr << "MainFrame::extractSourceAsNewTpl(): Error while creating file '" 
				  << saveFileDialog.GetPath()
				  << "'" << std::endl;
		return;
	}
	
	wxString content;
	out << parser.getSourceContent(content);
	out.close();
	
	wxFileName x(saveFileDialog.GetPath());
	m_inputFileName->SetValue(x.GetFullName());
	m_inputFileName->SetHint(x.GetFullPath());

	if ( !openFile() ) {
		std::cerr << "Error while open file: " << saveFileDialog.GetPath() << std::endl;
		return;
	}
	
	prepareAndShowMonitorTemplatePreview(true);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleMotionMonitorOptionPane(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	toggleMotionMonitorOptionPane(false);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleMotionMonitorStatisticPane(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	toggleMotionMonitorStatisticPane(false);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleMotionMonitorOptionPane(bool forceHide) {
/////////////////////////////////////////////////////////////////////
	if ( cnc3DVSplitterWindow != NULL )
		forceHide == true ? cnc3DVSplitterWindow->hideRightWindow() : cnc3DVSplitterWindow->toggleRightWindow();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleMotionMonitorStatisticPane(bool forceHide) {
/////////////////////////////////////////////////////////////////////
	if ( cnc3DHSplitterWindow != NULL ) {
		
		const int nc = MontiorBottomContextSelection::VAL::STATISTIC_PANEL;
		const int cc = cnc3DHSplitterWindow->getCurrentButtomContext();
		
		cnc3DHSplitterWindow->selectBottomContext(nc);
		
		if ( cnc3DHSplitterWindow->isBottomWindowShown() ) {
			cnc3DHSplitterWindow->hideBottomWindow();
			
			if ( nc != cc )
				forceHide == true ? cnc3DHSplitterWindow->hideBottomWindow() : cnc3DHSplitterWindow->showBottomWindow();
				
		} else {
			forceHide == true ? cnc3DHSplitterWindow->hideBottomWindow() : cnc3DHSplitterWindow->toggleBottomWindow();
			
		}
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::toggleMotionMonitorReplayPane(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( cnc3DHSplitterWindow != NULL ) {
		
		const int nc = MontiorBottomContextSelection::VAL::REPLAY_PANEL;
		const int cc = cnc3DHSplitterWindow->getCurrentButtomContext();
		
		cnc3DHSplitterWindow->selectBottomContext(nc);
		
		if ( cnc3DHSplitterWindow->isBottomWindowShown() ) {
			cnc3DHSplitterWindow->hideBottomWindow();
			
			if ( nc != cc)
				cnc3DHSplitterWindow->showBottomWindow();
				
		} else {
			cnc3DHSplitterWindow->toggleBottomWindow();
			
		}
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectMetricUnitFrom(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	selectMetricUnitTo(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectMetricUnitFromValue(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	selectMetricUnitTo(event);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::selectMetricUnitTo(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	const wxString uFrom(m_cbMetricUnitFrom->GetStringSelection());
	const wxString uTo(m_cbMetricUnitTo->GetStringSelection());
	
	const wxString valueFrom(m_metricValueFrom->GetValue());
	double value;
	valueFrom.ToDouble(&value);
	
	if      ( uFrom == "mm"      && uTo == "mm"     ) 		m_metricValueTo->ChangeValue(valueFrom);
	else if ( uFrom == "mm"      && uTo == "stepsX" ) 		m_metricValueTo->ChangeValue(wxString::Format("%.6lf", THE_CONFIG->convertX(CncMetric, CncSteps,  value)));
	else if ( uFrom == "mm"      && uTo == "stepsY" ) 		m_metricValueTo->ChangeValue(wxString::Format("%.6lf", THE_CONFIG->convertY(CncMetric, CncSteps,  value)));
	else if ( uFrom == "mm"      && uTo == "stepsZ" ) 		m_metricValueTo->ChangeValue(wxString::Format("%.6lf", THE_CONFIG->convertZ(CncMetric, CncSteps,  value)));
	
	else if ( uFrom == "stepsX"  && uTo == "stepsX" ) 		m_metricValueTo->ChangeValue(valueFrom);
	else if ( uFrom == "stepsX"  && uTo == "mm"     ) 		m_metricValueTo->ChangeValue(wxString::Format("%.6lf", THE_CONFIG->convertX(CncSteps,  CncMetric, value)));

	else if ( uFrom == "stepsY"  && uTo == "stepsY" ) 		m_metricValueTo->ChangeValue(valueFrom);
	else if ( uFrom == "stepsY"  && uTo == "mm"     ) 		m_metricValueTo->ChangeValue(wxString::Format("%.6lf", THE_CONFIG->convertY(CncSteps,  CncMetric, value)));
	
	else if ( uFrom == "stepsZ"  && uTo == "stepsZ" ) 		m_metricValueTo->ChangeValue(valueFrom);
	else if ( uFrom == "stepsZ"  && uTo == "mm"     ) 		m_metricValueTo->ChangeValue(wxString::Format("%.6lf", THE_CONFIG->convertZ(CncSteps,  CncMetric, value)));
	
	else													m_metricValueTo->ChangeValue("-");
}
/////////////////////////////////////////////////////////////////////
void MainFrame::enableSourceEditorMenuItems(bool enable) {
/////////////////////////////////////////////////////////////////////
	m_miNewTemplate->Enable(enable);
	m_miOpenTemplate->Enable(enable);
	m_miReloadTemplate->Enable(enable);
	m_miSaveTemplate->Enable(enable);
	m_miSaveTemplateAs->Enable(enable);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::showOSEnvironment(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( cncOsEnvDialog->IsShownOnScreen() == false )
		cncOsEnvDialog->Show();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::openGameportController(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	showGameportController(true);
}
/////////////////////////////////////////////////////////////////////
void MainFrame::showGameportController(bool show) {
/////////////////////////////////////////////////////////////////////
	if ( cncGameportDlg == NULL )
		cncGameportDlg = new CncGameportController(this);
	
	if ( show == true ) {
		if ( cncGameportDlg->IsShown() == false )
			cncGameportDlg->Show(true);
	}
	else {
		if ( cncGameportDlg->IsShown() == true )
			cncGameportDlg->Show(false);
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::dclickHeartbeatState(wxMouseEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( m_miRqtIdleMessages->IsEnabled() == false )
		return;
	
	m_miRqtIdleMessages->Check(!m_miRqtIdleMessages->IsChecked());
	decorateIdleState(m_miRqtIdleMessages->IsChecked());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::updateSpyDetailWindow() {
/////////////////////////////////////////////////////////////////////
	wxDataViewListCtrl* listCtrl = NULL;
	wxString headline;
	if      ( m_spyUnknownDetails->IsShownOnScreen() )		{ listCtrl = m_spyUnknownDetails;  headline.assign("Info"); }
	else if ( m_spyInboundDetails->IsShownOnScreen() ) 		{ listCtrl = m_spyInboundDetails;  headline.assign("Inbound Details"); }
	else if ( m_spyOutboundDetails->IsShownOnScreen() )		{ listCtrl = m_spyOutboundDetails; headline.assign("Outbound Details"); }
	
	if ( listCtrl != NULL && listCtrl->GetItemCount() > 0) {
		
		const int row = 0;
		wxString details;
		for ( unsigned int col = 0; col < listCtrl->GetColumnCount(); col++ ) {
			
			wxDataViewColumn* dvc = listCtrl->GetColumn(col);
			wxVariant value;
			listCtrl->GetValue(value, row, col);
			
			if ( listCtrl->GetColumnCount() > 1 && col == listCtrl->GetColumnCount() - 1 ) {
				details.append(wxString::Format("\n%s:\n%s", dvc->GetTitle(), value.GetString()));
			}
			else {
				const unsigned int size = 24;
				wxString title(dvc->GetTitle().Left(size));
				wxString spacer(' ',  ( listCtrl->GetColumnCount() > 1 ? size - title.Length() : 0 ));
				details.append(wxString::Format("%s%s: %s\n", title, spacer, value.GetString()));
			}
		}
		
		details.Replace(" | ", "\n", true);
		details.Replace(" |", "\n",  true);
		details.Replace("| ", "\n",  true);
		details.Replace("|", "\n", 	 true);
		
		if ( spyDetailWindow != NULL && spyDetailWindow->IsShownOnScreen() == true ) {
			spyDetailWindow->setHeadline(headline);
			spyDetailWindow->setMessage(details);
		}
	}
}
/////////////////////////////////////////////////////////////////////
void MainFrame::openSpyDetailWindow(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( spyDetailWindow == NULL ) {
		spyDetailWindow = new CncMessageDialog(this, "", "", "Serial Spy Details");
		spyDetailWindow->SetSize(800, 900);
		spyDetailWindow->setWordWrap(true);
	}
	
	if ( spyDetailWindow->IsShownOnScreen() == false )
		spyDetailWindow->Show();
	
	updateSpyDetailWindow();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onSelectSpyInboundDetails(wxDataViewEvent& event) {
/////////////////////////////////////////////////////////////////////
	m_spyInboundDetails->UnselectAll();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onSelectSpyOutboundDetails(wxDataViewEvent& event) {
/////////////////////////////////////////////////////////////////////
	m_spyOutboundDetails->UnselectAll();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onSelectSpyUnknownDetails(wxDataViewEvent& event) {
/////////////////////////////////////////////////////////////////////
	m_spyUnknownDetails->UnselectAll();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::showStacktraceStore(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	CncFileContentDialog dlg(this, CncFileNameService::getStackTraceFileName(), "All collected stacktraces", "Stacktrace Store");
	dlg.scrollToEOF();
	dlg.SetSize(800, 900);
	dlg.setWordWrap(false);
	dlg.ShowModal();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::freezeLogger(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	const bool isPrevFrozen = getLogger()->IsFrozen();
	
	const wxBitmap bmp = isPrevFrozen ? ImageLibLogger().Bitmap("BMP_FROZEN") : ImageLibLogger().Bitmap("BMP_THAW");
	const wxString tip = isPrevFrozen ? "Freeze Logger" : "Thaw Logger";
	const wxColour col = isPrevFrozen ? wxColour(0, 0, 0) : wxColour(80, 180, 180);
	
	if ( isPrevFrozen == true )		{ getLogger()->Thaw();		getLogger()->SetBackgroundColour(col); }
	else							{ getLogger()->Freeze();	getLogger()->SetBackgroundColour(col); }
	
	m_freezeLogger->SetValue(!isPrevFrozen);
	m_freezeLogger->SetToolTip(tip);
	m_freezeLogger->SetBitmap(bmp);
	m_freezeLogger->Refresh();
	m_freezeLogger->Update();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onShowLoggerOnDemand(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	getLogger()->setShowOnDemandState(m_showLoggerOnDemand->GetValue());
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onOpenGLContextObserver(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	wxASSERT( openGLContextObserver != NULL );
	
	const bool flag = openGLContextObserver->IsShownOnScreen();
	openGLContextObserver->Show(!flag);
	m_miOpenGLContextObserver->Check(!flag);
	
	Refresh();
}
/////////////////////////////////////////////////////////////////////
void MainFrame::onIndividualCommand(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	IndividualCommandEvent* ice = static_cast<IndividualCommandEvent*>(&event);

	typedef IndividualCommandEvent::EvtMainFrame 	EMF_ID;
	typedef IndividualCommandEvent::EvtSerialStub 	ESS_ID;
	typedef IndividualCommandEvent::ValueName 		VN;
	
	//---------------------------------------------------------------
	auto showArduinoEnv = [&](bool state) {
		if ( cncArduinoEnvironment && cncArduinoEnvironment->IsShownOnScreen() == !state )
			cncArduinoEnvironment->Show(state);
	};
	
	//---------------------------------------------------------------
	auto checkSerialThread = [&]() {
		if ( serialThread == NULL ) {
			return false;
		}
		
		return true;
	};
	
	switch ( ice->GetId() ) {
		//-----------------------------------------------------------
		case EMF_ID::DispatchAll:
		{
			dispatchAll();
			break;
		}
		//-----------------------------------------------------------
		case EMF_ID::DistpatchNext:
		{
			dispatchNext();
			break;
		}
		//-----------------------------------------------------------
		case EMF_ID::WaitActive:
		{
			if ( ice->hasValue(VN::VAL2) )
				waitActive(ice->getValue<unsigned int>(VN::VAL1), ice->getValue<bool>(VN::VAL2));
			else
				waitActive(ice->getValue<unsigned int>(VN::VAL1));
			break;
		}
		//-----------------------------------------------------------
		case EMF_ID::EnableControls:
		{
			enableControls(ice->getValue<bool>(VN::VAL1));
			break;
		}
		//-----------------------------------------------------------
		case EMF_ID::ExtViewBoxChange:
		{
			if ( ice->getValue<CncExternalViewBox*>(VN::VAL1) == cncExtMainPreview ) {
				if ( ice->getValue<int>(VN::VAL2) == CncExternalViewBox::Preview::FILE )
					lruFileView->selectFirstItem();
			}
			break;
		}
		//-----------------------------------------------------------
		case ESS_ID::NotifyConneting:
		{
			showArduinoEnv(true);
			
			if ( checkSerialThread() == false )
				return; 
				
			serialThread->notifyConnecting();
			cncArduinoEnvironment->notifyConnecting();
			break;
		}
		//-----------------------------------------------------------
		case ESS_ID::NotifyConneted:
		{
			showArduinoEnv(true);
			
			if ( checkSerialThread() == false )
				return; 
				
			serialThread->notifyConnected();
			cncArduinoEnvironment->notifyConnected();
			break;
		}
		//-----------------------------------------------------------
		case ESS_ID::NotifyDisconnected:
		{
			showArduinoEnv(false);
			
			if ( checkSerialThread() == false )
				return; 
				
			serialThread->notifyDisconnected();
			cncArduinoEnvironment->notifyDisconnected();
			break;
		}
		//-----------------------------------------------------------
		case ESS_ID::NotifyPauseBefore:
		{
			showArduinoEnv(true);
			cncArduinoEnvironment->notifyPaused();
			break;
		}
		//-----------------------------------------------------------
		case ESS_ID::NotifyResumeBefore:
		{
			showArduinoEnv(true);
			break;
		}
		//-----------------------------------------------------------
		case ESS_ID::NotifyResumeAfter:
		{
			showArduinoEnv(true);
			cncArduinoEnvironment->notifyResumed();
			break;
		}
	}
}
/////////////////////////////////////////////////////////////////////
SerialThread* MainFrame::getSerialThread(SerialThreadStub* sts) {
/////////////////////////////////////////////////////////////////////
	if ( dynamic_cast<SerialThreadStub*>(sts) != NULL )
		return serialThread;
	
	return NULL;
}
/////////////////////////////////////////////////////////////////////
void MainFrame::openSpeedPlayground(wxCommandEvent& event) {
/////////////////////////////////////////////////////////////////////
	if ( cncSpeedPlayground == NULL )
		cncSpeedPlayground = new CncSpeedPlayground(this);
		
	cncSpeedPlayground->Show(cncSpeedPlayground->IsShownOnScreen() == false);
}




void MainFrame::changeSpeedConfigSlider(wxScrollEvent& event)
{
}
void MainFrame::updatedSpeedConfigAccelAxis(wxCommandEvent& event)
{
}
void MainFrame::updatedSpeedConfigSteps(wxCommandEvent& event)
{
}
